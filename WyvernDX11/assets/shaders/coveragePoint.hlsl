#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

struct vertexInputCoveragePoint {
	float3 pos : POSITION;
	int id : TEXCOORD0;
	float3 normal : NORMAL;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 dist : TEXCOORD1;
	float3 color : COLOR;
	float3 normal : NORMAL;
	int id : TEXCOORD0;
};

StructuredBuffer<int> coverageBuffer : register(t0);

//----------------------------------------------------------------------------------------------------
// Show.
//----------------------------------------------------------------------------------------------------
PS_INPUT showCoverageVs(vertexInputCoveragePoint input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos + normalize(ObjectColor.xyz - input.pos) * 0.01, 1));
	output.id = input.id;
	output.normal = input.normal;
	float covValue = coverageBuffer[input.id] / 1000.0;

	float4 col = (1, 0, 0, 1);
	if (covValue >= 0.866) {
		// 30
		col = float4(0, 1, 0, 1);
	} else if (covValue >= 0.707) {
		// 45
		col = float4(0, 0.75, 0, 1);
	} else if (covValue >= 0.5) {
		// 60
		col = float4(0, 0.5, 0, 1);
	} else if (covValue >= 0.34) {
		// 70
		col = float4(1.0, 0.5, 0, 1);
	}

	output.color = col.rgb;
	
	output.dist.x = length(input.pos - ObjectColor.xyz);
	output.dist.y = dot(input.normal, normalize(ObjectColor.xyz - input.pos));

	return output;
}

float4 showCoveragePs(PS_INPUT input) : SV_Target {
	return float4(input.color, 1);
}

//----------------------------------------------------------------------------------------------------
// Write.
//----------------------------------------------------------------------------------------------------
RWStructuredBuffer<int> coverageBufferWrite : register(u1);

struct PS_INPUT_WRITE {
	float4 pos : SV_POSITION;
	float4 dist : TEXCOORD1;
	float3 color : COLOR;
	float3 normal : NORMAL;
	int id : TEXCOORD0;
};

PS_INPUT_WRITE writeCoverageVs(vertexInputCoveragePoint input) {
	PS_INPUT output;
	// output.pos = mul(ProjectionMatrix, float4(input.pos + normalize(ObjectColor.xyz - input.pos) * 0.01, 1));
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1));
	output.id = input.id;
	output.normal = input.normal;
	// output.color = float3(coverageBuffer[input.id], 0, 0);
	
	output.dist.x = length(input.pos - ObjectColor.xyz);
	output.dist.y = dot(input.normal, normalize(ObjectColor.xyz - input.pos));

	return output;
}

sampler pointSampler;
Texture2D depthTex : register(t0);

float4 writeCoveragePs(PS_INPUT_WRITE input) : SV_Target {
	float2 screenPos = input.pos.xy / 1024.0;
	float depthTexValue = depthTex.Sample(pointSampler, screenPos).r;

	// float depth = (input.pos.z * 10000.0) % 1.0;
	float depth = (depthTexValue * 10000.0) % 1.0;

	float diff = depthTexValue - input.pos.z;

	// TODO: Tolerance?
	if (diff >= 0) {
		if (input.dist.x >= (20.0 - 0.15) && input.dist.x <= (20 + 0.15)) {
			InterlockedMax(coverageBufferWrite[input.id], (int)(input.dist.y * 1000.0));
			
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
	}

	// TODO: Check if this is too slow.
	discard;
	return float4(1, 0, 1, 1);
}