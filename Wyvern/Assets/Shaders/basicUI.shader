Shader "UI/basicUI" {

	SubShader {
		Tags {
			"Queue"="Transparent"
			"IgnoreProjector"="True"
			"RenderType"="Transparent"
			"PreviewType"="Plane"
			"CanUseSpriteAtlas"="True"
		}

		Cull Off
		Lighting Off
		ZWrite Off
		ZTest Always
		Blend SrcAlpha OneMinusSrcAlpha
		
		Pass {
			Name "Default"

			CGPROGRAM
				#pragma vertex vert
				#pragma fragment frag
				#pragma target 2.0

				#include "UnityCG.cginc"
				#include "UnityUI.cginc"

				struct appdata_t {
					float4 vertex   : POSITION;
					float4 color    : COLOR;
				};

				struct v2f {
					float4 vertex   : SV_POSITION;
					fixed4 color    : COLOR;
					float4 screenPixelPos : TEXCOORD1;
				};

				fixed4 _Color;
				float4 _ClipRect;
				float4 ScreenParams;
				float4 Transform;

				v2f vert(appdata_t v) {
					v2f OUT;
					OUT.color = v.color * _Color;

					float4 screenPixelPos = float4(v.vertex.xyz, 1);
					screenPixelPos.xy *= Transform.z;
					screenPixelPos.xy += Transform.xy;
					screenPixelPos.xy += _ClipRect.xy;

					OUT.screenPixelPos = screenPixelPos;

					// Pixel to NDC.
					screenPixelPos.xy /= ScreenParams.xy;
					screenPixelPos.y = 1.0 - screenPixelPos.y;
					screenPixelPos.xy *= 2.0;
					screenPixelPos.xy -= 1.0;
					OUT.vertex = screenPixelPos;

					return OUT;
				}

				fixed4 frag(v2f IN) : SV_Target {
					half4 color = IN.color;
					color.a *= UnityGet2DClipping(IN.screenPixelPos.xy, _ClipRect);
					return color;
				}
			ENDCG
		}
	}
}