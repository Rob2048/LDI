Shader "Unlit/basicUnlit"
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
						return float4(i.color, 1) * _Color;
					}
				ENDCG
			}
		}
}
