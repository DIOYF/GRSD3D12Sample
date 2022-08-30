// Multi-Instance PBR IBL Vertex Shader
#include "GRS_Scene_CB_Def.hlsli"

struct ST_GRS_HLSL_PBR_VS_INPUT
{
	float4		m_v4LPos		: POSITION;
	float4		m_v4LNormal		: NORMAL;
	float2		m_v2UV			: TEXCOORD;
	float4x4	m_mxModel2World	: WORLD;
	float4		m_v4Albedo		: COLOR0;    // ������
	float		m_fMetallic		: COLOR1;    // ������
	float		m_fRoughness	: COLOR2;    // �ֲڶ�
	float		m_fAO			: COLOR3;	 // �����ڵ�����
	uint		m_nInstanceId	: SV_InstanceID;
};

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

ST_GRS_HLSL_PBR_PS_INPUT VSMain(ST_GRS_HLSL_PBR_VS_INPUT stVSInput)
{
	ST_GRS_HLSL_PBR_PS_INPUT stVSOutput;
	// �Ȱ�ÿ��ʵ��������任����������ϵ��
	stVSOutput.m_v4HPos = mul(stVSInput.m_v4LPos, stVSInput.m_mxModel2World);
	// �ٽ�����������ϵ����任,�õ�������������ϵ�е�����
	stVSOutput.m_v4WPos = mul(stVSOutput.m_v4HPos, g_mxWorld);
	// �任����-ͶӰ�ռ���
	stVSOutput.m_v4HPos = mul(stVSOutput.m_v4HPos, g_mxWVP);

	// �����ȱ任����������ϵ�У��ٽ�����������任
	stVSInput.m_v4LNormal.w = 0.0f;
	stVSOutput.m_v4WNormal = mul(stVSInput.m_v4LNormal, stVSInput.m_mxModel2World);
	stVSInput.m_v4LNormal.w = 0.0f;
	stVSOutput.m_v4WNormal = mul(stVSInput.m_v4LNormal, g_mxWorld);
	stVSInput.m_v4LNormal.w = 0.0f;
	stVSOutput.m_v4WNormal = normalize(stVSOutput.m_v4WNormal);

	stVSOutput.m_mxModel2World = stVSInput.m_mxModel2World;
	stVSOutput.m_v2UV = stVSInput.m_v2UV;
	stVSOutput.m_v3Albedo = stVSInput.m_v4Albedo.xyz;
	stVSOutput.m_fMetallic = stVSInput.m_fMetallic;
	stVSOutput.m_fRoughness = stVSInput.m_fRoughness;
	stVSOutput.m_fAO = stVSInput.m_fAO;
	return stVSOutput;
}