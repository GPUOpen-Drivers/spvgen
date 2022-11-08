# SPVGEN

SPVGEN is a library to generate SPIR-V binary. It integrates [glslang](https://github.com/KhronosGroup/glslang) and [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools).

## APIs
The APIs are listed in include/spvgen.h.

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

## How to build

SPVGEN is now built into amdllpc statically by default. If you want to build a standalone one, follow the steps below:
1. ```cd spvgen/external && python fetch_external_sources.py```
2. Use cmake to build.
```
cd spvgen
mkdir build
cd build
cmake ..
make -j8
```
