#include <string.h>
#include <jni.h>
#include <android/log.h>


jint JNICALL JNI_OnLoad(JavaVM *vm, void *)
{
    __android_log_print(ANDROID_LOG_VERBOSE, "Test1555553", "The value of 1 + 1 is %d", 1+1);
//    JNIEnv *env;

//    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) != JNI_OK)
//        return JNI_FALSE;

////    JNINativeMethod methods[] = {
////        {"backKeyPressed", "()V", reinterpret_cast<void *>(CJniHandler::backKeyPressed)},

////    };

//    jclass clazz = env->FindClass("org/solo/AndroidBinding");

//    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0)
//        return JNI_FALSE;

    return JNI_VERSION_1_4;
}

int main(int argc, char *argv[])
{
    __android_log_print(ANDROID_LOG_VERBOSE, "Test123123123", "The value of 1 + 1 is %d", 1+1);
}
