#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
	float4 dist : TEXCOORD1;
};

PS_INPUT mainVs(vertexInputBasic input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1.f));
	output.col = float4(input.col, 1);
	output.uv = input.uv;


	output.dist.x = length(input.pos - ObjectColor.xyz);
	float3 normal = input.col * 2.0 - 1.0;
	output.dist.y = dot(normal, normalize(ObjectColor.xyz - input.pos));

	return output;
}

sampler sampler0;
Texture2D texture0;

float4 mainPs(PS_INPUT input) : SV_Target {	
	float4 tex = texture0.Sample(sampler0, float2(input.uv.x, 1.0 - input.uv.y));

	if (tex.a < 0.8) {
		discard;
	}

	if (input.dist.x >= 1 && input.dist.x <= 1.3) {
		if (input.dist.y >= 0.866) {
			// 30
			return float4(0, 1, 0, 1);
		} else if (input.dist.y >= 0.707) {
			// 45
			return float4(0, 0.75, 0, 1);
		} else if (input.dist.y >= 0.5) {
			// 60
			return float4(0, 0.5, 0, 1);
		} else if (input.dist.y >= 0.34) {
			// 70
			return float4(1.0, 0.5, 0, 1);
		}


		return float4(1, 0, 0, 1);
	}

	return float4(input.dist.y, input.dist.y, input.dist.y, 1);

	return tex * input.col;
}