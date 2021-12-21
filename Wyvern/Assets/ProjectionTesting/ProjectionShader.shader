Shader "Unlit/ProjectionShader" {
	Properties {
		_MainTex ("Texture", 2D) = "white" {}
	}
	SubShader {
		Tags { "RenderType"="Opaque" }
		LOD 100

		Pass {
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"

			struct appdata {
				float4 vertex : POSITION;
				float3 normal : NORMAL;
				float2 uv : TEXCOORD0;
			};

			struct v2f {
				float4 vertex : SV_POSITION;
				float2 uv : TEXCOORD0;
				float3 normal : NORMAL;
				float3 worldPos : TEXCOORD1;
			};

			sampler2D depthPreTex;
			sampler2D candidateTex;
			sampler2D _MainTex;
			float4 _MainTex_ST;

			uniform float4x4 projVpMat;
			uniform float4x4 projPInvMat;
			uniform float3 camWorldPos;

			v2f vert (appdata v) {
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				// o.normal = mul(UNITY_MATRIX_V, mul(unity_ObjectToWorld, float4(pos, 1.0))).xyz;
				o.normal = UnityObjectToWorldNormal(v.normal);

				o.worldPos = mul(unity_ObjectToWorld, float4(v.vertex.xyz, 1.0));

				return o;
			}

			float4 frag (v2f i) : SV_Target {
				float4 projClipPos = mul(projVpMat, float4(i.worldPos, 1));
				float4 projClipPosPreW = projClipPos;
				float projToPixelDepth = length(i.worldPos - camWorldPos);

				projClipPos /= projClipPos.w;

				float maxAngle = 30.0;

				float3 fC = float3(0.2, 0.2, 0.2);
				float3 nC = (i.normal * 0.5 + 0.5) * 0.5;

				if (projClipPos.z > 0 && projClipPos.x > -1.0 && projClipPos.x < 1.0 && projClipPos.y > -1.0 && projClipPos.y < 1.0) {
					float2 projUv = projClipPos.xy * 0.5 + 0.5;

					// NOTE: Galvo at 20cm = 5cm square = 1000 x 50um.

					float2 texUv = float2(projUv.x, 1.0 - projUv.y);
					float depthPre = tex2D(depthPreTex, texUv).r;
					float3 candidateV = tex2D(candidateTex, texUv).rgb;

					
					// 	return float4(lerp(nC, candidateV, 0.8), 1);

					// NOTE: Offset is not in clip space.
					if (projClipPosPreW.z > depthPre - 0.001) {

						return float4(candidateV, 1);
						return float4(lerp(nC, candidateV, 0.8), 1);

						float3 camDir = normalize(camWorldPos - i.worldPos);
						float f = dot(i.normal.xyz, camDir);
						f = saturate(f);

						float angleDegree = acos(f) * 57.2958;
						// angleDegree = saturate((dv / 90.0));

						if (angleDegree > maxAngle) {
							float3 cA = float3(0, 1, 0);
							float3 cB = float3(0, 0, 1);
							fC = lerp(cA, cB, (angleDegree - maxAngle) / (90 - maxAngle));
							return float4(fC, 1);

						} else {
							return float4((trunc(projUv * 1000) / 5.0) % 1, 0, 1);

							// float4 col = tex2D(_MainTex, projUv);
							// return float4(col.rgb, 1);
							float3 cA = float3(0, 1, 0);
							float3 cB = float3(0, 0, 1);
							fC = lerp(cA, cB, angleDegree / maxAngle);
							return float4(fC, 1);
							// fC = float3(1, 0, 0);
						}
					}
				}

				return float4(lerp(nC, fC, 0.5), 1);
			}
			ENDCG
		}
	}
}
