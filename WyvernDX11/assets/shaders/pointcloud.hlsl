#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

cbuffer pointCloudConstantBuffer : register(b1) {
	float4 scaleFactor;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
};

PS_INPUT mainVs(vertexInputBasic input) {
	PS_INPUT output;

	// World space adjustment.
	float4 viewPos = mul(viewMat, float4(input.pos, 1.f));
	viewPos.x += (input.uv.x * scaleFactor.x) * (1.0f - scaleFactor.z);
	viewPos.y += (input.uv.y * scaleFactor.x) * (1.0f - scaleFactor.z);

	// Screen space adjustment.
	output.pos = mul(projMat, viewPos);
	output.pos.x += ((input.uv.x * output.pos.w) / (screenSize.x * 0.5) * scaleFactor.y) * scaleFactor.z;
	output.pos.y += ((input.uv.y * output.pos.w) / (screenSize.y * 0.5) * scaleFactor.y) * scaleFactor.z;

	output.col = float4(input.col, 1.0);

	return output;
}

float4 mainPs(PS_INPUT input) : SV_Target {	
	return input.col;
}