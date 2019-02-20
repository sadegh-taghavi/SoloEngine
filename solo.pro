QT -= gui core


CONFIG += c++14
CONFIG -= app_bundle

HEADERS += \
    solo/memory/S_Allocator.h \
    solo/memory/S_AlgorithmAllocator.h \
    solo/stl/S_Map.h \
    solo/stl/S_List.h \
    solo/stl/S_Vector.h \
    solo/stl/S_String.h \
    solo/math/S_Vec2.h \
    solo/math/S_Vec3.h \
    solo/math/S_Vec4.h \
    solo/math/S_Quat.h \
    solo/math/S_Mat4x4.h \
    solo/debug/S_Debug.h \
    solo/utility/S_ElapsedTime.h \
    Solo.h

SOURCES += \
    solo/memory/S_Allocator.cpp \
    solo/memory/S_AlgorithmAllocator.cpp \
    solo/math/S_Vec2.inl \
    solo/math/S_Vec3.inl \
    solo/math/S_Vec4.inl \
    solo/math/S_Quat.inl \
    solo/math/S_Mat4x4.inl \
    solo/debug/S_Debug.cpp \
    solo/utility/S_ElapsedTime.cpp \
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
    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/llvm-libc++/include \
    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/llvm-libc++/include/ext \
#    $$(ANDROID_NDK_ROOT)/sources/cxx-stl/llvm-libc++/libs/arm64-v8a/include

    ANDROID_EXTRA_LIBS = \
        $$(ANDROID_NDK_ROOT)/sources/cxx-stl/llvm-libc++/libs/arm64-v8a/libc++_shared.so

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
