#include "0-3 GRS_Scene_CB_Def.hlsli"
#include "0-2 GRS_PBR_Function.hlsli"
#include "0-1 HDR_COLOR_CONV.hlsli"

SamplerState g_sapLinear		    : register(s0);

// IBL Lights
TextureCube  g_texSpecularCubemap   : register(t0, space0); // ���淴��
TextureCube  g_texDiffuseCubemap    : register(t1, space0); // ���������
Texture2D    g_texLut               : register(t2, space0); // BRDF LUT

// PBR Material
Texture2D    g_texAlbedo            : register(t0, space1); // ���������ʣ�Base Color or Albedo)
Texture2D    g_texNormal            : register(t1, space1); // ����
Texture2D    g_texDisplacement      : register(t2, space1); // λ����ͼ(�߶�ͼ)
Texture2D    g_texMetalness         : register(t3, space1); // ������
Texture2D    g_texRoughness         : register(t4, space1); // �ֲڶ�
Texture2D    g_texAO                : register(t5, space1); // AO

struct ST_GRS_HLSL_PBR_PS_INPUT
{
    // Per Vertex Data
    float4		m_v4HPos		: SV_POSITION;
    float4		m_v4WPos		: POSITION;
    float4		m_v4WNormal		: NORMAL;
    float2		m_v2UV			: TEXCOORD;
};

float3 CalcNewNormal(float2 v2UV, float3 v3Pos, float3 v3OldNormal/*ST_GRS_HLSL_VS_OUTPUT stPSInput*/)
{
    float fDisplacementScale = 0.1f;
    // ˮƽ����uv������
    float2 v2UVdx = ddx_fine(v2UV);
    // ��ֱ����uv������
    float2 v2UVdy = ddy_fine(v2UV);
    // ��ǰ�û�����
    float fHeight = g_texDisplacement.Sample(g_sapLinear, v2UV).r;
    // ˮƽ������һ�����ص���û�����
    float fHx = g_texDisplacement.Sample(g_sapLinear, v2UV + v2UVdx).r;
    // ��ֱ������һ�����ص���û�����
    float fHy = g_texDisplacement.Sample(g_sapLinear, v2UV + v2UVdy).r;
    // ˮƽ�����û�����
    float fHdx = (fHx - fHeight) * fDisplacementScale;
    // ��ֱ�����û�����
    float fHdy = (fHy - fHeight) * fDisplacementScale;

    // ˮƽ���򶥵�������������Ϊ����
    float3 v3Tangent = ddx_fine(v3Pos.xyz);
    // ��ֱ���򶥵�������������Ϊ������
    float3 v3BinTangent = ddy_fine(v3Pos.xyz);

    // ������Ϊֹ����ʵ�Ѿ������� v3Tangent ��� v3BinTangent ���õ� v3NewNormal ��
    // ���ǻᷢ�� v3NewNormal ����ƽ��������������ջ����Ӳ��
    // ����취��ʹ�ô� vertex shader �������� normal �����о����������ϵ� normal ��ƽ����

    // corss ���־�����ƽ���� normal �����¼����µ� v3Tangent
    // ����ӷ���ʹ���û���������������Ŷ�
    float3 v3NewTangent = cross(v3BinTangent, v3OldNormal) + v3OldNormal * fHdx;
    // ͬ��
    float3 v3NewBinTangent = cross(v3OldNormal, v3Tangent) + v3OldNormal * fHdy;
    // ���յõ�ƽ���� v3NewNormal
    float3 v3NewNormal = cross(v3NewTangent, v3NewBinTangent);
    v3NewNormal = normalize(v3NewNormal);

    // ����Ϳ���ʹ����� v3NewNormal ������ռ�����
    return v3NewNormal;
}

float4 PSMain(ST_GRS_HLSL_PBR_PS_INPUT stPSInput) : SV_TARGET
{
    float3 v3Albedo = g_texAlbedo.Sample(g_sapLinear, stPSInput.m_v2UV).rgb;
    float3 N = g_texNormal.Sample(g_sapLinear, stPSInput.m_v2UV).xyz;
    float  fMetallic = g_texMetalness.Sample(g_sapLinear, stPSInput.m_v2UV).r;
    float  fRoughness = g_texRoughness.Sample(g_sapLinear, stPSInput.m_v2UV).r;
    float  fAO = g_texAO.Sample(g_sapLinear, stPSInput.m_v2UV).r;

    // ������ͼ��ȡ�õķ��ߵı�׼����
    N = 2.0f * N - 1.0f;
    N = normalize(N);

    // �����пռ�
    float3 up = float3(0.0f, 1.0f, 0.0f);
    if (dot(up, stPSInput.m_v4WNormal.xyz) < 1e-6)
    {
        up = float3(1.0f, 0.0f, 0.0f);
    }
    float3 right = normalize(cross(up, stPSInput.m_v4WNormal.xyz));
    up = cross(stPSInput.m_v4WNormal.xyz, right);

    // ���¼��������ط���
    N = right * N.x + up * N.y + stPSInput.m_v4WNormal.xyz * N.z;

    //N = CalcNewNormal(stPSInput.m_v2UV, stPSInput.m_v4WPos.xyz, N);

    //return float4(N, 1.0f);
    //return float4(v3Albedo, 1.0f);
    //return float4(fMetallic, 0.0f, 0.0f, 1.0f);
    //return float4(fRoughness, 0.0f, 0.0f, 1.0f);
    //return float4(fAO, 0.0f, 0.0f, 1.0f);

    // ��ɫת�������Կռ䣨������ɫ�ռ䱾��������Կռ�ʱ������Ҫ���ת����
    //v3Albedo = SRGBToLinear(v3Albedo);

    float3 V = normalize(g_v4EyePos.xyz - stPSInput.m_v4WPos.xyz); // ������
    float3 R = reflect(-V, N);    // �������ľ��淴������

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, v3Albedo, fMetallic);

    float3 Lo = float3(0.0f,0.0f,0.0f);

    float NdotV = max(dot(N, V), 0.0f);

    // ���Ȱ���֮ǰ�����Ӽ���ֱ�ӹ��յ�Ч��
    for (int i = 0; i < GRS_LIGHT_COUNT; ++i)
    {
        // ������Դ������
        // ��Ϊ�Ƕ�ʵ����Ⱦ�����Զ�ÿ����Դ����λ�ñ任�������൱��ÿ��ʵ�������Լ��Ĺ�Դ
        // ע���ⲻ�Ǳ�׼������һ��Ҫע����ʽ�Ĵ����в�Ҫ������
        float4 v4LightWorldPos = mul(g_v4LightPos[i], g_mxWorld);

        float3 L = v4LightWorldPos.xyz - stPSInput.m_v4WPos.xyz;       // �������
        float  distance = length(L);
        float  attenuation = 1.0 / (distance * distance);
        float3 radiance = g_v4LightColors[i].rgb * attenuation; // ����ƽ������˥��

        L = normalize(L);

        float3 H = normalize(V + L);    // ������������������м�����

        float  NdotL = max(dot(N, L), 0.0f);

        // Cook-Torrance BRDF
        float   NDF = DistributionGGX(N, H, fRoughness);
        float   G = GeometrySmith_DirectLight(NdotV ,NdotL , fRoughness);
        float3  F = FresnelSchlick(NdotV , F0);

        float3 numerator = NDF * G * F;
        float  denominator = 4.0 * NdotV * NdotL + 0.0001; // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;

        float3 kS = F;
        float3 kD = float3(1.0f,1.0f,1.0f) - kS;
        kD *= 1.0 - fMetallic;

        Lo += (kD * v3Albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ���ſ�ʼ����ǰ���Ԥ���ֽ������IBL���յ�Ч��

    // ���ݴֲڶȲ�ֵ���������ϵ������ֱ�ӹ��յķ�����ϵ�����㷽����ͬ��
    float3 F = FresnelSchlickRoughness(NdotV, F0, fRoughness);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= (1.0 - fMetallic); // �򵥵��ǳ���Ҫ�ļ��㲽�裬����˴�������û���������

    // IBL�����价���ⲿ��
    float3 irradiance = g_texDiffuseCubemap.Sample(g_sapLinear, N).rgb;
    float3 diffuse = irradiance * v3Albedo;

    float fCubeWidth = 0.0f;
    float fCubeHeight = 0.0f;
    float fCubeMapLevels = 0.0f;
    // ��ȡ CubeMap �� Max Maplevels 
    g_texSpecularCubemap.GetDimensions(0, fCubeWidth, fCubeHeight, fCubeMapLevels);
    // IBL���淴�价���ⲿ��
    float3 prefilteredColor = g_texSpecularCubemap.SampleLevel(g_sapLinear, R, fRoughness * fCubeMapLevels).rgb;
    float2 brdf = g_texLut.Sample(g_sapLinear, float2(NdotV, fRoughness)).rg;
    float3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);

    // IBL ���պϳ�
    float3 ambient = (kD * diffuse + specular) * fAO;

    // ֱ�ӹ��� + IBL����
    float3 color = ambient + Lo;

    //return float4(color, 1.0f);

    // Gamma
    return float4(LinearToSRGB(color),1.0f);
}