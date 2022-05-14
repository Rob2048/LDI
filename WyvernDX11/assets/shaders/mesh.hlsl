#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv  : TEXCOORD0;
};

PS_INPUT mainVs(vertexInputMesh input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1.f));
	output.normal = float4(input.normal, 1);
	output.uv = input.uv;

	return output;
}

sampler sampler0;
Texture2D texture0;

float4 mainPs(PS_INPUT input) : SV_Target {
	float3 lightPos = float3(0, 1, 0);
	float i = dot(lightPos, input.normal.xyz);

	return texture0.Sample(sampler0, float2(input.uv.x, 1.0 - input.uv.y)) * ObjectColor;

	return float4(input.uv.x, input.uv.y, 0, 1);
	
	return float4(input.normal.xyz * 0.5 + 0.5, 1.0) * ObjectColor;
}