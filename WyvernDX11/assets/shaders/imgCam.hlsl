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
	float camI = texture0.Sample(sampler0, input.uv).r;

	float brightness = params.x;
	float contrast = params.y;
	camI = saturate(camI * contrast + brightness);

	if (params.w > 0.5) {
		// Heatmap.

		// camI = GammaToLinear(camI);

		float3 c0 = float3(0.0, 0.0, 0.0);
		float3 c1 = float3(0.0, 0.0, 1.0);
		float3 c2 = float3(0.0, 1.0, 0.0);
		float3 c3 = float3(1.0, 0.0, 0.0);
		float3 c4 = float3(1.0, 1.0, 1.0);

		float3 cF = c0;

		if (camI >= 1.0) {
			cF = c4;
		} else if (camI >= 0.66) {
			cF = lerp(c2, c3, (camI - 0.66) * 3.0);
		} else if (camI >= 0.33) {
			cF = lerp(c1, c2, (camI - 0.33) * 3.0);
		} else if (camI >= 0.0) {
			cF = lerp(c0, c1, camI * 3.0);
		}
		
		return float4(cF, 1);
	} else {
		// Normal.
		return float4(camI, camI, camI, 1);
	}
}