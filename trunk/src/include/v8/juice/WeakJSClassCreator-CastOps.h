/**
   See the docs below...
*/
#if ! defined(WEAK_CLASS_TYPE)
#error "You must define WEAK_CLASS_TYPE before including this file!"
#endif

namespace v8 { namespace juice { namespace convert {

    /**
       This is a "supermacro" for installing JSToNative and NativeToJS
       specializations for types wrapped using WeakJSClassCreator.

       To use this supermacro, do the following from global-scope
       code:

       \code
       #define WEAK_CLASS_TYPE myns::MyClass
       #include <v8/juice/WeakJSClassCreator-CastOps.h>
       \endcode

       and repeat for each class. There is no need to undef WEAK_CLASS_TYPE
       because this macro does that.
    */
    template <>
    struct NativeToJS< WEAK_CLASS_TYPE >
    {
	typedef ::v8::juice::WeakJSClassCreator< WEAK_CLASS_TYPE > WT;
	Handle<Value> operator()( WT::WrappedType * p ) const
	{
	    ::v8::Handle< ::v8::Object > jo( WT::GetJSObject(p) );
	    if( jo.IsEmpty() ) return Null();
	    else return jo;
	}
    };

    /**
       See NativeToJS< WEAK_CLASS_TYPE > for full details.
     */
    template <>
    struct JSToNative< WEAK_CLASS_TYPE >
    {
	typedef ::v8::juice::WeakJSClassCreator< WEAK_CLASS_TYPE >  WT;
	typedef WT::WrappedType * ResultType;
	ResultType operator()( Handle<Value> const & h ) const
	{
	    return WT::GetNative(h);
	}
    };

} } }

#undef WEAK_CLASS_TYPE
