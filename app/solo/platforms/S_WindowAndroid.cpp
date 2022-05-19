#include "solo/platforms/S_SystemDetect.h"
#if defined( S_PLATFORM_ANDROID )
#include "S_WindowAndroid.h"
#include "S_WindowEvent.h"
#include "S_InputEvent.h"
#include "solo/debug/S_Debug.h"

using namespace solo;

android_app *solo::S_WindowAndroid::m_app = nullptr;

#define CALL_OBJ_METHOD( OBJ,METHOD,SIGNATURE, ...) jniEnv->CallObjectMethod (OBJ, jniEnv->GetMethodID(jniEnv->GetObjectClass(OBJ),METHOD,SIGNATURE), __VA_ARGS__)
#define CALL_BOOL_METHOD(OBJ,METHOD,SIGNATURE, ...) jniEnv->CallBooleanMethod(OBJ, jniEnv->GetMethodID(jniEnv->GetObjectClass(OBJ),METHOD,SIGNATURE), __VA_ARGS__)

void ShowKeyboard(bool visible,int flags){
    // Attach current thread to the JVM.
    JavaVM* javaVM = S_WindowAndroid::app()->activity->vm;
    JNIEnv* jniEnv = S_WindowAndroid::app()->activity->env;
    JavaVMAttachArgs Args = {JNI_VERSION_1_6, "NativeThread", nullptr};
    javaVM->AttachCurrentThread(&jniEnv, &Args);

    // Retrieve NativeActivity.
    jobject lNativeActivity = S_WindowAndroid::app()->activity->clazz;

    // Retrieve Context.INPUT_METHOD_SERVICE.
    jclass ClassContext = jniEnv->FindClass("android/content/Context");
    jfieldID FieldINPUT_METHOD_SERVICE =jniEnv->GetStaticFieldID(ClassContext, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject INPUT_METHOD_SERVICE =jniEnv->GetStaticObjectField(ClassContext, FieldINPUT_METHOD_SERVICE);

    // getSystemService(Context.INPUT_METHOD_SERVICE).
    jobject   lInputMethodManager = CALL_OBJ_METHOD(lNativeActivity, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", INPUT_METHOD_SERVICE);

    // getWindow().getDecorView().
    jobject lWindow    = CALL_OBJ_METHOD(lNativeActivity,"getWindow", "()Landroid/view/Window;",0);
    jobject lDecorView = CALL_OBJ_METHOD(lWindow, "getDecorView", "()Landroid/view/View;",0);
    if (visible)
    {
        CALL_BOOL_METHOD(lInputMethodManager, "showSoftInput", "(Landroid/view/View;I)Z", lDecorView, flags);
    } else
    {
        jobject  lBinder = CALL_OBJ_METHOD (lDecorView, "getWindowToken", "()Landroid/os/IBinder;",0);
        CALL_BOOL_METHOD(lInputMethodManager, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z", lBinder, flags);
    }
    // Finished with the JVM.
    javaVM->DetachCurrentThread();
}

int GetUnicodeChar(int eventType, int keyCode, int metaState)
{
    JavaVM* javaVM = S_WindowAndroid::app()->activity->vm;
    JNIEnv* jniEnv = S_WindowAndroid::app()->activity->env;

    JavaVMAttachArgs Args={JNI_VERSION_1_6, "NativeThread", nullptr};
    jint result = javaVM->AttachCurrentThread(&jniEnv, &Args);
    if (result == JNI_ERR) return 0;

    jclass class_key_event = jniEnv->FindClass("android/view/KeyEvent");

    jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "(I)I");
    jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
    jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);
    int unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char, metaState);

    javaVM->DetachCurrentThread();

    return unicodeKey;
}

void android_main(struct android_app* app)
{
    S_WindowAndroid::setApp( app );
    int ret = soloMain();
    ANativeActivity_finish(app->activity);
    exit(ret);
}

solo::S_WindowAndroid::S_WindowAndroid(unsigned int, unsigned int)
{
    m_width = 0;
    m_height = 0;
    m_running = true;

    while (!m_focus)
    {
        int events = 0;
        struct android_poll_source* source;
        int id = ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source));
        if (id == LOOPER_ID_MAIN)
        {
            int8_t cmd = android_app_read_cmd(m_app);
            android_app_pre_exec_cmd(m_app, cmd);
            if (m_app->onAppCmd != nullptr) m_app->onAppCmd(m_app, cmd);
            if (cmd == APP_CMD_INIT_WINDOW)
            {
                m_width  = static_cast<unsigned int>(ANativeWindow_getWidth(m_app->window));
                m_height = static_cast<unsigned int>(ANativeWindow_getHeight(m_app->window));
                m_eventFIFO.push(std::make_unique<S_WindowCreateEvent>());
                m_eventFIFO.push(std::make_unique<S_WindowResizeEvent>(m_width, m_height));
            }
            if (cmd == APP_CMD_GAINED_FOCUS)
            {
                m_focus = true;
                m_eventFIFO.push(std::make_unique<S_WindowFocusEvent>(true));
            }
            android_app_post_exec_cmd(m_app, cmd);
        }
    }
    ALooper_pollAll(100, nullptr, nullptr, nullptr);
}

solo::S_WindowAndroid::~S_WindowAndroid()
{

}

std::unique_ptr<S_Event> solo::S_WindowAndroid::getEvent(bool wait_for_event)
{
    std::unique_ptr<S_Event> event;
    static char buf[4] = {};
    if (!m_eventFIFO.isEmpty())
        return m_eventFIFO.pop();
    int events[128];
    struct android_poll_source* source;
    int timeoutMillis = wait_for_event ? -1 : 0;
    int id = ALooper_pollOnce( timeoutMillis, nullptr, events, reinterpret_cast<void**>(&source) );

    //ALooper_pollAll(-1, nullptr, events, (void**)&source);
    //    // if(id>=0) printf("id=%d events=%d, source=%d",id,(int)events, source[0]);
    //if(source) source->process(m_app, source);

    if (id == LOOPER_ID_MAIN)
    {
        int8_t cmd = android_app_read_cmd(m_app);
        android_app_pre_exec_cmd(m_app, cmd);
        if (m_app->onAppCmd != nullptr)
            m_app->onAppCmd(m_app, cmd);

        switch (cmd)
        {
        case APP_CMD_WINDOW_RESIZED:
        {
            unsigned int w  = static_cast<unsigned int>(ANativeWindow_getWidth(m_app->window));
            unsigned int h = static_cast<unsigned int>(ANativeWindow_getHeight(m_app->window));
            if( w != m_width || h != m_height )
            {
                m_width = w;
                m_height = h;
                m_eventFIFO.push(std::make_unique<S_WindowResizeEvent>(m_width, m_height));
            }
            break;
        }
        case APP_CMD_GAINED_FOCUS:
        {
            m_focus = true;
            event = std::make_unique<S_WindowFocusEvent>(true);
            break;
        }
        case APP_CMD_LOST_FOCUS:
        {
            m_focus = false;
            event = std::make_unique<S_WindowFocusEvent>(false);
            break;
        }
        case APP_CMD_DESTROY:
        {
            m_running = false;
            event = std::make_unique<S_WindowCloseEvent>();
            break;
        }
        default: break;
        }
        android_app_post_exec_cmd(m_app, cmd);
    }else if (id == LOOPER_ID_INPUT)
    {
        AInputEvent* a_event = nullptr;
        while (AInputQueue_getEvent(m_app->inputQueue, &a_event) >= 0)
        {
            if (AInputQueue_preDispatchEvent(m_app->inputQueue, a_event))
                continue;
            int32_t handled = 0;
            if (m_app->onInputEvent != nullptr)
                handled = m_app->onInputEvent(m_app, a_event);
            int32_t type = AInputEvent_getType(a_event);
            if (type == AINPUT_EVENT_TYPE_KEY)
            {
                int32_t a_action = AKeyEvent_getAction(a_event);
                int32_t keycode  = AKeyEvent_getKeyCode(a_event);
                uint8_t hidcode  = ANDROID_TO_HID[keycode];

                switch (a_action)
                {
                case AKEY_EVENT_ACTION_DOWN:
                {
                    int metaState = AKeyEvent_getMetaState(a_event);
                    int unicode   = GetUnicodeChar(AKEY_EVENT_ACTION_DOWN, keycode, metaState);
                    memcpy(buf, &unicode, 4);
                    event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>(hidcode), S_KeyboardEventState::Down );
                    if (buf[0])
                        m_eventFIFO.push(std::make_unique<S_CharacterEvent>(buf));
                    break;
                }
                case AKEY_EVENT_ACTION_UP:
                {
                    event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>(hidcode), S_KeyboardEventState::Up );
                    break;
                }
                default: break;
                }

            } else if (type == AINPUT_EVENT_TYPE_MOTION)
            {
                int32_t a_action = AMotionEvent_getAction(a_event);
                int action       = (a_action & AMOTION_EVENT_ACTION_MASK);
                m_touchEvent.setActiveCount( static_cast<unsigned int>(AMotionEvent_getPointerCount(a_event)) );
                if (action == AMOTION_EVENT_ACTION_MOVE)
                {
                    for( unsigned int i = 0; i < m_touchEvent.activeCount(); ++i )
                    {
                        int finger_id = AMotionEvent_getPointerId(a_event, static_cast<size_t>(i));
                        float x = AMotionEvent_getX(a_event, static_cast<size_t>(i));
                        float y = AMotionEvent_getY(a_event, static_cast<size_t>(i));
                        m_touchEvent.setPointByIndex( i, S_TouchPoint( S_TouchEventState::Move, x, y, finger_id ) );
                    }
                    event = std::make_unique<S_TouchEvent>();
                    (*dynamic_cast<S_TouchEvent*>(event.get())) = m_touchEvent;

                } else
                {
                    float x = 0;
                    float y = 0;
                    size_t inx = 0;
                    int finger_id = 0;
                    auto getTouchData = [&x, &y, &inx, &finger_id, &a_action, &a_event]
                    {
                        inx = static_cast<size_t>(a_action >> 8); // get index from top 24 bits
                        finger_id = AMotionEvent_getPointerId(a_event, inx);
                        x = AMotionEvent_getX(a_event, inx);
                        y = AMotionEvent_getY(a_event, inx);
                    };
                    switch (action)
                    {
                    case AMOTION_EVENT_ACTION_POINTER_DOWN:
                    case AMOTION_EVENT_ACTION_DOWN:
                        getTouchData();
                        m_touchEvent.setPointByID( finger_id, S_TouchPoint( S_TouchEventState::Down, x, y, finger_id) );
                        event = std::make_unique<S_TouchEvent>();
                        (*dynamic_cast<S_TouchEvent*>(event.get())) = m_touchEvent;
                        break;
                    case AMOTION_EVENT_ACTION_POINTER_UP:
                    case AMOTION_EVENT_ACTION_UP :
                        getTouchData();
                        m_touchEvent.setPointByID( finger_id, S_TouchPoint( S_TouchEventState::Up, x, y, finger_id) );
                        event = std::make_unique<S_TouchEvent>();
                        (*dynamic_cast<S_TouchEvent*>(event.get())) = m_touchEvent;
                        break;
                    case AMOTION_EVENT_ACTION_CANCEL:
                        getTouchData();
                        m_touchEvent.setActiveCount( 0 );
                        event = std::make_unique<S_TouchEvent>();
                        (*dynamic_cast<S_TouchEvent*>(event.get())) = m_touchEvent;
                        break;
                    default:
                        event = std::make_unique<S_Event>(S_EventType::Other);
                        break;
                    }
                }
                if(event->type() == S_EventType::Touch )
                {
                    S_TouchEvent *touchEvent = dynamic_cast<S_TouchEvent*>(event.get());
                    if( touchEvent->activeCount() == 1 )
                        m_eventFIFO.push( std::make_unique<S_MouseEvent>( S_MouseButton::Left,
                                                            static_cast<S_MouseEventState>(touchEvent->pointByIndex(0).state()),
                                                            static_cast<unsigned int>(touchEvent->pointByIndex(0).x()),
                                                            static_cast<unsigned int>(touchEvent->pointByIndex(0).y()),
                                                            0) );
                }
                handled = 0;
            }
            AInputQueue_finishEvent(m_app->inputQueue, a_event, handled);
            return event;
        }

    }
    if (m_app->destroyRequested)
        m_eventFIFO.push( std::make_unique<S_Event>() );

    if( event.get() == nullptr )
        event = std::make_unique<S_Event>(S_EventType::Other);
    return event;
}

void solo::S_WindowAndroid::setSize(unsigned int, unsigned int)
{

}

android_app *S_WindowAndroid::app()
{
    return m_app;
}

bool S_WindowAndroid::setApp(android_app *app)
{
    if( m_app == nullptr )
    {
        m_app = app;
        return  true;
    }
    return false;
}

#endif
