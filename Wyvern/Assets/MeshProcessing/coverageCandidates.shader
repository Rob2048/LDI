﻿Shader "Misc/coverageCandidates"
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
			// Cull Back
			// ZWrite On
			// ZTest LEqual

			Cull Off
            ZWrite Off
            ZTest Always
			Conservative True
			
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			#pragma target 5.0
			
			#include "UnityCG.cginc"

			uniform float4x4 vpMat;

			uniform float3 camWorldPos;
			
			sampler2D depthPreTex;
			sampler2D _MainTex;

			struct appdata {
				float4 vertex : POSITION;
				float4 normal : NORMAL;
				float2 uv : TEXCOORD0;
			};

			struct v2f {
				float4 vertex : SV_POSITION;
				float3 normal: NORMAL;
				float4 clipSpacePos : TEXCOORD0;
				float4 camDepth : TEXCOORD1;
				float4 worldPos : TEXCOORD2;
			};
			
			v2f vert (appdata v) {
				v2f o;

				o.worldPos = mul(unity_ObjectToWorld, float4(v.vertex.xyz, 1.0));

				o.camDepth.x = length(camWorldPos.xyz - o.worldPos.xyz);
				o.clipSpacePos = mul(vpMat, o.worldPos);
				//o.vertex = o.clipSpacePos;
				o.vertex = float4(v.uv.xy * 2 - 1, 1, 1) * float4(1, -1, 1, 1);

				o.normal = mul(unity_ObjectToWorld, float4(v.normal.xyz, 0.0)).xyz;

				return o;
			}
			
			fixed4 frag (v2f i) : SV_Target {
				// return float4(1, 0, 0, 1);

				float3 camDir = normalize(camWorldPos - i.worldPos);
				float f = dot(i.normal.xyz, camDir);

				float maxAngle = 20.0; // 6% cosign error.

				float u = (i.clipSpacePos.x / i.clipSpacePos.w) * 0.5 + 0.5;
				float v = (i.clipSpacePos.y / i.clipSpacePos.w) * 0.5 + 0.5;
				v = 1.0 - v;
				// return float4(u, v, 0, 1);

				if (u < 0 || u > 1 || v < 0 || v > 1) {
					// return float4(0, 1, 0, 1);
				} else {

					float4 depthPre = tex2D(depthPreTex, float2(u, v));

					//return float4(depthPre.r, 0, 0, 1);

					// NOTE: Check for visible points. Including puffy mesh. - TODO: Watch for depths that are allowed, but should not be. Like tiny/close details.
					if (i.clipSpacePos.z >= (depthPre.r - 0.008)) {
						// NOTE: Check for surface angles.
						if (f > (90.0 - maxAngle) / 90.0) {
							// NOTE: Check for focal depth.
							if (i.camDepth.x <= 20.15 && i.camDepth.x >= 19.85) {

								return float4(1, 1, 1, 1);
							} else {
								// return float4(0, 0, 1, 1);
							}
						} else {
							// return float4(0, 0, 0, 1);
						}
					}
				}

				clip(-1);
				return float4(0, 0, 0, 1);
			}
			ENDCG
		}
	}
}
