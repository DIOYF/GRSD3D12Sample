#pragma once
#include "GRS_Win.h"

//����WM_CHAR��Ϣ����LPARAM������λ��ṹ��
struct ST_GRS_WMCHAR_LPARAM
{
	DWORD m_dwRepCnt : 16;	//�ظ�����
	DWORD m_dwScanCode : 8;	//����ɨ����,����OEM����
	DWORD m_dwExKey : 1;	//�Ƿ������ֵ�ALT+CTRL��
	DWORD m_dwReserved : 4;	//����,��ԶΪ0
	DWORD m_dwALTKey : 1;	//Ϊ1 ��ʾALT������
	DWORD m_dwPrevKeyState : 1;	//Ϊ1 ��ʾ�ӵ�WM_CHAR��Ϣʱ���ǰ��µ�,������ѵ���
	DWORD m_dwKeyReleased : 1;	//Ϊ1 ��ʾ�����ͷ�
};

class CGRSWndMsgProc
{
public:
	CGRSWndMsgProc()
	{

	}

	virtual ~CGRSWndMsgProc()
	{

	}

public:
	virtual LRESULT CALLBACK WndMsgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) = 0;
};