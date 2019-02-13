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

struct SA
{
    int aaa;
    double bbb;
    char STr[25];
    SA() {}
};
int main(int argc, char *argv[])
{

    S_Vec3 v3 = S_Vec3( 3.1415f, 0.1415f, 0.0f );
    qDebug()<< S_Vec2(12, 10).length() << S_Vec3( 25, 10, 5 ).length() <<
               S_Vec4( 25, 10, 5, 5 ).length() << S_Quat().fromEularAnglesPYR( &v3 ).x();
    S_Allocator al;
//    int *dd = new int();
//    delete dd;

    S_List<SA> lst;
    std::list<SA> lst1;


    void *ttt[500000];

    QElapsedTimer et;




    SA vv;
    et.restart();
    for( int i = 0; i < 500000; ++i )
    {
        lst1.push_back( vv );
    }
    qDebug()<<"LST-CA-Al" << et.elapsed();
    et.restart();
    for( int i = 0; i < 500000; ++i )
    {
        lst1.pop_front();
    }
    qDebug()<<"LST-CA-De" << et.elapsed();


    et.restart();
    for( int i = 0; i < 500000; ++i )
    {
        lst.push_back( vv );
    }
    qDebug()<<"LST-ST-Al" << et.elapsed();
    et.restart();
    for( int i = 0; i < 500000; ++i )
    {
        lst.pop_front();
    }
    qDebug()<<"LST-ST-De" << et.elapsed();


    et.restart();
    for( int i = 0; i < 500000; ++i )
    {

        ttt[i] = S_Allocator::singleton()->allocate( (rand() % 64) + 10 );
    }
    qDebug()<<"AL-CA-Al" << et.elapsed();
    et.restart();
    for( int i = 0; i < 500000; ++i )
    {
        S_Allocator::singleton()->deallocate( ttt[i] );
    }
    qDebug()<<"DA-CA-De" << et.elapsed();


    et.restart();
    for( int i = 0; i < 500000; ++i )
    {
        ttt[i] = malloc( (rand() % 64) + 10 );
    }
    qDebug()<<"AL-MA-Al" << et.elapsed();
    et.restart();
    for( int i = 0; i < 500000; ++i )
    {
        free( ttt[i] );
    }
    qDebug()<<"AL-MA-De" << et.elapsed();


    return 0;
}

#endif

