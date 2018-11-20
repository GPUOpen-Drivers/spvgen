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

//Modifications Copyright (C) 2018 Advanced Micro Devices, Inc.

// spvgen.cpp : Defines the exported functions for the DLL application.
//

// this only applies to the standalone wrapper, not the front end in general
#include "glslang/Include/ShHandle.h"
#include "glslang/Include/revision.h"
#include "glslang/Public/ShaderLang.h"
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
#include "disassemble.h"
#include <sstream>
#include <string>
#include <vector>
#include <stdarg.h>

#include "spvgen.h"
#include "vfx.h"

#define MAX_PATH 256

extern "C" {
    SH_IMPORT_EXPORT void ShOutputHtml();
}

//
// Return codes from main.
//
enum TFailCode {
    ESuccess = 0,
    EFailUsage,
    EFailCompile,
    EFailLink,
    EFailCompilerCreate,
    EFailThreadCreate,
    EFailLinkerCreate
};

EShLanguage FindLanguage(const std::string& name);

void FreeFileData(char** data);
char** ReadFileData(const char* fileName);
void InfoLogMsg(const char* msg, const char* name, const int num);

int Vsnprintf(char* pOutput, size_t bufSize, const char* pFormat, va_list argList);
int Snprintf(char* pOutput, size_t bufSize, const char* pFormat, ...);
spv_result_t spvDiagnosticPrint(const spv_diagnostic diagnostic, char* pBuffer, size_t bufferSize);

// Globally track if any compile or link failure.
bool CompileFailed = false;
bool LinkFailed = false;

// Use to test breaking up a single shader file into multiple strings.
int NumShaderStrings;

TBuiltInResource Resources;
char ConfigFile[MAX_PATH];
int defaultOptions = EOptionDefaultDesktop | EOptionVulkanRules;

// Set the version of the input semantics.
int ClientInputSemanticsVersion = 100;
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

//
// Parse either a .conf file provided by the user or the default string above.
//
void ProcessConfigFile()
{
    char** configStrings = 0;
    char* config = 0;

    if (strlen(ConfigFile) > 0) {
        configStrings = ReadFileData(ConfigFile);
        if (configStrings)
            config = *configStrings;
        else {
            printf("Error opening configuration file; will instead use the default configuration\n");
        }
    }

    if (config == 0) {
        config = new char[strlen(DefaultConfig) + 1];
        strcpy(config, DefaultConfig);
    }

    const char* delims = " \t\n\r";
    const char* token = strtok(config, delims);
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
    if (configStrings)
        FreeFileData(configStrings);
}



const char* ExecutableName;

//
// *.conf => this is a config file that can set limits/resources
//
bool SetConfigFile(const std::string& name)
{
    if (name.size() < 5)
        return false;

    if (name.compare(name.size() - 5, 5, ".conf") == 0) {
        size_t copyLength = (name.size() < sizeof(ConfigFile)) ? name.size() : (sizeof(ConfigFile) - 1);
        strncpy(ConfigFile, name.c_str(), copyLength);
        ConfigFile[copyLength + 1] = '\0';

        return true;
    }

    return false;
}

//
// Translate the meaningful subset of command-line options to parser-behavior options.
//
void SetMessageOptions(EShMessages& messages, int options)
{
    messages = (EShMessages)(messages | EShMsgSpvRules);
    if (options & EOptionVulkanRules)
        messages = (EShMessages)(messages | EShMsgVulkanRules);
    if (options & EOptionReadHlsl)
        messages = (EShMessages)(messages | EShMsgReadHlsl);
    if (options & EOptionHlslOffsets)
        messages = (EShMessages)(messages | EShMsgHlslOffsets);
    if (options & EOptionDebug)
        messages = (EShMessages)(messages | EShMsgDebugInfo);
    if (options & EOptionOptimizeDisable)
        messages = (EShMessages)(messages | EShMsgHlslLegalization);
}

class OGLProgram : public glslang::TProgram
{
public:
    ~OGLProgram()
    {
        for(int i = 0; i< VkStageCount; ++i)
        {
            std::list<glslang::TShader*>::iterator it;
            for(it = stages[i].begin(); it != stages[i].end(); ++it)
            {
                delete *it;
            }
            stages[i].clear();
        }
    }
    void AddLog(const char* log) {
        programLog += log;
    };

    void FormatLinkInfo(const char* linkInfo, std::string& formatedString)
    {
        char buffer[256];
        char* infoEntry[VkStageCount] = {};
        int length = (int)strlen(linkInfo);
        char* pInfoCopy = new char [length + 1];
        char* pLog = pInfoCopy;
        strcpy(pLog, linkInfo);
        for (int i = 0; i < VkStageCount; ++i)
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

        for (int i = 0; i < VkStageCount; ++i)
        {
            if ((infoEntry[i] != nullptr) && (strlen(infoEntry[i]) > 0))
            {
                sprintf(buffer, "Linking %s stage:\n", glslang::StageName((EShLanguage)i));
                formatedString += buffer;
                formatedString += infoEntry[i];
            }
        }
    }
    std::string               programLog;
    std::vector<unsigned int> spirv[VkStageCount];
};

//
// Compile and link GLSL source from file list, and the result is stored in pProgram
//
// NOTE: Uses the new C++ interface instead of the old handle-based interface.
// and user must call spvDestroyProgram explictly to destroy program object
//
bool SH_IMPORT_EXPORT spvCompileAndLinkProgramFromFile(
    int          fileNum,
    const char*  fileList[],
    void**       pProgram,
    const char** ppLog)
{
    // keep track of what to free
    std::list<glslang::TShader*> shaders;

    EShMessages messages = EShMsgDefault;
    SetMessageOptions(messages, defaultOptions);

    //
    // Per-shader processing...
    //

    OGLProgram& program = *new OGLProgram;
    *pProgram = &program;

    for (int i = 0; i < fileNum; ++i)
    {
        EShLanguage stage = FindLanguage(fileList[i]);
        glslang::TShader* shader = new glslang::TShader(stage);

        char** shaderStrings = ReadFileData(fileList[i]);
        if (! shaderStrings) {
            return false;
        }

        shader->setStrings(shaderStrings, 1);

        if (! shader->parse(&Resources, (defaultOptions & EOptionDefaultDesktop) ? 110 : 100, false, messages))
            CompileFailed = true;

        program.addShader(shader);

        FreeFileData(shaderStrings);
    }

    //
    // Program-level processing...
    //

    if (! program.link(messages))
        LinkFailed = true;

    if (CompileFailed || LinkFailed)
    {
        *ppLog = program.programLog.c_str();
        return false;
    }

    for (int stage = 0; stage < VkStageCount; ++stage) {
        if (program.getIntermediate((EShLanguage)stage)) {
            glslang::GlslangToSpv(*program.getIntermediate((EShLanguage)stage), program.spirv[stage]);
        }
    }
    *ppLog = program.programLog.c_str();
    return true;
}

//
// Compile and link GLSL source strings, and the result is stored in pProgram
//
// NOTE: Uses the new C++ interface instead of the old handle-based interface.
// and user must call spvDestroyProgram explictly to destroy program object
//
bool SH_IMPORT_EXPORT spvCompileAndLinkProgram(
    int                  shaderStageSourceCounts[VkStageCount],
    const char* const *  shaderStageSources[VkStageCount],
    void**               pProgram,
    const char**         ppLog)
{
    return spvCompileAndLinkProgramWithOptions(shaderStageSourceCounts,
                                               shaderStageSources,
                                               pProgram,
                                               ppLog,
                                               defaultOptions);
}

//
// Compile and link GLSL source strings with options
//
// NOTE: Uses the new C++ interface instead of the old handle-based interface.
// and user must call spvDestroyProgram explictly to destroy program object
//
bool SH_IMPORT_EXPORT spvCompileAndLinkProgramWithOptions(
    int                  shaderStageSourceCounts[VkStageCount],
    const char* const *  shaderStageSources[VkStageCount],
    void**               pProgram,
    const char**         ppLog,
    int                  options)
{
    // keep track of what to free
    std::list<glslang::TShader*> shaders;

    EShMessages messages = EShMsgDefault;
    SetMessageOptions(messages, options);

    //
    // Per-shader processing...
    //

    OGLProgram& program = *new OGLProgram;
    *pProgram = &program;

    for (int i = 0; i < VkStageCount; ++i)
    {
        if (shaderStageSourceCounts[i] > 0)
        {
            assert(shaderStageSources[i] != nullptr);
            glslang::TShader* shader = new glslang::TShader(static_cast<EShLanguage>(i));

            shader->setStrings(shaderStageSources[i], shaderStageSourceCounts[i]);

            if (options & EOptionVulkanRules) {
                shader->setEnvInput((options & EOptionReadHlsl) ? glslang::EShSourceHlsl
                    : glslang::EShSourceGlsl,
                    (EShLanguage)i, glslang::EShClientVulkan, ClientInputSemanticsVersion);
                shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
            }
            else {
                shader->setEnvInput((options & EOptionReadHlsl) ? glslang::EShSourceHlsl
                    : glslang::EShSourceGlsl,
                    (EShLanguage)i, glslang::EShClientOpenGL, ClientInputSemanticsVersion);
                shader->setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
            }
            shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

            if ((options & EOptionFlattenUniformArrays) != 0 &&
                (options & EOptionReadHlsl) == 0) {
                printf("uniform array flattening only valid when compiling HLSL source.");
                return false;
            }
            shader->setFlattenUniformArrays((options & EOptionFlattenUniformArrays) != 0);

            if (options & EOptionHlslIoMapping)
                shader->setHlslIoMapping(true);

            if (options & EOptionAutoMapBindings)
                shader->setAutoMapBindings(true);

            if (options & EOptionAutoMapLocations)
                shader->setAutoMapLocations(true);

            if (options & EOptionInvertY)
                shader->setInvertY(true);

            if (!shader->parse(&Resources, (options & EOptionDefaultDesktop) ? 110 : 100, false, messages))
                CompileFailed = true;

            program.addShader(shader);

            const char* pInfoLog = shader->getInfoLog();
            const char* pDebugLog = shader->getInfoDebugLog();
            if ((strlen(pInfoLog) > 0) || (strlen(pDebugLog) > 0))
            {
                char buffer[256];
                sprintf(buffer, "Compiling %s stage:\n", glslang::StageName((EShLanguage)i));
                program.AddLog(buffer);
                program.AddLog(shader->getInfoLog());
                program.AddLog(shader->getInfoDebugLog());
            }
        }
    }

    if (CompileFailed)
    {
        *ppLog = program.programLog.c_str();
        return false;
    }

    //
    // Program-level processing...
    //

    if (!program.link(messages))
        LinkFailed = true;

    if (LinkFailed)
    {
        *ppLog = program.programLog.c_str();
        return false;
    }

    for (int stage = 0; stage < VkStageCount; ++stage) {
        if (program.getIntermediate((EShLanguage)stage)) {
            glslang::SpvOptions spvOptions;
            spvOptions.generateDebugInfo = (options & EOptionDebug) != 0;
            spvOptions.disableOptimizer = (options & EOptionOptimizeDisable) != 0;
            spvOptions.optimizeSize = (options & EOptionOptimizeSize) != 0;
            glslang::GlslangToSpv(*program.getIntermediate((EShLanguage)stage), program.spirv[stage], &spvOptions);
        }
    }

    *ppLog = program.programLog.c_str();
    return true;
}

//
// Destroies OGLProgram object
//
void SH_IMPORT_EXPORT spvDestroyProgram(
    void* hProgram)
{
    OGLProgram* pProgram = reinterpret_cast<OGLProgram*>(hProgram);
    delete pProgram;
}

//
// Get SPRIV binary from OGLProgram object with specified shader stage, and return the binary size in bytes
//
// NOTE: 0 is returned if SPIRVI binary isn't exist for specified shader stage
int SH_IMPORT_EXPORT spvGetSpirvBinaryFromProgram(
    void*                hProgram,
    int                  stage,
    const unsigned int** ppData)
{
    OGLProgram* pProgram = reinterpret_cast<OGLProgram*>(hProgram);
    int programSize = (int)(pProgram->spirv[stage].size() * sizeof(unsigned int));
    if (programSize > 0)
    {
        *ppData = &pProgram->spirv[stage][0];
    }
    else
    {
        *ppData = nullptr;
    }
    return programSize;
}

//
// Gets component version according to specified SpvGenVersion enum
//
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
            *pReversion = GLSLANG_MINOR_VERSION;
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
    default:
        {
            result = false;
            break;
        }
    }
    return result;
}

//
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
    spv_binary binary;
    spv_diagnostic diagnostic = nullptr;
    spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
    spv_result_t result = spvTextToBinary(context, pSpvText,
                                          strlen(pSpvText), &binary, &diagnostic);
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

//
// Disassemble SPIR-V binary token using khronos spirv-tools, and store the output text to pBuffer
//
// NOTE: The text will be clampped if buffer size is less than requirement.
bool SH_IMPORT_EXPORT spvDisassembleSpirv(
    unsigned int   size,
    const void*    pSpvToken,
    unsigned int   bufSize,
    char*          pBuffer)
{
    uint32_t options = SPV_BINARY_TO_TEXT_OPTION_INDENT;
    // If printing to standard output, then spvBinaryToText should
    // do the printing.  In particular, colour printing on Windows is
    // controlled by modifying console objects synchronously while
    // outputting to the stream rather than by injecting escape codes
    // into the output stream.
    // If the printing option is off, then save the text in memory, so
    // it can be emitted later in this function.
    spv_text text = nullptr;
    spv_diagnostic diagnostic = nullptr;
    spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
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

//
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
    spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
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

//
// Optimize SPIR-V binary token using khronos spirv-tools, and store optimized result to ppOptBuf and the log text
// to pLog
//
// NOTE: The text will be clampped if buffer size is less than requirement, and ppOptBuf should be freed by
// spvFreeBuffer
//
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
    spv_target_env target_env = SPV_ENV_UNIVERSAL_1_3;

    std::string errorMsg;
    spvtools::Optimizer optimizer(target_env);
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
        // TODO: Parse options and set specified optimization pass
    }

    std::vector<uint32_t> binary;

    bool ret = optimizer.Run((const uint32_t*)pSpvToken, size / sizeof(uint32_t), &binary);

    if (ret)
    {
        *pBufSize = binary.size() * sizeof(uint32_t);
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

//
// Free input buffer
//
void SH_IMPORT_EXPORT spvFreeBuffer(
    void* pBuffer)
{
    free(pBuffer);
}

//
// Export stubs for VFX functions.
//
bool SH_IMPORT_EXPORT vfxParseFile(
    const char*  pFilename,
    unsigned int numMacro,
    const char*  pMacros[],
    VfxDocType   type,
    void**       ppDoc,
    const char** ppErrorMsg)
{
    return Vfx::vfxParseFile(pFilename, numMacro, pMacros, type, ppDoc, ppErrorMsg);
}

void SH_IMPORT_EXPORT vfxCloseDoc(
    void* pDoc)
{
    Vfx::vfxCloseDoc(pDoc);
}

void SH_IMPORT_EXPORT vfxGetRenderDoc(
    void*              pDoc,
    VfxRenderStatePtr* pRenderState)
{
    Vfx::vfxGetRenderDoc(pDoc, pRenderState);
}

void SH_IMPORT_EXPORT vfxGetPipelineDoc(
    void*                pDoc,
    VfxPipelineStatePtr* pPipelineState)
{
    Vfx::vfxGetPipelineDoc(pDoc, pPipelineState);
}

void SH_IMPORT_EXPORT vfxPrintDoc(
    void*                pDoc)
{
    Vfx::vfxPrintDoc(pDoc);
}

//
//   Deduce the language from the filename.  Files must end in one of the
//   following extensions:
//
//   .vert = vertex
//   .tesc = tessellation control
//   .tese = tessellation evaluation
//   .geom = geometry
//   .frag = fragment
//   .comp = compute
//
EShLanguage FindLanguage(const std::string& name)
{
    size_t ext = name.rfind('.');
    if (ext == std::string::npos) {
        return EShLangVertex;
    }

    std::string suffix = name.substr(ext + 1, std::string::npos);
    if (suffix == "vert")
        return EShLangVertex;
    else if (suffix == "tesc")
        return EShLangTessControl;
    else if (suffix == "tese")
        return EShLangTessEvaluation;
    else if (suffix == "geom")
        return EShLangGeometry;
    else if (suffix == "frag")
        return EShLangFragment;
    else if (suffix == "comp")
        return EShLangCompute;

    return EShLangVertex;
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

//
//   Malloc a string of sufficient size and read a string into it.
//
char** ReadFileData(const char* fileName)
{
    FILE *in;
    int errorCode = fopen_s(&in, fileName, "r");

    char *fdata;
    int count = 0;
    const int maxSourceStrings = 5;
    char** return_data = (char**)malloc(sizeof(char *) * (maxSourceStrings+1));

    if (errorCode) {
        printf("Error: unable to open input file: %s\n", fileName);
        return 0;
    }

    while (fgetc(in) != EOF)
        count++;

    fseek(in, 0, SEEK_SET);

    if (!(fdata = (char*)malloc(count+2))) {
        printf("Error allocating memory\n");
        return 0;
    }
    if (fread(fdata,1,count, in) != (size_t)count) {
            printf("Error reading input file: %s\n", fileName);
            return 0;
    }
    fdata[count] = '\0';
    fclose(in);
    if (count == 0) {
        return_data[0]=(char*)malloc(count+2);
        return_data[0][0]='\0';
        NumShaderStrings = 0;
        return return_data;
    } else
        NumShaderStrings = 1;

    int len = (int)(ceil)((float)count/(float)NumShaderStrings);
    int ptr_len=0,i=0;
    while(count>0){
        return_data[i]=(char*)malloc(len+2);
        memcpy(return_data[i],fdata+ptr_len,len);
        return_data[i][len]='\0';
        count-=(len);
        ptr_len+=(len);
        if(count<len){
            if(count==0){
               NumShaderStrings=(i+1);
               break;
            }
           len = count;
        }
        ++i;
    }
    return return_data;
}

void FreeFileData(char** data)
{
    for(int i=0;i<NumShaderStrings;i++)
        free(data[i]);
}

void InfoLogMsg(const char* msg, const char* name, const int num)
{
    printf(num >= 0 ? "#### %s %s %d INFO LOG ####\n" :
           "#### %s %s INFO LOG ####\n", msg, name, num);
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

//
//  Print error message of spvTextToBinary() and spvBinaryToText()
//
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                       )
{
    memset(ConfigFile, 0, sizeof(ConfigFile));

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        ProcessConfigFile();
        glslang::InitializeProcess();
        spv::Parameterize();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        glslang::FinalizeProcess();
        break;
    }
    return TRUE;
}
#else // Linux
__attribute__((constructor)) static void Init()
{
    memset(ConfigFile, 0, sizeof(ConfigFile));
    ProcessConfigFile();
    glslang::InitializeProcess();
    spv::Parameterize();
}

__attribute__((destructor)) static void Destroy()
{
    glslang::FinalizeProcess();
}

#endif



