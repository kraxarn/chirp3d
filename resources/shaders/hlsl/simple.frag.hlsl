struct fs_input
{
	float4 color : COLOR;
};

float4 main(fs_input input) : SV_TARGET
{
	return input.color;
}
