QT -= gui core

CONFIG += c++11
CONFIG -= app_bundle

HEADERS += \
    core/memory/S_Allocator.h \  
    core/memory/S_AlgorithmAllocator.h \
    core/algorithm/S_List.h \
    core/algorithm/S_Vector.h \
    core/algorithm/S_String.h \  
    core/math/S_Vec2.h \
    core/math/S_Vec3.h \
    core/math/S_Vec4.h \
    core/math/S_Quat.h \
    core/math/S_Mat4x4.h \
    Solo.h

SOURCES += \
    core/memory/S_Allocator.cpp \
    core/memory/S_AlgorithmAllocator.cpp \
    core/math/S_Vec2.inl \
    core/math/S_Vec3.inl \
    core/math/S_Vec4.inl \
    core/math/S_Quat.inl \
    core/math/S_Mat4x4.inl \
    main.cpp

INCLUDEPATH += $$PWD/3rdparty \

android {

INCLUDEPATH += \
    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$$(ANDROID_NDK_TOOLCHAIN_VERSION)/include \
    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$$(ANDROID_NDK_TOOLCHAIN_VERSION)/include/backward \
    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$$(ANDROID_NDK_TOOLCHAIN_VERSION)/libs/armeabi-v7a/include

LIBS += -Lquate($$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$$(ANDROID_NDK_TOOLCHAIN_VERSION)/libs/armeabi-v7a) -lgnustl_shared

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
