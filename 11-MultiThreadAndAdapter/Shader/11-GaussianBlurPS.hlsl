//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

//���Shader�ı���΢��ٷ�D3D12ʾ����ɾ���˲���Ҫ��VS�������Լ����µ�Ϊ�������ܶ�����ļ�ѭ��

struct PSInput
{
	float4 m_v4Position : SV_POSITION;
	float2 m_v2UV : TEXCOORD;
};

static const float KernelOffsets[3] = { 0.0f, 1.3846153846f, 3.2307692308f };
static const float BlurWeights[3] = { 0.2270270270f, 0.3162162162f, 0.0702702703f };

// The input texture to blur.
Texture2D g_Texture : register(t0);
SamplerState g_LinearSampler : register(s0);

// Simple gaussian blur in the vertical direction.
float4 PSSimpleBlurV(PSInput input) : SV_TARGET
{
	float3 textureColor = float3(1.0f, 0.0f, 0.0f);
	float2 m_v2UV = input.m_v2UV;
	float2 v2TexSize;
	//��ȡ�������سߴ�
	g_Texture.GetDimensions(v2TexSize.x, v2TexSize.y);

	textureColor = g_Texture.Sample(g_LinearSampler, m_v2UV).xyz * BlurWeights[0];
	for (int i = 1; i < 3; i++)
	{
		float2 normalizedOffset = float2(0.0f, KernelOffsets[i]) / v2TexSize.y;
		textureColor += g_Texture.Sample(g_LinearSampler, m_v2UV + normalizedOffset).xyz * BlurWeights[i];
		textureColor += g_Texture.Sample(g_LinearSampler, m_v2UV - normalizedOffset).xyz * BlurWeights[i];
	}
	return float4(textureColor, 1.0);
}
// Simple gaussian blur in the horizontal direction.
float4 PSSimpleBlurU(PSInput input) : SV_TARGET
{
	float3 textureColor = float3(1.0f, 0.0f, 0.0f);
	float2 m_v2UV = input.m_v2UV;
	float2 v2TexSize;
	//��ȡ�������سߴ�
	g_Texture.GetDimensions(v2TexSize.x, v2TexSize.y);

	textureColor = g_Texture.Sample(g_LinearSampler, m_v2UV).xyz * BlurWeights[0];
	for (int i = 1; i < 3; i++)
	{
		float2 normalizedOffset = float2(KernelOffsets[i], 0.0f) / v2TexSize.x;
		textureColor += g_Texture.Sample(g_LinearSampler, m_v2UV + normalizedOffset).xyz * BlurWeights[i];
		textureColor += g_Texture.Sample(g_LinearSampler, m_v2UV - normalizedOffset).xyz * BlurWeights[i];
	}
	return float4(textureColor, 1.0);
}


/////-----------------------------------------------------------------------------------
//void main()
//{
//	vec4 textureColor;
//	vec4 outColor;
//	vec4 U;
//	vec4 V;
//
//	if (PixFormat == 1000.0f)
//	{
//		//RGB32
//		textureColor = vec4(texture2D(textureUniformRGBA, textureCoordinate));
//		textureColor.a = textureColor.a * RGB_Alpha;
//	}
//	else
//	{
//		//YUV420  TO  RGB32
//		textureColor = vec4((texture2D(textureUniformY, textureCoordinate).r - 16. / 255.) * 1.164);
//		U = vec4(texture2D(textureUniformU, textureCoordinate).r - 128. / 255.);
//		V = vec4(texture2D(textureUniformV, textureCoordinate).r - 128. / 255.);
//		textureColor += V * vec4(1.596, -0.813, 0, 0);
//		textureColor += U * vec4(0, -0.392, 2.017, 0);
//		textureColor.a = RGB_Alpha;
//	}
//	if (1 == dark)
//	{
//		float brightness = 0.45f;
//		//ģ��
//		vec2 firstOffset = vec2(1.3846153846 * texelWidthOffset, 1.3846153846 * texelHeightOffset) * blurSize;
//		vec2 secondOffset = vec2(3.2307692308 * texelWidthOffset, 3.2307692308 * texelHeightOffset) * blurSize;
//
//		centerTextureCoordinate = textureCoordinate;
//		oneStepLeftTextureCoordinate = textureCoordinate - firstOffset;
//		twoStepsLeftTextureCoordinate = textureCoordinate - secondOffset;
//		oneStepRightTextureCoordinate = textureCoordinate + firstOffset;
//		twoStepsRightTextureCoordinate = textureCoordinate + secondOffset;
//
//		vec4 fragmentColor = texture2D(textureUniformRGBA, centerTextureCoordinate) * 0.2270270270;
//		fragmentColor += texture2D(textureUniformRGBA, oneStepLeftTextureCoordinate) * 0.3162162162;
//		fragmentColor += texture2D(textureUniformRGBA, oneStepRightTextureCoordinate) * 0.3162162162;
//		fragmentColor += texture2D(textureUniformRGBA, twoStepsLeftTextureCoordinate) * 0.0702702703; /
//		fragmentColor += texture2D(textureUniformRGBA, twoStepsRightTextureCoordinate) * 0.0702702703;
//
//		//�Ӱ�
//		textureColor = vec4((fragmentColor.rgb * vec3(brightness)), fragmentColor.a);
//		//�����
//		gl_FragColor = textureColor;
//
//	}
//}
///////////////////////////////////////////////////////////////////////////////////////////////////////////