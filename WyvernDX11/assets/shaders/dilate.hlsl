sampler sampler0;
Texture2D<float4> SourceTex : register(t0);
Texture2D<float4> WorldPosTex : register(t1);
RWTexture2D<float4> DestTex : register(u0);

[numthreads(8, 8, 1)]
void mainCs(uint3 Id : SV_DispatchThreadID) {
	uint2 pos = Id.xy;

	const int MaxSteps = 2;
	const float TextureSize = 2048.0;
	const float texelSize = 1 / TextureSize;
	const float2 offsets[8] = {float2(-1,0), float2(1,0), float2(0,1), float2(0,-1), float2(-1,1), float2(1,1), float2(1,-1), float2(-1,-1)};
	
	float2 uv = float2((Id.x + 0.5) / TextureSize, (Id.y + 0.5) / TextureSize);
	float minDist = 10000000;

	float4 srcTex = SourceTex.SampleLevel(sampler0, uv, 0);
	float3 curMinSample = srcTex.rgb;

	// Calculate normal.
	float3 xN = WorldPosTex.SampleLevel(sampler0, uv + (float2(-1,  0) * texelSize), 0).rgb;
	float3 xP = WorldPosTex.SampleLevel(sampler0, uv + (float2( 1,  0) * texelSize), 0).rgb;
	float3 yN = WorldPosTex.SampleLevel(sampler0, uv + (float2( 0, -1) * texelSize), 0).rgb;
	float3 yP = WorldPosTex.SampleLevel(sampler0, uv + (float2( 0,  1) * texelSize), 0).rgb;

	xN *= 40.0;
	xP *= 40.0;
	yN *= 40.0;
	yP *= 40.0;

	// normal.x = xN - xP;
	// normal.y = yN - yP;
	// normal.z = 0.2;
	// normal = normalize(normal);

	float3 va = normalize(xP-xN);
	float3 vb = normalize(yP-yN);
	float3 normal = cross(va,vb);

	// // cross(float2(xN, yN), float2(xP, yP))
	
	// // DestTex[Id.xy] = float4(1, 0, 0, srcTex.r);
	DestTex[Id.xy] = float4(normal * 0.5 + 0.5, srcTex.r);
	return;

	// Calculate padding.
	if (srcTex.w == 0) {
		int i = 0;
		while (i < MaxSteps) {
			i++;
		
			int j = 0;
			while (j < 8) {
				float2 curUv = uv + offsets[j] * texelSize * i;
				float4 offsetSample = SourceTex.SampleLevel(sampler0, curUv, 0);

				if (offsetSample.w != 0) {
					float curDist = length(uv - curUv);

					if (curDist < minDist) {
						float2 projectUv = curUv + offsets[j] * texelSize * i * 0.25;
						float4 direction = SourceTex.SampleLevel(sampler0, projectUv, 0);
						minDist = curDist;

						if (direction.w != 0) {
							float3 delta = offsetSample.xyz - direction.xyz;
							curMinSample = offsetSample.xyz + delta * 4;
						} else {
							curMinSample = offsetSample.xyz;
						}
					}
				}

				j++;
			}
		}
	}

	DestTex[Id.xy] = float4(curMinSample, 1);
}

/*

float texelsize = 1 / TextureSize;
float mindist = 10000000;
float2 offsets[8] = {float2(-1,0), float2(1,0), float2(0,1), float2(0,-1), float2(-1,1), float2(1,1), float2(1,-1), float2(-1,-1)};

float3 sample = Tex.SampleLevel(TexSampler, UV, 0);
float3 curminsample = sample;

if(sample.x == 0 && sample.y == 0 && sample.z == 0)
{
	int i = 0;
	while(i < MaxSteps)
	{ 
		i++;
		int j = 0;
		while (j < 8)
		{
			float2 curUV = UV + offsets[j] * texelsize * i;
			float3 offsetsample = Tex.SampleLevel(TexSampler, curUV, 0);

			if(offsetsample.x != 0 || offsetsample.y != 0 || offsetsample.z != 0)
			{
				float curdist = length(UV - curUV);

				if (curdist < mindist)
				{
					float2 projectUV = curUV + offsets[j] * texelsize * i * 0.25;
					float3 direction = Tex.SampleLevel(TexSampler, projectUV, 0);
					mindist = curdist;

					if(direction.x != 0 || direction.y != 0 || direction.z != 0)
					{
						float3 delta = offsetsample - direction;
						curminsample = offsetsample + delta * 4
					}

				   else
					{
						curminsample = offsetsample;
					}
				}
			}
			j++;
		}
	}
}

*/