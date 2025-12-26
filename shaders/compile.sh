#!/bin/bash

names=("simple" "gouraud")
formats=("spv" "msl" "dxil")

for name in "${names[@]}"
do
	for format in "${formats[@]}"
	do
		shadercross "hlsl/$name.frag.hlsl" -o "$format/$name.frag.$format"
		shadercross "hlsl/$name.vert.hlsl" -o "$format/$name.vert.$format"
	done
done
