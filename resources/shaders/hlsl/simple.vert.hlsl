cbuffer uniforms : register(b0, space1)
{
	float4x4 mvp;
};

struct vs_input
{
	float3 position  : POSITION;
	float2 tex_coord : TEXCOORD1;
};

struct vs_output
{
	float2 tex_coord : TEXCOORD0;
	float4 position  : SV_POSITION;
};

vs_output main(vs_input input)
{
	vs_output output;

	output.position = mul(mvp, float4(input.position, 1.F));
	output.tex_coord = input.tex_coord;

	return output;
}
