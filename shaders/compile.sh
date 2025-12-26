#!/bin/bash

# GLSL -> SPIR-V
glslang -V -o "spv/simple.frag.spv" "glsl/simple.frag.glsl"
glslang -V -o "spv/simple.vert.spv" "glsl/simple.vert.glsl"

# SPIR-V -> MSL
shadercross "spv/simple.frag.spv" -o "msl/simple.frag.msl"
shadercross "spv/simple.vert.spv" -o "msl/simple.vert.msl"

# SPIR-V -> DXIL
shadercross "spv/simple.frag.spv" -o "dxil/simple.frag.dxil"
shadercross "spv/simple.vert.spv" -o "dxil/simple.vert.dxil"
