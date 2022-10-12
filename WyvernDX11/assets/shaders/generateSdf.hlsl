cbuffer constants : register(b0) {
	uint TriCount;
	float TestVal;
};

StructuredBuffer<float3> MeshTris : register(t0);
RWTexture3D<float> OutTex : register(u0);

float sphereSdf(float3 pos, float3 origin, float radius) {
	return length(pos - origin) - radius;
}

float dot2(float3 a) {
	return dot(a, a);
}

// https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float udTriangle(float3 p, float3 a, float3 b, float3 c) {
	float3 ba = b - a; float3 pa = p - a;
	float3 cb = c - b; float3 pb = p - b;
	float3 ac = a - c; float3 pc = p - c;
	float3 nor = cross( ba, ac );

	return sqrt(
	(sign(dot(cross(ba,nor),pa)) +
	 sign(dot(cross(cb,nor),pb)) +
	 sign(dot(cross(ac,nor),pc))<2.0)
	 ?
	 min( min(
	 dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0,1.0)-pa),
	 dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0,1.0)-pb) ),
	 dot2(ac*clamp(dot(ac,pc)/dot2(ac),0.0,1.0)-pc) )
	 :
	 dot(nor,pa)*dot(nor,pa)/dot2(nor) );
}

[numthreads(8, 8, 8)]
void mainCs(uint3 Id : SV_DispatchThreadID) {
	float3 pos = float3(Id.x + 0.5, Id.y + 0.5, Id.z + 0.5);

	pos -= float3(64, 64, 64);

	// float d = sphereSdf(pos, float3(64, 64, 64), 32);

	float closest = 10000.0;

	for (uint i = 0; i < TriCount; ++i) {
		float3 v0 = MeshTris[i * 3 + 0];
		float3 v1 = MeshTris[i * 3 + 1];
		float3 v2 = MeshTris[i * 3 + 2];

		float d = udTriangle(pos, v0, v1, v2);
		closest = min(closest, d);
	}

	OutTex[Id.xyz] = closest;
}