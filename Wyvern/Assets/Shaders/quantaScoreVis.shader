Shader "Misc/quantaScoreVis"
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

			sampler2D _MainTex;

			struct appdata
			{
				float4 vertex : POSITION;
				float4 normal : NORMAL;
				float2 uv : TEXCOORD0;
			};

			struct v2f
			{
				float4 vertex : SV_POSITION;
				// float4 clipSpacePos : TEXCOORD0;
				// float4 normal : TEXCOORD1;
				float2 uv : TEXCOORD0;
			};
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;

				return o;
			}
			
			float4 frag (v2f i) : SV_Target
			{
				float4 s = tex2D(_MainTex, i.uv);

				// float2 uvq = (i.uv * 4096.0) % 1.0;
				// return float4(uvq, 0, 1);

				float2 uvq = (i.uv * 4096.0) % 2.0;
				// return float4(uvq, 0, 1);

				float fade = 1.0;

				if (uvq.x < 1) {
					if (uvq.y < 1) {
						fade = 0;
					}
				} else {
					if (uvq.y >= 1) {
						fade = 0;
					}
				}

				fade = 1.0 - fade * 0.3;

				//return float4(fade, fade, fade, 1);

				// NOTE: s.r is 0.0 - 1.0 = 0 to 60 degrees.

				// float deg = acos(s.r) * 57.2958;
				// float dv = deg / 60.0;

				float dv = s.r;

				float3 cC = float3(1.0, 0.0, 0.0);
				float3 cA = float3(0.0, 1.0, 0.0);
				float3 cB = float3(0.0, 0.0, 1.0);
				float3 cF = lerp(cC, cA, dv);

				if (dv > 0.5) {
					cF = lerp(cB, cA, dv * 2.0 - 1.0);
				} else {
					cF = lerp(cC, cB, dv * 2.0);
				}
				
				float3 f = cF;

				if (s.a == 0) {
					f = float3(0, 0, 0);
				}

				// NOTE: Temp white background.
				// return float4(0.8, 0.8, 0.8, 1); 
				return float4(1, 1, 1, 1); 

				return float4(f * fade, 1);
			}
			ENDCG
		}
	}
}
