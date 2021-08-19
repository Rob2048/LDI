Shader "Misc/basic"
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

			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			#pragma target 5.0
			
			#include "UnityCG.cginc"

			uniform float4x4 vpMat;

			uniform float3 camWorldPos;

			struct appdata {
				float4 vertex : POSITION;
			};

			struct v2f {
				float4 vertex : SV_POSITION;
				float4 clipSpacePos : TEXCOORD1;
				float4 worldPos : TEXCOORD2;
			};
			
			v2f vert (appdata v) {
				v2f o;

				o.worldPos = mul(unity_ObjectToWorld, float4(v.vertex.xyz, 1.0));
				o.clipSpacePos = mul(vpMat, o.worldPos);
				o.vertex = o.clipSpacePos;

				return o;
			}
			
			float frag (v2f i) : SV_Target {
				return i.clipSpacePos.z;
			}
			ENDCG
		}
	}
}

