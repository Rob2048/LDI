Shader "Boxer/ModelDepth"
{
	Properties
	{
		
	}
	SubShader
	{
		Tags { "RenderType"="Opaque" }
		LOD 100

		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
			};

			struct v2f
			{
				float4 vertex : SV_POSITION;
				float3 viewPos : TEXCOORD0;
			};

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.viewPos = UnityObjectToViewPos(v.vertex);
				
				return o;
			}

			float frag (v2f i) : SV_Target
			{
				float depth = i.viewPos.z;

				// Depth offset from  camera = 20.
				// 0 to 1 = 0 to 20

				float cameraOffset = 20;
				float depthRange = 20;

				depth += cameraOffset;

				float layerDepth = 0.05f;

				// Quantize layers
				depth = trunc(depth / layerDepth + 1.0) * layerDepth;

				if (depth > 0) {
					depth /= depthRange;
					return depth;
				}

				return 0;
				//return float4(0, 0, 0, 1);
			}
			ENDCG
		}
	}
}
