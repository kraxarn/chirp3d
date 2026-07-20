#!/bin/bash

names=("default" "nkui")
formats=("msl" "dxil")

for name in "${names[@]}"
do
	echo "spv/$name.frag.spv"
	glslang -V -o "spv/$name.frag.spv" "glsl/$name.frag" || exit 1

	echo "spv/$name.vert.spv"
	glslang -V -o "spv/$name.vert.spv" "glsl/$name.vert" || exit 1
done

for name in "${names[@]}"
do
	for format in "${formats[@]}"
	do
		echo "$format/$name.frag.$format"
		shadercross "spv/$name.frag.spv" -o "$format/$name.frag.$format" || exit 1

		echo "$format/$name.vert.$format"
		shadercross "spv/$name.vert.spv" -o "$format/$name.vert.$format" || exit 1
	done
done
