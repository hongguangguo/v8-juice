#if !defined(CODE_GOOGLE_COM_P_V8_V8_FORWARDING_H_INCLUDED)
#define CODE_GOOGLE_COM_P_V8_V8_FORWARDING_H_INCLUDED 1
/*
   A min-framework to simplify casting between v8 JavaScript and
   C++ native objects. Requires the v8-bind.h header.

   v8: http://code.google.com/p/v8/

   Author: Stephan Beal (http://wanderinghorse.net/home/stephan/)

   License: same as v8 (see below)
*/
// Copyright 2007-2008 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//#include <cstdint> // arg! Requires C++0x!
#include <stdint.h> // hope the client's platform is recent!
#include <string>
#include <cstring>
#include <list>
#include <vector>
#include <map>
#include "bind.h"
#include "convert.h"
namespace v8 { namespace juice { namespace convert {
    using namespace v8;
    /** Convenience typedef. */
    typedef Handle<Value> ValueHandle;

#if ! defined(DOXYGEN)
    /** Internal library details. */
    namespace Detail
    {

    }
#endif // !DOXYGEN



    /**
       Base instantiation of a helper to forward v8::Arguments
       to arbitrary functors. Must be specialized to be useful.

       Arity is the minimum number of arguments the functor expects.
       RV is the return type. A partial specializations void RV=void
       is installed to support functions which return void.
    */
    template <int Arity_,typename RV>
    struct FunctorForwarder
    {
        enum { Arity = Arity_ };
    };

    /** Specialization for functors taking no arguments. */
    template <typename RV>
    struct FunctorForwarder<0,RV>
    {
        enum { Arity = 0 };
        template <typename Func>
        static Handle<Value> Call( Func f )
        {
            try
            {
                return convert::CastToJS<RV>( f() );
            }
            catch( std::exception const & ex )
            {
                return ::v8::ThrowException( ::v8::String::New(ex.what()) );
            }
            catch( ... )
            {
                return ::v8::ThrowException( ::v8::String::New("Native function threw an unknown native exception type!"));
            }
            return Undefined(); // cannot be reached.
        }
    };

    /** Specialization for functor taking no arguments and returning void. */
    template <>
    struct FunctorForwarder<0,void>
    {
        enum { Arity = 0 };
        template <typename Func>
        static Handle<Value> Call( Func f )
        {
            try
            {
                f();
            }
            catch( std::exception const & ex )
            {
                return ::v8::ThrowException( ::v8::String::New(ex.what()) );
            }
            catch( ... )
            {
                return ::v8::ThrowException( ::v8::String::New("Native function threw an unknown native exception type!"));
            }
            return Undefined(); // cannot be reached.
            return Undefined();
        }
    };


    /**
       This is the documentation for the whole family of FwdToFuncN()
       functions, where N is some positive integer.

       Uses CastFromJS() to convert function arguments for the given
       function. The return result is then cast to JS using
       CastToJS(), and returned.

       This routine will only work with types which work with
       CastFromJS() and CastToJS().

       For all functions of this family (FwdToFunc2(), etc.), if
       argv.Length() is not *exactly* the correct number then a
       JS-side exception is thrown - the return value will be that of
       ::v8::ThrowException() (what exactly that return value is,
       isn't really documented :/).

       Notable limitations:

       - Can only convert argument types handled by CastFromJS().

       Some of the related limitations (e.g. can't handle void
       returns) can be solved by adding another level or two of
       template-based indirection. Maybe that will happen someday,
       maybe not.
    */
    template <typename ReturnT, typename Func>
    ::v8::Handle< ::v8::Value > FwdToFunc0( Func func, ::v8::Arguments const & argv )
    {
        return FunctorForwarder<0,ReturnT>::Call( func, argv );
    }

    /**
       Functionally identical to FwdToFunc0(), but the return type can
       be deduced by the compiler.
    */
    template <typename ReturnT>
    ::v8::Handle< ::v8::Value > FwdToFunc( ReturnT (*func)(), ::v8::Arguments const & argv )
    {
        return FunctorForwarder<0,ReturnT>::Call( func );
    }

#include "forwarding-FunctorForwarder.h" // generated code


    /**
       See InvocationCallbackToArgv for details.
    */
    typedef ::v8::Handle< ::v8::Value > (*InvocationCallbackWithArray)( Handle<Object> self, int argc, Handle<Value> argv[] );

    /**
       A helper to allow re-use of certain JS/C++ functions. It's a bit of
       a long story...

       v8 defines the basic JS/C++ callback type as
       InvocationCallback, which takes a v8::Arguments list as its
       only argument. Since client code cannot create their own
       Arguments lists (only v8 can do that), it is impossible to
       re-use a subset of the arguments and pass them to another
       InvocationHandler. Such forwarding is often useful when
       implementing related functions which share a common basis, but
       v8's callback convention makes it impossible.

       So now the workaround:

       Instead of defining a callback as an InvocationCallback, it can
       be defined as a InvocationCallbackWithArray, and this type can
       be used to do the conversion.  For example, in conjunction with
       the WeakJSClassCreator:

       \code
       myobj->Set(String::New("func"), InvocationCallbackToArgv<MyCallback>::call );
       \endcode

       This is of course less efficient than directly calling an
       InvocationCallback, because we must synthesize an array of
       Value handles.

       The optional skipArgN parameter tells call() than it should skip
       over the first N arguments in the list, which can be useful when
       stripping a first argument for personal use then passing on the
       rest of the args.
    */
    template <InvocationCallbackWithArray proxy, int skipArgN = 0>
    struct InvocationCallbackToArgv
    {
	/**
	   Synthesizes an array of Value handles and calls
	   proxy(argv.This(),argCount,argsArray). If skipArgN is
	   greater than or equal to argv.Length() then
	   proxy(argv.This(),0,0) is called.
	*/
	static ::v8::Handle< ::v8::Value > call( ::v8::Arguments const & argv )
	{
	    typedef Handle<Value> HV;
	    if( skipArgN >= argv.Length() )
	    { // Is this sensible? Should we throw instead? Hmm.
		return proxy( argv.This(), 0, 0 );
	    }
	    const int argc = argv.Length() - skipArgN;
	    std::vector< HV > vec( static_cast<size_t>(argc), Null() );
	    int pos = 0;
	    for( int i = skipArgN; i < argc; ++i, ++pos )
	    {
		vec[pos] = argv[i];
	    }
            Handle<Value> * ar = vec.empty() ? &vec[0] : 0;
	    return proxy( argv.This(), argc, ar );
	}
    };

}}} /* namespaces */

#endif /* CODE_GOOGLE_COM_P_V8_V8_FORWARDING_H_INCLUDED */