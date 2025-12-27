#!/bin/bash

names=("simple")
formats=("spv" "msl" "dxil")

for name in "${names[@]}"
do
	for format in "${formats[@]}"
	do
		shadercross "hlsl/$name.frag.hlsl" -o "$format/$name.frag.$format" || exit 1
		shadercross "hlsl/$name.vert.hlsl" -o "$format/$name.vert.$format" || exit 1
	done
done
