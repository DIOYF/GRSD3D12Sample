//===================================================================================
//��Ŀ���ƣ�GRS ������� V2.0
//�ļ�������Windows���ڻ�����װ�࣬��Ҫ������Ϸ�������ڽ���
//ģ�����ƣ�Windows����
//
//��֯����˾�����ţ����ƣ�One Man Group
//���ߣ�Gamebaby Rock Sun
//���ڣ�2017-11-21
//===================================================================================
#include "GRS_Win.h"
#include "GRSException.h"
#include "GRSWndUtil.h"
#include "GRSWndMsgProc.h"

#pragma once

#define GRS_DEFAULT_WND_STYLE		WS_OVERLAPPED|WS_SYSMENU
#define GRS_DEFAULT_WND_EXSTYLE		WS_EX_OVERLAPPEDWINDOW

class CGRSWnd
{
	friend class CGRSWndClass;
	typedef CAtlMap<HWND, CGRSWnd*> CMapHwndToWnd;
protected:
	static CMapHwndToWnd		ms_MapWnd;
protected:
	HWND						m_hWnd;
	CGRSWndMsgProc*				m_pMsgProc;
public:
	CGRSWnd( const CGRSWnd& ) = delete;
	CGRSWnd& operator = ( const CGRSWnd& ) = delete;
public:
	CGRSWnd()
		:m_hWnd(nullptr)
		,m_pMsgProc(nullptr)
	{
	}

	CGRSWnd(CGRSWndMsgProc* WndMsgProc
		, LPCTSTR pszClassName
		, LPCTSTR pszWndName
		, int nWidth
		, int nHeight
		, DWORD dwStyle = GRS_DEFAULT_WND_STYLE
		, DWORD dwExStyle = GRS_DEFAULT_WND_EXSTYLE)
		:m_hWnd(nullptr)
		,m_pMsgProc(WndMsgProc)
	{
		Create( nWidth, nHeight , pszClassName , pszWndName,  dwStyle, dwExStyle);
	}

	CGRSWnd(CGRSWndMsgProc* WndMsgProc
		, LPCTSTR pszClassName
		, LPCTSTR pszName
		, RECT& rt
		, DWORD dwStyle = GRS_DEFAULT_WND_STYLE
		, DWORD dwExStyle = GRS_DEFAULT_WND_EXSTYLE)
		:m_hWnd(nullptr)
		, m_pMsgProc(WndMsgProc)
	{
		Create(pszClassName,pszName, rt, dwStyle, dwExStyle);
	}

	CGRSWnd(CGRSWndMsgProc* WndMsgProc)
		:m_hWnd(nullptr)
		,m_pMsgProc(WndMsgProc)
	{

	}

	virtual ~CGRSWnd()
	{
		m_pMsgProc = nullptr;
	}
public:
	BOOL Create(int nWidth , int nHeight , LPCTSTR pszClassName	, LPCTSTR pszWndName, DWORD dwStyle, DWORD dwExStyle)
	{
		return Create(pszClassName, pszWndName, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, dwStyle, dwExStyle);
	}

	BOOL Create( LPCTSTR pszClassName, LPCTSTR pszName, RECT& rt, DWORD dwStyle, DWORD dwExStyle	 )
	{
		return Create( pszClassName,pszName, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, dwStyle, dwExStyle );
	}

	BOOL Create(LPCTSTR pszClassName, LPCTSTR pszName, int x, int y	, int nWidth, int nHeight, DWORD dwStyle, DWORD dwExStyle)
	{//��������
		BOOL bRet = TRUE;
		try
		{
			RECT rtWnd = { 0, 0, static_cast<LONG>(nWidth), static_cast<LONG>(nHeight) };
			AdjustWindowRect(&rtWnd, dwStyle, FALSE);

			m_hWnd = ::CreateWindowEx( dwExStyle
				, pszClassName
				, pszName
				, dwStyle
				, x
				, y
				, rtWnd.right-rtWnd.left
				, rtWnd.bottom-rtWnd.top
				, nullptr
				, nullptr
				, (HINSTANCE)GetModuleHandle(nullptr)
				, (LPVOID)this );

			if ( nullptr == m_hWnd )
			{
				GRS_THROW_LASTERROR();
			}
		}
		catch (CGRSException& e)
		{
			bRet = FALSE;
			GRS_TRACE_EXPW( CGRSWnd::Create, e );
		}
		return bRet;
	}

	BOOL Destroy()
	{
		BOOL bRet = TRUE;
		try
		{
			if ( nullptr != m_hWnd )
			{
				if ( DestroyWindow(m_hWnd) )
				{
					GRS_THROW_LASTERROR();
				}
			}
		}
		catch ( CGRSException& e )
		{
			bRet = FALSE;
			GRS_TRACE_EXPW(CGRSWnd::Destroy, e);
		}
		return bRet;
	}
public:
	operator HWND () const
	{//����ת�������,���㽫����Ϊ(HWND)��������
		return m_hWnd;
	}

	CGRSWndMsgProc* GetWndProc() const
	{
		return m_pMsgProc;
	}

	CGRSWndMsgProc* SetWndProc(CGRSWndMsgProc* pMsgProc)
	{
		CGRSWndMsgProc* pRetMsgProc = m_pMsgProc;
		m_pMsgProc = pMsgProc;
		return pRetMsgProc;
	}
protected:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{//���ڹ���;
		LRESULT lRet = 0;
		CGRSWnd* pWnd = nullptr;
		try
		{
			ms_MapWnd.Lookup(hwnd, pWnd);
			if ( WM_NCCREATE == uMsg )
			{
				LPCREATESTRUCT pCS = reinterpret_cast<LPCREATESTRUCT>(lParam);
				pWnd = reinterpret_cast<CGRSWnd*>(pCS->lpCreateParams);
				GRS_ASSERT( nullptr != pWnd );
				ms_MapWnd.SetAt(hwnd, pWnd);
				lRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
			else
			{
				if (nullptr != pWnd && nullptr != pWnd->m_pMsgProc)
				{
					lRet = pWnd->m_pMsgProc->WndMsgProc(hwnd, uMsg, wParam, lParam);

					if ( 0 > lRet )
					{
						lRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
					}
				}
				else
				{
					lRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
				}							
			}
		}
		catch (CGRSException& e)
		{
			GRS_TRACE_EXPW(CGRSWnd::WindowProc, e);
		}

		return lRet;
	}
};

