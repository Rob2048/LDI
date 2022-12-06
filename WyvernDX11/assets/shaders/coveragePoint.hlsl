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
StructuredBuffer<int> surfelMask : register(t1);

//----------------------------------------------------------------------------------------------------
// Show.
//----------------------------------------------------------------------------------------------------
PS_INPUT showCoverageVs(vertexInputCoveragePoint input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1));
	// output.pos = mul(ProjectionMatrix, float4(input.pos + normalize(ObjectColor.xyz - input.pos) * 0.01, 1));
	output.id = input.id;
	output.normal = input.normal;

	int masked = surfelMask[input.id];
	float3 col = float3(0, 0, 0);

	if (masked != 0) {
		float r = random((float)masked + 0);
		float g = random((float)masked + 1);
		float b = random((float)masked + 2);

		col = float3(r, g, b);
	}

	output.color = col;

	// float covValue = coverageBuffer[input.id] / 1000.0;

	// float4 col = (1, 0, 0, 1);
	// if (covValue >= 0.866) {
	// 	// 30
	// 	col = float4(0, 1, 0, 1);
	// } else if (covValue >= 0.707) {
	// 	// 45
	// 	col = float4(0, 0.75, 0, 1);
	// } else if (covValue >= 0.5) {
	// 	// 60
	// 	col = float4(0, 0.5, 0, 1);
	// } else if (covValue >= 0.34) {
	// 	// 70
	// 	col = float4(1.0, 0.5, 0, 1);
	// }

	// output.color = col.rgb;
	
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
RWStructuredBuffer<int> areaBuffer : register(u2);

struct PS_INPUT_WRITE {
	float4 pos : SV_POSITION;
	float4 dist : TEXCOORD1;
	int id : TEXCOORD0;
};

PS_INPUT_WRITE writeCoverageVs(vertexInputCoveragePoint input) {
	PS_INPUT_WRITE output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1));
	output.id = input.id;
	output.dist.x = length(input.pos - ObjectColor.xyz);
	output.dist.y = dot(input.normal, normalize(ObjectColor.xyz - input.pos));

	return output;
}

sampler pointSampler;
Texture2D depthTex : register(t0);

cbuffer basicConstantBufer : register(b1) {
	int slotId;
};

float4 writeCoveragePs(PS_INPUT_WRITE input) : SV_Target {
	// TODO: Texture size hardcoded here.
	float2 screenTexUv = input.pos.xy / 1024.0;
	float depthTexValue = depthTex.Sample(pointSampler, screenTexUv).r;
	float diff = depthTexValue - input.pos.z;

	if (diff >= 0) {
		if (surfelMask[input.id] != 0) {
			return float4(0, 1, 1, 1);
		}

		if (input.dist.x >= (20.0 - 0.15) && input.dist.x <= (20 + 0.15)) {

			//if (input.dist.y >= 0.707) {
			if (input.dist.y >= 0.5) {
				int distValue = (int)(input.dist.y * 1000.0);
				InterlockedMax(coverageBufferWrite[input.id], distValue);
				InterlockedAdd(areaBuffer[slotId], distValue);
			
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
			}

			return float4(1, 0, 0, 1);
		}

		return float4(input.dist.y, input.dist.y, input.dist.y, 1);
	}

	// TODO: Check if this is too slow.
	discard;
	return float4(1, 0, 1, 1);
}

float4 writeNoCoveragePs(PS_INPUT_WRITE input) : SV_Target {
	// TODO: Texture size hardcoded here.
	float2 screenTexUv = input.pos.xy / 1024.0;
	float depthTexValue = depthTex.Sample(pointSampler, screenTexUv).r;
	float diff = depthTexValue - input.pos.z;

	if (diff >= 0) {
		if (surfelMask[input.id] != 0) {
			return float4(0, 1, 1, 1);
		}

		if (input.dist.x >= (20.0 - 0.15) && input.dist.x <= (20 + 0.15)) {

			//if (input.dist.y >= 0.707) {
			if (input.dist.y >= 0.5) {
				int distValue = (int)(input.dist.y * 1000.0);
				InterlockedAdd(areaBuffer[slotId], distValue);
				
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
			}

			return float4(1, 0, 0, 1);
		}

		return float4(input.dist.y, input.dist.y, input.dist.y, 1);
	}

	// TODO: Check if this is too slow.
	discard;
	return float4(1, 0, 1, 1);
}