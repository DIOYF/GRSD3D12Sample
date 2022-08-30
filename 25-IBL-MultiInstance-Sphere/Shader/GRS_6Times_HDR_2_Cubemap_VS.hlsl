#include "GRS_Scene_CB_Def.hlsli"

struct ST_GRS_HLSL_VS_IN
{
    float4 m_v4LPos		: POSITION;         // Model Local position
};

struct ST_GRS_HLSL_VS_OUT
{
    float4 m_v4HPos : SV_POSITION;          // Projection coord
    float4 m_v4WPos : POSITION;             // World position
};

ST_GRS_HLSL_VS_OUT VSMain(ST_GRS_HLSL_VS_IN stVSInput)
{
    ST_GRS_HLSL_VS_OUT stVSOutput;

    stVSOutput.m_v4WPos = stVSInput.m_v4LPos;
    // ��ģ�Ϳռ�ת������������ϵ
    stVSOutput.m_v4HPos = mul(stVSInput.m_v4LPos, g_mxWVP);

    return stVSOutput;
}
