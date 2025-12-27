Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct fs_input
{
	float2 tex_coord : TEXCOORD0;
};

float4 main(fs_input input) : SV_Target0
{
	return Texture.Sample(Sampler, input.tex_coord);
}
