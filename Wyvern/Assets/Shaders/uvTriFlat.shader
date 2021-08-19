Shader "Misc/uvTriFlat"
{
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

			struct v2f
			{
				float4 pos : SV_POSITION;
				float4 col : COLOR;
				float2 uv : TEXCOORD0;
			};

			v2f vert(appdata_base v) 
			{
				v2f o;
				o.pos = float4(v.texcoord.xy * 2 - 1, 1, 1);
				o.pos.y = -o.pos.y;
				o.col = float4(0, 0, 0, 1);
				o.uv = v.texcoord.xy;

				return o;
			}

			float4 frag(v2f i) : COLOR
			{
				return float4(0, 0, 0, 1);
				//return float4(i.uv.xy, 0, 1);
			}

			ENDCG
		}        
	}
}
