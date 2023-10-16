#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv  : TEXCOORD0;
	float4 world : TEXCOORD1;
};

PS_INPUT mainVs(vertexInputMesh input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1.0));
	output.normal = mul(worldMat, float4(input.normal, 0.0));
	output.uv = input.uv;
	output.world = mul(worldMat, float4(input.pos, 1.0));

	return output;
}

sampler sampler0;
Texture2D texture0;

float4 mainPs(PS_INPUT input) : SV_Target {
	float3 lightPos0 = float3(50, 80, 0);
	float3 lightColor0 = float3(1, 0.95, 0.8);
	float3 lightDir0 = normalize(lightPos0 - input.world.xyz);
	float3 i0 = lightColor0 * saturate(dot(lightDir0, normalize(input.normal.xyz)));

	float3 lightPos1 = float3(-50, -80, 0);
	float3 lightColor1 = float3(0.8, 0.95, 1.0);
	float3 lightDir1 = normalize(lightPos1 - input.world.xyz);
	float3 i1 = lightColor1 * saturate(dot(lightDir1, normalize(input.normal.xyz)));
	
	float3 i = i0 + i1;

	// Ambient light
	i += float3(0.2, 0.2, 0.2);

	i = saturate(i);

	return float4(i, 1) * ObjectColor;

	// return texture0.Sample(sampler0, float2(input.uv.x, 1.0 - input.uv.y)) * ObjectColor;
	// return float4(input.uv.x, input.uv.y, 0, 1);
	// return float4(input.normal.xyz * 0.5 + 0.5, 1.0) * ObjectColor;
}