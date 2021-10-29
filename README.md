# SPVGEN

SPVGEN is a library to generate SPIR-V binary. It integrates [glslang](https://github.com/KhronosGroup/glslang) and [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools).

## APIs
The APIs are listed in include/spvgen.h and vfx.h.

#### Initialization
* InitSpvGen()

#### Convert GLSL to SPIR-V binary
* spvCompileAndLinkProgram()
* spvGetSpirvBinaryFromProgram()
* spvDestroyProgram()

#### Assemble SPIR-V
* spvAssembleSpirv()

#### Disassemble SPIR-V
* spvDisassembleSpirv()

#### Optimize SPIR-V
* spvOptimizeSpirv()
* spvFreeBuffer()

#### Validate SPIR-V
* spvValidateSpirv()

#### Parse .pipe file
* vfxParseFile()
* vfxGetPipelineDoc()
* vfxCloseDoc()

## How to build

SPVGEN is now built as part of the AMDVLK build system, but is not built by default.

First, follow the AMDVLK instructions to get sources and use `cmake` to set up the build.

Then, use
```
cd spvgen/external && python fetch_external_sources.py
cd xgl && cmake --build {build directory} --target spvgen
```

