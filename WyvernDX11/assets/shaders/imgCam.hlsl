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

cbuffer pixelConstants : register(b0)
{
	float4 params;
};

float4 mainPs(PS_INPUT input) : SV_Target {
	// NOTE: Split each bayer filter color.
	// float pixelX = floor(input.uv.x * 1920.0);
	// float pixelY = floor(input.uv.y * 1080.0);

	// float offsetX = floor(pixelX / (1920.0 / 2.0));
	// float offsetY = floor(pixelY / (1080.0 / 2.0));

	// pixelX = fmod(pixelX, (1920.0 / 2.0));
	// pixelY = fmod(pixelY, (1080.0 / 2.0));

	// pixelX += 0.5 * offsetX;
	// pixelY += 0.5 * offsetY;

	// float pixelToUv = 1.0 / 1600.0;
	// float halfPixelX = 1.0 / 1920.0 / 2.0;
	// float halfPixelY = 1.0 / 1080.0 / 2.0;

	// float2 finalUv;
	// finalUv.x = (pixelX * 2) / 1920.0 + halfPixelX;
	// finalUv.y = (pixelY * 2) / 1080.0 + halfPixelY;

	// float camI = texture0.Sample(sampler0, finalUv).r;

	// return float4(camI, camI, camI, 1);
	
	float pixelX = floor(input.uv.x * 1920);
	float pixelY = floor(input.uv.y * 1080);

	float oddX = fmod(pixelX, 2.0);
	float oddY = fmod(pixelY, 2.0);

	float gainRed = params.x;
	float gainGreen = params.y;
	float gainBlue = params.z;

	float camI = texture0.Sample(sampler0, input.uv).r;
	return float4(camI, camI, camI, 1);

	camI = GammaToLinear(camI);

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

	camI = LinearToGamma(camI);

	return float4(camI, camI, camI, 1);
}