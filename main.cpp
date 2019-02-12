#include <QDebug>
#include <QElapsedTimer>
#include "Solo.h"

#ifdef __ANDROID__

#include "android_native_app_glue.h"

void android_main(struct android_app* app)
{
    android_poll_source* lSource;
    android_app* application= app;
//    application->onAppCmd= activity_event_callback;
//    application->onInputEvent = callback_input;

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

int main(int argc, char *argv[])
{
    S_Allocator allocator;

    void *ttt[128];

    QElapsedTimer et;
    et.start();
    for( int i = 0; i < 1; ++i )
    {
        ttt[i] = allocator.allocate( 2 );
    }
    qDebug()<<"ET-Al" << et.elapsed();
    et.restart();
    for( int i = 1; i >= 0; --i )
    {
        allocator.deAllocate( &ttt[i] );
    }
    qDebug()<<"ET-De" << et.elapsed();


    return 0;
}

#endif

