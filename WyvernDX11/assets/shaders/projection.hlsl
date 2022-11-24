#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv  : TEXCOORD0;
	float3 worldPos: TEXCOORD1;
};

sampler sampler0;
sampler samplerWrap;
Texture2D<float> texture0 : register(t0);
Texture2D decalTex0 : register(t1);

PS_INPUT mainVs(vertexInputMesh input) {
	PS_INPUT output;
	
	float2 invUv = float2(input.uv.x, 1.0 - input.uv.y);
	output.pos = float4(invUv * 2 - 1, 0.0f, 1.f);
	output.normal = float4(input.normal, 1);
	output.uv = input.uv;
	output.worldPos = input.pos;

	return output;
}

float projectBlob(float3 worldPos, float3 projPos, float projSize) {
	float pDist = length(worldPos - projPos) - projSize;
	float pds = 0;

	if (pDist < 0) {
		pds = -pDist;
		// pds = saturate(pow(pds + 0.6, 4)) * 0.1;
		// pds
	}

	return pds;
}

struct PS_OUTPUT {
	float4 disp : SV_Target0;
	float4 worldPos : SV_Target1;
};

PS_OUTPUT mainPs(PS_INPUT input) {
	// float3 n = input.normal.xyz;
	float time = screenSize.x;
	float t1Blend = screenSize.y;
	float t2Blend = screenSize.z;
	float4x4 decal0ProjViewMat = projMat;
	
	float t1Base = texture0.Sample(sampler0, input.uv);
	t1Base = lerp(0, t1Base, t1Blend);

	float f = t1Base;
	float projMax = 0;

	projMax = max(projMax, projectBlob(input.worldPos, float3(sin(time), 1, cos(time)), 1.0));
	projMax = max(projMax, projectBlob(input.worldPos, float3(sin(time * 2) * 0.5, -1, cos(time * 2) * 0.5), 0.75));
	projMax = max(projMax, projectBlob(input.worldPos, float3(sin(time * 4) * 0.5, -1, cos(time * 4) * 0.5), 0.75));

	// f = max(t1Base, projMax * t2Blend);
	f = t1Base + projMax * t2Blend;

	// float disp = f;
	// disp -= 0.5;
	// disp *= 2.0;
	// float3 worldPos = input.worldPos + normalize(input.normal.xyz) * disp;
	// return float4(worldPos, 1.0);

	float4 decal0ClipSpace = mul(decal0ProjViewMat, float4(input.worldPos, 1));
	decal0ClipSpace /= decal0ClipSpace.w;

	if (decal0ClipSpace.x >= -1 && decal0ClipSpace.x <= 1 && decal0ClipSpace.y >= -1 && decal0ClipSpace.y <= 1 && decal0ClipSpace.z <= 1) {
	// if (decal0ClipSpace.x >= -0.5 && decal0ClipSpace.x <= 0.5 && decal0ClipSpace.y >= -0.5 && decal0ClipSpace.y <= 0.5 && decal0ClipSpace.z <= 1.0) {
		float distToCenter = length(float2(decal0ClipSpace.xy));
		distToCenter = saturate(1.0 - distToCenter);
		float2 decalUv = decal0ClipSpace.xy * 0.5 + 0.5;
		// float2 decalUv = decal0ClipSpace.xy + 0.5;
		// return float4(decal0ClipSpace.xyz, 1);
		// return float4(decalUv, 0, 1);

		float decalTex = decalTex0.Sample(sampler0, decalUv).r * 0.05 * distToCenter;//0.04;
		f = f + decalTex;
		// return float4(decalTex, decalTex, decalTex, 1);
	}

	// Triplanar proj.
	{
		float iX = abs(dot(input.normal, float3(1, 0, 0)));
		float iY = abs(dot(input.normal, float3(0, 1, 0)));
		float iZ = abs(dot(input.normal, float3(0, 0, 1)));
		
		f += decalTex0.Sample(samplerWrap, input.worldPos.yz).r * 0.05 * iX;
		f += decalTex0.Sample(samplerWrap, input.worldPos.xz).r * 0.05 * iY;
		f += decalTex0.Sample(samplerWrap, input.worldPos.xy).r * 0.05 * iZ;
	}

	PS_OUTPUT output;
	f = 0.5 + f * 0.5;
	output.disp = float4(f, f, f, 1.0);

	float disp = (f - 0.5) * 2.0;
	float3 worldPos = input.worldPos + input.normal.xyz * disp;

	output.worldPos = float4(worldPos, 1);

	return output;

	// float3 finalN = normalize(lerp(n, t1n, target1Blend));

	// return float4(finalN * 0.5 + 0.5, 1.0) * ObjectColor;

	// return texture1.Sample(sampler0, input.uv) * ObjectColor;
	// return float4(input.uv.x, input.uv.y, 0, 1);
	// return float4(input.normal.xyz * 0.5 + 0.5, 1.0) * ObjectColor;
}