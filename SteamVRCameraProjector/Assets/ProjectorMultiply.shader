// Upgrade NOTE: replaced '_Projector' with 'unity_Projector'
// Upgrade NOTE: replaced '_ProjectorClip' with 'unity_ProjectorClip'

Shader "Projector/Multiply" {
	Properties {
		_MainTex ("Base", 2D) = "gray" {}
		_DistortionTex("DistortionMap", 2D) = "white" {}
		//_Threshold("Thresold", Color) = (165 / 255.f, 117 / 255.f, 94 / 255.f, 1.f) // 165 / 255.f, 117 / 255.f, 94 / 255.f
		_SkinDetAlg("SkinMethod", int) = 0 //(0.64,0.45,0.36,1)
		_SkinApplyInvGamma("ApplyInvGamma", int) = 0 //(0.64,0.45,0.36,1)
		_SkinGammaExp("GammaExp", float) = 2.2 //(0.64,0.45,0.36,1)
		_SkinDetect("SkinDet", int) = 0
		//_FalloffTex ("FallOff", 2D) = "white" {}
	}
	Subshader {
		Tags {"Queue"="Transparent"}
		Pass {
			ZWrite Off
			ColorMask RGB
			//Blend Off
			Blend SrcAlpha OneMinusSrcAlpha //OneMinusSrcAlpha //OneMinusSrcAlpha
			//Blend DstColor Zero // Multiplicative
			Offset -1, -1

			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			#pragma multi_compile_fog
			#include "UnityCG.cginc"
			
			struct v2f {
				float4 uvShadow : TEXCOORD0;
				//float2 uvShadow : TEXCOORD0;
				float4 uvFalloff : TEXCOORD1;
				UNITY_FOG_COORDS(2)
				float4 pos : SV_POSITION;
			};
			
			sampler2D _MainTex;
			sampler2D _DistortionTex;

			float4 _MainTex_ST;
			//sampler2D _MainTex;
			float4x4 unity_Projector;
			float4x4 unity_ProjectorClip;
			
			v2f vert (float4 vertex : POSITION)
			{
				v2f o;
				o.pos = mul (UNITY_MATRIX_MVP, vertex);
				// o.uvShadow = TRANSFORM_TEX(UNITY_PROJ_COORD(mul(unity_Projector, vertex)), _MainTex);
				o.uvShadow = mul(unity_Projector, vertex);
				
				//o.uvShadow *= float4(_MainTex_ST.xy,1,1);
				//o.uvShadow += float4(_MainTex_ST.zw,0,0);

				//o.uvFalloff = mul (unity_ProjectorClip, vertex);
				//UNITY_TRANSFER_FOG(o,o.pos);
				return o;
			}
			
			
			//sampler2D _FalloffTex;
			int _SkinDetAlg;
			int _SkinApplyInvGamma;
			float _SkinGammaExp;
			int _SkinDetect;

			bool isSkin0(float4 texel);
			bool isSkin1(float4 texel);
			bool isSkin2(float4 texel);
			bool isSkin3(float4 texel);
			bool isSkin4(float4 texel);
			bool isSkin5(float4 texel);
			bool isSkin6(float4 texel);
			bool isSkin7(float4 texel);

			fixed4 frag (v2f i) : SV_Target
			{
				float4 texS;

				float2 texCoord = (i.uvShadow.xy / i.uvShadow.w);
				texCoord.y = 1.0f - texCoord.y;
				
				if (texCoord.x < 1 && texCoord.y < 1 && texCoord.x > 0 && texCoord.y > 0) //if(i.uvShadow.x < 1 && i.uvShadow.y < 1 && i.uvShadow.x > 0 && i.uvShadow.y > 0)
                  {
					texCoord = texCoord * _MainTex_ST.xy + _MainTex_ST.zw;
					// sample mainTex only
					//texS = tex2D(_MainTex, texCoord); //tex2D(_MainTex, UNITY_PROJ_COORD(i.uvShadow));
					//texS = tex2D(_MainTex, i.uvShadow);

					// sampling from (i,j) distortion map
					//float4 distSample = tex2D(_DistortionTex, texCoord);
					//texS = tex2D(_MainTex, float2(distSample.b, 1.f - distSample.g));
					
					// sampling from (r,g,b,a) distortion map  
					float4 distSample = tex2D(_DistortionTex, texCoord);
					texS = tex2D(_MainTex, float2(((distSample.b + distSample.g * 256.f) * 256.f) / 612.f,
						                          1.f - (distSample.r + distSample.a * 256.f) * 256.f / 460.f));
					if (_SkinDetect != 0) {
						bool isSkin = false;
						float4 testColor = (_SkinApplyInvGamma == 0) ? texS : pow(texS, 1 / _SkinGammaExp);


						switch (_SkinDetAlg) {
						case 0:
							if (isSkin0(testColor)) {
								isSkin = true;
							}
							break;
						case 1:
							if (isSkin1(testColor)) {
								isSkin = true;
							}
							break;
						case 2:
							if (isSkin2(testColor)) {
								isSkin = true;
							}
							break;
						case 3:
							if (isSkin3(testColor)) {
								isSkin = true;
							}
							break;
						case 4:
							if (isSkin4(testColor)) {
								isSkin = true;
							}
							break;
						case 5:
							if (isSkin5(testColor)) {
								isSkin = true;
							}
							break;
						case 6:
							if (isSkin6(testColor)) {
								isSkin = true;
							}
							break;
						case 7:
							if (isSkin7(testColor)) {
								isSkin = true;
							}
							break;
						default:
							break;
						}

						texS.a = (isSkin) ? 1.f : 0.f;

						if (isSkin) {
							float dotp = dot(texS.rgb, texS.rgb);
							// exclude darker colors
							if (dotp < 0.20f * 0.20f) {
								texS.a = 0.f;
								//return float4(0, 1, 0, 1);
							}

						}

					}
					else {
						// no skin detection
						texS.a = 1.f;
					}
					//if (isSkin0(pow(texS, 1 / 2.2))) {	
					
                  }else
                  {
                  	texS = float4(0,1,0,1);
                  }
				 //float4 upc = UNITY_PROJ_COORD(i.uvShadow);

				  //texS = float4(upc.x, upc.y+1, 0, 1);

				  
				  //texS = tex2D(_MainTex, texCoord); // UNITY_PROJ_COORD(i.uvShadow));
				  
				  //texS = tex2D(_MainTex, i.uvShadow);

//				texS.a = 1.0-texS.a;

//				fixed4 texF = tex2Dproj (_FalloffTex, UNITY_PROJ_COORD(i.uvFalloff));
//				fixed4 res = lerp(fixed4(1,1,1,0), texS, texF.a);

//				UNITY_APPLY_FOG_COLOR(i.fogCoord, res, fixed4(1,1,1,1));
//				return res;
				return texS;
			
			}

			bool isSkin0(float4 texel) {
				//float rgbSum = 1.f;// texel.r + texel.g + texel.b;
				float r = texel.r;
				float g = texel.g;
				float b = texel.b;
				float minDiff = max(max(r, g), b) - min(min(r, g), b);
				return (r > 95 / 255.f &&
					g > 40 / 255.f &&
					b > 20 / 255.f &&
					minDiff > 15 / 255.f &&
					abs(r - g) > 15 / 255.f &&
					r > g &&
					r > b);
			}

			bool isSkin1(float4 texel) {
				//float rgbSum = 1.f;//texel.r + texel.g + texel.b;
				float r = texel.r;
				float g = texel.g;
				float b = texel.b;
				return (r > 220 / 255.f &&
					g > 210 / 255.f &&
					b > 170 / 255.f &&
					abs(r - g) <= 15 / 255.f &&
					r > b &&
					g > b);
			}

			bool isSkin2(float4 texel) {
				float3 rgbNorm = texel.rgb / (texel.r + texel.g + texel.b);
				float rgbNormSum = rgbNorm.r + rgbNorm.g + rgbNorm.b;
				float exp1 = rgbNorm.r / rgbNorm.g;
				float exp2 = (rgbNorm.r * rgbNorm.b) / (rgbNormSum * rgbNormSum);
				float exp3 = (rgbNorm.r * rgbNorm.g) / (rgbNormSum * rgbNormSum);
				return (exp1 > 1.185f && exp2 > 0.107f && exp3 > 0.112f);
			}

			bool isSkin3(float4 texel) {
				float3 rgbNorm = texel.rgb / (texel.r + texel.g + texel.b);
				float rgbNormSum = rgbNorm.r + rgbNorm.g + rgbNorm.b;
				float exp1 = rgbNorm.g / rgbNorm.r;
				float exp2 = (rgbNorm.g - rgbNorm.b) / (rgbNormSum);
				float exp3 = (rgbNorm.g * rgbNorm.b) / (rgbNormSum * rgbNormSum);
				float exp4 = rgbNorm.b / rgbNorm.g;
				float exp5 = rgbNorm.g / (3 * rgbNormSum);
				return (exp1 <= 0.839f && exp2 <= 0.054f && 
					    exp3 > 0.067f && exp3 <= 0.098f && 
					    exp4 <= 1.048 && exp5 <= 0.108);
			}

			bool isSkin4(float4 texel) {
				float3 rgbNorm = texel.rgb / (texel.r + texel.g + texel.b);
				float rgbNormSum = rgbNorm.r + rgbNorm.g + rgbNorm.b;
				float exp1 = (rgbNorm.r + rgbNorm.g) / (rgbNormSum);
				float exp2 = (rgbNorm.g - rgbNorm.r) / (rgbNormSum);
				float exp3 = (rgbNorm.g * rgbNorm.b) / (rgbNormSum * rgbNormSum);
				float exp4 = rgbNorm.b / rgbNorm.g;
				float exp5 = rgbNorm.g / rgbNormSum;
				return (exp1 > 0.685f && exp2 <= -0.049f &&
					exp3 > 0.067f && exp4 <= 1.249f &&
					exp5 <= 0.324f);
			}

			bool isSkin5(float4 texel) {
				float3 rgbNorm = texel.rgb / (texel.r + texel.g + texel.b);
				float rgbNormSum = rgbNorm.r + rgbNorm.g + rgbNorm.b;
				float exp1 = rgbNorm.b / rgbNorm.g;
				float exp2 = 1.f / (3 * (rgbNorm.r / rgbNormSum));
				float exp3 = (1 / 3.f) - rgbNorm.b / rgbNormSum;
				float exp4 = rgbNorm.g / rgbNormSum * (1 / 3.f);
				return (exp1 < 1.249f && exp2 > 0.696f &&
					exp3 > 0.014f && exp4 < 0.108);
			}

			bool isSkin6(float4 texel) {
				float3 rgbNorm = texel.rgb / (texel.r + texel.g + texel.b);
				float rgbNormSum = rgbNorm.r + rgbNorm.g + rgbNorm.b;
				float exp1 = (3 * rgbNorm.b * rgbNorm.r * rgbNorm.r) / (rgbNormSum*rgbNormSum*rgbNormSum);
				float exp2 = rgbNormSum / (3 * rgbNorm.r) + (rgbNorm.r - rgbNorm.g) / rgbNormSum;
				float exp3 = (rgbNorm.r*rgbNorm.b + rgbNorm.g*rgbNorm.g) / (rgbNorm.g * rgbNorm.b);
				return (exp1 > 0.1276f && exp2 <= 0.9498f &&
					exp3 <= 2.7775f);
			}

			bool isSkin7(float4 texel) {
				float3 rgbNorm = texel.rgb / (texel.r + texel.g + texel.b);
				float rgbNormSum = rgbNorm.r + rgbNorm.g + rgbNorm.b;
				float exp1 = (1 / 3.f - rgbNorm.b / rgbNormSum) / (rgbNorm.r / rgbNorm.g);
				
				float exp2 = ((rgbNorm.r*rgbNorm.b) / (rgbNormSum*rgbNormSum)) * ((rgbNorm.r- rgbNorm.g) / rgbNormSum);
				
				float exp3 = (rgbNorm.g / rgbNorm.b) / ((rgbNorm.r - rgbNorm.g) / rgbNormSum);
				
				float exp4 = (1.f / 3.f) - (rgbNorm.r / rgbNormSum) * (rgbNormSum / (3 * rgbNorm.r));

				float exp5 = ((1.f / 3.f) - (rgbNorm.r / rgbNormSum)) / ((1.f / 3.f) - (rgbNorm.b / rgbNormSum));

				return (exp1 > 0.014f && exp2 > 0.0075f &&
					exp3 > 3.4857f && exp4 > -0.0976f && exp5 <= -1.1022);
			}

			ENDCG
		}
	}
}
