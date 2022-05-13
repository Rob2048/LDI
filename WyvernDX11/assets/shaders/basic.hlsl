cbuffer vertexBuffer : register(b0) {
	float4x4 ProjectionMatrix;
};

struct VS_INPUT {
	float3 pos : POSITION;
	float3 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

PS_INPUT mainVs(VS_INPUT input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1.f));
	output.col = float4(input.col, 1);
	output.uv = input.uv;

	return output;
}

float4 mainPs(PS_INPUT input) : SV_Target {	
	//float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
	return input.col;
}