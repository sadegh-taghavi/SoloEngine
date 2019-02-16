//#include <QDebug>
//#include <QElapsedTimer>
#include "Solo.h"
#include <stdio.h>

struct SA
{
    int aaa;
    double bbb;
    char STr[25];
    SA() {}
};
void test()
{
    S_Allocator al;
    int *i = new int();
    delete i;

    S_List<SA> lst;
    std::list<SA> lst1;


    void *ttt[1500000];

    SA vv;

//    for( int i = 0; i < 1500000; ++i )
//    {
//        lst1.push_back( vv );
//    }


//    for( int i = 500000; i < 1000000; ++i )
//    {
//        lst1.pop_front();
//    }

//    for( int i = 0; i < 500000; ++i )
//    {
//        lst1.push_back( vv );
//    }

    char str[50];
    char str1[50];
    char str2[50];
    std::string teststr;
    teststr = "SAAAAAAAAAAAAA";
    teststr += "SAAAAAAAAAA";
    teststr += "SAAAAAAAAAADDDDDDDDDDDDDDDDDDDDD";


    sprintf( str, " %llu ",  al.getTotalAllocatedItems() );
    sprintf( str1, " %llu ",  al.getTotalUsedPools() );
    sprintf( str2, " %llu ",  al.getTotalAllocatedBytes() );
    S_Debug()<<"Test" << str << str1 << str2;

    //    S_Vec3 v3 = S_Vec3( 3.1415f, 0.1415f, 0.0f );
    //    S_Mat4x4 m;
    //    m.translate( S_Vec3( 0, 0, 0 ) );
    //    m.identity();
    //    qDebug()<< S_Vec2(12, 10).length() << S_Vec3( 25, 10, 5 ).length() <<
    //               S_Vec4( 25, 10, 5, 5 ).length() << S_Quat().fromEularAnglesPYR( &v3 ).x() <<
    //               m(1, 1);
    //    S_Allocator al;
    ////    int *dd = new int();
    ////    delete dd;

    //    S_List<SA> lst;
    //    std::list<SA> lst1;


    //    void *ttt[500000];

    //    QElapsedTimer et;




    //    SA vv;
    //    et.restart();
    //    for( int i = 0; i < 500000; ++i )
    //    {
    //        lst1.push_back( vv );
    //    }
    //    qDebug()<<"LST-CA-Al" << et.elapsed();
    //    et.restart();
    //    for( int i = 0; i < 500000; ++i )
    //    {
    //        lst1.pop_front();
    //    }
    //    qDebug()<<"LST-CA-De" << et.elapsed();


    //    et.restart();
    //    for( int i = 0; i < 500000; ++i )
    //    {
    //        lst.push_back( vv );
    //    }
    //    qDebug()<<"LST-ST-Al" << et.elapsed();
    //    et.restart();
    //    for( int i = 0; i < 500000; ++i )
    //    {
    //        lst.pop_front();
    //    }
    //    qDebug()<<"LST-ST-De" << et.elapsed();


    //    et.restart();
    //    for( int i = 0; i < 500000; ++i )
    //    {

    //        ttt[i] = S_Allocator::singleton()->allocate( (rand() % 64) + 10 );
    //    }
    //    qDebug()<<"AL-CA-Al" << et.elapsed();
    //    et.restart();
    //    for( int i = 0; i < 500000; ++i )
    //    {
    //        S_Allocator::singleton()->deallocate( ttt[i] );
    //    }
    //    qDebug()<<"DA-CA-De" << et.elapsed();


    //    et.restart();
    //    for( int i = 0; i < 500000; ++i )
    //    {
    //        ttt[i] = malloc( (rand() % 64) + 10 );
    //    }
    //    qDebug()<<"AL-MA-Al" << et.elapsed();
    //    et.restart();
    //    for( int i = 0; i < 500000; ++i )
    //    {
    //        free( ttt[i] );
    //    }
    //    qDebug()<<"AL-MA-De" << et.elapsed();

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

//    printf( "#############################1111111111111111111############");

        app_dummy();

//    main( 0, nullptr );

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

