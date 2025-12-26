#!/bin/bash

name="simple"
formats=("spv" "msl" "dxil")

for format in "${formats[@]}"
do
	shadercross "hlsl/$name.frag.hlsl" -o "$format/$name.frag.$format"
    shadercross "hlsl/$name.vert.hlsl" -o "$format/$name.vert.$format"
done
