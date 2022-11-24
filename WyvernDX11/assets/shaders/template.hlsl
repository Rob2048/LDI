#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv  : TEXCOORD0;
};

sampler sampler0;
Texture2D texture0 : register(t0);

PS_INPUT mainVs(vertexInputMesh input) {
	float target1Blend = screenSize.x;

	float disp = texture0.SampleLevel(sampler0, input.uv, 0).w;
	disp -= 0.5;
	disp *= 2.0;
	disp *= target1Blend;
	// disp *= 0.5;//200.0 / 255.0;
	
	float3 worldPos = input.pos + input.normal.xyz * disp;

	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(worldPos, 1.0));
	output.normal = float4(input.normal, 1);
	output.uv = input.uv;

	return output;
}

Texture2D texture1 : register(t1);

float4 mainPs(PS_INPUT input) : SV_Target {
	float target1Blend = screenSize.x;

	float3 disp = texture0.Sample(sampler0, input.uv).rgb;

	return float4(disp, 1) * ObjectColor;

	// float3 n = input.normal.xyz;	
	// float3 t1n = texture0.Sample(sampler0, input.uv).rgb * 2.0 - 1.0;

	// float nX = n.x 
	
	// float3 finalN = normalize(lerp(n, t1n, target1Blend));
	// float3 finalN = normalize(n + t1n);

	// return float4(finalN * 0.5 + 0.5, 1.0) * ObjectColor;
	// return float4(n * 0.5 + 0.5, 1.0) * ObjectColor;

	// return texture1.Sample(sampler0, input.uv) * ObjectColor;
	// return float4(input.uv.x, input.uv.y, 0, 1);
	// return float4(input.normal.xyz * 0.5 + 0.5, 1.0) * ObjectColor;
}