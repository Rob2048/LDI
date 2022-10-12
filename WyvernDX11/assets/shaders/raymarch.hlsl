#define BASIC_CONSTANT_BUFFER

#include "common.hlsl"

//----------------------------------------------------------------------------------------------------
// Vertex shader.
//----------------------------------------------------------------------------------------------------
struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
	float4 clipPos : TEXCOORD1;
};

PS_INPUT mainVs(vertexInputBasic input) {
	PS_INPUT output;
	// output.pos = mul(ProjectionMatrix, float4(input.pos, 1.f));
	output.pos = float4(input.pos.xyz, 1.0);
	output.col = float4(input.col, 1);
	output.uv = input.uv;

	output.clipPos = output.pos;

	return output;
}

//----------------------------------------------------------------------------------------------------
// Pixel shader.
//----------------------------------------------------------------------------------------------------
static const float3 lightPos = float3(-10, 10, 10);
static const float3 lightDirection = normalize(lightPos);

sampler sampler0;
Texture3D texture0;

// NOTE: Many of the SDF functions are derivatives from https://iquilezles.org/articles/distfunctions/
// https://www.shadertoy.com/view/Xds3zN

float sphereSdf(float3 pos, float3 origin, float radius) {
	return length(pos - origin) - radius;
}

float planeSdf(float3 pos) {
	return pos.y;
}

float sdBox(float3 pos, float3 origin, float3 bounds) {
	float3 d = abs(pos - origin) - bounds;
	return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float sdRoundBox(float3 pos, float3 origin, float3 bounds, float radius) {
	float3 q = abs(pos - origin) - bounds;
	return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - radius;
}

float sdTorus(float3 pos, float3 origin, float2 t) {
	float3 p = pos - origin;

	float2 q = float2(length(p.xz)-t.x,p.y);
	return length(q)-t.y;
}

float opSmoothUnion(float d1, float d2, float k) {
	float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
	return lerp( d2, d1, h ) - k*h*(1.0-h); 
}

float opSmoothSubtraction(float d1, float d2, float k) {
	float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
	return lerp( d2, -d1, h ) + k*h*(1.0-h); 
}

float4 opU(float4 d1, float4 d2) {
	return (d1.w < d2.w) ? d1 : d2;
}

float4 opS(float4 d1, float4 d2) {
	return (d1.w > d2.w) ? d1 : d2;
}

// NOTE: https://www.shadertoy.com/view/tscBz8
// Constructive Solid Geometry (CSG) Operators:
float4 smoothUnionColor(float4 surface1, float4 surface2, float smoothness) {
	float sdf1 = surface1.w;
	float sdf2 = surface2.w;

	float interpolation = clamp(0.5 + 0.5 * (sdf2 - sdf1) / smoothness, 0.0, 1.0);
	return float4(lerp(surface2.rgb, surface1.rgb, interpolation),
					lerp(sdf2, sdf1, interpolation) - smoothness * interpolation * (1.0 - interpolation));
}

float4 map(float3 pos) {
	float time = ObjectColor.w;
	float4 t = 0;
	
	// t = float4(1, 0.5, 0.2, opSmoothUnion(
	// 	sphereSdf(pos, float3(-1, 1, 1), 1),
	// 	sphereSdf(pos, float3(1, 1, 0), 1),
	// 	1.0));

	float3 volOrigin = float3(0, 64.0 / 20.0, 0);
	float3 volSize = float3(128, 128, 128) / 20.0;
	float3 volCorner = volOrigin - volSize / 2.0;
	float volDist = sdBox(pos, volOrigin, volSize / 2.0);

	if (volDist <= 0.01) {
		// We consider being within the 3d volume.

		// Read dist from position.
		float3 volPos = saturate((pos - volCorner) / volSize);
		float4 volTex = texture0.Sample(sampler0, volPos);

		t = float4(0.8, 0.3, 0.8, volTex.r / 20.0);
	} else {
		t = float4(0, 0, 0, volDist);
	}

	// t = smoothUnionColor(
	// 	float4(1, 0.5, 0.2, sphereSdf(pos, float3(sin(time), 1, cos(time)), 1)),
	// 	float4(0, 1, 0, sphereSdf(pos, float3(1, 1, 0), 1)),
	// 	1.0);	

	// t = smoothUnionColor(
	// 	float4(1, 0.5, 0.2, sphereSdf(pos, float3(sin(time), 1, cos(time)), 1)),
	// 	t,
	// 	1.0);
	
	// t = opS(t, float4(0.2, 1, 0.2, -sphereSdf(pos, float3(sin(time * 2), 2, cos(time * 2)), 1.0)));
	
	// t = max(t, -sdRoundBox(pos, float3(4, 0.9, 0), float3(0.45, 0.45, 0.45), 0.1));
	
	// t = opU(t, float4(0.2, 0.5, 1.0, sdRoundBox(pos, float3(4, 0.5 + 0.2, 0), float3(0.5, 0.5, 0.5), 0.05)));

	// t = smoothUnionColor(
	// 		float4(1, 0, 0, sdTorus(pos, float3(3, 0.5 + sin(time), 0), float2(1, 0.3))),
	// 		t,
	// 		0.1);


	// float dist = t.w;
	// dist = saturate(dist * 4 + 0.3);
	// t = opU(t, float4(dist, dist, dist, planeSdf(pos + 0.01)));

	float dist = t.w % 0.2;
	dist = saturate(dist  +0.35);
	t = opU(t, float4(dist, dist, dist, planeSdf(pos + 0.01)));

	return t;
}

float3 getNormal(float3 pos) {
	float d = map(pos).w;

	float3 normal = d - float3(
		map(pos - float3(0.001, 0.00, 0.00)).w,
		map(pos - float3(0.00, 0.001, 0.00)).w,
		map(pos - float3(0.00, 0.00, 0.001)).w
	);

	return normalize(normal);
}

float getDist(float3 rO, float3 rD) {
	float dist;

	float rayT = 0.0;
	
	for (int i = 0; i < 128; ++i) {
		float error = 0.0004 * rayT;

		float3 rayWorldPos = rO + rD * rayT;

		float t = map(rayWorldPos).w;

		if (t <= error) {
			return rayT;
		}

		rayT += t;
	}

	return 1000.0;
}

// https://iquilezles.org/articles/nvscene2008/rwwtt.pdf
float calcAO(float3 pos, float3 nor) {
	float occ = 0.0;
	float sca = 1.0;
	for( int i=0; i<5; i++ )
	{
		float h = 0.01 + 0.12*float(i)/4.0;
		float d = map(pos + h*nor).x;
		occ += (h-d)*sca;
		sca *= 0.95;
		if( occ>0.35 ) break;
	}
	return clamp( 1.0 - 3.0*occ, 0.0, 1.0 ) * (0.5+0.5*nor.y);
}

float4 rayMarch(float3 rO, float3 rD) {
	float3 color = float3(0.2, 0.23, 0.26);
	float dist = 1000;

	float rayT = 0.0;
	
	[loop] for (int i = 0; i < 128; ++i) {
		float error = 0.0004 * rayT;

		float3 rayWorldPos = rO + rD * rayT;

		float4 mapR = map(rayWorldPos);
		float t = mapR.w;

		if (t <= error) {
			float3 normal = getNormal(rayWorldPos);
			float3 lightD = normalize(lightPos - rayWorldPos);
			float lightI = saturate(dot(lightD, normal));
			
			float lightTrace = getDist(rayWorldPos + normal * 0.01, lightD);
			float lightDist = length(lightPos - rayWorldPos);

			if (lightTrace >= lightDist) {
				
			} else {
				lightI = 0.0;
			}

			// float occ = calcAO(rayWorldPos, normal);

			float3 reflection = reflect(lightD, normal);
			float specularAngle = max(0.0, dot(reflection, rD));
			float3 lightColor = float3(0.8, 0.8, 0.8);
			float3 surfaceSpecularColor = float3(1.0, 1.0, 1.0);
			float surfaceShininess = 20.0;
			float3 illuminationSpecular = clamp(pow(specularAngle, surfaceShininess), 0.0, 1.0) * surfaceSpecularColor * lightColor;

			color = lerp(float3(0.02, 0.07, 0.15), float3(0.8, 0.8, 0.8), lightI);
			color *= mapR.rgb + illuminationSpecular;
			// col = float3(0.2, 0.7, 1.0);
			dist = rayT;
			break;
		}

		rayT += t;
	}

	return float4(color, dist);
}

struct PS_OUT {
	float4 color : SV_Target;
	float depth : SV_Depth;
};

PS_OUT mainPs(PS_INPUT input) {
	//float4 tex = texture0.Sample(sampler0, float3(input.uv.x, input.uv.y, 0.2));
	
	// PS_OUT output = (PS_OUT)0;
	// output.color = float4(tex.rrr, 1);
	// output.depth = 1.0;

	// return output;

	float4x4 invProjView = ProjectionMatrix;
	float4x4 projView = projMat;

	float4 rayStartWorld = mul(invProjView, float4(input.clipPos.xy, 0, 1));
	rayStartWorld /= rayStartWorld.w;
	float4 rayEndWorld = mul(invProjView, float4(input.clipPos.xy, 1, 1));
	rayEndWorld /= rayEndWorld.w;

	float3 rayDir = normalize(rayEndWorld.xyz - rayStartWorld.xyz);

	float4 result = rayMarch(rayStartWorld.xyz, rayDir);

	float4 worldHitPos = float4(rayStartWorld.xyz + rayDir * result.w, 1);
	float4 whpProj = mul(projView, worldHitPos);
	whpProj.z /= whpProj.w;
	
	float3 gammaCorrected = pow(result.xyz, 0.455);
	// float3 gammaCorrected = result.xyz;
	// float3 gammaCorrected = pow(result.xyz, 2.2);

	PS_OUT output = (PS_OUT)0;
	output.color = float4(gammaCorrected, 1);
	// output.depth = 1.0 - result.w / (100.0 - 0.01);
	output.depth = whpProj.z;// (100.0 - 0.01);

	return output;
	// return float4(input.uv.x, input.uv.y, 0, 1);
}