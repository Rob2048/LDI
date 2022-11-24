#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

//----------------------------------------------------------------------------------------------------
// Vertex shader.
//----------------------------------------------------------------------------------------------------

// struct vertexInputMesh {
// 	float3 pos : POSITION;
// 	float3 normal : NORMAL;
// 	float2 uv  : TEXCOORD0;
// };

struct HS_INPUT {
	float3 pos: POSITION;
	float3 normal : NORMAL;
	float2 uv: TEXCOORD0;
};

HS_INPUT mainVs(vertexInputMesh input) {
	HS_INPUT output;
	output.pos = input.pos;
	output.normal = input.normal;
	output.uv = input.uv;

	return output;
}


//----------------------------------------------------------------------------------------------------
// Hull shader.
//----------------------------------------------------------------------------------------------------

// Output control point
struct HS_OUTPUT {
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT {
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
	// float3 vTangent[4]    : TANGENT;
	// float2 vUV[4]         : TEXCOORD;
	// float3 vTanUCorner[4] : TANUCORNER;
	// float3 vTanVCorner[4] : TANVCORNER;
	// float4 vCWts          : TANWEIGHTS;
};

#define MAX_POINTS 32

// Patch Constant Function
HS_CONSTANT_DATA_OUTPUT patchConstantFunction(InputPatch<HS_INPUT, 3> inputPatch, uint patchID : SV_PrimitiveID) {
	float tessellationAmount = 8.0;
	
	HS_CONSTANT_DATA_OUTPUT output;
	output.edges[0] = tessellationAmount;
	output.edges[1] = tessellationAmount;
	output.edges[2] = tessellationAmount;
	output.inside = tessellationAmount;

	return output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("patchConstantFunction")]
HS_OUTPUT mainHs(InputPatch<HS_INPUT, 3> inputPatch, uint pointId : SV_OutputControlPointID, uint patchID : SV_PrimitiveID) {
	HS_OUTPUT output;
	output.pos = inputPatch[pointId].pos;
	output.normal = inputPatch[pointId].normal;
	output.uv = inputPatch[pointId].uv;

	return output;
}

//----------------------------------------------------------------------------------------------------
// Domain shader.
//----------------------------------------------------------------------------------------------------
struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv  : TEXCOORD0;
};

sampler sampler0;
Texture2D texture0 : register(t0);

[domain("tri")]
PS_INPUT mainDs(HS_CONSTANT_DATA_OUTPUT input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 3> patch) {
	float target1Blend = screenSize.x;

	float3 vertexPosition = uvwCoord.x * patch[0].pos + uvwCoord.y * patch[1].pos + uvwCoord.z * patch[2].pos;
	float3 normal = uvwCoord.x * patch[0].normal + uvwCoord.y * patch[1].normal + uvwCoord.z * patch[2].normal;
	float2 uv = uvwCoord.x * patch[0].uv + uvwCoord.y * patch[1].uv + uvwCoord.z * patch[2].uv;

	float disp = texture0.SampleLevel(sampler0, uv, 0).w;
	disp -= 0.5;
	disp *= 2.0;
	disp *= target1Blend;
	// disp *= 0.5;//200.0 / 255.0;
	
	float3 worldPos = vertexPosition + normal * disp;

	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(worldPos, 1.0));
	output.normal = float4(normal, 1.0);
	output.uv = uv;
	
	return output;
}