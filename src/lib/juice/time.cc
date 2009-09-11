/**
   Time/delay-related functions for v8-juice.

   Requires pthreads, sleep(3), and usleep(3). The mutex-locking code
   required for a proper implementation of cancelTimeout() requires
   some accompanying mutex code, but those bits can easily be replaced
   with a mutex of your choice.

   This code is intentionally kept free of v8-juice-specific APIs
   so that it can easily be ported to other v8-based projects.

   Author: Stephan Beal (http://wanderinghorse.net/home/stephan/)

   License: Public Domain


   TODOs: remove the hard-coded pthread dep and instead use Marc Duerner's
   thread class.
*/
#include <v8/juice/time.h>
#include <pthread.h>
#include <unistd.h> // sleep(), usleep()
#include <map>
#include <set>

/** Set USE_WHNET_MUTEX to a true value to enable the mutex
    package from the v8-juice tree. If set to false it uses
    a dummy/no-op mutex, which the client can replace with
    his own if he likes.
*/
#define USE_WHNET_MUTEX 0

#if USE_WHNET_MUTEX
#  include "mutex.hpp"
#endif

#include <iostream> // only for debuggering
// #ifndef CERR
#define CERR std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : "
// #endif
// #ifndef COUT
// #define COUT std::cout << __FILE__ << ":" << std::dec << __LINE__ << " : "
// #endif

namespace v8 { namespace juice {

    using namespace v8;


    namespace Detail
    {
        struct js_thread_info;
        /**
           MutexType can be any type compatible with the MutexSentry
           interface (see below).
           
           Conventionally has lock() and unlock() methods which lock a
           specific mutex.

           Must be allocable as a static variable.
        */
#if USE_WHNET_MUTEX
        typedef v8::juice::mutex MutexType;
#else
        struct MutexType
        {
            void lock(){}
            void unlock(){}
        };           
#endif
        /**
           MutexSentry can be any type with the interface:

           MutextSentry::MutextSentry( MutextType & );

           Must lock the mutex as construction and unlock it
           at destruction. Or it may be a no-op which does
           nothing.
        */
#if USE_WHNET_MUTEX
        typedef v8::juice::mutex_sentry MutexSentry;
#else
        /**
           A dummy/no-op MutexSentry impl.
        */
        struct MutexSentry
        {
        private:
            MutexType & mutex;
        public:
            MutexSentry( MutexType & m ) : mutex(m)
            {
                this->mutex.lock();
            }
            ~MutexSentry()
            {
                this->mutex.unlock();
            }
        };
#endif
        /**
           Internal utility type to lock access to internal
           set of setTimeout()/cancelTimeout() timer IDs.
        */
        class TimerLock : public MutexSentry
        {
            /** This class' shared mutex. */
            static MutexType & timeMutex()
            {
                static MutexType bob;
                return bob;
            }
        public:
            typedef uint32_t TimerIDType;
            /**
               Locks a mutex shared by all instances of this class.
               Does not return until the lock is acquired.
            */
            TimerLock() : MutexSentry(timeMutex())
            {
            }
            /**
               Unlocks this class's mutex.
             */
            virtual ~TimerLock()
            {
            }
            typedef std::set<TimerIDType> SetType;
            /** Returns the shared timer set. */
            SetType & set()
            {
                static SetType bob;
                return bob;
            }
            /**
               Removes the given timer id from the internal
               list. Returns 0 if no timer was found, else some other
               value.
             */
            size_t take(TimerIDType id)
            {
                return set().erase(id);

            }
            /**
               Generates a unique timer ID and inserts it into
               the internal list.
            */
            TimerIDType next_id()
            {
                static TimerIDType x = 0;
                // Generate unique timer ID, avoiding collisions after overflow:
                while( ! this->set().insert( ++x ).second )
                { (void)1; }
                return x;
            }
        };

        /**
           Internal holder for setTimeout() callback data.
        */
        struct js_thread_info
        {
        public:
            /** Timeout delay, in miliseconds. */
            uint32_t delay;
            /** The Function/String handle passed as argv[0] to setTimeout(). */
            v8::Persistent<v8::Value> jv;
            /** The 'this' object (the global object) for making the
                callback. If we don't store this here we get segfaults when we
                Call() the callback. */
            v8::Persistent<v8::Object> jself;
            /** Set by the ctor. */
            TimerLock::TimerIDType id;
            /**
               Creates an empty (except for this->id) holder object
               for setTimeout() metadata. After calling this,
               this->id is guaranteed to be unique within the current
               set of timeout timers.
            */
            js_thread_info()
                : delay(0),
                  id(TimerLock().next_id()),
                  jv(),
                  jself()
            {
            }
        };        
    } // namespace Detail

    /**
       pthread_create() callback for implementing the delayed start of
       JS code via setTimeout().

       arg must be-a pointer to a fully-populated Detail::js_thread_info
       object. This function will free that object.

       Uses v8::Unlocker to unlock v8 for the duration of the timeout.
    */
    static void * thread_setTimeout( void * arg )
    {
#define THREAD_RETURN ::pthread_exit( (void *)0 )
        if( v8::V8::IsDead() ) THREAD_RETURN;
        v8::Locker locker;
        v8::HandleScope hsc;
        Detail::js_thread_info * jio = reinterpret_cast<Detail::js_thread_info*>( arg );
        /** reminder: copy jio to the stack so we can free it up immediately
            and not worry about a leak via an exception propagating below... */
        const Detail::js_thread_info ji(*jio);
        delete jio;
        unsigned int d = ji.delay * 1000;
        int src = 0;
        {
            v8::Unlocker ul;
            /**
               FIXME: wake up periodically and see if we should still be
               waiting, otherwise this might keep the app from exiting
               from an arbitrarily long time after the main thread has
               gone. But how to know that?
            */
            /**
               FIXME: posix has obsoleted usleep() and recommends
               nanonosleep(), with it's much more complicated
               interface. OTOH, according to APUE, nanosleep() is only
               required to be defined on platforms which implement "the
               real-time extensions". What to do?
            */
            src = ::usleep( d );
        }
        {
            // Check for cancellation:
            Detail::TimerLock lock;
            if( ! lock.take(ji.id) ) THREAD_RETURN;
        }
        if( ji.jv->IsFunction() )
        {
            v8::Handle<v8::Function> fh( v8::Function::Cast( *(ji.jv) ) );
            fh->Call( ji.jself, 0, 0 );
        }
        THREAD_RETURN;
        return NULL; // can't happen
#undef THREAD_RETURN
    }
    v8::Handle<v8::Value> cancelTimeout(const v8::Arguments& argv )
    {
        if( argv.Length() != 1 )
        {
            return v8::False();
        }
        v8::Handle<v8::Value> arg = argv[0];
        if( arg.IsEmpty() || ! arg->IsNumber() ) return v8::False();
        // do we need this: v8::Locker locker;
        Detail::TimerLock::TimerIDType id = static_cast<Detail::TimerLock::TimerIDType>( arg->ToInteger()->Value() );
        Detail::TimerLock lock;
        return v8::Boolean::New( lock.take(id) ? true : false );
    }

    v8::Handle<v8::Value> setTimeout(const v8::Arguments& argv )
    {
        if( argv.Length() < 2 )
        {
            return v8::ThrowException( v8::String::New("setTimeout() requires two arguments!") );
        }
        v8::Locker locker;
        v8::HandleScope hsc; // this must exist to avoid a weird error
        v8::Handle<v8::Value> fh( argv[0] );
        if( ! fh->IsFunction() )
        {
            return v8::ThrowException( v8::String::New("setTimeout() requires a function [FIXME: or string] as its first argument!") );
        }
        Detail::TimerLock::TimerIDType tm = static_cast<Detail::TimerLock::TimerIDType>( argv[1]->ToNumber()->Value() );
        Detail::js_thread_info * ji = new Detail::js_thread_info;
        ji->delay = tm;
        ji->jv = v8::Persistent<v8::Value>::New( fh );
        ji->jself = v8::Persistent<v8::Object>::New( argv.This() );
        v8::Handle<v8::Value> id = v8::Integer::New( ji->id );
        pthread_t tid;
        pthread_attr_t tattr;
        pthread_attr_init( &tattr );
        pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);
        ::pthread_create( &tid, &tattr, thread_setTimeout, ji );
        pthread_attr_destroy(&tattr);
        return hsc.Close( id ); // fixme: return a timer ID
    }

    v8::Handle<v8::Value> sleep(const v8::Arguments& argv)
    {
        int st = argv.Length() ? static_cast<uint32_t>( argv[0]->ToInteger()->Value() ) : -1;
        int rc = -1;
        if(0 <= st)
        {
            v8::Unlocker ul;
            rc = ::sleep( st );
        }
        return v8::Integer::New(rc);
    }

}}