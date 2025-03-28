#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv  : TEXCOORD0;
	float4 dist : TEXCOORD1;
	int id : TEXCOORD2;
};

//---------------------------------------------------------------------------------------------------------------
// Live view.
//---------------------------------------------------------------------------------------------------------------

// Orthographic on Y, perspective on X.
PS_INPUT orthoYPerspX(vertexInputMesh input) {
	PS_INPUT output;
	// NOTE: Convert to view space. ProjectionMatrix should just be modelView.
	float4 vP = mul(ProjectionMatrix, float4(input.pos, 1));

	const float zNear = 0.1;
	const float zFar = 100.0;
	const float yTop = 5.0;
	const float yBottom = -5.0;
	const float xFov = 11.0;
	const float xRatioHalf =  0.19169; //sin(glm::radians(xFov) / 2);

	float z = -vP.z;
	float Pz = (z - zNear) / (zFar - zNear);

	float y = vP.y;
	float Py = 2 * (y - yBottom) / (yTop - yBottom) - 1;

	float Px = vP.x / (z * xRatioHalf);

	output.pos = float4(Px, Py, Pz, 1.0);
	output.normal = float4(input.normal, 1.0);

	// TODO: Account for ortho on Y.
	// Distance to camera.
	output.dist.x = length(input.pos - ObjectColor.xyz);
	// TODO: Account for ortho on Y.
	// Angle to camera.
	output.dist.y = dot(input.normal.xyz, normalize(ObjectColor.xyz - input.pos));

	return output;
}

// Perspective on X and Y.
PS_INPUT mainVs(vertexInputCoveragePoint input) {
	PS_INPUT output;
	
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1));
	output.normal = float4(input.normal, 1.0);
	output.id = input.id;

	// Distance to camera.
	output.dist.x = length(input.pos - ObjectColor.xyz);
	// Angle to camera.
	output.dist.y = dot(input.normal.xyz, normalize(ObjectColor.xyz - input.pos));

	return output;
}

RWStructuredBuffer<int> coverageBufferWrite : register(u1);

float4 mainPs(PS_INPUT input) : SV_Target {	
	// float r = random((float)input.id + 0);
	// float g = random((float)input.id + 1);
	// float b = random((float)input.id + 2);
	// return float4(r, g, b, 1.0);

	// return float4(input.normal.xyz * 0.5 + 0.5, 1.0);
	
	// if (input.dist.x >= (20.0 - 0.15) && input.dist.x <= (20 + 0.15)) {
	// 	return input.normal;
	// }

	float3 color = float3(1.0, 0, 0);
	
	if (input.dist.y >= 0.866) {
		// 30
		color = float3(0, 1, 0);
		if (input.dist.x >= (20.0 - 0.15) && input.dist.x <= (20 + 0.15)) {
			coverageBufferWrite[input.id] = 1;
		}
	} else if (input.dist.y >= 0.707) {
		// 45
		color = float3(0, 0.75, 0);
	} else if (input.dist.y >= 0.5) {
		// 60
		color = float3(0, 0.5, 0);
	} else if (input.dist.y >= 0.34) {
		// 70
		color = float3(1.0, 0.5, 0);
	}
	
	if (input.dist.x < (20.0 - 0.15) || input.dist.x > (20 + 0.15)) {
		float lum = color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722;
		color = float3(lum, lum, lum);
	}

	return float4(color, 1.0);

	// return float4(tex.rgb, 1) * float4(input.dist.y, input.dist.y, input.dist.y, 1);
}