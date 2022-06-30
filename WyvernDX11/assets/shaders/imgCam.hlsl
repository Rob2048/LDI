#include "common.hlsl"

cbuffer vertexBuffer : register(b0)
{
	float4x4 ProjectionMatrix;
};

struct VS_INPUT
{
	float2 pos : POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

PS_INPUT mainVs(VS_INPUT input) {
	PS_INPUT output;

	output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
	output.col = input.col;
	output.uv  = input.uv;

	return output;
}

sampler sampler0;
Texture2D texture0;

float4 mainPs(PS_INPUT input) : SV_Target {
	float pixelX = floor(input.uv.x * 1920);
	float pixelY = floor(input.uv.y * 1080);

	float oddX = fmod(pixelX, 2.0);
	float oddY = fmod(pixelY, 2.0);

	float gainRed = 1.4f;
	float gainGreen = 1.4f;
	float gainBlue = 3.0f;

	float camI = texture0.Sample(sampler0, input.uv).r;

	if (oddX != 0 && oddY != 0) {
		// Red
		camI *= gainRed;
	} else if (oddX == 0 && oddY == 0) {
		// Blue
		camI *= gainBlue;
	} else {
		// Green
		camI *= gainGreen;
	}
	
	if (camI >= (254.0 / 255.0)) {
		return float4(0.5, 0, 0, 1);
	}

	return float4(camI, camI, camI, 1);
}