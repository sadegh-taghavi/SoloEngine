QT -= gui core

CONFIG += c++14
CONFIG -= app_bundle

HEADERS += \
    core/memory/S_Allocator.h \
    core/memory/S_AlgorithmAllocator.h \
    core/stl/S_Map.h \
    core/stl/S_List.h \
    core/stl/S_Vector.h \
    core/stl/S_String.h \
    core/math/S_Vec2.h \
    core/math/S_Vec3.h \
    core/math/S_Vec4.h \
    core/math/S_Quat.h \
    core/math/S_Mat4x4.h \
    core/debug/S_Debug.h \
    core/utility/S_ElapsedTime.h \
    Solo.h

SOURCES += \
    core/memory/S_Allocator.cpp \
    core/memory/S_AlgorithmAllocator.cpp \
    core/math/S_Vec2.inl \
    core/math/S_Vec3.inl \
    core/math/S_Vec4.inl \
    core/math/S_Quat.inl \
    core/math/S_Mat4x4.inl \
    core/debug/S_Debug.cpp \
    core/utility/S_ElapsedTime.cpp \
    main.cpp \


INCLUDEPATH += \
    $$PWD/3rdparty \
    $$PWD/3rdparty/EASTL/include \
    $$PWD/3rdparty/EASTL/test/packages/EAAssert/include \
    $$PWD/3rdparty/EASTL/test/packages/EABase/include/Common \
    $$PWD/3rdparty/EASTL/test/packages/EAMain/include \
    $$PWD/3rdparty/EASTL/test/packages/EAStdC/include \
    $$PWD/3rdparty/EASTL/test/packages/EATest/include \
    $$PWD/3rdparty/EASTL/test/packages/EAThread/include

SOURCES += \
    3rdparty/EASTL/source/allocator_eastl.cpp \
    3rdparty/EASTL/source/assert.cpp \
    3rdparty/EASTL/source/fixed_pool.cpp \
    3rdparty/EASTL/source/hashtable.cpp \
    3rdparty/EASTL/source/intrusive_list.cpp \
    3rdparty/EASTL/source/numeric_limits.cpp \
    3rdparty/EASTL/source/red_black_tree.cpp \
    3rdparty/EASTL/source/string.cpp \
    3rdparty/EASTL/source/thread_support.cpp

android {

INCLUDEPATH += \
    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$$(ANDROID_NDK_TOOLCHAIN_VERSION)/include \
    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$$(ANDROID_NDK_TOOLCHAIN_VERSION)/include/backward \
    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$$(ANDROID_NDK_TOOLCHAIN_VERSION)/libs/armeabi-v7a/include

    ANDROID_EXTRA_LIBS = \
        $$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$$(ANDROID_NDK_TOOLCHAIN_VERSION)/libs/armeabi-v7a/libgnustl_shared.so

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
