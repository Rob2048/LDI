Shader "Misc/QuantaVis"
{
	Properties
	{
		_orthoDepthTex ("Spray Depth", 2D) = "white" {}
	}

	SubShader
	{
		Pass
		{
			LOD 200
			Cull Off
			ZWrite Off
			ZTest Always
			Conservative True

			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag

			#include "UnityCG.cginc"

			uniform float4x4 vpMat;
			uniform float4x4 viewMat;
			uniform float4x4 modelMat;

			uniform float4 viewForward;
			
			sampler2D _orthoDepthTex;

			struct v2f
			{
				float4 pos : SV_POSITION;
				float3 normal: NORMAL;
				float2 uv : TEXCOORD0;
				float4 clipSpacePos : TEXCOORD1;
			};

			v2f vert(appdata_full v) 
			{
				v2f o;
				o.pos = float4(v.texcoord1.xy * 2 - 1, 1, 1);
				o.pos.y = -o.pos.y;
				o.uv = v.texcoord1.xy;
				
				// TODO: Also view.
				// o.normal = mul(mul(modelMat, viewMat), float4(v.normal.xyz, 0.0)).xyz;
				o.normal = mul(modelMat, float4(v.normal.xyz, 0.0)).xyz;

				o.clipSpacePos = mul(vpMat, mul(modelMat, float4(v.vertex.xyz, 1.0)));

				return o;
			}

			float4 frag(v2f i) : COLOR
			{
				float2 pUV;
				pUV.x =  i.clipSpacePos.x / 2.0 + 0.5;
    			pUV.y = -i.clipSpacePos.y / 2.0 + 0.5;

				float sD = tex2D(_orthoDepthTex, pUV).r;

				float f = 1.0f;

				if (i.clipSpacePos.z < sD - 0.00001) {
					f = 0.0f;
				}

				float nD = dot(viewForward.xyz, normalize(i.normal));
				nD = saturate(nD);

				// Convert range to 0 - 45 degrees.
				// float hv = saturate((nD - 0.7071) * 3.14159);
				// 60 deg.				
				// float hv = saturate((nD - 0.5) * 2.0);
				float hv = saturate(nD);

				// Stretch range to favor head on angle.
				// hv = pow(hv, 10);

				hv *= f;

				//float3 n = i.normal * 0.5 + 0.5;
				//return float4(n, 1.0);

				return float4(hv, hv, hv, 1.0);

				// return cF;

				// 30 deg: 0.8660
				// 45 deg: 0.7071
				// float a = 1;
				// if (nD >= 0.7071) {
				// 	a = 1.0;
				// } else {
				// 	a = 0.0;
				// }

				// // f *= a;

				// return cF * a;

				// return float4(nD, nD, nD, 1);


				// float3 n = i.normal * 0.5 + 0.5;
				// return float4(f, f, f, 1);
				//return float4(i.uv.xy, 0, 1);
			}

			ENDCG
		}        
	}
}
