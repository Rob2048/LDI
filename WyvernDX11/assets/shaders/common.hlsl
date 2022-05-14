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