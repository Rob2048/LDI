Shader "Unlit/BasicWire" {

	Properties {
		_Color ("Main Color", Color) = (1,1,1,1)
	}

	SubShader {
		Tags { "RenderType"="Transparent" }
		LOD 100

		Blend SrcAlpha OneMinusSrcAlpha
		
		Pass {
			
			CGPROGRAM
				#pragma vertex vert
				#pragma fragment frag
				#pragma target 2.0
				#pragma multi_compile_fog

				#include "UnityCG.cginc"

				
				struct appdata_t {
					float4 vertex : POSITION;
					UNITY_VERTEX_INPUT_INSTANCE_ID
				};

				struct v2f {
					float4 vertex : SV_POSITION;
				};

				fixed4 _Color;

				v2f vert (appdata_t v)
				{
					v2f o;
					UNITY_SETUP_INSTANCE_ID(v);
					o.vertex = UnityObjectToClipPos(v.vertex);

					float frustumDepth = _ProjectionParams.z - _ProjectionParams.y;
					o.vertex.z += 0.02 / frustumDepth;
					return o;
				}

				fixed4 frag (v2f i) : COLOR
				{
					fixed4 col = _Color;
					return col;
				}
			ENDCG
		}
	}

}