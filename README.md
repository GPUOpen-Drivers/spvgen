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

```
cd external/
python fetch_external_sources.py
cd ..
cmake -H. -Brbuild64 -DCMAKE_BUILD_TYPE=release -DXGL_LLPC_PATH=<PATH_TO_LLPC> 
cd rbuild64
make
cd ..
cmake -H. -Bdbuild64 -DCMAKE_BUILD_TYPE=Debug -DXGL_LLPC_PATH=<PATH_TO_LLPC>
cd dbuild64
make
```
To build 32bit library, please add "-DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32" in cmake.

SPVGEN references [LLPC](https://github.com/GPUOpen-Drivers/llpc) header files. You need to download the files from llpc/include/.



