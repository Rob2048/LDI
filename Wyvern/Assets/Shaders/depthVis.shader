Shader "Misc/depthVis"
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

			uniform float4x4 projMat;
			uniform float4x4 modelMat;
			uniform float4x4 viewMat;
			uniform float4x4 vpMat;

			struct appdata
			{
				float4 vertex : POSITION;
				float4 normal : NORMAL;
			};

			struct v2f
			{
				float4 vertex : SV_POSITION;
				float4 clipSpacePos : TEXCOORD0;
				float4 normal : TEXCOORD1;
			};
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);

				// o.vertex = mul(UNITY_MATRIX_VP, mul(modelMat, float4(v.vertex.xyz, 1.0)));
				// o.vertex = mul(vpMat, mul(modelMat, float4(v.vertex.xyz, 1.0)));

				// o.clipSpacePos = mul(vpMat, mul(modelMat, float4(v.vertex.xyz, 1.0)));
				// o.vertex = mul(UNITY_MATRIX_VP, float4(posWorld, 1.0));

				// o.normal = mul(modelMat, float4(v.normal.xyz, 0.0));
				// o.normal = v.normal;

				// float4x4 mvp = mul(mul(modelMat, viewMat), projMat);
				// float4x4 mvp = mul(modelMat, mul(projMat, viewMat));

				// o.vertex = mul(modelMat, float4(v.vertex.xyz, 1.0));
				// o.vertex = mul(viewMat, o.vertex);
				// o.vertex = mul(projMat, o.vertex);

				// float4 worldPos = mul(modelMat, float4(v.vertex.xyz, 1.0));

				// o.vertex = mul(float4(mul(unity_ObjectToWorld, float4(v.vertex.xyz, 1.0)).xyz, 1.0), mul(projMat, viewMat));
				// o.vertex = mul(float4(mul(unity_ObjectToWorld, float4(v.vertex.xyz, 1.0)).xyz, 1.0), mul(projMat, viewMat));

				// o.vertex = mul(mul(viewMat, projMat), worldPos);

				//o.vertex = mul(modelMat, float4(v.vertex.xyz, 1.0));
				//o.vertex = mul(projMat, o.vertex);

				// o.posProj = mul(modelMat, float4(v.vertex.xyz, 1.0));
				// o.posProj = mul(projMat, o.posProj);

				return o;
			}
			
			float4 frag (v2f i) : SV_Target
			{
				return float4(0, 1, 0, 1);

				// return float4(zp, zp, zp, 1.0);

				// return float4(0, 1, 0, 1);
				//float d = i.posProj.z / i.posProj.w;


				// float nI = dot(normalize(i.normal.xyz), float3(0, 0, 1));

				// nI = 1.0 - nI;

				// nI = nI * 0.5;
				// nI = saturate(nI);

				// return float4(nI, nI, nI, 1.0);
				// return float4(i.normal.rgb * 0.5 + 0.5, 1.0);
				// return float4(0, 1, 0, 1);

				// return d;
				//return 1.0 - i.vertex.z / i.vertex.w;
			}
			ENDCG
		}
	}
}
