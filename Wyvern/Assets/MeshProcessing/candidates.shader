Shader "Misc/candidates"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
	}
	SubShader
	{
		Tags { "RenderType"="Opaque" }
		LOD 100

		Pass
		{
			Cull Back
			ZWrite On
			ZTest LEqual
			//ZTest Always
			
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			#pragma target 5.0
			
			#include "UnityCG.cginc"

			uniform float4x4 vpMat;

			uniform float3 camWorldPos;

			RWStructuredBuffer<int> candidateCounter : register(u1);
			RWStructuredBuffer<float> candidateMeta : register(u2);

			sampler2D depthPreTex;
			sampler2D _MainTex;
			sampler2D coverageTex;

			struct appdata {
				float4 vertex : POSITION;
				float4 normal : NORMAL;
				float2 uv : TEXCOORD0;
				float2 uv2 : TEXCOORD1;
			};

			struct v2f {
				float4 vertex : SV_POSITION;
				float3 normal: NORMAL;
				float4 clipSpacePos : TEXCOORD0;
				float4 camDepth : TEXCOORD1;
				float4 worldPos : TEXCOORD2;
				float2 uv : TEXCOORD3;
				float2 uv2 : TEXCOORD4;
			};
			
			v2f vert (appdata v) {
				v2f o;

				o.worldPos = mul(unity_ObjectToWorld, float4(v.vertex.xyz, 1.0));

				o.camDepth.x = length(camWorldPos.xyz - o.worldPos.xyz);
				o.clipSpacePos = mul(vpMat, o.worldPos);
				o.vertex = o.clipSpacePos;
				o.uv = v.uv;
				o.uv2 = v.uv2;

				o.normal = mul(unity_ObjectToWorld, float4(v.normal.xyz, 0.0)).xyz;

				return o;
			}
			
			fixed4 frag (v2f i) : SV_Target {
				float3 camDir = normalize(camWorldPos - i.worldPos);
				float f = dot(i.normal.xyz, camDir);

				float maxAngle = 20.0; // 6% cosign error.

				float u = (i.clipSpacePos.x / i.clipSpacePos.w) * 0.5 + 0.5;
				float v = (i.clipSpacePos.y / i.clipSpacePos.w) * 0.5 + 0.5;
				v = 1.0 - v;
				// return float4(u, v, 0, 1);

				float4 depthPre = tex2D(depthPreTex, float2(u, v));

				//return float4(depthPre.r, 0, 0, 1);

				// NOTE: Check for point depth. Including puffy mesh. - TODO: Watch for depths that are allowed, but should not be. Like tiny/close details.
				if (i.clipSpacePos.z >= (depthPre.r - 0.008)) {
					// NOTE: Check for surface angles.
					if (f > (90.0 - maxAngle) / 90.0) {
						// NOTE: Check for focal depth.
						if (i.camDepth.x <= 20.15 && i.camDepth.x >= 19.85) {
							// NOTE: Check for coverage.
							float4 coverage = tex2D(coverageTex, i.uv);

							if (coverage.g != 1.0) {
								int cCounter = 0;
								InterlockedAdd(candidateCounter[0], 1, cCounter);
								int idx = cCounter * 7;

								candidateMeta[idx + 0] = i.worldPos.x;
								candidateMeta[idx + 1] = i.worldPos.y;
								candidateMeta[idx + 2] = i.worldPos.z;

								candidateMeta[idx + 3] = i.normal.x;
								candidateMeta[idx + 4] = i.normal.y;
								candidateMeta[idx + 5] = i.normal.z;

								float pScale = 10.0;
								float p = floor(i.worldPos.y * pScale) + floor(i.worldPos.z * pScale) + floor(i.worldPos.x * pScale);
								p = frac(p * 0.5);
								p *= (i.worldPos.y - 2.0) / 2.0;

								// p = saturate(p) + 0.15;

								// float4 col = tex2D(_MainTex, i.worldPos.xy * float2(1, 0.5) * 0.8 + float2(0.5, 0.5));
								float4 col = tex2D(_MainTex, i.uv2);

								// float lum = col.r * 0.2126f + col.g * 0.7152f + col.b * 0.0722f;
								float lum = col.r;
								p = lum;

								// candidateMeta[idx + 6] = saturate(i.worldPos.y / 8.0);
								candidateMeta[idx + 6] = saturate(p);

								return float4(1, 0, 0, 1);
							} else {
								return float4(0, 1, 1, 1);
							}
						} else {
							return float4(0, 0, 1, 1);
						}
					} else {
						return float4(0, 0, 0, 1);
					}
				}

				return float4(1, 1, 0, 1);
			}
			ENDCG
		}
	}
}
