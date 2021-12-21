Shader "Misc/candidatesSoft"
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

			sampler2D depthPreTex;
			sampler2D _MainTex;
			sampler2D coverageTex;

			struct appdata {
				float4 vertex : POSITION;
				float4 normal : NORMAL;
				float4 color: COLOR;
				float2 uv : TEXCOORD0;
				float2 uv2 : TEXCOORD1;
			};

			struct v2f {
				float4 vertex : SV_POSITION;
				float3 normal: NORMAL;
				float4 color: COLOR;
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
				o.normal = normalize(o.normal);

				o.color = v.color;

				return o;
			}
			
			float4 frag (v2f i) : SV_Target {
				float3 col = GammaToLinearSpace(i.color.rgb);
				float maxAngle = 35.0;

				float3 camDir = normalize(camWorldPos - i.worldPos);
				float f = dot(i.normal.xyz, camDir);
				f = saturate(f);
				float angleDegree = acos(f) * 57.2958;

				float u = (i.clipSpacePos.x / i.clipSpacePos.w) * 0.5 + 0.5;
				float v = (i.clipSpacePos.y / i.clipSpacePos.w) * 0.5 + 0.5;
				v = 1.0 - v;
				
				float4 depthPre = tex2D(depthPreTex, float2(u, v));

				//return float4(depthPre.r, 0, 0, 1);

				// NOTE: Check for point depth. Including puffy mesh.
				if (i.clipSpacePos.z > (depthPre.r - 0.001)) {
					// NOTE: Check for surface angles.
					if (angleDegree > maxAngle) {
						return float4(0, 0, 1, 1);
					}

					if (i.camDepth.x <= 20.15 && i.camDepth.x >= 19.85) {
						// return float4(col, 1);
						return float4((trunc(float2(u, v) * 1024) / 8.0) % 1, 0, 1);
						// return float4(0, 1, 0, 1);
					}

					return float4(1, 0, 0, 1);
				}

				return float4(1, 0, 0, 1);
			}
			ENDCG
		}
	}
}
