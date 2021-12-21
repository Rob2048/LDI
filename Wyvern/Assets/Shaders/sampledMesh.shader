Shader "Unlit/sampledMesh"
{
	Properties {
		_Color ("Main Color", Color) = (1,1,1,1)
	}
		SubShader {
			Tags { "RenderType"="Opaque" }
			LOD 100
			// ZTest Always

			Pass {

				CGPROGRAM
					#pragma vertex vert
					#pragma fragment frag
					
					#include "UnityCG.cginc"

					struct appdata {
						float4 vertex : POSITION;
						float3 color : COLOR;
					};

					struct v2f {
						float3 color : COLOR;
						float4 vertex : SV_POSITION;
					};

					float4 _Color;

					v2f vert (appdata v) {
						v2f o;
						o.vertex = UnityObjectToClipPos(v.vertex);
						o.color = v.color;
						return o;
					}

					fixed4 frag (v2f i) : SV_Target {
						return float4(0, 0, 0, 1);
						float3 col = i.color;
						// col = pow(col, 2.2);
						col = GammaToLinearSpace(col);
						return float4(col.rrr, 1);

						float lum = col.r * 0.2126f + col.g * 0.7152f + col.b * 0.0722f;
						lum = pow(lum, 2.2);

						return float4(lum, lum, lum, 1);
					}
				ENDCG
			}
		}
}
