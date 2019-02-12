QT -= gui core

CONFIG += c++11
CONFIG -= app_bundle

HEADERS += \
    Solo.h \
    core/memory/S_Allocator.h \
    core/math/S_Math.h \

SOURCES += \
        core/memory/S_Allocator.cpp \
        main.cpp

android {

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

LIBS += -landroid

HEADERS += \
    android_native_app_glue.h

SOURCES += \
    android_native_app_glue.c

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    android/src/org/solo/test/MainActivity.java \
    android/src/org/solo/core/AndroidBinding.java

}
