Shader "Misc/coverageMeshView"
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
				float2 uv2 : TEXCOORD1;
			};

			struct v2f
			{
				float4 vertex : SV_POSITION;
				// float4 clipSpacePos : TEXCOORD0;
				// float4 normal : TEXCOORD1;
				float2 uv : TEXCOORD0;
				float2 uv2 : TEXCOORD1;
			};
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv2;
				o.uv2 = v.uv2;

				return o;
			}
			
			float4 frag (v2f i) : SV_Target
			{
				// Surfel.
				float triScale = 0.08 * 20;
				float2 uvs = ((i.uv) * 4096.0 / triScale) % 2.0;

				// Texel.
				// NOTE: 1 texel = 50um, this could change.
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

				float2 uvRG = i.uv * fade;

				// NOTE: s.r is 0.0 - 1.0 = 0 to 60 degrees (quanta_vis.shader).
				float4 s = tex2D(_MainTex, i.uv);
				float dv = acos(s.r) * 57.2958;
				// NOTE: Convert 0 to 90 to 0 to 60 (clamp 0 to 1).
				// dv = saturate(1.0 - (dv / 60));
				dv = saturate((dv / 90.0));
				// float 

				// float t = s.r;

				// if (dv > 0.5) {
				// 	return float4(1, 0, 0, 1);
				// }

				// float fo = 1.0 - dv / 90;
				float fo = 1.0 - dv;
				// return float4(fo, fo, fo, 1);

				// float t = s.r;

				// // if (t < 0.99619469809) { // 5 deg
				// // if (t < 0.98480775301) { // 10 deg
				// if (t < 0.98480775301) { // 10 deg
				// 	return float4(1, 0, 0, 1);
				// }

				// return float4(t, t, t, 1);
				
				float3 cC = float3(1.0, 0.0, 0.0);
				float3 cA = float3(0.0, 1.0, 0.0);
				float3 cB = float3(0.0, 0.0, 1.0);
				float3 cF = lerp(cC, cA, dv);

				if (dv < (25.0 / 90.0)) {
					cF = lerp(cA, cB, dv * (1.0 / (25.0 / 90.0)));
				} else {
					cF = lerp(cB, cC, (dv - (25.0 / 90.0)) * (1.0 / ((60.0 - 25.0) / 90.0)) );
				}

				if (dv < (25.0 / 90.0)) {
					// cF = float3(0, 1, 0);
					cF = lerp(cA, cB, dv * (1.0 / (25.0 / 90.0)));
				} else if (dv < (35.0 / 90.0)) {
					cF = float3(0, 1, 1);
				} else if (dv < (45.0 / 90.0)) {
					cF = float3(1, 1, 0);
				} else if (dv < (60.0 / 90.0)) {
					cF = float3(1, 0, 1);
				} else if (dv < (70.0 / 90.0)) {
					cF = float3(1, 0, 0);
				} else {
					cF = float3(0, 0, 0);
				}
				
				float3 f = cF;

				if (s.a == 0) {
					f = float3(0, 0, 0);
				}
				
				// NOTE: Show non-coverage areas.
				// if (s.a == 0) {
				// 	return float4(1, 0, 0, 1);
				// } else {
				// 	return float4(fade, fade, fade, 1);
				// }

				return float4(f, 1);
				// return float4(f * fade, 1);
			}
			ENDCG
		}
	}
}
