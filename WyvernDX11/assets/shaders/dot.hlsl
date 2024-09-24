#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

PS_INPUT mainVs(vertexInputBasic input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1.f));
	output.col = float4(input.col, 1);
	output.uv = input.uv;
	float3 normal = input.col * 2.0 - 1.0;

	return output;
}

sampler sampler0;
Texture2D texture0;

float4 mainPs(PS_INPUT input) : SV_Target {	
	// float4 tex = texture0.Sample(sampler0, float2(input.uv.x, 1.0 - input.uv.y));

	// if (tex.a < 0.2) {
	// 	discard;
	// }

	float2 midPoint = float2(0.5, 0.5);
	float2 pos = input.uv;

	float dist = length(pos - midPoint) * 2.0;
	dist = pow(dist, 6);
	dist = saturate(dist);

	// return tex * input.col;

	// float v = LinearToGamma(dist);
	float v = dist;

	return float4(0, 0, 0, 1.0 - v);
	// return float4(v, v, v, 1.0);
}