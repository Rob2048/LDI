cbuffer vertexBuffer : register(b0) {
	float4x4 ProjectionMatrix;
};

struct VS_INPUT {
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv  : TEXCOORD0;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv  : TEXCOORD0;
};

PS_INPUT mainVs(VS_INPUT input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1.f));
	output.normal = float4(input.normal, 1);
	output.uv = input.uv;

	return output;
}

float4 mainPs(PS_INPUT input) : SV_Target {	
	//float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
	return float4(input.normal.xyz * 0.5 + 0.5, 1.0);
	return float4(1, 0, 0, 1);
}