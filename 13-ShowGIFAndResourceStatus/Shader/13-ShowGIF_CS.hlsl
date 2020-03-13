
cbuffer ST_GRS_GIF_FRAME_PARAM : register(b0)
{
	float4	m_c4BkColor;		
	uint2	m_nLeftTop;
	uint2	m_nFrameWH;
	uint	m_nFrame;
	uint	m_nDisposal;
};

// ֡�����ߴ���ǵ�ǰ֡����Ĵ�С��ע�����������������GIF�Ĵ�С
Texture2D			g_tGIFFrame : register(t0);

// ���մ�������GIF����������С��������GIF�Ĵ�С
RWTexture2D<float4> g_tPaint	: register(u0);

[numthreads(1, 1, 1)]
void ShowGIFCS( uint3 n3ThdID : SV_DispatchThreadID)
{
	if ( 0 == m_nFrame )
	{// ��һ֡ʱ�ñ���ɫ��������һ���ȣ��൱��һ��Clear����
		g_tPaint[n3ThdID.xy] = m_c4BkColor;
	}

	// ע�⻭�������������Ҫƫ�ƣ����������ǣ�m_nLeftTop.xy + n3ThdID.xy
	// m_nLeftTop.xy���ǵ�ǰ֡����������������Ͻǵ�ƫ��ֵ
	// n3ThdID.xy���ǵ�ǰ֡�ж�ӦҪ���Ƶ����ص�����

	if ( 0 == m_nDisposal )
	{//DM_NONE ��������
		g_tPaint[ m_nLeftTop.xy + n3ThdID.xy ] = g_tGIFFrame[n3ThdID.xy];
	}
	else if (1 == m_nDisposal)
	{//DM_UNDEFINED ֱ����ԭ����������Ͻ���Alpha��ɫ����
		// ע��g_tGIFFrame[n3ThdID.xy].wֻ�Ǽ򵥵�0��1ֵ������û��Ҫ����������Alpha��ɫ����
		// ������Ҳû��ʹ��IF Else�жϣ��������˿�����С��?:��Ԫ���ʽ�����м���
		g_tPaint[ m_nLeftTop.xy + n3ThdID.xy ] 
			= g_tGIFFrame[n3ThdID.xy].w ? g_tGIFFrame[n3ThdID.xy] : g_tPaint[m_nLeftTop.xy + n3ThdID.xy];
	}
	else if ( 2 == m_nDisposal )
	{// DM_BACKGROUND �ñ���ɫ��仭�壬Ȼ����ƣ��൱���ñ���ɫ����Alpha��ɫ������ͬ��
		g_tPaint[m_nLeftTop.xy + n3ThdID.xy] = g_tGIFFrame[n3ThdID.xy].w ? g_tGIFFrame[n3ThdID.xy] : m_c4BkColor;
	}

	else if( 3 == m_nDisposal )
	{// DM_PREVIOUS ����ǰһ֡
		g_tPaint[ m_nLeftTop.xy + n3ThdID.xy ] = g_tPaint[n3ThdID.xy + m_nLeftTop.xy];
	}
}