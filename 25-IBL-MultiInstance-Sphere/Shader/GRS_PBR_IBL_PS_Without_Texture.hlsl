#include "GRS_Scene_CB_Def.hlsli"
#include "GRS_PBR_Function.hlsli"
#include "HDR_COLOR_CONV.hlsli"

SamplerState g_sapLinear		    : register(s0);
TextureCube  g_texSpecularCubemap   : register(t0);
TextureCube  g_texDiffuseCubemap    : register(t1);
Texture2D    g_texLut			    : register(t2);

struct ST_GRS_HLSL_PBR_PS_INPUT
{
	float4		m_v4HPos		: SV_POSITION;
	float4		m_v4WPos		: POSITION;
	float4		m_v4WNormal		: NORMAL;
	float2		m_v2UV			: TEXCOORD;
	float4x4	m_mxModel2World	: WORLD;
	float3		m_v3Albedo		: COLOR0;    // ������
	float		m_fMetallic		: COLOR1;    // ������
	float		m_fRoughness	: COLOR2;    // �ֲڶ�
	float		m_fAO			: COLOR3;    // �����ڵ�����
};

float4 PSMain(ST_GRS_HLSL_PBR_PS_INPUT stPSInput): SV_TARGET
{
    float3 N = stPSInput.m_v4WNormal.xyz;
    float3 V = normalize(g_v4EyePos.xyz - stPSInput.m_v4WPos.xyz);
    float3 R = reflect(-V, N);

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, stPSInput.m_v3Albedo, stPSInput.m_fMetallic);

    float3 Lo = float3(0.0f,0.0f,0.0f);

    // ���Ȱ���֮ǰ�����Ӽ���ֱ�ӹ��յ�Ч��
    for (int i = 0; i < GRS_LIGHT_COUNT; ++i)
    {
        // ������Դ������
        // ��Ϊ�Ƕ�ʵ����Ⱦ�����Զ�ÿ����Դ����λ�ñ任�������൱��ÿ��ʵ�������Լ��Ĺ�Դ
        // ע���ⲻ�Ǳ�׼������һ��Ҫע����ʽ�Ĵ����в�Ҫ������
        float4 v4LightWorldPos = mul(g_v4LightPos[i], stPSInput.m_mxModel2World);
        v4LightWorldPos = mul(v4LightWorldPos, g_mxWorld);

        float3 L = normalize(v4LightWorldPos.xyz - stPSInput.m_v4WPos.xyz);
        float3 H = normalize(V + L);
        float  distance = length(v4LightWorldPos.xyz - stPSInput.m_v4WPos.xyz);
        float  attenuation = 1.0 / (distance * distance); // ����ƽ��˥��
        float3 radiance = g_v4LightColors[i].rgb * attenuation;

        // Cook-Torrance BRDF
        float   NDF = DistributionGGX(N, H, stPSInput.m_fRoughness);
        float   G   = GeometrySmith_DirectLight(N, V, L, stPSInput.m_fRoughness);
        float3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 numerator    = NDF * G * F;
        float  denominator  = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        float3 specular     = numerator / denominator;

        float3 kS = F;

        float3 kD = float3(1.0f,1.0f,1.0f) - kS;

        kD *= 1.0 - stPSInput.m_fMetallic;

        float NdotL = max(dot(N, L), 0.0f);

        Lo += (kD * stPSInput.m_v3Albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ���ſ�ʼ����ǰ���Ԥ���ֽ������IBL���յ�Ч��
    // IBL�����价���ⲿ��
    float3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, stPSInput.m_fRoughness);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - stPSInput.m_fMetallic;

    float3 irradiance = g_texDiffuseCubemap.Sample(g_sapLinear, N).rgb;
    float3 diffuse = irradiance * stPSInput.m_v3Albedo;

    // IBL���淴�价���ⲿ��
    const float MAX_REFLECTION_LOD = 4.0;
    float3 prefilteredColor = g_texSpecularCubemap.SampleLevel(g_sapLinear, R, stPSInput.m_fRoughness * MAX_REFLECTION_LOD).rgb;
    float2 brdf = g_texLut.Sample(g_sapLinear, float2(max(dot(N, V), 0.0), stPSInput.m_fRoughness)).rg;
    float3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);

    // IBL ���պϳ�
    float3 ambient = (kD * diffuse + specular) * stPSInput.m_fAO;
    // ֱ�ӹ��� + IBL����
    float3 color = ambient + Lo;

    // Gamma
    //return float4(color, 1.0f);
    return float4(LinearToSRGB(color),1.0f);  
}