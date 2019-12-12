
cbuffer ST_GRS_GIF_FRAME_PARAM : register(b0)
{
	uint m_nFrame;
	uint m_nDisposal;
	float4 m_c4BkColor;
};

Texture2D			g_tGIFFrame : register(t0);
RWTexture2D<float4> g_tPaint	: register(u0);

[numthreads(1, 1, 1)]
void ShowGIFCS( uint3 n3ThdID : SV_DispatchThreadID)
{
	if (0 == m_nFrame)
	{// ��һ֡ʱ�ñ���ɫ����һ����
		g_tPaint[n3ThdID.xy] = m_c4BkColor;
	}

	if ( 2 == m_nDisposal )
	{//DM_BACKGROUND �ñ���ɫ����������棬�൱��һ��Clear��������Ϊ�����б��е�ClearUAV����ֻ������һ��floatֵ�����Էŵ��������������
		g_tPaint[n3ThdID.xy] = m_c4BkColor;
	}
	else if (0 == m_nDisposal || 1 == m_nDisposal)
	{//DM_UNDEFINED or DM_NONE �����������ֱ����ԭ����������Ͻ���Alpha��ɫ����
		// Simple Alpha Blending
		g_tPaint[n3ThdID.xy] = g_tGIFFrame[n3ThdID.xy].w * g_tGIFFrame[n3ThdID.xy]
			+ (1.0f - g_tGIFFrame[n3ThdID.xy].w) * g_tPaint[n3ThdID.xy];
	}
}