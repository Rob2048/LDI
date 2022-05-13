cbuffer vertexBuffer : register(b0) {
	float4x4 ProjectionMatrix;
};

struct VS_INPUT {
	float3 pos : POSITION;
	float3 col : COLOR0;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
};

PS_INPUT mainVs(VS_INPUT input) {
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos, 1.f));
	output.col = float4(input.col, 1);

	return output;
}

float4 mainPs(PS_INPUT input) : SV_Target {	
	return float4(input.col.rgb, 1.0);
}