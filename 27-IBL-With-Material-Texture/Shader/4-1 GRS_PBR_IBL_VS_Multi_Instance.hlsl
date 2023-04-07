// Multi-Instance PBR IBL Vertex Shader
#include "0-3 GRS_Scene_CB_Def.hlsli"

struct ST_GRS_HLSL_PBR_VS_INPUT
{
	float4		m_v4LPos		: POSITION;
	float4		m_v4LNormal		: NORMAL;
	float2		m_v2UV			: TEXCOORD;
};

struct ST_GRS_HLSL_PBR_PS_INPUT
{
	float4		m_v4HPos		: SV_POSITION;
	float4		m_v4WPos		: POSITION;
	float4		m_v4WNormal		: NORMAL;
	float2		m_v2UV			: TEXCOORD;
};

SamplerState g_sapLinear		    : register(s0);
Texture2D    g_texDisplacement      : register(t2, space1); // λ����ͼ(�߶�ͼ)

ST_GRS_HLSL_PBR_PS_INPUT VSMain(ST_GRS_HLSL_PBR_VS_INPUT stVSInput)
{
	ST_GRS_HLSL_PBR_PS_INPUT stVSOutput;

	// ����λ����ͼֱ�Ӹı��������Σ����õ�������ʹ�� Tesselation
	float fDis = (g_texDisplacement.SampleLevel(g_sapLinear, stVSInput.m_v2UV, 0.0f).r * 0.1f);
	stVSInput.m_v4LPos += fDis * normalize(stVSInput.m_v4LNormal);

	// �ֲ�����ת��������ռ�Ͳü��ռ�
	stVSOutput.m_v4WPos = mul(stVSInput.m_v4LPos, g_mxModel2World);
	stVSOutput.m_v4WPos = mul(stVSOutput.m_v4WPos, g_mxWorld);
	stVSOutput.m_v4HPos = mul(stVSOutput.m_v4WPos, g_mxVP);

	// �����ȱ任����������ϵ�У��ٽ�����������任;
	stVSInput.m_v4LNormal.w = 0.0f;
	stVSOutput.m_v4WNormal = mul(stVSInput.m_v4LNormal, g_mxModel2World);
	stVSOutput.m_v4WNormal = normalize(mul(stVSOutput.m_v4WNormal, g_mxWorld));
	stVSOutput.m_v2UV = stVSInput.m_v2UV;

	return stVSOutput;
}