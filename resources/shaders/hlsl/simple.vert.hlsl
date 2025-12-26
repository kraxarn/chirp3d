cbuffer uniforms : register(b0, space1)
{
	float4x4 mvp;
	float4   color;
};

struct vs_input
{
	float3 position : POSITION;
	float4 color    : COLOR;
};

struct vs_output
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
};

vs_output main(vs_input input)
{
	vs_output output;

	output.position = mul(mvp, float4(input.position, 1.0));
	output.color = color;

	return output;
}
