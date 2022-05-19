
cmake_minimum_required(VERSION 3.7.2)
project(solo VERSION 0.1.0)
add_compile_definitions(UNICODE)

set(CMAKE_CXX_STANDARD 17)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

if( UNIX AND NOT APPLE )
if( NOT ANDROID )
        set(LINUX TRUE)
endif()
endif()

add_compile_definitions(
        SOLO_ENABLE_DEBUG_LAYER=1
        # SOLO_ENABLE_VULKAN_VALIDATION_LAYER=1
        )

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/tinygltf
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/rapidjson
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/include
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/lib
        )

set(SOURCE
        ${SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/memory/S_Allocator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/memory/S_Allocator.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/memory/S_AlgorithmAllocator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/base/S_Object.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Vec2.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Vec3.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Vec4.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Quat.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Mat4x4.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Math.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/debug/S_Debug.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/thread/S_Mutex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/thread/S_Thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/utility/S_ElapsedTime.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_Event.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_InputEvent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_WindowEvent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_Window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_WindowWin32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_WindowX11.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_WindowAndroid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_BaseApplication.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanAllocator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanAllocator.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanDeviceAllocator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanItemsRequest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanPipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanRendererAPI.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanVertexBuffer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanShader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanTexture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanTextureSampler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanItemsManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_RendererAPI.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_VertexBuffer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Camera.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_CameraController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Shader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Texture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_TextureSampler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Model.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Scene.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/resource/S_ResourceManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/resource/S_Resources.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/resource/S_Resources.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/file/S_File.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/application/S_Application.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/lib/checkheader.c
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/lib/errstr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/lib/hashlist.c
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/lib/swap.c
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/lib/texture.c
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/lib/filestream.c
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/3rdparty/ktx/lib/memstream.c
)
set(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/memory/S_Allocator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/memory/S_AlgorithmAllocator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/memory/S_NewDeleteOverrides.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/base/S_Object.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/stl/S_Hash.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/stl/S_Map.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/stl/S_UnorderedMap.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/stl/S_List.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/stl/S_Vector.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/stl/S_Array.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/stl/S_Tuple.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/stl/S_String.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Vec2.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Vec3.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Vec4.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Quat.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Mat4x4.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/math/S_Math.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/thread/S_Mutex.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/thread/S_Thread.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/utility/S_ElapsedTime.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/utility/S_Animation.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/utility/S_Behavior.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/debug/S_Debug.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_SystemDetect.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_Event.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_InputEvent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_WindowEvent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_Window.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_WindowWin32.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_WindowX11.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_WindowAndroid.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/platforms/S_BaseApplication.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanAllocator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanDeviceAllocator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanItemsRequest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanPipeline.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanRendererAPI.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanVertexBuffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanShaderReflection.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanShader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanTexture.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanTextureSampler.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/vulkan/S_VulkanItemsManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_RendererAPI.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Renderer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_VertexBuffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Camera.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_CameraController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Shader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Texture.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_TextureSampler.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Model.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/renderer/S_Scene.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/resource/S_ResourceManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/resource/S_Resources.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/file/S_File.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/application/S_Application.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solo/Solo.h
)

find_package(Vulkan REQUIRED FATAL_ERROR)

if (NOT ANDROID)
        add_executable(
                ${PROJECT_NAME}
                ${SOURCE}
                ${HEADERS}
        )

        include_directories(
                ${Vulkan_INCLUDE_DIR}
        )

        if(LINUX)
                find_library(PTHREAD REQUIRED)
                find_library(X11 REQUIRED)
                target_link_libraries(
                        ${PROJECT_NAME}
                        pthread
                        X11
                )
        endif(LINUX)

        target_link_libraries(
                ${PROJECT_NAME}
                ${Vulkan_LIBRARY}
        )

        add_compile_definitions(
                VULKAN_SDK="$ENV{VULKAN_SDK}"
        )
        set(
                soloresourcegenerator_SOURCE 
                ${soloresourcegenerator_SOURCE}
                ${CMAKE_CURRENT_SOURCE_DIR}/SoloResourceGenerator/main.cpp
        )
        add_executable(soloresourcegenerator ${soloresourcegenerator_SOURCE})
        add_custom_target(
                soloresourcegenerator_run
                COMMAND soloresourcegenerator
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "run generated soloresourcegenerator in ${CMAKE_CURRENT_SOURCE_DIR}"
                SOURCES ${soloresourcegenerator_SOURCE}
        )

        add_dependencies(${PROJECT_NAME} soloresourcegenerator_run)
endif(NOT ANDROID)

if (ANDROID)
        find_library(
                log
                vulkan
        )

        add_library(native_app_glue STATIC
                ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
                )

        target_include_directories(
                native_app_glue PUBLIC
                ${ANDROID_NDK}/sources/android/native_app_glue
        )

        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

        add_library(
                ${PROJECT_NAME}
                SHARED
                ${SOURCE}
                ${HEADERS}
        )

        target_link_libraries(
                ${PROJECT_NAME}
                android
                log
                native_app_glue
                vulkan
        )
endif(ANDROID)
