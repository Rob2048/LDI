//---------------------------------------------------------------------------------------------------------------
// Constant buffers.
//---------------------------------------------------------------------------------------------------------------
#ifdef BASIC_CONSTANT_BUFFER
	cbuffer basicConstantBufer : register(b0) {
		float4 screenSize;
		float4x4 ProjectionMatrix;
		float4 ObjectColor;
		float4x4 viewMat;
		float4x4 projMat;
		float4x4 worldMat;
	};
#endif

//---------------------------------------------------------------------------------------------------------------
// Vertex layout definitions.
//---------------------------------------------------------------------------------------------------------------
struct vertexInputSimple {
	float3 pos : POSITION;
	float3 col : COLOR0;
};

struct vertexInputBasic {
	float3 pos : POSITION;
	float3 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

struct vertexInputMesh {
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv  : TEXCOORD0;
};

struct vertexInputCoveragePoint {
	float3 pos : POSITION;
	int id : TEXCOORD0;
	float3 normal : NORMAL;
};

//---------------------------------------------------------------------------------------------------------------
// Image space helper functions.
//---------------------------------------------------------------------------------------------------------------
float GammaToLinear(float In) {
	float linearRGBLo = In / 12.92f;
	float linearRGBHi = pow(max(abs((In + 0.055f) / 1.055f), 1.192092896e-07f), 2.4f);

	return (In <= 0.04045) ? linearRGBLo : linearRGBHi;
}

float3 GammaToLinear(float3 In) {
	return float3(GammaToLinear(In.r), GammaToLinear(In.g), GammaToLinear(In.b));
}

float LinearToGamma(float In) {
	float sRGBLo = In * 12.92f;
	float sRGBHi = (pow(max(abs(In), 1.192092896e-07f), 1.0f / 2.4f) * 1.055f) - 0.055f;
	return (In <= 0.0031308f) ? sRGBLo : sRGBHi;
}

float3 LinearToGamma(float3 In) {
	return float3(LinearToGamma(In.r), LinearToGamma(In.g), LinearToGamma(In.b));
}

//---------------------------------------------------------------------------------------------------------------
// Utility functions.
//---------------------------------------------------------------------------------------------------------------
// NOTE: Modified from https://stackoverflow.com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader
float random(float p) {
	float K1 = 23.14069263277926;
	return frac(cos(dot(p,K1)) * 12345.6789);
}