/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2015-2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************************************************/
/**
***********************************************************************************************************************
* @file  spvgen.cpp
* @brief SPVGEN source file: defines the exported functions for the DLL application.
***********************************************************************************************************************
*/

//Note: this file is based on https://github.com/KhronosGroup/glslang/blob/876a0e392e93c16b4dfa66daf382a53005c1e7b0/StandAlone/StandAlone.cpp

//Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
//Copyright (C) 2013 LunarG, Inc.
//
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//

// this only applies to the standalone wrapper, not the front end in general
#include "glslang/Include/ShHandle.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/build_info.h"
#include "StandAlone/DirStackFileIncluder.h"
#include "SPIRV/GlslangToSpv.h"

#include "spirv-tools/libspirv.h"
#include "spirv-tools/optimizer.hpp"

#include "doc.h"
namespace spv {
    extern "C" {
        // Include C-based headers that don't have a namespace
        #include "GLSL.std.450.h"
        #include "GLSL.ext.KHR.h"
        #include "GLSL.ext.AMD.h"
    }
}

using namespace spv;
#include "spirv_cross_util.hpp"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "spirv_parser.hpp"
#include "spirv_reflect.hpp"

#include "disassemble.h"
#include <sstream>
#include <string>
#include <vector>
#include <stdarg.h>

#include "spvgen.h"

// Forward declarations
EShLanguage SpvGenStageToEShLanguage(SpvGenStage stage);
bool ReadFileData(const char* pFileName, std::string& data);
int Vsnprintf(char* pOutput, size_t bufSize, const char* pFormat, va_list argList);
int Snprintf(char* pOutput, size_t bufSize, const char* pFormat, ...);
spv_result_t spvDiagnosticPrint(const spv_diagnostic diagnostic, char* pBuffer, size_t bufferSize);

TBuiltInResource Resources;
std::string* pConfigFile;
int DefaultOptions = SpvGenOptionDefaultDesktop | SpvGenOptionVulkanRules;

//
// These are the default resources for TBuiltInResources, used for both
//  - parsing this string for the case where the user didn't supply one
//  - dumping out a template for user construction of a config file
//
const char* DefaultConfig =
    "MaxLights 32\n"
    "MaxClipPlanes 6\n"
    "MaxTextureUnits 32\n"
    "MaxTextureCoords 32\n"
    "MaxVertexAttribs 64\n"
    "MaxVertexUniformComponents 4096\n"
    "MaxVaryingFloats 64\n"
    "MaxVertexTextureImageUnits 65535\n"
    "MaxCombinedTextureImageUnits 65535\n"
    "MaxTextureImageUnits 65535\n"
    "MaxFragmentUniformComponents 4096\n"
    "MaxDrawBuffers 8\n"
    "MaxVertexUniformVectors 128\n"
    "MaxVaryingVectors 8\n"
    "MaxFragmentUniformVectors 16\n"
    "MaxVertexOutputVectors 32\n"
    "MaxFragmentInputVectors 32\n"
    "MinProgramTexelOffset -64\n"
    "MaxProgramTexelOffset 63\n"
    "MaxClipDistances 8\n"
    "MaxComputeWorkGroupCountX 65535\n"
    "MaxComputeWorkGroupCountY 65535\n"
    "MaxComputeWorkGroupCountZ 65535\n"
    "MaxComputeWorkGroupSizeX 1024\n"
    "MaxComputeWorkGroupSizeY 1024\n"
    "MaxComputeWorkGroupSizeZ 64\n"
    "MaxComputeUniformComponents 1024\n"
    "MaxComputeTextureImageUnits 65535\n"
    "MaxComputeImageUniforms 65535\n"
    "MaxComputeAtomicCounters 8\n"
    "MaxComputeAtomicCounterBuffers 1\n"
    "MaxVaryingComponents 60\n"
    "MaxVertexOutputComponents 128\n"
    "MaxGeometryInputComponents 128\n"
    "MaxGeometryOutputComponents 128\n"
    "MaxFragmentInputComponents 128\n"
    "MaxImageUnits 65536\n"
    "MaxCombinedImageUnitsAndFragmentOutputs 65536\n"
    "MaxCombinedShaderOutputResources 65536\n"
    "MaxImageSamples 8\n"
    "MaxVertexImageUniforms 65536\n"
    "MaxTessControlImageUniforms 65536\n"
    "MaxTessEvaluationImageUniforms 65536\n"
    "MaxGeometryImageUniforms 65536\n"
    "MaxFragmentImageUniforms 65536\n"
    "MaxCombinedImageUniforms 65536\n"
    "MaxGeometryTextureImageUnits 65536\n"
    "MaxGeometryOutputVertices 256\n"
    "MaxGeometryTotalOutputComponents 1024\n"
    "MaxGeometryUniformComponents 1024\n"
    "MaxGeometryVaryingComponents 64\n"
    "MaxTessControlInputComponents 128\n"
    "MaxTessControlOutputComponents 128\n"
    "MaxTessControlTextureImageUnits 16\n"
    "MaxTessControlUniformComponents 1024\n"
    "MaxTessControlTotalOutputComponents 4096\n"
    "MaxTessEvaluationInputComponents 128\n"
    "MaxTessEvaluationOutputComponents 128\n"
    "MaxTessEvaluationTextureImageUnits 16\n"
    "MaxTessEvaluationUniformComponents 1024\n"
    "MaxTessPatchComponents 120\n"
    "MaxPatchVertices 32\n"
    "MaxTessGenLevel 64\n"
    "MaxViewports 16\n"
    "MaxVertexAtomicCounters 8\n"
    "MaxTessControlAtomicCounters 8\n"
    "MaxTessEvaluationAtomicCounters 8\n"
    "MaxGeometryAtomicCounters 8\n"
    "MaxFragmentAtomicCounters 8\n"
    "MaxCombinedAtomicCounters 8\n"
    "MaxAtomicCounterBindings 8\n"
    "MaxVertexAtomicCounterBuffers 8\n"
    "MaxTessControlAtomicCounterBuffers 8\n"
    "MaxTessEvaluationAtomicCounterBuffers 8\n"
    "MaxGeometryAtomicCounterBuffers 8\n"
    "MaxFragmentAtomicCounterBuffers 8\n"
    "MaxCombinedAtomicCounterBuffers 8\n"
    "MaxAtomicCounterBufferSize 16384\n"
    "MaxTransformFeedbackBuffers 4\n"
    "MaxTransformFeedbackInterleavedComponents 64\n"
    "MaxCullDistances 8\n"
    "MaxCombinedClipAndCullDistances 8\n"
    "MaxSamples 4\n"
    "MaxMeshOutputVerticesEXT 256\n"
    "MaxMeshOutputPrimitivesEXT 256\n"
    "MaxMeshWorkGroupSizeX_EXT 128\n"
    "MaxMeshWorkGroupSizeY_EXT 128\n"
    "MaxMeshWorkGroupSizeZ_EXT 128\n"
    "MaxTaskWorkGroupSizeX_EXT 128\n"
    "MaxTaskWorkGroupSizeY_EXT 128\n"
    "MaxTaskWorkGroupSizeZ_EXT 128\n"
    "MaxMeshViewCountEXT 4\n"
    "MaxDualSourceDrawBuffersEXT 1\n"

    "nonInductiveForLoops 1\n"
    "whileLoops 1\n"
    "doWhileLoops 1\n"
    "generalUniformIndexing 1\n"
    "generalAttributeMatrixVectorIndexing 1\n"
    "generalVaryingIndexing 1\n"
    "generalSamplerIndexing 1\n"
    "generalVariableIndexing 1\n"
    "generalConstantMatrixVectorIndexing 1\n"
    ;

// =====================================================================================================================
// Parse either a .conf file provided by the user or the default string above.
void ProcessConfigFile()
{
    const char* config = 0;
    std::string configData;

    if (pConfigFile == nullptr)
    {
        pConfigFile = new(std::string);
    }

    if (pConfigFile->size() > 0)
    {
        if (ReadFileData(pConfigFile->c_str(), configData))
        {
            config = configData.c_str();
        }
        else
        {
            printf("Error opening configuration file; will instead use the default configuration\n");
        }
    }

    if (config == 0)
    {
        configData = DefaultConfig;
        config = configData.c_str();
    }

    const char* delims = " \t\n\r";
    const char* token = strtok(const_cast<char*>(config), delims);
    while (token) {
        const char* valueStr = strtok(0, delims);
        if (valueStr == 0 || ! (valueStr[0] == '-' || (valueStr[0] >= '0' && valueStr[0] <= '9'))) {
            printf("Error: '%s' bad .conf file.  Each name must be followed by one number.\n", valueStr ? valueStr : "");
            return;
        }
        int value = atoi(valueStr);

        if (strcmp(token, "MaxLights") == 0)
            Resources.maxLights = value;
        else if (strcmp(token, "MaxClipPlanes") == 0)
            Resources.maxClipPlanes = value;
        else if (strcmp(token, "MaxTextureUnits") == 0)
            Resources.maxTextureUnits = value;
        else if (strcmp(token, "MaxTextureCoords") == 0)
            Resources.maxTextureCoords = value;
        else if (strcmp(token, "MaxVertexAttribs") == 0)
            Resources.maxVertexAttribs = value;
        else if (strcmp(token, "MaxVertexUniformComponents") == 0)
            Resources.maxVertexUniformComponents = value;
        else if (strcmp(token, "MaxVaryingFloats") == 0)
            Resources.maxVaryingFloats = value;
        else if (strcmp(token, "MaxVertexTextureImageUnits") == 0)
            Resources.maxVertexTextureImageUnits = value;
        else if (strcmp(token, "MaxCombinedTextureImageUnits") == 0)
            Resources.maxCombinedTextureImageUnits = value;
        else if (strcmp(token, "MaxTextureImageUnits") == 0)
            Resources.maxTextureImageUnits = value;
        else if (strcmp(token, "MaxFragmentUniformComponents") == 0)
            Resources.maxFragmentUniformComponents = value;
        else if (strcmp(token, "MaxDrawBuffers") == 0)
            Resources.maxDrawBuffers = value;
        else if (strcmp(token, "MaxVertexUniformVectors") == 0)
            Resources.maxVertexUniformVectors = value;
        else if (strcmp(token, "MaxVaryingVectors") == 0)
            Resources.maxVaryingVectors = value;
        else if (strcmp(token, "MaxFragmentUniformVectors") == 0)
            Resources.maxFragmentUniformVectors = value;
        else if (strcmp(token, "MaxVertexOutputVectors") == 0)
            Resources.maxVertexOutputVectors = value;
        else if (strcmp(token, "MaxFragmentInputVectors") == 0)
            Resources.maxFragmentInputVectors = value;
        else if (strcmp(token, "MinProgramTexelOffset") == 0)
            Resources.minProgramTexelOffset = value;
        else if (strcmp(token, "MaxProgramTexelOffset") == 0)
            Resources.maxProgramTexelOffset = value;
        else if (strcmp(token, "MaxClipDistances") == 0)
            Resources.maxClipDistances = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountX") == 0)
            Resources.maxComputeWorkGroupCountX = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountY") == 0)
            Resources.maxComputeWorkGroupCountY = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountZ") == 0)
            Resources.maxComputeWorkGroupCountZ = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeX") == 0)
            Resources.maxComputeWorkGroupSizeX = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeY") == 0)
            Resources.maxComputeWorkGroupSizeY = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeZ") == 0)
            Resources.maxComputeWorkGroupSizeZ = value;
        else if (strcmp(token, "MaxComputeUniformComponents") == 0)
            Resources.maxComputeUniformComponents = value;
        else if (strcmp(token, "MaxComputeTextureImageUnits") == 0)
            Resources.maxComputeTextureImageUnits = value;
        else if (strcmp(token, "MaxComputeImageUniforms") == 0)
            Resources.maxComputeImageUniforms = value;
        else if (strcmp(token, "MaxComputeAtomicCounters") == 0)
            Resources.maxComputeAtomicCounters = value;
        else if (strcmp(token, "MaxComputeAtomicCounterBuffers") == 0)
            Resources.maxComputeAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxVaryingComponents") == 0)
            Resources.maxVaryingComponents = value;
        else if (strcmp(token, "MaxVertexOutputComponents") == 0)
            Resources.maxVertexOutputComponents = value;
        else if (strcmp(token, "MaxGeometryInputComponents") == 0)
            Resources.maxGeometryInputComponents = value;
        else if (strcmp(token, "MaxGeometryOutputComponents") == 0)
            Resources.maxGeometryOutputComponents = value;
        else if (strcmp(token, "MaxFragmentInputComponents") == 0)
            Resources.maxFragmentInputComponents = value;
        else if (strcmp(token, "MaxImageUnits") == 0)
            Resources.maxImageUnits = value;
        else if (strcmp(token, "MaxCombinedImageUnitsAndFragmentOutputs") == 0)
            Resources.maxCombinedImageUnitsAndFragmentOutputs = value;
        else if (strcmp(token, "MaxCombinedShaderOutputResources") == 0)
            Resources.maxCombinedShaderOutputResources = value;
        else if (strcmp(token, "MaxImageSamples") == 0)
            Resources.maxImageSamples = value;
        else if (strcmp(token, "MaxVertexImageUniforms") == 0)
            Resources.maxVertexImageUniforms = value;
        else if (strcmp(token, "MaxTessControlImageUniforms") == 0)
            Resources.maxTessControlImageUniforms = value;
        else if (strcmp(token, "MaxTessEvaluationImageUniforms") == 0)
            Resources.maxTessEvaluationImageUniforms = value;
        else if (strcmp(token, "MaxGeometryImageUniforms") == 0)
            Resources.maxGeometryImageUniforms = value;
        else if (strcmp(token, "MaxFragmentImageUniforms") == 0)
            Resources.maxFragmentImageUniforms = value;
        else if (strcmp(token, "MaxCombinedImageUniforms") == 0)
            Resources.maxCombinedImageUniforms = value;
        else if (strcmp(token, "MaxGeometryTextureImageUnits") == 0)
            Resources.maxGeometryTextureImageUnits = value;
        else if (strcmp(token, "MaxGeometryOutputVertices") == 0)
            Resources.maxGeometryOutputVertices = value;
        else if (strcmp(token, "MaxGeometryTotalOutputComponents") == 0)
            Resources.maxGeometryTotalOutputComponents = value;
        else if (strcmp(token, "MaxGeometryUniformComponents") == 0)
            Resources.maxGeometryUniformComponents = value;
        else if (strcmp(token, "MaxGeometryVaryingComponents") == 0)
            Resources.maxGeometryVaryingComponents = value;
        else if (strcmp(token, "MaxTessControlInputComponents") == 0)
            Resources.maxTessControlInputComponents = value;
        else if (strcmp(token, "MaxTessControlOutputComponents") == 0)
            Resources.maxTessControlOutputComponents = value;
        else if (strcmp(token, "MaxTessControlTextureImageUnits") == 0)
            Resources.maxTessControlTextureImageUnits = value;
        else if (strcmp(token, "MaxTessControlUniformComponents") == 0)
            Resources.maxTessControlUniformComponents = value;
        else if (strcmp(token, "MaxTessControlTotalOutputComponents") == 0)
            Resources.maxTessControlTotalOutputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationInputComponents") == 0)
            Resources.maxTessEvaluationInputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationOutputComponents") == 0)
            Resources.maxTessEvaluationOutputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationTextureImageUnits") == 0)
            Resources.maxTessEvaluationTextureImageUnits = value;
        else if (strcmp(token, "MaxTessEvaluationUniformComponents") == 0)
            Resources.maxTessEvaluationUniformComponents = value;
        else if (strcmp(token, "MaxTessPatchComponents") == 0)
            Resources.maxTessPatchComponents = value;
        else if (strcmp(token, "MaxPatchVertices") == 0)
            Resources.maxPatchVertices = value;
        else if (strcmp(token, "MaxTessGenLevel") == 0)
            Resources.maxTessGenLevel = value;
        else if (strcmp(token, "MaxViewports") == 0)
            Resources.maxViewports = value;
        else if (strcmp(token, "MaxVertexAtomicCounters") == 0)
            Resources.maxVertexAtomicCounters = value;
        else if (strcmp(token, "MaxTessControlAtomicCounters") == 0)
            Resources.maxTessControlAtomicCounters = value;
        else if (strcmp(token, "MaxTessEvaluationAtomicCounters") == 0)
            Resources.maxTessEvaluationAtomicCounters = value;
        else if (strcmp(token, "MaxGeometryAtomicCounters") == 0)
            Resources.maxGeometryAtomicCounters = value;
        else if (strcmp(token, "MaxFragmentAtomicCounters") == 0)
            Resources.maxFragmentAtomicCounters = value;
        else if (strcmp(token, "MaxCombinedAtomicCounters") == 0)
            Resources.maxCombinedAtomicCounters = value;
        else if (strcmp(token, "MaxAtomicCounterBindings") == 0)
            Resources.maxAtomicCounterBindings = value;
        else if (strcmp(token, "MaxVertexAtomicCounterBuffers") == 0)
            Resources.maxVertexAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxTessControlAtomicCounterBuffers") == 0)
            Resources.maxTessControlAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxTessEvaluationAtomicCounterBuffers") == 0)
            Resources.maxTessEvaluationAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxGeometryAtomicCounterBuffers") == 0)
            Resources.maxGeometryAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxFragmentAtomicCounterBuffers") == 0)
            Resources.maxFragmentAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxCombinedAtomicCounterBuffers") == 0)
            Resources.maxCombinedAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxAtomicCounterBufferSize") == 0)
            Resources.maxAtomicCounterBufferSize = value;
        else if (strcmp(token, "MaxTransformFeedbackBuffers") == 0)
            Resources.maxTransformFeedbackBuffers = value;
        else if (strcmp(token, "MaxTransformFeedbackInterleavedComponents") == 0)
            Resources.maxTransformFeedbackInterleavedComponents = value;
        else if (strcmp(token, "MaxCullDistances") == 0)
            Resources.maxCullDistances = value;
        else if (strcmp(token, "MaxCombinedClipAndCullDistances") == 0)
            Resources.maxCombinedClipAndCullDistances = value;
        else if (strcmp(token, "MaxSamples") == 0)
            Resources.maxSamples = value;
        else if (strcmp(token, "MaxMeshOutputVerticesEXT") == 0)
            Resources.maxMeshOutputVerticesEXT = value;
        else if (strcmp(token, "MaxMeshOutputPrimitivesEXT") == 0)
            Resources.maxMeshOutputPrimitivesEXT = value;
        else if (strcmp(token, "MaxMeshWorkGroupSizeX_EXT") == 0)
            Resources.maxMeshWorkGroupSizeX_EXT = value;
        else if (strcmp(token, "MaxMeshWorkGroupSizeY_EXT") == 0)
            Resources.maxMeshWorkGroupSizeY_EXT = value;
        else if (strcmp(token, "MaxMeshWorkGroupSizeZ_EXT") == 0)
            Resources.maxMeshWorkGroupSizeZ_EXT = value;
        else if (strcmp(token, "MaxTaskWorkGroupSizeX_EXT") == 0)
            Resources.maxTaskWorkGroupSizeX_EXT = value;
        else if (strcmp(token, "MaxTaskWorkGroupSizeY_EXT") == 0)
            Resources.maxTaskWorkGroupSizeY_EXT = value;
        else if (strcmp(token, "MaxTaskWorkGroupSizeZ_EXT") == 0)
            Resources.maxTaskWorkGroupSizeZ_EXT = value;
        else if (strcmp(token, "MaxMeshViewCountEXT") == 0)
            Resources.maxMeshViewCountEXT = value;
        else if (strcmp(token, "MaxDualSourceDrawBuffersEXT") == 0)
            Resources.maxDualSourceDrawBuffersEXT = value;

        else if (strcmp(token, "nonInductiveForLoops") == 0)
            Resources.limits.nonInductiveForLoops = (value != 0);
        else if (strcmp(token, "whileLoops") == 0)
            Resources.limits.whileLoops = (value != 0);
        else if (strcmp(token, "doWhileLoops") == 0)
            Resources.limits.doWhileLoops = (value != 0);
        else if (strcmp(token, "generalUniformIndexing") == 0)
            Resources.limits.generalUniformIndexing = (value != 0);
        else if (strcmp(token, "generalAttributeMatrixVectorIndexing") == 0)
            Resources.limits.generalAttributeMatrixVectorIndexing = (value != 0);
        else if (strcmp(token, "generalVaryingIndexing") == 0)
            Resources.limits.generalVaryingIndexing = (value != 0);
        else if (strcmp(token, "generalSamplerIndexing") == 0)
            Resources.limits.generalSamplerIndexing = (value != 0);
        else if (strcmp(token, "generalVariableIndexing") == 0)
            Resources.limits.generalVariableIndexing = (value != 0);
        else if (strcmp(token, "generalConstantMatrixVectorIndexing") == 0)
            Resources.limits.generalConstantMatrixVectorIndexing = (value != 0);
        else
            printf("Warning: unrecognized limit (%s) in configuration file.\n", token);

        token = strtok(0, delims);
    }
}

// =====================================================================================================================
// Get SPIR-V target environment from the input SPIR-V binary
spv_target_env GetSpirvTargetEnv(
    const uint32_t* pSpvToken)
{
    spv_target_env targetEnv = SPV_ENV_UNIVERSAL_1_0;

    assert(pSpvToken[0] == spv::MagicNumber);
    unsigned int version = pSpvToken[1];
    unsigned int versionMajor = ((version >> 16) & 0xFF);
    unsigned int versionMinor = ((version >> 8) & 0xFF);

    if ((versionMajor == 1) && (versionMinor == 0))
    {
        targetEnv = SPV_ENV_UNIVERSAL_1_0;
    }
    else if ((versionMajor == 1) && (versionMinor == 1))
    {
        targetEnv = SPV_ENV_UNIVERSAL_1_1;
    }
    else if ((versionMajor == 1) && (versionMinor == 2))
    {
        targetEnv = SPV_ENV_UNIVERSAL_1_2;
    }
    else if ((versionMajor == 1) && (versionMinor == 3))
    {
        targetEnv = SPV_ENV_UNIVERSAL_1_3;
    }
    else if ((versionMajor == 1) && (versionMinor == 4))
    {
        targetEnv = SPV_ENV_UNIVERSAL_1_4;
    }
    else if ((versionMajor == 1) && (versionMinor == 5))
    {
        targetEnv = SPV_ENV_UNIVERSAL_1_5;
    }
    else if ((versionMajor == 1) && (versionMinor == 6))
    {
        targetEnv = SPV_ENV_UNIVERSAL_1_6;
    }
    else
    {
        assert(!"Unknown SPIR-V version"); // Should be known version
    }

    return targetEnv;
}

// =====================================================================================================================
// Get SPIR-V target environment from the input SPIR-V text
spv_target_env GetSpirvTargetEnv(
    const char* pSpvText)
{
    spv_target_env targetEnv = SPV_ENV_UNIVERSAL_1_3; // Set the default to SPIR-V 1.3

    std::string spvText(pSpvText);

    auto pos = spvText.find("; Version: ");
    if (pos != std::string::npos)
    {
        pos += strlen("; Version: ");
        unsigned int versionMajor = spvText[pos] - '0';
        unsigned int versionMinor = spvText[pos + 2] - '0';
        if ((versionMajor == 1) && (versionMinor == 0))
        {
            targetEnv = SPV_ENV_UNIVERSAL_1_0;
        }
        else if ((versionMajor == 1) && (versionMinor == 1))
        {
            targetEnv = SPV_ENV_UNIVERSAL_1_1;
        }
        else if ((versionMajor == 1) && (versionMinor == 2))
        {
            targetEnv = SPV_ENV_UNIVERSAL_1_2;
        }
        else if ((versionMajor == 1) && (versionMinor == 3))
        {
            targetEnv = SPV_ENV_UNIVERSAL_1_3;
        }
        else if ((versionMajor == 1) && (versionMinor == 4))
        {
            targetEnv = SPV_ENV_UNIVERSAL_1_4;
        }
        else if ((versionMajor == 1) && (versionMinor == 5))
        {
            targetEnv = SPV_ENV_UNIVERSAL_1_5;
        }
        else if ((versionMajor == 1) && (versionMinor == 6))
        {
            targetEnv = SPV_ENV_UNIVERSAL_1_6;
        }
    }

    return targetEnv;
}

// =====================================================================================================================
// *.conf => this is a config file that can set limits/resources
bool SetConfigFile(
    const std::string& name)
{
    if (name.size() < 5)
        return false;

    if (name.compare(name.size() - 5, 5, ".conf") == 0) {
        *pConfigFile = name;
        return true;
    }

    return false;
}

// =====================================================================================================================
// Translate the meaningful subset of command-line options to parser-behavior options.
void SetMessageOptions(EShMessages& messages, int options)
{
    messages = (EShMessages)(messages | EShMsgSpvRules);
    if (options & SpvGenOptionVulkanRules)
        messages = (EShMessages)(messages | EShMsgVulkanRules);
    if (options & SpvGenOptionReadHlsl)
        messages = (EShMessages)(messages | EShMsgReadHlsl);
    if (options & SpvGenOptionHlslOffsets)
        messages = (EShMessages)(messages | EShMsgHlslOffsets);
    if (options & SpvGenOptionDebug)
        messages = (EShMessages)(messages | EShMsgDebugInfo);
    if (options & SpvGenOptionOptimizeDisable)
        messages = (EShMessages)(messages | EShMsgHlslLegalization);
    if (options & SpvGenOptionHlslDX9compatible)
        messages = (EShMessages)(messages | EShMsgHlslDX9Compatible);
    if (options & SpvGenOptionHlslEnable16BitTypes)
        messages = (EShMessages)(messages | EShMsgHlslEnable16BitTypes);
}

// =====================================================================================================================
// Represents the result of spvCompileAndLinkProgram*
class SpvProgram
{
public:
    // Constructor
    SpvProgram(uint32_t stageCount)
        :
        spirvs(stageCount)
    {
        programs.push_back(new glslang::TProgram);
    }

    // Destructor
    ~SpvProgram()
    {
        for (uint32_t i = 0; i < programs.size(); ++i)
        {
            delete programs[i];
        }
        programs.clear();
    }

    // Add shader to current program
    void addShader(glslang::TShader* shader)
    {
        GetCurrentProgram()->addShader(shader);
    }

    // Link shader for current program
    bool link(EShMessages message)
    {
        return GetCurrentProgram()->link(message);
    }

    // Map IO for current program
    bool mapIO()
    {
        return GetCurrentProgram()->mapIO();
    }

    // Get link log from current program
    const char* getInfoLog()
    {
        return GetCurrentProgram()->getInfoLog();
    }

    // Get link debug log from current program
    const char* getInfoDebugLog()
    {
        return GetCurrentProgram()->getInfoLog();
    }

    // Get intermediate tree from current program with specified shader stage
    glslang::TIntermediate* getIntermediate(
        EShLanguage stage
        ) const
    {
        return GetCurrentProgram()->getIntermediate(stage);
    }

    // Add log to SPV program log
    void AddLog(
        const char* log)
    {
        programLog += log;
    };

    // Format link info
    void FormatLinkInfo(
        const char*  linkInfo,
        std::string& formatedString)
    {
        char buffer[256];
        char* infoEntry[EShLangCount] = {};
        int length = (int)strlen(linkInfo);
        char* pInfoCopy = new char [length + 1];
        char* pLog = pInfoCopy;
        strcpy(pLog, linkInfo);
        for (int i = 0; i < EShLangCount; ++i)
        {
            sprintf(buffer, "\nLinked %s stage:\n\n", glslang::StageName((EShLanguage)i));
            infoEntry[i] = strstr(pLog, buffer);
            if (infoEntry[i] != NULL)
            {
                infoEntry[i][0] = '\0';
                infoEntry[i] += strlen(buffer);
                pLog = infoEntry[i];
            }
        }

        for (int i = 0; i < EShLangCount; ++i)
        {
            if ((infoEntry[i] != nullptr) && (strlen(infoEntry[i]) > 0))
            {
                sprintf(buffer, "Linking %s stage:\n", glslang::StageName((EShLanguage)i));
                formatedString += buffer;
                formatedString += infoEntry[i];
            }
        }
        delete[] pInfoCopy;
    }

    // Get current program
    glslang::TProgram* GetCurrentProgram()
    {
        return programs.back();
    }

    const glslang::TProgram* GetCurrentProgram() const
    {
        return programs.back();
    }

    // Append a new program
    void AddProgram()
    {
        programs.push_back(new glslang::TProgram);
    }

    std::string                             programLog;
    std::vector<glslang::TProgram*>         programs;
    std::vector<std::vector<unsigned int> > spirvs;
};

// =====================================================================================================================
// Compile and link GLSL source from file list
bool SH_IMPORT_EXPORT spvCompileAndLinkProgramFromFile(
    int          fileNum,
    const char*  fileList[],
    void**       ppProgram,
    const char** ppLog)
{
    return spvCompileAndLinkProgramFromFileEx(fileNum,
                                              fileList,
                                              nullptr,
                                              ppProgram,
                                              ppLog,
                                              DefaultOptions);
}

// =====================================================================================================================
// Compile and link GLSL source from file list with full parameters
bool SH_IMPORT_EXPORT spvCompileAndLinkProgramFromFileEx(
    int             fileNum,
    const char*     fileList[],
    const char*     entryPoints[],
    void**          ppProgram,
    const char**    ppLog,
    int             options)
{
    std::vector<std::string> sources(fileNum);
    std::vector<SpvGenStage> stageTypes(fileNum);
    std::vector<int>         sourceCount(fileNum);
    std::vector<const char*> sourcesPtr(fileNum);
    std::vector<const char*const*> sourceListPtr(fileNum);
    std::vector<const char*const*> fileListPtr(fileNum);
    bool isHlsl = false;

    for (int i = 0; i < fileNum; ++i)
    {
        stageTypes[i] = spvGetStageTypeFromName(fileList[i], &isHlsl);
        if (ReadFileData(fileList[i], sources[i]))
        {
            return false;
        }
        sourceCount[i] = 1;
    }

    for (int i = 0; i < fileNum; ++i)
    {
        sourcesPtr[i] = sources[i].c_str();
        sourceListPtr[i] = &sourcesPtr[i];
        fileListPtr[i] = &fileList[i];
    }

    if (isHlsl)
    {
        options |= SpvGenOptionReadHlsl;
    }
    return spvCompileAndLinkProgramEx(fileNum,
                                      &stageTypes[0],
                                      &sourceCount[0],
                                      &sourceListPtr[0],
                                      &fileListPtr[0],
                                      entryPoints,
                                      ppProgram,
                                      ppLog,
                                      options);
}

// =====================================================================================================================
// Compile and link GLSL source strings, and the result is stored in pProgram
bool SH_IMPORT_EXPORT spvCompileAndLinkProgram(
    int                  shaderStageSourceCounts[SpvGenNativeStageCount],
    const char* const *  shaderStageSources[SpvGenNativeStageCount],
    void**               ppProgram,
    const char**         ppLog)
{
    static const SpvGenStage stageTypes[SpvGenNativeStageCount] =
    {
        SpvGenStageTask,
        SpvGenStageVertex,
        SpvGenStageTessControl,
        SpvGenStageTessEvaluation,
        SpvGenStageGeometry,
        SpvGenStageMesh,
        SpvGenStageFragment,
        SpvGenStageCompute,
    };
    static const char* const* fileList[SpvGenNativeStageCount] = {};
    return spvCompileAndLinkProgramEx(SpvGenNativeStageCount,
                                     stageTypes,
                                     shaderStageSourceCounts,
                                     shaderStageSources,
                                     fileList,
                                     nullptr,
                                     ppProgram,
                                     ppLog,
                                     DefaultOptions);
}

// =====================================================================================================================
// Compile and link GLSL source strings with full parameters
bool SH_IMPORT_EXPORT spvCompileAndLinkProgramEx(
    int                  stageCount,
    const SpvGenStage*   stageTypeList,
    const int*           shaderStageSourceCounts,
    const char* const *  shaderStageSources[],
    const char* const *  fileList[],
    const char*          entryPoints[],
    void**               ppProgram,
    const char**         ppLog,
    int                  options)
{
    // Set the version of the input semantics.
    const int ClientInputSemanticsVersion = 100;

    EShMessages messages = EShMsgDefault;
    SetMessageOptions(messages, options);

    bool compileFailed = false;
    bool linkFailed = false;

    SpvProgram* pProgram = new SpvProgram(stageCount);
    *ppProgram = pProgram;

    uint32_t stageMask = 0;
    std::vector<glslang::TShader*> shaders(stageCount);
    for (int i = 0, linkIndexBase = 0; i < stageCount; ++i)
    {
        if (shaderStageSourceCounts[i] > 0)
        {
            assert(shaderStageSources[i] != nullptr);
            assert(stageTypeList[i] < SpvGenStageCount);
            EShLanguage stage = SpvGenStageToEShLanguage(stageTypeList[i]);

            // Per-shader processing...
            glslang::TShader* pShader = new glslang::TShader(stage);
            shaders[i] = pShader;

            if (fileList == nullptr || fileList[i] == nullptr)
            {
                pShader->setStrings(shaderStageSources[i], shaderStageSourceCounts[i]);
            }
            else
            {
                pShader->setStringsWithLengthsAndNames(shaderStageSources[i], nullptr, fileList[i], shaderStageSourceCounts[i]);
            }

            if (options & SpvGenOptionVulkanRules)
            {
                pShader->setEnvInput((options & SpvGenOptionReadHlsl) ? glslang::EShSourceHlsl : glslang::EShSourceGlsl,
                                    stage,
                                    glslang::EShClientVulkan,
                                    ClientInputSemanticsVersion);
                pShader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
            }
            else
            {
                assert((options & SpvGenOptionReadHlsl) == 0); // Assume OGL don't use HLSL
                pShader->setEnvInput((options & SpvGenOptionReadHlsl) ? glslang::EShSourceHlsl : glslang::EShSourceGlsl,
                                    stage,
                                    glslang::EShClientOpenGL,
                                    ClientInputSemanticsVersion);
                pShader->setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
            }

            pShader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

            if (entryPoints && entryPoints[i])
            {
                pShader->setEntryPoint(entryPoints[i]);
            }

            if ((options & SpvGenOptionFlattenUniformArrays) != 0 &&
                (options & SpvGenOptionReadHlsl) == 0)
            {
                pProgram->AddLog("uniform array flattening only valid when compiling HLSL source.");
                return false;
            }

            pShader->setFlattenUniformArrays((options & SpvGenOptionFlattenUniformArrays) != 0);

            if (options & SpvGenOptionHlslIoMapping)
            {
                pShader->setHlslIoMapping(true);
            }

            if (options & SpvGenOptionAutoMapBindings)
            {
                pShader->setAutoMapBindings(true);
            }

            if (options & SpvGenOptionAutoMapLocations)
            {
                pShader->setAutoMapLocations(true);
            }

            if (options & SpvGenOptionInvertY)
            {
                pShader->setInvertY(true);
            }

            DirStackFileIncluder includer;
            compileFailed = !pShader->parse(&Resources,
                (options & SpvGenOptionDefaultDesktop) ? 110 : 100,
                false,
                messages,
                includer);

            if (compileFailed == false)
            {
                pProgram->addShader(pShader);
                stageMask |= (1 << stageTypeList[i]);
            }

            if ((options & SpvGenOptionSuppressInfolog) == false)
            {
                const char* pInfoLog = pShader->getInfoLog();
                const char* pDebugLog = pShader->getInfoDebugLog();
                if ((strlen(pInfoLog) > 0) || (strlen(pDebugLog) > 0))
                {
                    char buffer[256];
                    sprintf(buffer, "Compiling %s stage:\n", glslang::StageName(stage));
                    pProgram->AddLog(buffer);
                    pProgram->AddLog(pShader->getInfoLog());
                    pProgram->AddLog(pShader->getInfoDebugLog());
                }
            }
        }

        bool doLink = false;
        if (compileFailed == false)
        {
            if (i == stageCount - 1)
            {
                doLink = true;
            }
            else if (shaderStageSourceCounts[i + 1] > 0)
            {
                if (stageMask & (1 << stageTypeList[i + 1]))
                {
                    doLink = true;
                }
            }

            if (doLink)
            {
                // Program-level processing...
                linkFailed = !pProgram->link(messages);

                // Map IO, consistent with glslangValidator StandAlone: https://github.com/KhronosGroup/glslang/blob/master/StandAlone/StandAlone.cpp line 1138
                linkFailed = !pProgram->mapIO();

                if ((options & SpvGenOptionSuppressInfolog) == false)
                {
                    const char* pInfoLog = pProgram->getInfoLog();
                    const char* pDebugLog = pProgram->getInfoDebugLog();
                    std::string formatLog;
                    pProgram->FormatLinkInfo(pInfoLog, formatLog);
                    if (formatLog.size() > 0 || strlen(pDebugLog) > 0)
                    {
                        pProgram->AddLog(formatLog.c_str());
                        pProgram->AddLog(pDebugLog);
                    }
                }

                if (linkFailed)
                {
                    *ppLog = pProgram->programLog.c_str();
                    return false;
                }

                for (int linkIndex = linkIndexBase; linkIndex <= i; ++linkIndex)
                {
                    if (shaders[linkIndex] != nullptr)
                    {
                        EShLanguage linkStage = shaders[linkIndex]->getStage();
                        auto pIntermediate = pProgram->getIntermediate(linkStage);
                        glslang::SpvOptions spvOptions = {};
                        spvOptions.generateDebugInfo = (options & SpvGenOptionDebug) != 0;
                        spvOptions.disableOptimizer = (options & SpvGenOptionOptimizeDisable) != 0;
                        spvOptions.optimizeSize = (options & SpvGenOptionOptimizeSize) != 0;
                        glslang::GlslangToSpv(*pIntermediate, pProgram->spirvs[linkIndex], &spvOptions);
                    }
                }
                linkIndexBase = i + 1;
                pProgram->AddProgram();
                stageMask = 0;
            }
        }
    }

    for (size_t i = 0; i < shaders.size(); ++i)
    {
        delete shaders[i];
    }
    shaders.clear();

    *ppLog = pProgram->programLog.c_str();
    return (compileFailed || linkFailed) ? false : true;
}

// =====================================================================================================================
// Destroies SpvProgram object
void SH_IMPORT_EXPORT spvDestroyProgram(
    void* hProgram)
{
    SpvProgram* pProgram = reinterpret_cast<SpvProgram*>(hProgram);
    delete pProgram;
}

// =====================================================================================================================
// Get SPRIV binary from OGLProgram object with specified shader stage, and return the binary size in bytes
//
// NOTE: 0 is returned if SPIRVI binary isn't exist for specified shader stage
int SH_IMPORT_EXPORT spvGetSpirvBinaryFromProgram(
    void*                hProgram,
    int                  stage,
    const unsigned int** ppData)
{
    SpvProgram* pProgram = reinterpret_cast<SpvProgram*>(hProgram);
    int programSize = (int)(pProgram->spirvs[stage].size() * sizeof(unsigned int));
    if (programSize > 0)
    {
        *ppData = &pProgram->spirvs[stage][0];
    }
    else
    {
        *ppData = nullptr;
    }
    return programSize;
}

// =====================================================================================================================
// Deduce the language from the filename.  Files must end in one of the following extensions:
SpvGenStage SH_IMPORT_EXPORT spvGetStageTypeFromName(
    const char* pName,
    bool*       pIsHlsl)
{
    std::string name = pName;
    size_t ext = name.rfind('.');
    if (ext == std::string::npos)
    {
        return SpvGenStageInvalid;
    }

    std::string suffix = name.substr(ext + 1, std::string::npos);
    if (suffix == "glsl" || suffix == "hlsl")
    {
        if (suffix == "hlsl")
        {
            *pIsHlsl = true;
        }

        // for .vert.glsl, .tesc.glsl, ..., .comp.glsl/.hlsl compound suffixes
        name = name.substr(0, ext);
        ext = name.rfind('.');
        if (ext == std::string::npos) {
            return SpvGenStageInvalid;
        }
        suffix = name.substr(ext + 1, std::string::npos);
    }

    if (suffix == "task")
        return SpvGenStageTask;
    else if (suffix == "vert")
        return SpvGenStageVertex;
    else if (suffix == "tesc")
        return SpvGenStageTessControl;
    else if (suffix == "tese")
        return SpvGenStageTessEvaluation;
    else if (suffix == "geom")
        return SpvGenStageGeometry;
    else if (suffix == "mesh")
        return SpvGenStageMesh;
    else if (suffix == "frag")
        return SpvGenStageFragment;
    else if (suffix == "comp")
        return SpvGenStageCompute;
    else if (suffix == "rgen")
        return SpvGenStageRayTracingRayGen;
    else if (suffix == "rint")
        return SpvGenStageRayTracingIntersect;
    else if (suffix == "rahit")
        return SpvGenStageRayTracingAnyHit;
    else if (suffix == "rchit")
        return SpvGenStageRayTracingClosestHit;
    else if (suffix == "rmiss")
        return SpvGenStageRayTracingMiss;
    else if (suffix == "rcall")
        return SpvGenStageRayTracingCallable;

    return SpvGenStageInvalid;
}

// =====================================================================================================================
// Gets component version according to specified SpvGenVersion enum
bool SH_IMPORT_EXPORT spvGetVersion(
    SpvGenVersion version,
    unsigned int* pVersion,
    unsigned int* pReversion)
{
    bool result = true;
    switch (version)
    {
    case SpvGenVersionGlslang:
        {
            *pVersion = glslang::GetSpirvGeneratorVersion();
            *pReversion = GLSLANG_VERSION_MINOR;
            break;
        }
    case SpvGenVersionSpirv:
        {
            *pVersion = spv::Version;
            *pReversion = spv::Revision;
            break;
        }
    case SpvGenVersionStd450:
        {
            *pVersion = spv::GLSLstd450Version;
            *pReversion = spv::GLSLstd450Revision;
            break;
        }
    case SpvGenVersionExtAmd:
        {
            *pVersion = spv::GLSLextAMDVersion;
            *pReversion = spv::GLSLextAMDRevision;
            break;
        }
    case SpvGenVersionSpvGen:
        {
            *pVersion = SPVGEN_VERSION;
            *pReversion = SPVGEN_REVISION;
            break;
        }
    default:
        {
            result = false;
            break;
        }
    }

    return result;
}

// =====================================================================================================================
// Assemble SPIR-V text, store the result in pBuffer and return SPIRV code size in byte
//
// NOTE: If assemble success, *ppLog is nullptr, otherwise, *ppLog is the error message and -1 is returned.
int SH_IMPORT_EXPORT spvAssembleSpirv(
    const char*    pSpvText,
    unsigned int   bufSize,
    unsigned int*  pBuffer,
    const char**   ppLog)
{
    int retval = -1;

    uint32_t options = SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS;
    spv_binary binary;
    spv_diagnostic diagnostic = nullptr;
    spv_context context = spvContextCreate(GetSpirvTargetEnv(pSpvText));
    spv_result_t result = spvTextToBinaryWithOptions(context, pSpvText,
                                                     strlen(pSpvText), options, &binary, &diagnostic);
    spvContextDestroy(context);
    if (result == SPV_SUCCESS)
    {
        unsigned int codeSize = static_cast<unsigned int>(binary->wordCount * sizeof(uint32_t));
        if (codeSize > bufSize)
        {
            codeSize = bufSize;
        }
        memcpy(pBuffer, binary->code, codeSize);
        *ppLog = nullptr;
        retval = codeSize;
        spvBinaryDestroy(binary);
    }
    else
    {
        *ppLog = reinterpret_cast<const char*>(pBuffer);
        spvDiagnosticPrint(diagnostic, reinterpret_cast<char*>(pBuffer), static_cast<size_t>(bufSize));
        spvDiagnosticDestroy(diagnostic);
        retval = -1;
    }

    return retval;
}

// =====================================================================================================================
// Disassemble SPIR-V binary token using khronos spirv-tools, and store the output text to pBuffer
//
// NOTE: The text will be clampped if buffer size is less than requirement.
bool SH_IMPORT_EXPORT spvDisassembleSpirv(
    unsigned int   size,
    const void*    pSpvToken,
    unsigned int   bufSize,
    char*          pBuffer)
{
    uint32_t options = SPV_BINARY_TO_TEXT_OPTION_INDENT | SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES;
    // If printing to standard output, then spvBinaryToText should
    // do the printing.  In particular, colour printing on Windows is
    // controlled by modifying console objects synchronously while
    // outputting to the stream rather than by injecting escape codes
    // into the output stream.
    // If the printing option is off, then save the text in memory, so
    // it can be emitted later in this function.
    spv_text text = nullptr;
    spv_diagnostic diagnostic = nullptr;
    spv_context context = spvContextCreate(GetSpirvTargetEnv(static_cast<const uint32_t*>(pSpvToken)));
    spv_result_t result =
        spvBinaryToText(context, reinterpret_cast<const uint32_t*>(pSpvToken),
                        static_cast<const size_t>(size) / sizeof(uint32_t),
                        options, &text, &diagnostic);
    spvContextDestroy(context);
    bool success = (result == SPV_SUCCESS);
    if (success)
    {
        unsigned int textSize = (unsigned int)(text->length);
        memcpy(pBuffer, text->str, textSize);
        pBuffer[textSize] = '\0';
    }
    else
    {
        spvDiagnosticPrint(diagnostic, pBuffer, bufSize);
        spvDiagnosticDestroy(diagnostic);
    }

    spvTextDestroy(text);
    return success;
}

// =====================================================================================================================
// convert SPIR-V binary token to GLSL using Khronos SPIRV-Cross,
//
// NOTE: ppGlslSource should be freed by spvFreeBuffer
bool SH_IMPORT_EXPORT spvCrossSpirv(
    SpvSourceLanguage   sourceLanguage,
    unsigned int        size,
    const void*         pSpvToken,
    char**              ppSourceString)
{
    return spvCrossSpirvEx(sourceLanguage, 0, size, pSpvToken, ppSourceString);
}

// =====================================================================================================================
// convert SPIR-V binary token to other shader languages using Khronos SPIRV-Cross,
//
// NOTE: ppGlslSource should be freed by spvFreeBuffer
// You need to calculate the version number of sourceLanguage; if version is set to 0, will use the default version (GLSL 450).
// For HLSL, the default version is 30 (shader model 3); version = major * 10 + minor;
// For MSL, the default version is 1.2; use make_msl_version to calculate the version number: version = (major * 10000) + (minor * 100) + patch;
bool SH_IMPORT_EXPORT spvCrossSpirvEx(
    SpvSourceLanguage   sourceLanguage,
    uint32_t            version,
    unsigned int        size,
    const void*         pSpvToken,
    char**              ppSourceString)
{
    bool success = true;
    std::string sourceString = "";
    try
    {
        std::vector<uint32_t> spvBinary(size / sizeof(uint32_t));
        memcpy(spvBinary.data(), pSpvToken, size);
        spirv_cross::Parser spvParser(std::move(spvBinary));
        spvParser.parse();

        bool combineImageSamplers = false;
        bool buildDummySampler = false;
        std::unique_ptr<spirv_cross::CompilerGLSL> pCompiler;
        if (sourceLanguage == SpvSourceLanguageMSL)
        {
            pCompiler.reset(new spirv_cross::CompilerMSL(std::move(spvParser.get_parsed_ir())));
            auto* pMslCompiler = static_cast<spirv_cross::CompilerMSL*>(pCompiler.get());
            auto mslOptions = pMslCompiler->get_msl_options();
            if (version != 0)
            {
                mslOptions.msl_version = version;
            }
            mslOptions.capture_output_to_buffer = false;
            mslOptions.swizzle_texture_samples = false;
            mslOptions.invariant_float_math = false;
            mslOptions.pad_fragment_output_components = false;
            mslOptions.tess_domain_origin_lower_left = false;
            mslOptions.argument_buffers = false;
            mslOptions.argument_buffers = false;
            mslOptions.texture_buffer_native = false;
            mslOptions.multiview = false;
            mslOptions.view_index_from_device_index = false;
            mslOptions.dispatch_base = false;
            mslOptions.enable_decoration_binding = false;
            mslOptions.force_active_argument_buffer_resources = false;
            mslOptions.force_native_arrays = false;
            mslOptions.enable_frag_depth_builtin = true;
            mslOptions.enable_frag_stencil_ref_builtin = true;
            mslOptions.enable_frag_output_mask = 0xffffffff;
            mslOptions.enable_clip_distance_user_varying = true;
            pMslCompiler->set_msl_options(mslOptions);
        }
        else if (sourceLanguage == SpvSourceLanguageHLSL)
        {
            pCompiler.reset(new spirv_cross::CompilerHLSL(std::move(spvParser.get_parsed_ir())));
        }
        else
        {
            if (sourceLanguage == SpvSourceLanguageVulkan)
            {
                combineImageSamplers = false;
            }
            else
            {
                buildDummySampler = true;
            }
            pCompiler.reset(new spirv_cross::CompilerGLSL(std::move(spvParser.get_parsed_ir())));
        }

        spirv_cross::CompilerGLSL::Options commonOptions = pCompiler->get_common_options();
        if (sourceLanguage == SpvSourceLanguageESSL)
        {
            commonOptions.es = true;
        }
        if (version != 0)
        {
            commonOptions.version = version;
        }
        commonOptions.force_temporary = false;
        commonOptions.separate_shader_objects = false;
        commonOptions.flatten_multidimensional_arrays = false;
        commonOptions.enable_420pack_extension = true;
        commonOptions.vulkan_semantics = true;
        commonOptions.vertex.fixup_clipspace = false;
        commonOptions.vertex.flip_vert_y = false;
        commonOptions.vertex.support_nonzero_base_instance = true;
        commonOptions.emit_push_constant_as_uniform_buffer = false;
        commonOptions.emit_uniform_buffer_as_plain_uniforms = false;
        commonOptions.emit_line_directives = false;
        commonOptions.enable_storage_image_qualifier_deduction = true;
        commonOptions.force_zero_initialized_variables = false;
        pCompiler->set_common_options(commonOptions);

        if (sourceLanguage == SpvSourceLanguageHLSL)
        {
            auto* pHlslCompiler = static_cast<spirv_cross::CompilerHLSL*>(pCompiler.get());
            auto hlslOptions = pHlslCompiler->get_hlsl_options();
            if (version != 0)
            {
                hlslOptions.shader_model = version;
            }
            hlslOptions.support_nonzero_base_vertex_base_instance = false;
            hlslOptions.force_storage_buffer_as_uav = false;
            hlslOptions.nonwritable_uav_texture_as_srv = false;
            hlslOptions.enable_16bit_types = false;
            pHlslCompiler->set_hlsl_options(hlslOptions);

            pHlslCompiler->set_resource_binding_flags(0);
        }

        if (buildDummySampler)
        {
            uint32_t sampler = pCompiler->build_dummy_sampler_for_combined_images();
            if (sampler != 0)
            {
                // Set some defaults to make validation happy.
                pCompiler->set_decoration(sampler, DecorationDescriptorSet, 0);
                pCompiler->set_decoration(sampler, DecorationBinding, 0);
            }
        }

        if (combineImageSamplers)
        {
            pCompiler->build_combined_image_samplers();
        }

        if (sourceLanguage == SpvSourceLanguageHLSL)
        {
            auto* pHlslCompiler = static_cast<spirv_cross::CompilerHLSL*>(pCompiler.get());
            uint32_t newBuiltin = pHlslCompiler->remap_num_workgroups_builtin();
            if (newBuiltin != 0)
            {
                pHlslCompiler->set_decoration(newBuiltin, DecorationDescriptorSet, 0);
                pHlslCompiler->set_decoration(newBuiltin, DecorationBinding, 0);
            }
        }
        sourceString = pCompiler->compile();

    }
    catch(const std::exception& e)
    {
        printf("SPIRV-Cross threw an exception : %s\n", e.what());
        assert(!e.what());
        success = false;
    }

    size_t sourceStringSize = sourceString.length() + 1;
    *ppSourceString = static_cast<char*>(malloc(sourceStringSize));
    memcpy(*ppSourceString, sourceString.c_str(), sourceStringSize);
    return success;
}

// =====================================================================================================================
// Validate SPIR-V binary token using khronos spirv-tools, and store the log text to pLog
//
// NOTE: The text will be clampped if buffer size is less than requirement.
bool SH_IMPORT_EXPORT spvValidateSpirv(
    unsigned int   size,
    const void*    pSpvToken,
    unsigned int   logSize,
    char*          pLog)
{
    spv_const_binary_t binary =
    {
        reinterpret_cast<const uint32_t*>(pSpvToken),
        static_cast<size_t>(size) / sizeof(uint32_t)
    };

    spv_diagnostic diagnostic = nullptr;
    spv_context context = spvContextCreate(GetSpirvTargetEnv(static_cast<const uint32_t*>(pSpvToken)));
    spv_result_t result = spvValidate(context, &binary, &diagnostic);
    spvContextDestroy(context);
    bool success = (result == SPV_SUCCESS);
    if (success == false)
    {
        spvDiagnosticPrint(diagnostic, pLog, logSize);
        spvDiagnosticDestroy(diagnostic);
    }

    return success;
}

// =====================================================================================================================
// Optimize SPIR-V binary token using khronos spirv-tools, and store optimized result to ppOptBuf and the log text
// to pLog
//
// NOTE: The text will be clampped if buffer size is less than requirement, and ppOptBuf should be freed by
// spvFreeBuffer
bool SH_IMPORT_EXPORT spvOptimizeSpirv(
    unsigned int   size,
    const void*    pSpvToken,
    int            optionCount,
    const char*    options[],
    unsigned int*  pBufSize,
    void**         ppOptBuf,
    unsigned int   logSize,
    char*          pLog)
{
    std::string errorMsg;
    spvtools::Optimizer optimizer(GetSpirvTargetEnv(static_cast<const uint32_t*>(pSpvToken)));
    optimizer.SetMessageConsumer([&](spv_message_level_t level,
                                     const char* source,
                                     const spv_position_t& position,
                                     const char* message)
        {
            const char* level_string = nullptr;
            switch (level)
            {
                case SPV_MSG_FATAL:
                    level_string = "fatal";
                    break;
                case SPV_MSG_INTERNAL_ERROR:
                    level_string = "internal error";
                    break;
                case SPV_MSG_ERROR:
                    level_string = "error";
                    break;
                case SPV_MSG_WARNING:
                    level_string = "warning";
                    break;
                case SPV_MSG_INFO:
                    level_string = "info";
                    break;
                case SPV_MSG_DEBUG:
                    level_string = "debug";
                    break;
            }
            std::ostringstream oss;
            oss << level_string << ": ";
            if (source) oss << source << ":";
            oss << position.line << ":" << position.column << ":";
            oss << position.index << ": ";
            if (message) oss << message;

            errorMsg += oss.str();
            errorMsg += "\n";
        }
    );

    if (optionCount == 0)
    {
        optimizer.RegisterPerformancePasses();
    }
    else
    {
        for (unsigned i = 0; i < optionCount; i++)
        {
            optimizer.RegisterPassFromFlag(options[i]);
        }
    }

    std::vector<uint32_t> binary;

    bool ret = optimizer.Run((const uint32_t*)pSpvToken, size / sizeof(uint32_t), &binary);

    if (ret)
    {
        *pBufSize = static_cast<uint32_t>(binary.size() * sizeof(uint32_t));
        *ppOptBuf = malloc(*pBufSize);
        memcpy(*ppOptBuf, binary.data(), *pBufSize);
    }

    if (logSize > 0)
    {
        if (errorMsg.empty())
        {
            pLog[0] = 0;
        }
        else
        {
            strncpy(pLog, errorMsg.c_str(), logSize);
        }
    }

    return ret;
}

// =====================================================================================================================
// Free input buffer
void SH_IMPORT_EXPORT spvFreeBuffer(
    void* pBuffer)
{
    free(pBuffer);
}

#if !defined _MSC_VER && !defined MINGW_HAS_SECURE_API

#include <errno.h>

int fopen_s(
   FILE** pFile,
   const char* filename,
   const char* mode
)
{
   if (!pFile || !filename || !mode) {
      return EINVAL;
   }

   FILE* f = fopen(filename, mode);
   if (! f) {
      if (errno != 0) {
         return errno;
      } else {
         return ENOENT;
      }
   }
   *pFile = f;

   return 0;
}

#endif

// =====================================================================================================================
// Convert SpvGenStage enumerant to corresponding EShLanguage enumerant.
EShLanguage SpvGenStageToEShLanguage(
    SpvGenStage stage) // SpvGenStage enumerant
{
    switch (stage)
    {
    case SpvGenStageTask:
        return EShLangTask;
    case SpvGenStageVertex:
        return EShLangVertex;
    case SpvGenStageTessControl:
        return EShLangTessControl;
    case SpvGenStageTessEvaluation:
        return EShLangTessEvaluation;
    case SpvGenStageGeometry:
        return EShLangGeometry;
    case SpvGenStageMesh:
        return EShLangMesh;
    case SpvGenStageFragment:
        return EShLangFragment;
    case SpvGenStageCompute:
        return EShLangCompute;
    case SpvGenStageRayTracingRayGen:
        return EShLangRayGen;
    case SpvGenStageRayTracingIntersect:
        return EShLangIntersect;
    case SpvGenStageRayTracingAnyHit:
        return EShLangAnyHit;
    case SpvGenStageRayTracingClosestHit:
        return EShLangClosestHit;
    case SpvGenStageRayTracingMiss:
        return EShLangMiss;
    case SpvGenStageRayTracingCallable:
        return EShLangCallable;
    default:
        assert(!"Unexpected SpvGenStage enumerant");
        return EShLangCount;
    }
}

// =====================================================================================================================
// Malloc a string of sufficient size and read a string into it.
bool ReadFileData(
    const char* pFileName,
    std::string& data)
{
    std::ifstream inFile(pFileName);
    if (!inFile.good())
    {
        return false;
    }
    inFile.seekg(0, std::ios::end);
    if (!inFile.good())
    {
        return false;
    }
    size_t size = inFile.tellg();
    data.resize(size, '\0');
    inFile.seekg(0);
    if (!inFile.good())
    {
        return false;
    }
    inFile.read(&data[0], size);
    if (!inFile.good())
    {
        return false;
    }
    data.resize(inFile.gcount());
    return true;
}

// =====================================================================================================================
// Compiler-specific wrapper of the standard vsnprintf implementation. If buffer is a nullptr it returns the length of
// the string that would be printed had a buffer with enough space been provided.
int Vsnprintf(
    char*       pOutput,  // [out] Output string.
    size_t      bufSize,  // Available space in pOutput (in bytes).
    const char* pFormat,  // Printf-style format string.
    va_list     argList)  // Pre-started variable argument list.
{
    // It is undefined to request a (count > 0) to be copied while providing a null buffer. Covers common causes of
    // crash in different versions of vsnprintf.
    assert((pOutput == nullptr) ? (bufSize == 0) : (bufSize > 0));

    int length = -1;

#if defined(_WIN32)
    if (pOutput == nullptr)
    {
        // Returns the resultant length of the formatted string excluding the null terminator.
        length = _vscprintf(pFormat, argList);
    }
    else
    {
        // MS compilers provide vsnprintf_s, which will truncate the sprintf to prevent buffer overruns, always
        // guarantee that pOutput is null-terminated, and verifies that "pFormat" doesn't have anything "bad" in it.
        length = vsnprintf_s(pOutput, bufSize, _TRUNCATE, pFormat, argList);
    }
#else
    // vsnprintf prints upto (bufSize - 1) entries leaving space for the terminating null character. On C99
    // compatible platforms, if a null buffer and size of zero is provided, it returns the length of the string.
    length = vsnprintf(pOutput, bufSize, pFormat, argList);
#endif

    return length;
}

// =====================================================================================================================
// Variable argument wrapper on sprintf function to be used when output needs to be written to a string and no prefix
// information is required.
int Snprintf(
    char*       pOutput,  // [out] Output string.
    size_t      bufSize,  // Available space in pOutput (in bytes).
    const char* pFormat,  // Printf-style format string.
    ...)                  // Printf-style argument list.
{
    va_list argList;
    va_start(argList, pFormat);

    int ret = Vsnprintf(pOutput, bufSize, pFormat, argList);

    va_end(argList);
    return ret;
}

// =====================================================================================================================
//  Print error message of spvTextToBinary() and spvBinaryToText()
spv_result_t spvDiagnosticPrint(const spv_diagnostic diagnostic, char* pBuffer, size_t bufferSize)
{
  if (!diagnostic) return SPV_ERROR_INVALID_DIAGNOSTIC;

  if (diagnostic->isTextSource) {
    // NOTE: This is a text position
    // NOTE: add 1 to the line as editors start at line 1, we are counting new
    // line characters to start at line 0
    Snprintf(pBuffer, bufferSize, "error: %d: %d: %s\n", diagnostic->position.line + 1, diagnostic->position.column + 1, diagnostic->error);
    return SPV_SUCCESS;
  } else {
    // NOTE: Assume this is a binary position
    Snprintf(pBuffer, bufferSize, "error: %d: %s\n", diagnostic->position.index, diagnostic->error);
    return SPV_SUCCESS;
  }
}

// =====================================================================================================================
// Internal initilization
static void internalInit()
{
    static bool init = false;
    if (!init) {
        ProcessConfigFile();
        glslang::InitializeProcess();
        spv::Parameterize();
        init = true;
    }
}

// =====================================================================================================================
// Cleanup
static void internalFinal()
{
    glslang::FinalizeProcess();
}

// =====================================================================================================================
// Initilize the static library
bool InitSpvGen(const char* pSpvGenDir)
{
#if defined(_WIN32)
    internalInit();
#endif
    return true;
}

// =====================================================================================================================
// Finalize the static library
void FinalizeSpvgen()
{
#if defined(_WIN32)
    internalFinal();
#endif
}

// =====================================================================================================================
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                       )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        internalInit();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        internalFinal();
        break;
    }
    return TRUE;
}
#else // Linux
__attribute__((constructor)) static void Init()
{
    ProcessConfigFile();
    glslang::InitializeProcess();
    spv::Parameterize();
}

__attribute__((destructor)) static void Destroy()
{
    if (pConfigFile != nullptr)
    {
        delete pConfigFile;
    }

    glslang::FinalizeProcess();
}

#endif

