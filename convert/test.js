/**
   v8::convert test/demo app for use with v8-juice-shell (or compatible).
   To run it without v8-juice you need to remove the loadPlugin() call,
   build an app which includes the ConvertDemo.cpp code, and run this
   script through v8.

   Note that many of these tests/assertions depend on default state
   set in the demo app (ConvertDemo.cpp), and changing them there might
   (should) cause these tests to fail.
*/
if( 'function' == typeof loadPlugin )
    (function(){
        var lp = loadPlugin("juice-plugin/v8-juice-ConvertDemo");
        print("Loaded v8-juice plugin. "+lp);
    })();


function printStackTrace(indention)
{
    var li = getStacktrace();
    if( li ) print('Stacktrace: '+JSON.stringify(li,0,indention));
}

function getCallLocation(framesBack){
    framesBack = (framesBack && (framesBack>0)) ? framesBack : 2;
    var li = getStacktrace(framesBack);
    if( !li ) return undefined;
    else return li[framesBack-1];
}


function assert(cond,msg)
{
    if( ! cond ) {
        msg = 'Assertion failed at (or around) line '+getCallLocation(3).line+': '+(msg||'')+
            '\nStacktrace: '+JSON.stringify(getStacktrace(),0,0);
        throw new Error(msg);
    }
    else {
        print("Assertion OK: "+msg);
    }
}

function asserteq(got,expect,msg)
{
    msg = msg || (got+' == '+expect);
    if(1) {
        if( got != expect ) {
            var st = getStacktrace(4);
            msg = 'Assertion failed at line '+st[1].line+': '+msg+
                '\nStacktrace: '+JSON.stringify(st,0,0);
            throw new Error(msg);
        }
        else print("Assertion OK: "+msg);
    }
    else {
        if( got != expect ) {
            throw new Error('Assertion failed: '+msg);
        }
        else print("Assertion OK: "+msg);
    }
        
}

function assertThrows( func ) {
    var ex = undefined;
    try { func(); }
    catch(e) { ex = e; }
    assert( !!ex, "Got expected exception: "+ex );
    if(0) printStackTrace(4);
}

function test1()
{
    var f = new BoundNative();
    print('f='+f);
    f.puts("hi, world");
    f.cputs("hi, world");
    f.doFoo();
    f.doFoo2(1,3.3);
    f.invoInt(1,2,3,4);
    f.doFooConst();
    f.nativeParam(f);
    f.runGC();

    f.overloaded();
    f.overloaded(1);
    f.overloaded(2,3);
    //assertThrows( function() { f.overloaded(1,2,3); } );
    f.overloaded(1,2,3);
    // We set up f.publicIntRO to throw on write.
    assertThrows( function(){ f.publicIntRO = 1;} );

    /* Note the arguably unintuitive behaviour when incrementing
       f.publicIntRO: the return value of the ++ op is the incremented
       value, but we do not actually modify the underlying native int.
       This is v8's behaviour, not mine.
    */
    asserteq( ++f.publicIntRW, 43 );
    asserteq( f.publicIntRO, f.publicIntRW );

    ++f.publicStaticIntRW;
    asserteq( f.publicStaticIntRW, f.publicStaticIntRO );
    asserteq( ++f.publicStaticIntRO, 1+f.publicStaticIntRW );
    asserteq( f.publicStaticIntRW, f.publicStaticIntRO );
    asserteq( ++f.publicStaticIntRO, 1+f.publicStaticIntRW );
    asserteq( f.publicStaticIntRW, 43 );
    asserteq( f.publicStaticIntRO, f.publicStaticIntRW );
    asserteq('hi, world', f.message);
    asserteq('hi, world', f.staticString);
    f.staticString = 'hi';
    asserteq('hi', f.staticString);
    asserteq( f.sharedString2, f.staticStringRO );
    f.sharedString2 = "hi again";
    asserteq( f.sharedString2, f.staticStringRO );
    assertThrows( function(){ f.staticStringRO = 'bye';} );

    asserteq(42, f.answer);
    assert( /BoundNative/.exec(f.toString()), 'toString() seems to work: '+f);

    asserteq( f.theInt, f.theIntNC );
    asserteq( ++f.theInt, f.theIntNC );
    asserteq( f.theInt, f.theIntNC );

    assertThrows( function(){ f.anton(); } );
    assertThrows( function(){ f.anton2(); } );
    assertThrows( function(){ f.nativeParamRef(null); } );
    assertThrows( function(){ f.nativeParamConstRef(null); } );
    f.nativeParamRef(f);
    f.nativeParamConstRef(f);
    print("Return native pointer : "+f.nativeReturn());
    print("Return native const pointer : "+f.nativeReturnConst());
    print("Return native reference : "+f.nativeReturnRef());
    print("Return native const ref : "+f.nativeReturnConstRef());
    asserteq( true, !!f.self );
    asserteq( true, !!f.selfConst );
    asserteq( f.self, f.selfConst );
    asserteq( f.selfRef, f.selfConst );
    asserteq( f.selfConstRef, f.self );
    assertThrows( function() { f.self = 'configured to throw when set.'; } );
    assertThrows( function() { f.selfConstRef = 'configured to throw when set.'; } );

    assert( f.destroy(), 'f.destroy() seems to work');
    assertThrows( function(){ f.doFoo();} );
}

function test2()
{
    var s = new BoundSubNative();
    assert(s instanceof BoundNative, "BoundSubNative is-a BoundNative");
    print('s='+s);
    assert( /BoundSubNative/.exec(s.toString()), 'toString() seems to work: '+s);
    asserteq(true, s.destroy(), 's.destroy()');
}

function test3()
{
    function MySub()
    {
        this.prototype = this.__proto__ = new BoundSubNative();
    }
    //MySub.constructor.prototype = BoundSubNative;
    var m = new MySub();
    assert(m instanceof BoundNative,'m is-a BoundNative');
    assert(m instanceof BoundSubNative,'m is-a BoundSubNative');
    m.puts("Hi from JS-side subclass");
    assert( /BoundSubNative/.exec(m.toString()), 'toString() seems to work: '+m);
    assert(m.destroy(), 'm.destroy()');
    assertThrows( function() {m.doFoo();} );

}

function test4()
{
    print("Creating several unreferenced objects and manually running GC...");
    var i =0;
    var max = 5;
    for( ; i < max; ++i ) {
        (i%2) ? new BoundSubNative() : new BoundNative();
    }
    for( i = 0; !BoundNative.prototype.runGC() && (i<max); ++i )
    {
        print("Waiting on GC to finish...");
        sleep(1);
    }
    if( max == i )
    {
        print("Gave up waiting after "+i+" iterations.");
    }
}

if(0) {
    /**
       Interesting: if we have a native handle in the global object
       then v8 is never GC'ing it, even if we dispose the context
       and run a V8::IdleNotification() loop. Hmmm.
     */
    var originalBoundObject = new BoundNative();
    print("Created object which we hope to see cleaned up at app exit: "+originalBoundObject);
}


test1();
test2();
test3();
//test4();
if(1) {
    try {
        asserteq(1,2,"Intentional error to check fetching of current line number.");
    }
    catch(e){
        print(e);
    }
    /** The line numbers printed out below should match the lines on which
        getCallLocation() is called. v8 uses 1-based rows/columns,
        by the way, not 1-based rows and 0-based columns as most editors do.
     */
    print( JSON.stringify(getCallLocation()) );
    print( JSON.stringify(getCallLocation()) );
    print( JSON.stringify(getCallLocation()) );
}
print("Done!");