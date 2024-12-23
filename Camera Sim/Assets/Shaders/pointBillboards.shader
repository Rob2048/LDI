Shader "Custom/pointBillboards"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
		_Scale ("Point scale", Float) = 0.5
	}
	SubShader
	{
		Tags { "Queue"="AlphaTest" }
		LOD 100

		Pass
		{
			
			ZWrite On
			ZTest LEqual

			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
				float4 color : COLOR;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				float4 vertex : SV_POSITION;
				float4 color : COLOR;
			};

			sampler2D _MainTex;
			float4 _MainTex_ST;
			// float4 _ScreenParams;
			float _Scale;

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex) + float4(v.uv.xy, 0, 0) * 10.0 / float4(_ScreenParams.x, _ScreenParams.y, 1, 1) * _Scale;
				o.color = v.color;
				
				// o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				return o;
			}

			fixed4 frag (v2f i) : SV_Target
			{
				// fixed4 col = tex2D(_MainTex, i.uv);
				return i.color;
			}
			ENDCG
		}
	}
}
