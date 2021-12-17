Shader "Misc/orthoDepth"
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
			
			#include "UnityCG.cginc"

			uniform float4x4 vpMat;

			struct appdata {
				float4 vertex : POSITION;
				float4 normal : NORMAL;
			};

			struct v2f {
				float4 vertex : SV_POSITION;
				float4 clipSpacePos : TEXCOORD0;
			};
			
			v2f vert (appdata v) {
				v2f o;
				o.clipSpacePos = mul(vpMat, mul(unity_ObjectToWorld, float4(v.vertex.xyz, 1.0)));
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
