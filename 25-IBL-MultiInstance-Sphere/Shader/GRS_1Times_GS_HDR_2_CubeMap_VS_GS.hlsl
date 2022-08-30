
#include "GRS_Scene_CB_Def.hlsli"

struct ST_GRS_HLSL_VS_IN
{
    float4 m_v4LPos		: POSITION;         // Model Local position
};

struct ST_GRS_HLSL_GS_IN
{
    float4 m_v4WPos		: SV_POSITION;      // World position
};

struct ST_GRS_HLSL_GS_OUT
{
    float4 m_v4HPos     : SV_POSITION;              // Projection coord
    float4 m_v4WPos		: POSITION;                 // World position
    uint RTIndex        : SV_RenderTargetArrayIndex;// Render Target Array Index
};

ST_GRS_HLSL_GS_IN VSMain(ST_GRS_HLSL_VS_IN stVSInput)
{
    ST_GRS_HLSL_GS_IN stVSOutput;

    // ��ģ�Ϳռ�ת������������ϵ
    stVSOutput.m_v4WPos = mul(stVSInput.m_v4LPos, g_mxWorld);

    return stVSOutput;
}

[maxvertexcount(18)]
void GSMain(triangle ST_GRS_HLSL_GS_IN stGSInput[3], inout TriangleStream<ST_GRS_HLSL_GS_OUT> CubeMapStream)
{
    for (int f = 0; f < GRS_CUBE_MAP_FACE_CNT; ++f)
    {
        
        ST_GRS_HLSL_GS_OUT stGSOutput;

        stGSOutput.RTIndex = f; //�趨����Ļ�������

        for (int v = 0; v < 3; v++)
        {
            stGSOutput.RTIndex = f;
            // ����ĳ˻��ǿ����Ż��ģ�������ǰ�� CPP �� ��6��View����ֱ�����Projection������ˣ��ٴ���Shader
            // ��Ȼ��Ϊ�����ǹ̶���Cube
            stGSOutput.m_v4WPos = stGSInput[v].m_v4WPos;
            stGSOutput.m_v4HPos = mul(stGSInput[v].m_v4WPos, g_mxGSCubeView[f]);
            stGSOutput.m_v4HPos = mul(stGSOutput.m_v4HPos, g_mxProj);
            CubeMapStream.Append(stGSOutput);
        }
        CubeMapStream.RestartStrip();
    }
}