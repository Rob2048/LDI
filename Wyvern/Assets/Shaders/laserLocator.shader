Shader "Misc/laserLocator"
{
	Properties
	{
		_Color ("Color", Color) = (1,1,1,1)
		_MainTex ("Albedo (RGB)", 2D) = "white" {}
		_Glossiness ("Smoothness", Range(0,1)) = 0.5
		_Metallic ("Metallic", Range(0,1)) = 0.0
	}
	SubShader
	{
		Tags { "RenderType"="Opaque" }
		LOD 200

		CGPROGRAM
		// Physically based Standard lighting model, and enable shadows on all light types
		#pragma surface surf Standard fullforwardshadows

		// Use shader model 3.0 target, to get nicer looking lighting
		#pragma target 3.0

		uniform float4 laserPos;
		sampler2D _MainTex;

		struct Input
		{
			float2 uv_MainTex;
			float3 worldPos;
			float3 worldNormal;
		};

		half _Glossiness;
		half _Metallic;
		fixed4 _Color;

		// Add instancing support for this shader. You need to check 'Enable Instancing' on materials that use the shader.
		// See https://docs.unity3d.com/Manual/GPUInstancing.html for more information about instancing.
		// #pragma instancing_options assumeuniformscaling
		UNITY_INSTANCING_BUFFER_START(Props)
			// put more per-instance properties here
		UNITY_INSTANCING_BUFFER_END(Props)

		// void vert (inout appdata_full v, out Input o) {
		// 	UNITY_INITIALIZE_OUTPUT(Input,o);
		// 	o.customColor = v.normal;
		// }

		void surf (Input IN, inout SurfaceOutputStandard o)
		{
			// Albedo comes from a texture tinted by color
			fixed4 c = tex2D (_MainTex, IN.uv_MainTex) * _Color;
			// o.Albedo = c.rgb;
			float dist = distance(laserPos, IN.worldPos);
			float3 f = _Color.rgb;

			if (dist < 15.05 && dist > 14.95) {
				o.Albedo = float3(1.0, 1.0, 1.0);
			} else {
				f = IN.worldNormal;

				float3 laserDir = normalize(laserPos - IN.worldPos);
				float i = dot(IN.worldNormal, laserDir);
				i = saturate((i - 0.5) * 2.0);
				// i = saturate(i);

				float3 cC = float3(1.0, 0.0, 0.0);
				float3 cA = float3(0.0, 1.0, 0.0);
				float3 cB = float3(0.0, 0.0, 1.0);
				float3 cF = lerp(cC, cA, i);

				if (i > 0.5) {
					cF = lerp(cB, cA, i * 2.0 - 1.0);
				} else {
					cF = lerp(cC, cB, i * 2.0);
				}
				
				// float3 f = cF;

				o.Albedo = cF;
			}

			// Metallic and smoothness come from slider variables
			o.Metallic = _Metallic;
			o.Smoothness = _Glossiness;
			o.Alpha = c.a;
		}
		ENDCG
	}
	FallBack "Diffuse"
}
