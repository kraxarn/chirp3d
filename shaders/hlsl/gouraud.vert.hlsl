struct point_light
{
	float3 position : POSITION;
	float4 color    : COLOR;
	float4 ambient  : COLOR;
};

cbuffer uniforms : register(b0, space1)
{
	float4x4    mvp;
	float4      color;
	float3      camera_position;
	point_light lights[2];
};

struct vs_input
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
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

	output.position = mul(mvp, float4(input.position, 1.F));

	float4 result = float4(0, 0, 0, 0);
	for (int i = 0; i < 2; i++)
	{
		float3 light_dir = normalize(lights[i].position - input.position);
		float diff = max(dot(input.normal, light_dir), 0.F);
		float4 diffuse = diff * lights[i].color;

		float3 view_dir = normalize(camera_position - input.position);
		float3 reflect_dir = reflect(-light_dir, input.normal);
		float spec = pow(max(dot(view_dir, reflect_dir), 0.F), 64.F);
		float4 specular = 0.5F * spec * lights[i].color;

		result += (lights[i].ambient + diffuse + specular) * color;
	}

	result[3] = 1.F;
	output.color = result;

	return output;
}
