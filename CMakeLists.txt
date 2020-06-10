##
 #######################################################################################################################
 #
 #  Copyright (c) 2020 Advanced Micro Devices, Inc. All Rights Reserved.
 #
 #  Permission is hereby granted, free of charge, to any person obtaining a copy
 #  of this software and associated documentation files (the "Software"), to deal
 #  in the Software without restriction, including without limitation the rights
 #  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 #  copies of the Software, and to permit persons to whom the Software is
 #  furnished to do so, subject to the following conditions:
 #
 #  The above copyright notice and this permission notice shall be included in all
 #  copies or substantial portions of the Software.
 #
 #  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 #  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 #  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 #  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 #  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 #  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 #  SOFTWARE.
 #
 #######################################################################################################################

cmake_minimum_required(VERSION 3.5)

PROJECT(spvgen VERSION 1 LANGUAGES C CXX)

option(SPVGEN_ENABLE_WERROR "Build with -Werror enabled" OFF)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti")
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if("${XGL_LLPC_PATH}" STREQUAL "")
    set(XGL_LLPC_PATH ${PROJECT_SOURCE_DIR}/../llpc/llpc)
endif()

set(VULKAN_HEADER_PATH ${PROJECT_SOURCE_DIR}/../xgl/icd/api/include/khronos CACHE PATH "Specify the path to vulkan header files.")

set(SPVGEN_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

add_definitions(-DVFX_INSIDE_SPVGEN=1)

add_definitions(-DNV_EXTENSIONS)

add_definitions(-DAMD_EXTENSIONS)

add_definitions(-DENABLE_HLSL)

# These two directories only exist after the user does "cd external && python fetch_external_sources.py".
# You cannot build spvgen without them, but we do not want the cmake stage to fail when they are not
# present, as spvgen is not built as part of the "all" target.
if(EXISTS ${PROJECT_SOURCE_DIR}/external/glslang/CMakeLists.txt)
    add_subdirectory(external/glslang)
endif()
if(EXISTS ${PROJECT_SOURCE_DIR}/external/SPIRV-tools/CMakeLists.txt)
    set(SPIRV_WERROR ${SPVGEN_ENABLE_WERROR} CACHE BOOL "${PROJECT_NAME} override." FORCE)
    add_subdirectory(external/SPIRV-tools)
endif()

add_library(spvgen SHARED "")

set_target_properties(spvgen PROPERTIES PREFIX "")

# vkgcDefs.h is included in llpc/tool/vfx/vfx.h
# llpc.h is included in llpc/llpc/tool/vfx/vfx.h
if(EXISTS ${XGL_VKGC_PATH}/tool/vfx/vfx.h)
    set(XGL_VFX_PATH ${XGL_VKGC_PATH}/tool/vfx)
    set(XGL_LLPC_INC_DIR ${XGL_VKGC_PATH}/include)
elseif(EXISTS ${XGL_LLPC_PATH}/../tool/vfx/vfx.h)
    set(XGL_VFX_PATH ${XGL_LLPC_PATH}/../tool/vfx)
    set(XGL_LLPC_INC_DIR ${XGL_LLPC_PATH}/../include)
else()
    set(XGL_VFX_PATH ${XGL_LLPC_PATH}/tool/vfx)
    set(XGL_LLPC_INC_DIR ${XGL_LLPC_PATH}/include)
endif()

include_directories(
	include
	external/glslang
	external/glslang/SPIRV
	external/SPIRV-tools/include
	external/llpc/include
	${VULKAN_HEADER_PATH}
	${XGL_LLPC_INC_DIR}
	${XGL_VFX_PATH}
)

if(EXISTS ${XGL_VKGC_PATH}/tool/vfx/vfxVkSection.cpp)
    add_definitions(-DVFX_SUPPORT_VK_PIPELINE=1)
    add_definitions(-DVFX_SUPPORT_RENDER_DOCOUMENT=1)
    target_sources(spvgen PRIVATE
        source/spvgen.cpp
        ${XGL_VFX_PATH}/vfxParser.cpp
        ${XGL_VFX_PATH}/vfxPipelineDoc.cpp
        ${XGL_VFX_PATH}/vfxRenderDoc.cpp
        ${XGL_VFX_PATH}/vfxSection.cpp
        ${XGL_VFX_PATH}/vfxEnumsConverter.cpp
        ${XGL_VFX_PATH}/vfxVkSection.cpp
        ${XGL_VFX_PATH}/vfxRenderSection.cpp
    )
else()
    target_sources(spvgen PRIVATE
        source/spvgen.cpp
        ${XGL_VFX_PATH}/vfxParser.cpp
        ${XGL_VFX_PATH}/vfxPipelineDoc.cpp
        ${XGL_VFX_PATH}/vfxRenderDoc.cpp
        ${XGL_VFX_PATH}/vfxSection.cpp
        ${XGL_VFX_PATH}/vfxEnumsConverter.cpp
    )
endif()

target_link_libraries(spvgen glslang HLSL OGLCompiler SPIRV SPIRV-Tools SPIRV-Tools-opt)
