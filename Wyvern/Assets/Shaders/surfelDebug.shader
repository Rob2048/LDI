Shader "Unlit/surfelDebug"
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
			// Offset -1, -1
			
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
				float4 color: COLOR;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				float4 vertex : SV_POSITION;
				float4 color : COLOR;
				int id: TEXCOORD1;
			};

			StructuredBuffer<float4> surfelData;

			sampler2D _MainTex;
			float4 _MainTex_ST;

			v2f vert (appdata v)
			{
				v2f o;

				int id0 = v.color.r * 255;
				int id1 = v.color.g * 255;
				int id2 = v.color.b * 255;
				int id = id0 | (id1 << 8) | (id2 << 16);
				o.id = id;

				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				o.color = v.color;
				
				return o;
			}

			float4 frag (v2f i) : SV_Target
			{
				// sample the texture
				float4 col = tex2D(_MainTex, i.uv);
				clip(col.a - 0.1);

				// Color32 c = new Color32((byte)(i & 0xFF), (byte)((i >> 8) & 0xFF), (byte)((i >> 16) & 0xFF), 255);

				// int id0 = i.color.r * 255;
				// int id1 = i.color.g * 255;
				// int id2 = i.color.b * 255;
				// int id = id0 | (id1 << 8) | (id2 << 16);

				// if (i.id > 10000) {
				// 	return float4(1, 0, 1, 1);
				// }

				// return i.color;

				float4 color = surfelData[i.id];

				return float4(color.rgb, 1);
				
				// float4 outC = float4(intensity, intensity, intensity, 1);
				// outC.rgb = pow(outC.rgb, 2.2f);

				// return outC;
			}
			ENDCG
		}
	}
}
