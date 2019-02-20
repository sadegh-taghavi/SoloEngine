//#include <QDebug>
//#include <QElapsedTimer>
#include "Solo.h"

struct SA
{
    int aaa;
    double bbb;
    char STr[25];
    SA() {}
};
void test()
{
    int64_t *iv = new int64_t;


    S_List<SA> lst;
    SA vv;


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

    s_debug( "Test", S_Allocator::singleton()->getTotalAllocatedItems() ,
             S_Allocator::singleton()->getTotalUsedPools() , S_Allocator::singleton()->getTotalAllocatedBytes(),
             "Invocation" , S_Allocator::singleton()->getTotalAllocateInvoked(),
             S_Allocator::singleton()->getTotalDeallocateInvoked(), (*iv) );

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

