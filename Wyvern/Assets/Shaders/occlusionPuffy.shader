Shader "Misc/occlusionPuffy"
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
			ZTest Less
			//ZTest Always
			
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			#pragma target 5.0
			
			#include "UnityCG.cginc"

			uniform float4x4 vpMat;
			uniform float4x4 vMat;
			uniform float4x4 pMat;
			uniform float3 camWorldPos;

			struct appdata {
				float4 vertex : POSITION;
				float4 normal : NORMAL;
			};

			struct v2f {
				float4 vertex : SV_POSITION;
				float4 normal: NORMAL;
				float4 clipSpacePos : TEXCOORD0;
				float4 camDepth : TEXCOORD1;
				float4 worldPos : TEXCOORD2;
			};
			
			v2f vert (appdata v) {
				v2f o;

				o.worldPos = mul(unity_ObjectToWorld, float4(v.vertex.xyz, 1.0));
				o.normal = mul(unity_ObjectToWorld, float4(v.normal.xyz, 0.0));
				float camDepth = length(camWorldPos.xyz - o.worldPos.xyz);
				o.camDepth.x = camDepth;

				//float4 viewSpacePos = mul(vMat, o.worldPos);

				// float scaleFactor = 0.01;
				// o.worldPos += float4(o.normal.xz * scaleFactor, 0, 0);
				// o.worldPos += float4(0.5, 0, 0, 0);
				
				o.clipSpacePos = mul(vpMat, o.worldPos);

				// o.normal = mul(vpMat, float4(o.normal.xyz, 0.0));
				o.normal = mul(vpMat, o.normal);
				o.normal = normalize(o.normal);

				float vertScale = max(20.0 - camDepth, 0) * 0.5;
				o.clipSpacePos += float4(o.normal.x * vertScale, o.normal.y * vertScale, 0.0, 0);

				o.vertex = o.clipSpacePos;

				return o;
			}
			
			fixed4 frag (v2f i) : SV_Target {
				// float3 n = (i.normal.xyz) / 2.0 + 0.5;
				// return float4(n.rgb, 1);

				return i.clipSpacePos.z;
				
			}
			ENDCG
		}
	}
}
