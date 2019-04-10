##
 #######################################################################################################################
 #
 #  Copyright (c) 2019 Advanced Micro Devices, Inc. All Rights Reserved.
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

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti")
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(XGL_LLPC_PATH ${PROJECT_SOURCE_DIR}/../llpc CACHE PATH "Specify the path to the LLPC.")

set(VULKAN_HEADER_PATH ${PROJECT_SOURCE_DIR}/../xgl/icd/api/include/khronos CACHE PATH "Specify the path to vulkan header files.")

set(SPVGEN_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

add_definitions(-DVFX_INSIDE_SPVGEN=1)

add_definitions(-DNV_EXTENSIONS)

add_definitions(-DAMD_EXTENSIONS)

# These two directories only exist after the user does "cd external && python fetch_external_sources.py".
# You cannot build spvgen without them, but we do not want the cmake stage to fail when they are not
# present, as spvgen is not built as part of the "all" target.
if(EXISTS ${PROJECT_SOURCE_DIR}/external/glslang/CMakeLists.txt)
    add_subdirectory(external/glslang)
endif()
if(EXISTS ${PROJECT_SOURCE_DIR}/external/SPIRV-tools/CMakeLists.txt)
    add_subdirectory(external/SPIRV-tools)
endif()

add_library(spvgen SHARED "")

set_target_properties(spvgen PROPERTIES PREFIX "")

include_directories(
	include
	external/glslang
	external/glslang/SPIRV
	external/SPIRV-tools/include
	external/llpc/include
	${XGL_LLPC_PATH}/include
	${XGL_LLPC_PATH}/tool/vfx
	${VULKAN_HEADER_PATH}
)

target_sources(spvgen PRIVATE
	source/spvgen.cpp
    ${XGL_LLPC_PATH}/tool/vfx/vfxParser.cpp
    ${XGL_LLPC_PATH}/tool/vfx/vfxPipelineDoc.cpp
    ${XGL_LLPC_PATH}/tool/vfx/vfxRenderDoc.cpp
    ${XGL_LLPC_PATH}/tool/vfx/vfxSection.cpp
    ${XGL_LLPC_PATH}/tool/vfx/vfxEnumsConverter.cpp
)

target_link_libraries(spvgen glslang HLSL OGLCompiler SPIRV SPIRV-Tools SPIRV-Tools-opt)

