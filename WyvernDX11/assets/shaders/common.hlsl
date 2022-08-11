#ifdef BASIC_CONSTANT_BUFFER
	cbuffer basicConstantBufer : register(b0) {
		float4 screenSize;
		float4x4 ProjectionMatrix;
		float4 ObjectColor;
		float4x4 viewMat;
		float4x4 projMat;
	};
#endif

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

//---------------------------------------------------------------------------------------------------------------
// Image space helper functions.
//---------------------------------------------------------------------------------------------------------------
float GammaToLinear(float In) {
	float linearRGBLo = In / 12.92f;
	float linearRGBHi = pow(max(abs((In + 0.055f) / 1.055f), 1.192092896e-07f), 2.4f);

	return (In <= 0.04045) ? linearRGBLo : linearRGBHi;
}

float LinearToGamma(float In) {
	float sRGBLo = In * 12.92f;
	float sRGBHi = (pow(max(abs(In), 1.192092896e-07f), 1.0f / 2.4f) * 1.055f) - 0.055f;
	return (In <= 0.0031308f) ? sRGBLo : sRGBHi;
}