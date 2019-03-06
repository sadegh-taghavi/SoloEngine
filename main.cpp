//#include <QDebug>
//#include <QElapsedTimer>
#include "solo/Solo.h"

struct SA
{
    int aaa;
    double bbb;
    char STr[25];
    SA() {}
};

class Trd: public S_RunnableThread
{
public:
    void run();

};
void Trd::run()
{
    s_debug( "FROM THREADDDDDDDDDDDDDDDDDDDDDDD11111111" );
    S_Thread::sleep( 3000 );
    s_debug( "FROM THREADDDDDDDDDDDDDDDDDDDDDDD22222222" );
}

void test()
{
    SA *iv = new SA();

    Trd th;
    th.start();

    S_String strvvv = "Test";
    strvvv += " test222";
    strvvv += " vvvv";
    S_List<SA> lst;
    SA vv;
    uint64_t **ttt = new uint64_t*[500000];

    S_ElapsedTime et;

    et.start();

    for( int i = 0; i < 500000; ++i )
    {
        lst.push_back( vv );
    }

    s_debug( "ETA", et.restart() );

    for( int i = 0; i < 500000; ++i )
    {
        lst.pop_front();
    }

    s_debug( "ETV", et.restart() );

    for( int j = 0; j < 10; ++j )
    {
        s_debug( "ET###########", j,  et.restart() );
        for( int i = 0; i < 500000; ++i )
        {
            ttt[i] = new uint64_t();
        }
        s_debug( "AL-CA-Al", et.restart() );

        for( int i = 5000; i < 60000; ++i )
        {
            delete ttt[i];
        }

        s_debug( "DA-CA-De", et.restart() );


        for( int i = 0; i < 500000; ++i )
        {
            ttt[i] = (uint64_t *)malloc( sizeof(uint64_t) );
        }
        s_debug( "AL-MA-Al", et.restart() );

        for( int i = 5000; i < 60000; ++i )
        {
            free( ttt[i] );
        }
        s_debug( "AL-MA-De", et.restart() );
    }

    s_debug( "Test", S_Allocator::singleton()->getTotalAllocatedItems() ,
             S_Allocator::singleton()->getTotalUsedPools() , S_Allocator::singleton()->getTotalAllocatedBytes(),
             (iv->aaa), strvvv, sizeof( uint64_t ) );
    s_debug( "Invocation" , S_Allocator::singleton()->getTotalAllocateInvoked(),
             S_Allocator::singleton()->getTotalDeallocateInvoked(), S_Thread::hardwareThreadCount(), th.isRunning() );

    delete iv;

    S_Vec3 v3 = S_Vec3( 3.1415f, 0.1415f, 0.0f );
    S_Mat4x4 m;
    m.translate( S_Vec3( 0, 0, 0 ) );
    m.identity();

    s_debug( S_Vec2(12, 10).length(), S_Vec3( 25, 10, 5 ).length() ,
             S_Vec4( 25, 10, 5, 5 ).length() , S_Quat().fromEularAnglesPYR( &v3 ).x() ,
             m(1, 1) );

}

#ifdef __ANDROID__

#include "android_native_app_glue.h"

int main(int, char **);

void android_main(struct android_app* app)
{
    android_poll_source* lSource;
    android_app* application= app;
    //    application->onAppCmd= activity_event_callback;
    //    application->onInputEvent = callback_input;

    test();

    app_dummy();

    while (true) {
        // Loop the events accumulated (it can be several)
        int lResult;
        int lEvents;
        while ((lResult = ALooper_pollAll(16, NULL, &lEvents, (void**)&lSource)) >= 0)
        {
            if (lSource != NULL) {// Lifecycle or input event
                // Launch event app processing via application->onAppCmd
                // or application->onInputEvent
                lSource->process(application, lSource);
            }

            // Check if we are exiting.
            if (application->destroyRequested) {
                return;
            }
        }

        // DO TASKS

    }
}
#elif __WIN32__


int main(int, char **)
{
    test();

    return 0;
}

#endif

