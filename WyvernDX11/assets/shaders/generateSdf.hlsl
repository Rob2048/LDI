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

	return (
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

struct TriIntersect {
	float dist;
	float intersect;
};

TriIntersect udTriIntersect(float3 p, float3 a, float3 b, float3 c) {
	float3 ba = b - a; float3 pa = p - a;
	float3 cb = c - b; float3 pb = p - b;
	float3 ac = a - c; float3 pc = p - c;
	float3 nor = cross( ba, ac );

	TriIntersect result;
	result.intersect = 0;
	
	bool intersect = sign(dot(cross(ba,nor),pa)) +
	 sign(dot(cross(cb,nor),pb)) +
	 sign(dot(cross(ac,nor),pc))<2.0;

	result.dist = intersect ?
	 min(min(
	 dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0,1.0)-pa),
	 dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0,1.0)-pb) ),
	 dot2(ac*clamp(dot(ac,pc)/dot2(ac),0.0,1.0)-pc) )
	 :
	 dot(nor,pa)*dot(nor,pa)/dot2(nor);

	// Ray cast UP.
	float3 v0 = a;
	float3 v1 = b;
	float3 v2 = c;

	float3 rD = float3(0, 1, 0);
	float3 e1 = v1 - v0;
	float3 e2 = v2 - v0;

	float3 iH = cross(rD, e2);
	float iA = dot(e1, iH);

	if (iA > -0.00001 && iA < 0.00001) {
		return result;
	}

	float iF = 1.0 / iA;
	float3 iS = p - v0;
	float iU = iF * dot(iS, iH);

	if (iU < 0.0 || iU > 1.0) {
		return result;
	}
	
	float3 iQ = cross(iS, e1);
	float iV = iF * dot(rD, iQ);

	if (iV < 0.0 || iU + iV > 1.0) {
		return result;
	}

	// at this stage we can compute t to find out where
	// the intersection point is on the line
	float iT = iF * dot(e2, iQ);

 	// ray intersection
	if (iT > 0.00001) {
		result.intersect = 1;
	}
	
	return result;
}

[numthreads(8, 8, 8)]
void mainCs(uint3 Id : SV_DispatchThreadID) {
	float3 pos = float3(Id.x + 0.5, Id.y + 0.5, Id.z + 0.5);

	// float d = sphereSdf(pos, float3(64, 64, 64), 32);

	float closest = 1000000.0;
	float totalTris = 0;

	for (uint i = 0; i < TriCount; ++i) {
		float3 v0 = MeshTris[i * 3 + 0];
		float3 v1 = MeshTris[i * 3 + 1];
		float3 v2 = MeshTris[i * 3 + 2];

		// float d = udTriangle(pos, v0, v1, v2);
		TriIntersect r = udTriIntersect(pos, v0, v1, v2);
		closest = min(closest, r.dist);
		totalTris += r.intersect;
	}

	float sgn = totalTris % 2 == 0 ? +1 : -1;

	OutTex[Id.xyz] = sqrt(closest) * sgn;
}