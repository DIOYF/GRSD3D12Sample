#include "0-2 GRS_PBR_Function.hlsli"

TextureCube  g_texHDREnvCubemap : register(t0);
SamplerState g_sapLinear		: register(s0);

struct ST_GRS_HLSL_PS_INPUT
{
    float4 m_v4HPos : SV_POSITION;
    float3 m_v4WPos : POSITION;
};

float4 PSMain(ST_GRS_HLSL_PS_INPUT stPSInput) :SV_Target
{
    float3 N = normalize(stPSInput.m_v4WPos.xyz);

    float3 irradiance = float3(0.0f,0.0f,0.0f);

    // �����пռ�
    float3 Up = float3(0.0f, 1.0f, 0.0f);
    float fdot = dot(Up, N);
    if ((1.0 - fdot) > 1e-6)
    {
        Up = float3(1.0f, 0.0f, 0.0f);
    }

    float3 Right = normalize(cross(Up, N));
    Up = normalize(cross(N, Right));
    N = normalize(cross(Right, Up));

    float theta = 0.0f;
    float phi = 0.0f;
    float2 Xi;
    float3 L;

    const uint SAMPLE_NUM = 4096u;

    for (uint i = 0u; i < SAMPLE_NUM; i++)
    {
        // �������ȷֲ������
        Xi = Hammersley(i, SAMPLE_NUM);

        // ���� CDF ����������ָ���ֲ����������
        phi = TWO_PI * Xi.x;
        theta = asin(sqrt(Xi.y));

        // ������ת�����ѿ������꣨�пռ䣩
        L = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
        // �пռ�ת������������ռ�
        L = L.x * Right + L.y * Up + L.z * N;

        // �������ͳ��������ն�
        irradiance += g_texHDREnvCubemap.Sample(g_sapLinear, L).xyz;
    }
    // ȡƽ���õ����ս��
    irradiance *= 1.0 / float(SAMPLE_NUM);

    return float4(irradiance,1.0f);
}