#include "ConvertDemo.hpp"
#include "cvv8/ClassCreator.hpp"
#include "cvv8/properties.hpp"

#define TRY_ARGS_CODE 1
#if TRY_ARGS_CODE
#include "cvv8/arguments.hpp"
#endif


//char const * cvv8::TypeName< BoundNative >::Value = "BoundNative";
//char const * cvv8::TypeName< BoundSubNative >::Value = "BoundSubNative";

int BoundNative::publicStaticInt = 42;

void doFoo()
{
    CERR << "hi!\n";
}


void doNothing()
{
    CERR << "doNothing()!\n";
}

int doSomething(int x)
{
    CERR << "doSomething("<<x<<")!\n";
    return x;
}

ValueHandle sampleCallback( v8::Arguments const & argv )
{
    CERR << "sampleCallback() Arity=-1\n";
    return v8::Undefined();
}

void throwStdString()
{
    throw std::string("std::string thrown as an exception.");
}

namespace cvv8 {
    CVV8_TypeName_IMPL((BoundNative),"BoundNative");
    CVV8_TypeName_IMPL((BoundSubNative),"BoundSubNative");

    // A helper to support converting from BoundNative to its JS handle.
    typedef NativeToJSMap<BoundNative> BMap;
    typedef NativeToJSMap<BoundSubNative> BSubMap;
    
    BoundNative * ClassCreator_Factory<BoundNative>::Create( v8::Persistent<v8::Object> & jsSelf, v8::Arguments const & argv )
    {
        typedef cv::CtorArityDispatcher<BoundNativeCtors> Proxy;
        BoundNative * b = Proxy::Call( argv );
        if( b ) BMap::Insert( jsSelf, b );
        return b;
    }
    void ClassCreator_Factory<BoundNative>::Delete( BoundNative * obj )
    {
        BMap::Remove( obj );
        delete obj;
    }
    BoundSubNative * ClassCreator_Factory<BoundSubNative>::Create( v8::Persistent<v8::Object> & jsSelf, v8::Arguments const & argv )
    {
        BoundSubNative * b = new BoundSubNative;
        BSubMap::Insert( jsSelf, b );
        return b;
    }
    void ClassCreator_Factory<BoundSubNative>::Delete( BoundSubNative * obj )
    {
        BSubMap::Remove( obj );
        delete obj;
    }

}

v8::Handle<v8::Value> bogo_callback_arityN( v8::Arguments const & argv )
{
    CERR << "Arg count="<<argv.Length()<<'\n';
    return v8::Integer::New(argv.Length());
}
int bogo_callback2( v8::Arguments const & argv );

int16_t bogo_callback_int16( int16_t v )
{
    CERR << "int_16 overload: " << v<<".\n";
    return v;
}
int32_t bogo_callback_int32( int32_t v )
{
    CERR << "int_32 overload: " << v<<".\n";
    return v;
}
double bogo_callback_double( double v )
{
    CERR << "double overload: " << v<<".\n";
    return v;
}
int bogo_callback_array( v8::Handle<v8::Array> const & ar )
{
    assert( ! ar.IsEmpty() );
    int len = ar->Length();
    CERR << "array overload. length="<<len<<"\n";
    return len;
}
bool bogo_callback_object( v8::Handle<v8::Object> const & obj )
{
    CERR << "object overload.\n";
    assert( ! obj.IsEmpty() );
    return true;
}

v8::Handle<v8::Value> bogo_callback_function( v8::Handle<v8::Function> const & f )
{
    CERR << "function overload.\n";
    assert( ! f.IsEmpty() );
    return cv::CallForwarder<3>::Call( f, f, 3, 42.24, "hi" );
    //return f->Call( f, 0, NULL );
}

int bogo_callback2( v8::Arguments const & argv )
{
    CERR << "native this=@"<< (void const *) cv::CastFromJS<BoundNative>(argv.This())
         << ", arg count="<<argv.Length()
         << '\n';
    return 1;
}

char const * bogo_callback_fsf( v8::Handle<v8::Function> const & f1,
                                char const * str,
                                v8::Handle<v8::Function> const & f2 )
{
    CERR << "(Function, cstring, Function) overload.\n";
    assert( ! f1.IsEmpty() );
    assert( ! f2.IsEmpty() );
    f1->Call( f1, 0, NULL );
    CERR << str << '\n';
    f2->Call( f2, 0, NULL );
    return str;
}

v8::Handle<v8::Value> bogo_callback_fvf( v8::Handle<v8::Function> const & f1,
                                         v8::Handle<v8::Value> const &v,
                                         v8::Handle<v8::Function> const & f2 )
{
    CERR << "(Function, Value, Function) overload.\n";
    assert( ! f1.IsEmpty() );
    assert( ! v.IsEmpty() );
    assert( ! f2.IsEmpty() );
    f1->Call( f1, 0, NULL );
    CERR << cv::JSToStdString(v) << '\n';
    f2->Call( f2, 0, NULL );
    return v;
}

ValueHandle bogo_callback( v8::Arguments const & argv )
{
    CERR << "Arg count="<<argv.Length()
          << ". Dispatching based on predicate rules...\n";
    using namespace cvv8;

    /**
        Create some logic (via a Predicate template) to use in 
        associating an InCa with each set of dispatching rules....
    */

    typedef PredicatedInCa< ArgAt_IsA<0,int16_t>, FunctionToInCa<int16_t (int16_t), bogo_callback_int16> > PredIsaInt16;
    typedef PredicatedInCa< ArgAt_IsA<0,int32_t>, FunctionToInCa<int32_t (int32_t), bogo_callback_int32> > PredIsaInt32;
    typedef PredicatedInCa< ArgAt_IsA<0,double>, FunctionToInCa<double (double), bogo_callback_double> > PredIsaDouble;
    typedef PredicatedInCa< ArgAt_IsArray<0>, FunctionToInCa<int (v8::Handle<v8::Array> const &), bogo_callback_array> > PredIsaArray;
    typedef PredicatedInCa< ArgAt_IsObject<0>, FunctionToInCa<bool (v8::Handle<v8::Object> const &), bogo_callback_object> > PredIsaObject;
    typedef PredicatedInCa< ArgAt_IsFunction<0>,
            FunctionToInCa<v8::Handle<v8::Value>  (v8::Handle<v8::Function> const &), bogo_callback_function>
    > PredIsaFunction;

    // Group the rules into a PredicatedInCaDispatcher "container".
    typedef PredicatedInCaDispatcher< CVV8_TYPELIST((
        // The order IS significant for overloads which can evaluate ambiguously,
        // e.g. Int16/Int32/Double.
        PredIsaFunction, PredIsaArray, PredIsaObject, PredIsaInt16, PredIsaInt32, PredIsaDouble
    ))> ByTypeOverloads;

    // Create a parent rule which only checks ByTypeOverloads if called
    // with 1 argument:
    typedef PredicatedInCa< Argv_Length<1>, ByTypeOverloads > Group1;

    // Set up some other logic paths...

    // For 2 arguments:
    typedef PredicatedInCa< Argv_Length<2>, InCaLikeFunction<int,bogo_callback2> > Group2;

    // For 0 or 3-5 args
    typedef PredicatedInCa<
        Argv_Or< Argv_Length<0>, Argv_Length<3,5> >,
        InCaToInCa<bogo_callback_arityN>
    > GroupN;

    // Special case for the weird (Function, cstring, Function) overload...
    typedef Argv_AndN< CVV8_TYPELIST((
            Argv_Length<3>,
            ArgAt_IsFunction<0>,
            ArgAt_IsString<1>,
            ArgAt_IsFunction<2>
        ))> Is_Func_String_Func;
    typedef PredicatedInCa< Is_Func_String_Func,
            FunctionToInCa< char const * (
                                v8::Handle<v8::Function> const &,
                                char const *,
                                v8::Handle<v8::Function> const &),
                            bogo_callback_fsf
                        >
    > PredFSF;

    // Special case for the weird (Function, Value, Function) overload...
    typedef Argv_AndN< CVV8_TYPELIST((
        Argv_Length<3>,
        ArgAt_IsFunction<0>,
        ArgAt_IsFunction<2>
    ))> Is_Func_Value_Func;
    typedef PredicatedInCa< Is_Func_Value_Func,
                            FunctionToInCa< v8::Handle<v8::Value> (
                                v8::Handle<v8::Function> const &,
                                v8::Handle<v8::Value> const &,
                                v8::Handle<v8::Function> const & ),
                            bogo_callback_fvf
                        >
    > PredFVF;

    /*
        Reminder to self: i would like to restructure the two "funky" 3-arg
        overloads to a structure which looks like:

         (argc==3 && (Is_Func_String_Func || Is_Func_Value_Func))

        Because that's a tad bit more efficient.

        But the overloading mechanism cannot backtrack, so we then would
        shadow the other 3-arg overload(s).

        We could certainly handle back-tracking by modeling the dispatcher
        to be more like a PEG parser, where the atomic tokens are the
        Arguments entries and the actions are the InvocationCallbacks.
    */

    // Now create the "top-most" callback, which performs the above-defined
    // dispatching at runtime:
    typedef PredicatedInCaDispatcher< CVV8_TYPELIST((
            PredFSF, PredFVF, Group1, Group2, GroupN
    ))> AllOverloads;

    // Everything up to here is done at compile-time, by the way.
    // Now we can do what we've been working towards all along:
    // dispatching the callback:
    return AllOverloads::Call(argv);
}



ValueHandle BoundNative_toString( v8::Arguments const & argv )
{
    /*
      INTERESTING: the following JS code calls this function but we
      have a NULL 'this'.

      function test2()
      {
          var s = new BoundSubNative();
          assert(s instanceof BoundNative, "BoundSubNative is-a BoundNative");
          print('s='+s);
          s.destroy();
          // do not do this at home, kids: i'm testing weird stuff here...
          var f = new BoundNative();
          s.toString = f.toString;
          print('f='+f);
          print('s='+s); // <---- HERE
      }

      That happens, i think, because CastFromJS<BoundNative>()
      does not recognize BoundSubNative objects. Why not? To be honest, i'm
      not certain.
    */
    BoundNative * f = cv::CastFromJS<BoundNative>(argv.This());
    return cv::StringBuffer() << "[object BoundNative@"<<f<<"]";
}

v8::Handle<v8::Value> bind_BoundSubNative( v8::Handle<v8::Object> dest );
char const * cstring_test( char const * c )
{
    std::cerr << "cstring_test( @"<<(void const *)c
              <<") ["<<(c ? c : "<NULL>")<<"]\n";
    return c;
}

std::string sharedString("hi, world") /* may not be static for templating reasons. */;
std::string getSharedString()
{
    CERR << "getSharedString()=="<<sharedString<<'\n';
    return sharedString;
}
void setSharedString(std::string const &s)
{
    CERR << "setSharedString("<<s<<")\n";
    sharedString = s;
}



template <bool IsUsingUnlock>
void test_using_locker()
{
    CERR << "Callback "<<(IsUsingUnlock?"with":"without")
        << " cvv8's Unlocker support. "
        << "Briefly locking v8... "
        << "If something is broken in our locking setup then the "
        << "following will likely assert in v8...\n";
    {
        v8::Locker const lock;
    }
    CERR << "We're back...\n";
}

namespace cvv8 {



    template <>
    struct ClassCreator_SetupBindings<BoundNative>
    {
        static void SetupBindings( v8::Handle<v8::Object> const & dest )
        {
            using namespace v8;

            ////////////////////////////////////////////////////////////
            // Bootstrap class-wrapping code...
            typedef cv::ClassCreator<BoundNative> CC;
            CC & cc( CC::Instance() );
            if( cc.IsSealed() )
            {
                cc.AddClassTo( "BoundNative", dest );
                bind_BoundSubNative(dest);
                return;
            }

            typedef CVV8_TYPELIST((
                cv::MethodToInCa<BoundNative, void(), &BoundNative::overload0>,
                cv::MethodToInCa<BoundNative, void(int), &BoundNative::overload1>,
                //cv::InCaToInCa< cv::MethodToInvocationCallback<BoundNative, void(int,int), &BoundNative::overload2> >
                cv::MethodToInCa<BoundNative, void(int,int), &BoundNative::overload2>,
                cv::MethodToInCa<BoundNative, void(v8::Arguments const &), &BoundNative::overloadN>
            )) OverloadList;
            typedef cv::ArityDispatchList< OverloadList > OverloadInCas;

            if(1) // just an experiment
            {
                typedef Signature<void (int,double)> AL2;
                assert( 2 == sl::Arity<AL2>::Value );
                assert( 2 == sl::Length<AL2>::Value );
                assert( (tmp::SameType< void, AL2::ReturnType >::Value) );
                assert( (tmp::SameType< int, sl::At<0,AL2>::Type >::Value) );
                assert( (tmp::SameType< double, sl::At<1,AL2>::Type >::Value) );
                //assert( (tmp::SameType< double, AL2::TypeAt<1>::Type >::Value) );
                
                typedef cv::FunctionPtr< int(char const *), ::puts> FPPuts;
                FPPuts::Function("Hi, world.");
                typedef Signature< FPPuts::FunctionType > ALPuts;
                assert( 1 == sl::Arity<ALPuts>::Value );
                
                typedef
                    Signature< int (BoundNative::*)(char const *) const >
                    BNPutsC;
                typedef
                    ConstMethodSignature< BoundNative, int (BoundNative::*)(char const *) const >
                    BNPutsC2;
                typedef
                    MethodSignature< BoundNative, int (char const *) >
                    BNPutsC3;
                using cv::tmp::Assertion;
                Assertion<true> ass;
#define ASS ass = Assertion
                ASS<BNPutsC::IsConst>();
                ASS<BNPutsC2::IsConst>();
                ASS<!BNPutsC3::IsConst>();

                typedef Signature< int (BoundNative::*)(char const *) > BNPuts;
                ASS< 1 == sl::Length<BNPutsC>::Value >();
                ASS< 1 == sl::Length<BNPuts>::Value >();
                ASS< !BNPuts::IsConst >();
                ASS< tmp::SameType< char const *, sl::At<0,BNPuts>::Type >::Value >();
                ASS< 0 == sl::Index< char const *, BNPuts >::Value >();
#undef ASS
            }
#define CATCHER cv::InCaCatcher_std
            ////////////////////////////////////////////////////////////
            // Bind some member functions...
            cc("cputs",
               cv::FunctionToInCa<int (char const *),::puts>::Call )
                ("overloaded",
                  OverloadInCas::Call )
                ("doFoo",
                 cv::MethodToInCa<BoundNative,void (),&BoundNative::doFoo>::Call)
                ("doFoo2",
                 cv::MethodToInCa<BoundNative,double (int,double),&BoundNative::doFoo2>::Call)
                ("toString",
                 cv::FunctionToInCa<ValueHandle (v8::Arguments const &),BoundNative_toString>::Call)
                ("puts",
                 cv::ConstMethodToInCa<BoundNative,void (char const *),&BoundNative::puts>::Call)
                ("doFooConst",
                 cv::ConstMethodToInCa<BoundNative,void (),&BoundNative::doFooConst>::Call)
                ("invoInt",
                 cv::MethodToInCa<BoundNative, int (v8::Arguments const &), &BoundNative::invoInt>::Call)
                 //cv::ToInCa<BoundNative, int (v8::Arguments const &), &BoundNative::invoInt,true>::Call) // this must fail to compile
                ("nativeParam",
                 cv::MethodToInCa<BoundNative, void (BoundNative const *), &BoundNative::nativeParam>::Call)
                ("nativeParamRef",
                 CATCHER< cv::MethodToInCa<BoundNative, void (BoundNative &), &BoundNative::nativeParamRef> >::Call)
                ("nativeParamConstRef",
                 CATCHER< cv::ConstMethodToInCa<BoundNative, void (BoundNative const &), &BoundNative::nativeParamConstRef> >::Call)
                 // Hopefully someday:
                 //CATCHER< cv::MethodToInCa<BoundNative, void (BoundNative const &) const, &BoundNative::nativeParamConstRef> >::Call)
                ("cstr",
                 //cv::FunctionToInvocationCallback< char const * (char const *), cstring_test>)
                 cv::FunctionToInCa< char const * (char const *), cstring_test>::Call)
                ("destroy", CC::DestroyObjectCallback )
                ("message", "hi, world")
                ("answer", 42)
                ("throwStdString",
                    cv::InCaCatcher<std::string,
                        char const * (),
                        &std::string::c_str,
                        cv::FunctionToInCa< void (), throwStdString >
                    >::Call)
#if 1 // converting natives to JS requires more lower-level plumbing than converting from JS to native...
                 ("nativeReturn",
                 cv::MethodToInCa<BoundNative, BoundNative * (), &BoundNative::nativeReturn, true>::Call)
                 ("nativeReturnConst",
                 cv::ConstMethodToInCa<BoundNative, BoundNative const * (), &BoundNative::nativeReturnConst, true>::Call)
                 // Hopefully someday:
                 // cv::MethodToInCa<BoundNative, BoundNative const * () const, &BoundNative::nativeReturnConst>::Call)
                 ("nativeReturnRef",
                 CATCHER< cv::MethodToInCa<BoundNative, BoundNative & (), &BoundNative::nativeReturnRef, true> >::Call)
                 ("nativeReturnConstRef",
                 CATCHER< cv::ConstMethodToInCa<BoundNative, BoundNative const & (), &BoundNative::nativeReturnConstRef, true> >::Call)
                 // Hopefully someday:
                 //CATCHER< cv::MethodToInCa<BoundNative, BoundNative const & () const, &BoundNative::nativeReturnConstRef> >::Call)
#endif
                ;
#undef CATCHER
            ////////////////////////////////////////////////////////////////////////
            // We can of course bind them directly to the prototype, instead
            // of via the cc object:
            Handle<ObjectTemplate> const & proto( cc.Prototype() );
            proto->Set(JSTR("bogo"),
                       cv::CastToJS(cv::FunctionToInCa<ValueHandle (v8::Arguments const &), bogo_callback>::Call)
                       );
            proto->Set(JSTR("bogo2"),
                       cv::CastToJS(cv::FunctionToInCa<int (v8::Arguments const &),bogo_callback2>::Call)
                       );
            proto->Set(JSTR("runGC"),
                       cv::CastToJS(cv::FunctionToInCa<bool (),v8::V8::IdleNotification>::Call)
                       );
            ////////////////////////////////////////////////////////////////////////
            // Bind some JS properties to native properties:
            typedef cv::MemberPropertyBinder<BoundNative> PB;
#if 0 // refactoring these...
            PB::BindMemVar<int,&BoundNative::publicInt>( "publicIntRW", proto );
            PB::BindMemVarRO<int,&BoundNative::publicInt>( "publicIntRO", proto, true );
#endif
            typedef BoundNative T;
            // More generically, accessors can be bound using this approach:
            proto->SetAccessor( JSTR("self"),
                                PB::MethodToAccessorGetter< T * (), &T::self>,
                                ThrowingSetter::Set );
            proto->SetAccessor( JSTR("selfRef"),
                                PB::MethodToAccessorGetter< T & (), &T::selfRef>,
                                ThrowingSetter::Set );
            proto->SetAccessor( JSTR("selfConst"),
                                PB::ConstMethodToAccessorGetter< T const * (), &T::self>,
                                ThrowingSetter::Set );
            proto->SetAccessor( JSTR("selfConstRef"),
                                PB::ConstMethodToAccessorGetter< T const & (), &T::selfRefConst>,
                                ThrowingSetter::Set );

            proto->SetAccessor( JSTR("publicIntRW"),
                                MemberToGetter<T,int,&T::publicInt>::Get,
                                MemberToSetter<T,int,&T::publicInt>::Set );
            proto->SetAccessor( JSTR("publicIntRO"),
                                MemberToGetter<T,int,&T::publicInt>::Get,
                                ThrowingSetter::Set );
            proto->SetAccessor( JSTR("publicStaticIntRW"),
                                VarToGetter<int,&T::publicStaticInt>::Get,
                                VarToSetter<int,&T::publicStaticInt>::Set );
            proto->SetAccessor( JSTR("publicStaticIntRO"),
                                VarToGetter<int,&T::publicStaticInt>::Get,
                                ThrowingSetter::Set );
            proto->SetAccessor( JSTR("staticString"),
                                VarToGetter<std::string,&sharedString>::Get,
                                VarToSetter<std::string,&sharedString>::Set );
            proto->SetAccessor( JSTR("staticStringRO"),
                                VarToGetter<std::string,&sharedString>::Get,
                                ThrowingSetter::Set );
            
#if 0
#if 0
            PB::BindGetterFunction<std::string (), getSharedString>("sharedString2", proto);
#else
            PB::BindGetterSetterFunctions<std::string (),
                getSharedString,
                void (std::string const &),
                setSharedString>("sharedString2", proto);
#endif
            PB::BindGetterSetterMethods<int (), &BoundNative::getInt,
                void (int), &BoundNative::setInt
                >("theInt", proto);
            PB::BindNonConstGetterSetterMethods<int (), &BoundNative::getIntNonConst,
                void (int), &BoundNative::setInt
                >("theIntNC", proto);
#endif
            v8::Handle<v8::Function> ctor( cc.CtorFunction() );
            ctor->Set(JSTR("testLocker"),
                cv::CastToJS(cv::FunctionToInCa<void (), test_using_locker<true>, true >::Call)
            );
            ctor->Set(JSTR("testLockerNoUnlocking"),
                cv::CastToJS(cv::FunctionToInCa<void (), test_using_locker<false>, false>::Call)
            );

            ////////////////////////////////////////////////////////////
            // Add class to the destination object...
            //dest->Set( JSTR("BoundNative"), cc.CtorFunction() );
            cc.AddClassTo( "BoundNative", dest );

            CERR << "Added BoundNative to JS.\n";
            if(1)
            { // sanity checking. This code should crash if the basic stuff is horribly wrong
                Handle<Value> vinst = cc.NewInstance(0,NULL);
                BoundNative * native = cv::CastFromJS<BoundNative>(vinst);
                assert( 0 != native );
                CERR << "Instantiated native BoundNative@"<<(void const *)native
                     << " via JS.\n";
                CC::DestroyObject( vinst );
            }
            bind_BoundSubNative(dest);
            CERR << "Finished binding BoundNative.\n";
        }
    };
}

void BoundNative::SetupBindings( v8::Handle<v8::Object> const & dest )
{
    cv::ClassCreator<BoundNative>::Instance().SetupBindings(dest);
}

v8::Handle<v8::Value> bind_BoundSubNative( v8::Handle<v8::Object> dest )
{
    using namespace v8;
    typedef cv::ClassCreator<BoundSubNative> CC;
    CC & cc( CC::Instance() );
    if( cc.IsSealed() )
    {
        cc.AddClassTo( "BoundSubNative", dest );
        return cc.CtorFunction();
    }

    cc
        ("subFunc",
         cv::ConstMethodToInCa<BoundSubNative,void (),&BoundSubNative::subFunc>::Call)
        ("toString",
         cv::ConstMethodToInCa<BoundSubNative,ValueHandle (),&BoundSubNative::toString>::Call)
         ("nonBoundNative",
         cv::MethodToInCaVoid<BoundSubNative, BoundSubNative::NonBoundType & (), &BoundSubNative::nonBoundNative>::Call)
         ("nonBoundNativeConst",
         cv::ConstMethodToInCaVoid<BoundSubNative, BoundSubNative::NonBoundType const& (), &BoundSubNative::nonBoundNative>::Call)
         //cv::ToInCa<BoundSubNative, BoundSubNative::NonBoundType &(), &BoundSubNative::nonBoundNative>::Call) // must fail to compile OR link
         ("puts",
         cv::FunctionToInCa<int (char const *),::puts>::Call)
        ;

    //typedef cv::ClassCreator<BoundNative> CCFoo;
    //cc.CtorTemplate()->Inherit( CCFoo::Instance().CtorTemplate() );
    cc.Inherit<BoundNative>();

    cc.AddClassTo("BoundSubNative",dest);
    return dest;
}
#undef JSTR


namespace { // testing ground for some compile-time assertions...
    struct CtorFwdTest
    {
        CtorFwdTest(int){}
        CtorFwdTest(){}
        CtorFwdTest(int,int){}
        CtorFwdTest(v8::Arguments const &) {}
        virtual ~CtorFwdTest() {}
        
        int afunc(int);
        int bfunc(int,int) const;
    };

    struct CtorFwdTestSub : CtorFwdTest
    {
        CtorFwdTestSub(){}
        virtual ~CtorFwdTestSub() {}
    };

    void compile_time_assertions()
    {
        namespace tmp = cv::tmp;
        namespace sl = cv::sl;
#define ASS ass = cv::tmp::Assertion
        tmp::Assertion<true> ass;
        ASS< (0 > sl::Arity< cv::MethodToInCa<BoundNative, int (v8::Arguments const &), &BoundNative::invoInt> >::Value)>();
        typedef CtorFwdTest CFT;
        typedef cv::CtorForwarder<CFT * ()> C0;
        //typedef cv::CtorForwarder<CFT, CtorFwdTestSub *()> C0Sub;
        typedef cv::CtorForwarder<CFT * (int)> C1;
        typedef cv::CtorForwarder<CFT * (int,int)> C2;
        typedef cv::CtorForwarder<CFT * (v8::Arguments const &)> Cn;
        ASS<( -1 == sl::Arity< Cn >::Value )>();
        typedef CFT * (*CFTCtor)( v8::Arguments const & );
        CFTCtor ctor;
        ctor = C0::Call;
        ctor = C1::Call;
        ctor = C2::Call;
        //ctor = C0Sub::Ctor;
        typedef cv::Signature< CFT (C0, C1, C2) > CtorList;
        typedef cv::CtorArityDispatcher<CtorList> CDispatch;
        typedef CtorFwdTest * (*FacT)( v8::Arguments const &  argv );
        FacT fac;
        fac = CDispatch::Call;
        typedef int (CFT::*M1)(int) ;
        typedef int (CFT::*M2)(int,int) const;
        ASS<( !(tmp::IsConst<CFT>::Value) )>();
        ASS<( (tmp::IsConst<CFT const>::Value) )>();
        ASS<( 1 == (sl::Arity< cv::Signature<M1> >::Value) )>();
        ASS<( 2 == (sl::Arity< cv::ConstMethodToInCa<CFT, int (int,int), &CFT::bfunc> >::Value) )>();
        typedef int (CFT::*X2)(int,int) const;
        ASS<( 2 == (sl::Arity< cv::Signature<X2> >::Value) )>();

        ASS<( !(cv::Signature< M1 >::IsConst) )>();
        ASS<( (cv::Signature< M2 >::IsConst) )>();
        ASS<( 2 == (sl::Arity< cv::Signature< M2 > >::Value) )>();
        ASS<( !(cv::Signature< int (int) >::IsConst) )>();
        ASS<( (cv::Signature< int (CFT::*)() const >::IsConst) )>();
        ASS<( (cv::Signature<X2>::IsConst) )>();
        ASS<( !(cv::Signature<int (int)>::IsConst) )>();
        ASS<( 1 == (sl::Arity< cv::MethodToInCa<CFT,M1,&CFT::afunc> >::Value) )>();
        ASS<( 1 == (sl::Arity< cv::MethodToInCa<CFT,int (int),&CFT::afunc> >::Value) )>();
        ASS<( 2 == (sl::Arity< cv::ConstMethodToInCa<CFT,M2,&CFT::bfunc> >::Value) )>();
        ASS<( 2 == (sl::Arity< cv::ConstMethodToInCa<CFT,int (int,int),&CFT::bfunc> >::Value) )>();
        //ASS<( 2 == (cv::ToInCa<CFT,int (int,int),&CFT::bfunc>::Arity) )>();
        ASS<( 1 == (sl::Arity< cv::MethodToInCa<CFT,M1,&CFT::afunc> >::Value) )>();
        ASS<( 2 == (sl::Arity< cv::ConstMethodToInCa<CFT,M2,&CFT::bfunc> >::Value) )>();
        ASS<( 2 == (sl::Arity< cv::ConstMethodToInCa<CFT,int (int,int),&CFT::bfunc> >::Value) )>();
        //ASS<( 2 == (cv::ToInCa<CFT,M2,&CFT::bfunc>::Arity) )>();
        //typedef cv::FunctionSignature<FacT> FacSig;
        typedef cv::FunctionSignature< CtorFwdTest * ( v8::Arguments const &  argv )> FacSig;
        ASS<( sl::Arity<FacSig>::Value < 0 )>();
        ASS<( (tmp::IsConst<cv::sl::At< 0, FacSig >::Type>::Value) )>();
        ASS<( (tmp::SameType< v8::Arguments const &, cv::sl::At< 0, cv::Signature<CtorFwdTest * ( v8::Arguments const &  argv )> >::Type>::Value))>();
        ASS<( (tmp::SameType< v8::Arguments const &, cv::sl::At< 0, FacSig >::Type>::Value))>();
        typedef cv::sl::At< 0, cv::FunctionSignature<int (int)> >::Type A0;
        ASS<( (tmp::SameType< int, A0>::Value))>();
        typedef cv::sl::At< 0, cv::FunctionToInCa<int (char const *),::puts> >::Type A1;
        ASS<( (tmp::SameType< char const *, A1>::Value))>();

        typedef cv::Signature< void (int, double, char const *) > CanUnlock;
        typedef cv::Signature< void (int, v8::Handle<v8::Value>, double) > CannotUnlock;
        typedef cv::Signature< void (v8::Handle<v8::Value>, double, int) > CannotUnlock2;
        ASS< 3 == cv::sl::Length< CannotUnlock >::Value >();
        ASS< 3 == cv::sl::Length< CannotUnlock2 >::Value >();
        ASS< cv::IsUnlockable<void>::Value >();
        ASS< cv::IsUnlockable<double>::Value>();
        ASS< cv::IsUnlockable<int>::Value>();
        ASS< !cv::IsUnlockable< v8::Handle<v8::Value> >::Value >();
        ASS< !cv::IsUnlockable< v8::Arguments >::Value >();
        ASS<cv::TypeListIsUnlockable<CanUnlock>::Value>();
        ASS<!cv::TypeListIsUnlockable<CannotUnlock2>::Value>();
        ASS<!cv::TypeListIsUnlockable<CannotUnlock>::Value>();
        typedef cv::Signature< void (v8::Handle<v8::Value>::*)(int) > CtxSig;
        ASS<!cv::SignatureIsUnlockable<CtxSig>::Value>();
        ASS<cv::TypeListIsUnlockable<CtxSig>::Value>();
#define SIU cv::SignatureIsUnlockable
        ASS< SIU< cv::Signature<int (int, double, char)> >::Value >();
        ASS< !SIU< cv::Signature<int (v8::Arguments)> >::Value >();
        ASS< !SIU< cv::Signature<int (int, v8::Arguments)> >::Value >();
        ASS< !cv::IsUnlockable< v8::Handle<v8::Object> >::Value >();
        ASS< !SIU< cv::Signature<v8::Handle<v8::Object> (int, double)> >::Value >();
        ASS< !SIU< 
                cv::MethodPtr<
                    BoundNative,
                    int (v8::Arguments const &),
                    &BoundNative::invoInt
                > >::Value >();
        ASS< !SIU< cv::MethodToInCa<BoundNative, int (v8::Arguments const &), &BoundNative::invoInt > >::Value>();
        

        v8::InvocationCallback cb;
        cb = cv::InCaLikeMethod<BoundNative, int, &BoundNative::invoInt>::Call;
        cb = cv::InCaLikeConstMethod<BoundNative, int, &BoundNative::invoIntConst>::Call;
        //cb = cv::InCaLike<BoundNative, int, &BoundNative::invoInt>::Call;
        //cb = cv::InCaLike<BoundNative, int, &BoundNative::invoIntConst>::Call;
        ASS< -1 == sl::Arity< cv::InCaLikeMethod<BoundNative, int, &BoundNative::invoInt> >::Value >();
        ASS< -1 == sl::Arity< cv::InCaLikeConstMethod<BoundNative, int, &BoundNative::invoIntConst> >::Value >();
        
        typedef cv::Signature<void (char, double, int)> TList1;
        ASS< (0 == cv::sl::Index<char, TList1>::Value) >();
        ASS< (1 == cv::sl::Index<double, TList1>::Value) >();
        ASS< (2 == cv::sl::Index<int, TList1>::Value) >();
        ASS< (0 > cv::sl::Index<uint32_t, TList1>::Value) >();
        ASS< (0 > cv::sl::Index<uint32_t, cv::Signature<void ()> >::Value) >();

        typedef cv::FunctionToInCa<int (char const *),::puts> PutsInCa;
        ASS< 1 == sl::Arity< PutsInCa >::Value >();
        ASS< 1 == sl::Arity< cv::Signature<PutsInCa::FunctionType> >::Value >();
        ASS< -1 == sl::Arity< cv::Signature<v8::InvocationCallback> >::Value >();
#undef SIU
#undef ASS
    }

    void test_new_typelist()
    {
        using namespace cvv8;
        using namespace cvv8::tmp;

        typedef void (RawSignature)(int, double, char);
        typedef Signature< RawSignature > BLSig;
        typedef Signature< void (char, int) > BL2;
        typedef Signature< void () > BL0;
        typedef FunctionSignature< void (int, double, v8::Arguments const &, char) > BL4a;
        typedef FunctionSignature< void (int, double, char, v8::Arguments const &) > BL4b;
        typedef FunctionSignature< void (int, double, int, char) > BL4c;
        typedef Signature<void (v8::Arguments const &)> ICSig;
        tmp::Assertion<true> ass;
#define ASS ass = tmp::Assertion
        ASS< -1 == sl::Arity< ICSig >::Value >();
        ASS< 2 == sl::Arity< BL2 >::Value >();
        ASS< 0 == sl::Arity< BL0 >::Value >();
        ASS< 3 == sl::Arity< BLSig >::Value >();
        ASS< 4 == sl::Arity< BL4a >::Value >();
        ASS< 4 == sl::Arity< BL4c >::Value >();
        ASS< 4 == sl::Arity< BL4b >::Value >();
        ASS< tmp::SameType< sl::At<2,BLSig>::Type, char >::Value >();
        ASS< 0 == sl::Length<BL0>::Value >();
        ASS< tmp::SameType< double, sl::At<1,BLSig>::Type >::Value >();
        ASS< sl::Arity<BLSig>::Value == sl::Length<BLSig>::Value >();
        ASS< 2 == sl::Length<BL2>::Value >();
        ASS< tmp::SameType< int, sl::At<1,BL2>::Type >::Value >();
        ASS< !tmp::SameType< int, sl::At<0,BL2>::Type >::Value >();
        
        ASS< tmp::SameType< int, sl::At<0,BL4a>::Type >::Value >();
        ASS< tmp::SameType< double, sl::At<1,BL4a>::Type >::Value >();
        ASS< tmp::SameType< char, sl::At<3,BL4a>::Type >::Value >();
        ASS< tmp::SameType< v8::Arguments const &, sl::At<2,BL4a>::Type >::Value >();
        
        ASS< tmp::SameType< int, sl::At<0,BL4c>::Type >::Value >();
        ASS< tmp::SameType< double, sl::At<1,BL4c>::Type >::Value >();
        ASS< tmp::SameType< int, sl::At<2,BL4c>::Type >::Value >();
        ASS< tmp::SameType< char, sl::At<3,BL4c>::Type >::Value >();
        
        typedef FunctionSignature< void (v8::Arguments const &) > BLA1;
        typedef FunctionSignature< void (v8::Arguments const &, char) > BLA2a;
        typedef FunctionSignature< void (char, v8::Arguments const &) > BLA2b;
        ASS< tmp::SameType< char, sl::At<0,BLA2b>::Type >::Value >();
        ASS< tmp::SameType< v8::Arguments const &, sl::At<0,BLA2a>::Type >::Value >();
        ASS< tmp::SameType< v8::Arguments const &, sl::At<1,BLA2b>::Type >::Value >();
        
        ASS< tmp::SameType< int, sl::At<0,BL4b>::Type >::Value >();
        ASS< tmp::SameType< double, sl::At<1,BL4b>::Type >::Value >();
        ASS< tmp::SameType< char, sl::At<2,BL4b>::Type >::Value >();
        ASS< tmp::SameType< v8::Arguments const &, v8::Arguments const & >::Value >();
        ASS< tmp::SameType< v8::Arguments const &, sl::At<3,BL4b>::Type >::Value >();

        ASS< Signature< int (BoundNative::*)( int, int ) const >::IsConst >();
        ASS< ! Signature< int ( int, int ) >::IsConst >();
        ASS< ! Signature< int (CtorFwdTest::*)( int, int ) >::IsConst >();
        //ASS< tmp::SameType< SigList_At<SigList_Length<BL3>::Value,BL3>::Type, char >::Value >(); // must fail to compile

        // damn. ASS< tmp::IsConst< int (void) const >::Value >();
        ASS< !tmp::IsConst< int (void) >::Value >();

        ASS< sl::IsFunction< BLA1 >::Value >();
        ASS< !sl::IsMethod< BLA1 >::Value >();

        {
            typedef Signature< void (v8::Arguments::*)(int,double) > BogoMethod;
            ASS< !sl::IsFunction< BogoMethod >::Value >();
            ASS< sl::IsMethod< BogoMethod >::Value >();
            ASS< sl::IsNonConstMethod< BogoMethod >::Value >();
            ASS< !sl::IsConstMethod< BogoMethod >::Value >();

            typedef Signature< void (v8::Arguments::*)(int,double) const > BogoConstMethod;
            ASS< !sl::IsFunction< BogoConstMethod >::Value >();
            ASS< sl::IsMethod< BogoConstMethod >::Value >();
            ASS< !sl::IsNonConstMethod< BogoConstMethod >::Value >();
            ASS< sl::IsConstMethod< BogoConstMethod >::Value >();
        }
#undef ASS
    }

#if TRY_ARGS_CODE
    void test_args_code()
    {
        using namespace cvv8;
        typedef ArgAt<0> AA0;
    }
#endif
} // namespace


#undef CERR
#undef JSTR
#undef TRY_ARGS_CODE