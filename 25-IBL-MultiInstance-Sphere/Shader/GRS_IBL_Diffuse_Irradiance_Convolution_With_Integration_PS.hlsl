
#include "GRS_PBR_Function.hlsli"

TextureCube  g_texHDREnvCubemap : register(t0);
SamplerState g_sapLinear		: register(s0);

struct ST_GRS_HLSL_PS_INPUT
{
	float4 m_v4HPos : SV_POSITION;
	float3 m_v4WPos : POSITION;
};

float4 PSMain(ST_GRS_HLSL_PS_INPUT pin):SV_Target
{
	float3 N = normalize(pin.m_v4WPos.xyz);

	float3 irradiance = float3(0.0f, 0.0f, 0.0f);

	float3 up = float3(0.0f, 1.0f, 0.0f);
	//float3 right = normalize(cross(N, up));
	float3 right = normalize(cross(up, N));
	up = normalize(cross(N, right));
	//up = normalize(cross(right,N));
	N = normalize(cross(right,up));

	float nrSamples = 0.0f;

	float deltaPhi = (2.0f * PI) / 180.0f;
	float deltaTheta = (0.5f * PI) / 90.0f;

	// ������㣬ֱ�ӷ����Թ�ʽ
	for (float phi = 0.0f; phi < 2.0f * PI; phi += deltaPhi)
	{
		for (float theta = 0.0f; theta < 0.5f * PI; theta += deltaTheta)
		{
			// ������ת�����ѿ������꣨�пռ䣩
			float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

			// �пռ�ת������������ռ�
			float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
			//float3 sampleVec = right;
			irradiance += (g_texHDREnvCubemap.Sample(g_sapLinear, sampleVec).xyz * cos(theta) * sin(theta));

			nrSamples += 1.0f;
		}
	}

	irradiance = PI * irradiance * (1.0 / float(2 * nrSamples));
	return float4(irradiance, 1.0f);
}