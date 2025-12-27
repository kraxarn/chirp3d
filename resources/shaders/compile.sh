#!/bin/bash

names=("default")
formats=("msl" "dxil")

for name in "${names[@]}"
do
	glslang -V -o "spv/$name.frag.spv" "glsl/$name.frag" || exit 1
	glslang -V -o "spv/$name.vert.spv" "glsl/$name.vert" || exit 1
done

for name in "${names[@]}"
do
	for format in "${formats[@]}"
	do
		shadercross "spv/$name.frag.spv" -o "$format/$name.frag.$format" || exit 1
		shadercross "spv/$name.vert.spv" -o "$format/$name.vert.$format" || exit 1
	done
done
