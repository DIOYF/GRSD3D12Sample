#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ���ų�����ʹ�õ�����
#include <windows.h>
#include <tchar.h>
#include <atlconv.h>
#include <atlcoll.h>  //for atl array
#include <strsafe.h>  //for StringCchxxxxx function

#define GRS_WND_CLASS_NAME _T("GRS Game Window Class")
#define GRS_WND_TITLE	_T("GRS DirectX12 Multi Thread Shadow Sample")

//����̨�������
#define GRS_INIT_OUTPUT() 	if (!::AllocConsole()){throw CGRSCOMException(HRESULT_FROM_WIN32(::GetLastError()));}\
		SetConsoleTitle(GRS_WND_TITLE);
#define GRS_FREE_OUTPUT()	::_tsystem(_T("PAUSE"));\
							::FreeConsole();

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};TCHAR pszOutput[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	StringCchPrintf(pszOutput,1024,_T("��ID:% 8u����%s"),::GetCurrentThreadId(),pBuf);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pszOutput,lstrlen(pszOutput),nullptr,nullptr);

class CGRSCOMException
{
public:
	CGRSCOMException(HRESULT hr) : m_hrError(hr)
	{
	}
	HRESULT Error() const
	{
		return m_hrError;
	}
private:
	const HRESULT m_hrError;
};

// ��Ⱦ���̲߳���
struct ST_GRS_THREAD_PARAMS
{
	UINT								m_nThreadIndex;				//���
	DWORD								m_dwThisThreadID;
	HANDLE								m_hThisThread;
	DWORD								m_dwMainThreadID;
	HANDLE								m_hMainThread;

	HANDLE								m_hRunEvent;

	UINT								m_nStatus;					//��ǰ��Ⱦ״̬

};

const UINT								g_nMaxThread = 6;
ST_GRS_THREAD_PARAMS					g_stThreadParams[g_nMaxThread] = {};
HANDLE									g_hEventShadowOver;
HANDLE									g_hEventRenderOver;
SYNCHRONIZATION_BARRIER					g_stCPUThdBarrier = {};

UINT __stdcall RenderThread(void* pParam);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR    lpCmdLine, int nCmdShow)
{
	int						iWndWidth = 1024;
	int						iWndHeight = 768;
	HWND					hWnd = nullptr;
	MSG						msg = {};

	CAtlArray<HANDLE>		arHWaited;
	CAtlArray<HANDLE>		arHSubThread;
	GRS_USEPRINTF();
	try
	{
		GRS_INIT_OUTPUT();
		// ��������
		{
			WNDCLASSEX wcex = {};
			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.style = CS_GLOBALCLASS;
			wcex.lpfnWndProc = WndProc;
			wcex.cbClsExtra = 0;
			wcex.cbWndExtra = 0;
			wcex.hInstance = hInstance;
			wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);		//��ֹ���ĵı����ػ�
			wcex.lpszClassName = GRS_WND_CLASS_NAME;
			RegisterClassEx(&wcex);

			DWORD dwWndStyle = WS_OVERLAPPED | WS_SYSMENU;
			RECT rtWnd = { 0, 0, iWndWidth, iWndHeight };
			AdjustWindowRect(&rtWnd, dwWndStyle, FALSE);

			// ���㴰�ھ��е���Ļ����
			INT posX = (GetSystemMetrics(SM_CXSCREEN) - rtWnd.right - rtWnd.left) / 2;
			INT posY = (GetSystemMetrics(SM_CYSCREEN) - rtWnd.bottom - rtWnd.top) / 2;

			hWnd = CreateWindowW(GRS_WND_CLASS_NAME, GRS_WND_TITLE, dwWndStyle
				, posX, posY, rtWnd.right - rtWnd.left, rtWnd.bottom - rtWnd.top
				, nullptr, nullptr, hInstance, nullptr);

			if (!hWnd)
			{
				throw CGRSCOMException(HRESULT_FROM_WIN32(GetLastError()));
			}
		}

		{
			g_hEventShadowOver = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
			g_hEventRenderOver = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

			InitializeSynchronizationBarrier(&g_stCPUThdBarrier, g_nMaxThread, -1);
		}

		// ���������Ⱦ�߳�
		{
			GRS_PRINTF(_T("���߳�������%d�������߳�\n"),g_nMaxThread);
			// ���ø��̵߳Ĺ��Բ���
			for (int i = 0; i < g_nMaxThread; i++)
			{
				g_stThreadParams[i].m_nThreadIndex = i;		//��¼���
				g_stThreadParams[i].m_hRunEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
				g_stThreadParams[i].m_nStatus = 0;

				//����ͣ��ʽ�����߳�
				g_stThreadParams[i].m_hThisThread = (HANDLE)_beginthreadex(nullptr,
					0, RenderThread, (void*)&g_stThreadParams[i],
					CREATE_SUSPENDED, (UINT*)&g_stThreadParams[i].m_dwThisThreadID);

				//Ȼ���ж��̴߳����Ƿ�ɹ�
				if (nullptr == g_stThreadParams[i].m_hThisThread
					|| reinterpret_cast<HANDLE>(-1) == g_stThreadParams[i].m_hThisThread)
				{
					throw CGRSCOMException(HRESULT_FROM_WIN32(GetLastError()));
				}

				arHSubThread.Add(g_stThreadParams[i].m_hThisThread);
			}

			//��һ�����߳�
			for (int i = 0; i < g_nMaxThread; i++)
			{
				::ResumeThread(g_stThreadParams[i].m_hThisThread);
			}
		}

		//����Ϣѭ��״̬����:0����ʾ��Ҫ���GPU�ϵĵڶ���Copy���� 1����ʾ֪ͨ���߳�ȥ��Ⱦ 2����ʾ���߳���Ⱦ�����¼���������ύִ����
		UINT nStates = 0; //��ʼ״̬Ϊ0
		DWORD dwRet = 0;
		DWORD dwWaitCnt = 0;
		UINT64 n64fence = 0;
		BOOL bExit = FALSE;

		// ����һ����ʱ������ģ��GPU��Ⱦʱ����ʱ
		HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
		LARGE_INTEGER liDueTime = {};
		liDueTime.QuadPart = -1i64;//1���ʼ��ʱ
		SetWaitableTimer(hTimer, &liDueTime, 10, NULL, NULL, 0);

		arHWaited.Add(hTimer);

		// ��ʼ����������ʾ����
		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);
		GRS_PRINTF(_T("���߳̿�ʼ��Ϣѭ��:\n"));
		//15����ʼ����Ϣѭ�����������в�����Ⱦ
		while (!bExit)
		{
			//���߳̽���ȴ�
			dwWaitCnt = static_cast<DWORD>(arHWaited.GetCount());
			dwRet = ::MsgWaitForMultipleObjects(dwWaitCnt, arHWaited.GetData(), FALSE, INFINITE, QS_ALLINPUT);
			dwRet -= WAIT_OBJECT_0;

			if (0 == dwRet)
			{
				switch (nStates)
				{
				case 0://״̬0����ʾ�ȵ������̼߳�����Դ��ϣ���ʱִ��һ�������б���ɸ����߳�Ҫ�����Դ�ϴ��ĵڶ���Copy����
				{
					nStates = 1;

					arHWaited.RemoveAll();

					// ģ��ȴ�GPUͬ��
					arHWaited.Add(hTimer);
					GRS_PRINTF(_T("���߳�״̬��0������ʼ��һ��ִ�������б��������Դ�ϴ��Դ�ĵڶ���Copy����\n"));
				}
				break;
				case 1:// ״̬1 �ȵ��������ִ�н�����Ҳ��CPU�ȵ�GPUִ�н���������ʼ��һ����Ⱦ
				{
					//OnUpdate()
					{
						GRS_PRINTF(_T("���߳�״̬��1����OnUpdate()\n"));
					}

					nStates = 2;
					arHWaited.RemoveAll();
					// ׼���ȴ���Ӱ��Ⱦ����
					arHWaited.Add(g_hEventShadowOver);

					GRS_PRINTF(_T("���߳�״̬��1����֪ͨ���߳̿�ʼ��һ����Ӱ��Ⱦ\n"));

					//֪ͨ���߳̿�ʼ��Ⱦ
					for (int i = 0; i < g_nMaxThread; i++)
					{
						//�趨��Ⱦ״̬Ϊ��Ⱦ��Ӱ
						g_stThreadParams[i].m_nStatus = 1;

						SetEvent(g_stThreadParams[i].m_hRunEvent);
					}
				}
				break;
				case 2:// ״̬2 ��ʾ���е���Ⱦ�����б���¼����ˣ���ʼ�����ִ�������б�
				{
					nStates = 3;

					arHWaited.RemoveAll();
					// ׼���ȴ�������Ⱦ����
					arHWaited.Add(g_hEventRenderOver);

					GRS_PRINTF(_T("���߳�״̬��2����֪ͨ���߳̿�ʼ�ڶ���������Ⱦ\n"));

					//֪ͨ���߳̿�ʼ��Ⱦ
					for (int i = 0; i < g_nMaxThread; i++)
					{
						//�趨��Ⱦ״̬Ϊ�ڶ���������Ⱦ
						g_stThreadParams[i].m_nStatus = 2;

						SetEvent(g_stThreadParams[i].m_hRunEvent);
					}
				}
				break;
				case 3:
				{
					nStates = 1;

					arHWaited.RemoveAll();
					// ģ��ȴ�GPUͬ��
					arHWaited.Add(hTimer);

					GRS_PRINTF(_T("���߳�״̬��3������˳��ִ�����е������б���׼���ȴ�GPU��Ⱦ���֪ͨ\n"));
				}
				break;
				default:// �����ܵ��������Ϊ�˱�������ı��뾯�����������������default
				{
					bExit = TRUE;
				}
				break;
				}
			}
			else if( 1 == dwRet )
			{
				GRS_PRINTF(_T("���߳�״̬��%d����������Ϣ\n"),nStates);
				//������Ϣ
				while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD))
				{
					if (WM_QUIT != msg.message)
					{
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
					else
					{
						bExit = TRUE;
					}
				}
			}
			else
			{


			}

			//---------------------------------------------------------------------------------------------
			//���һ���̵߳Ļ�����������߳��Ѿ��˳��ˣ����˳�ѭ��
			dwRet = WaitForMultipleObjects(static_cast<DWORD>(arHSubThread.GetCount()), arHSubThread.GetData(), FALSE, 0);
			dwRet -= WAIT_OBJECT_0;
			if (dwRet >= 0 && dwRet < g_nMaxThread)
			{
				bExit = TRUE;
			}
		}

	}
	catch (CGRSCOMException & e)
	{//������COM�쳣
		e;
	}

	try
	{
		GRS_PRINTF(_T("���߳�֪ͨ�����߳��˳�\n"));
		// ֪ͨ���߳��˳�
		for (int i = 0; i < g_nMaxThread; i++)
		{
			::PostThreadMessage(g_stThreadParams[i].m_dwThisThreadID, WM_QUIT, 0, 0);
		}

		// �ȴ��������߳��˳�
		DWORD dwRet = WaitForMultipleObjects(static_cast<DWORD>(arHSubThread.GetCount()), arHSubThread.GetData(), TRUE, INFINITE);

		// �����������߳���Դ
		for (int i = 0; i < g_nMaxThread; i++)
		{
			::CloseHandle(g_stThreadParams[i].m_hThisThread);
			::CloseHandle(g_stThreadParams[i].m_hRunEvent);
		}

		DeleteSynchronizationBarrier(&g_stCPUThdBarrier);
		GRS_PRINTF(_T("�����߳�ȫ���˳������߳�������Դ���˳�\n"));
	}
	catch (CGRSCOMException & e)
	{//������COM�쳣
		e;
	}
	::CoUninitialize();
	GRS_FREE_OUTPUT();
	return 0;
}

UINT __stdcall RenderThread(void* pParam)
{
	ST_GRS_THREAD_PARAMS* pThdPms = static_cast<ST_GRS_THREAD_PARAMS*>(pParam);
	DWORD dwRet = 0;
	BOOL  bQuit = FALSE;
	MSG   msg = {};
	TCHAR pszPreSpace[MAX_PATH] = {};
	GRS_USEPRINTF();
	try
	{
		if (nullptr == pThdPms)
		{//�����쳣�����쳣��ֹ�߳�
			throw CGRSCOMException(E_INVALIDARG);
		}

		for (UINT i = 0; i <= pThdPms->m_nThreadIndex; i++)
		{
			pszPreSpace[i] = _T(' ');
		}
		GRS_PRINTF(_T("%s���̡߳�%d����ʼ������Ⱦѭ��\n"), pszPreSpace,pThdPms->m_nThreadIndex);
		//6����Ⱦѭ��
		while (!bQuit)
		{
			// �ȴ����߳�֪ͨ��ʼ��Ⱦ��ͬʱ���������߳�Post��������Ϣ��Ŀǰ����Ϊ�˵ȴ�WM_QUIT��Ϣ
			dwRet = ::MsgWaitForMultipleObjects(1, &pThdPms->m_hRunEvent, FALSE, INFINITE, QS_ALLPOSTMESSAGE);
			switch (dwRet - WAIT_OBJECT_0)
			{
			case 0:
			{
				switch (pThdPms->m_nStatus)
				{
				case 1:
				{
					// OnUpdate()
					{
						GRS_PRINTF(_T("%s���̡߳�%d��״̬��%d����OnUpdate()\n"), pszPreSpace, pThdPms->m_nThreadIndex, pThdPms->m_nStatus);
					}

					// OnThreadRender()
					{
						GRS_PRINTF(_T("%s���̡߳�%d��״̬��%d����OnThreadRender()\n"), pszPreSpace, pThdPms->m_nThreadIndex, pThdPms->m_nStatus);
					}

					//�����Ⱦ�����ر������б�������ͬ������֪ͨ���߳̿�ʼִ�У�

					if (EnterSynchronizationBarrier(&g_stCPUThdBarrier, 0))
					{
						GRS_PRINTF(_T("%s���̡߳�%d��״̬��%d����֪ͨ���߳���ɵ�һ����Ӱ��Ⱦ\n"), pszPreSpace, pThdPms->m_nThreadIndex, pThdPms->m_nStatus);

						SetEvent(g_hEventShadowOver);
					}
				}
				break;
				case 2:
				{
					// OnUpdate()
					{
						GRS_PRINTF(_T("%s���̡߳�%d��״̬��%d����OnUpdate()\n"), pszPreSpace, pThdPms->m_nThreadIndex, pThdPms->m_nStatus);
					}

					// OnThreadRender()
					{
						GRS_PRINTF(_T("%s���̡߳�%d��״̬��%d����OnThreadRender()\n"), pszPreSpace, pThdPms->m_nThreadIndex, pThdPms->m_nStatus);
					}
					//�����Ⱦ�����ر������б�������ͬ������֪ͨ���߳̿�ʼִ�У�

					if (EnterSynchronizationBarrier(&g_stCPUThdBarrier, 0))
					{
						GRS_PRINTF(_T("%s���̡߳�%d��״̬��%d����֪ͨ���߳���ɵڶ���������Ⱦ\n"), pszPreSpace, pThdPms->m_nThreadIndex, pThdPms->m_nStatus);
						SetEvent(g_hEventRenderOver);
					}
				}
				break;
				default:
				{

				}
				break;
				}

			}
			break;
			case 1:
			{//������Ϣ
				GRS_PRINTF(_T("%s���̡߳�%d��״̬��%d����������Ϣ\n"), pszPreSpace, pThdPms->m_nThreadIndex, pThdPms->m_nStatus);
				while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{//����ֻ�����Ǳ���̷߳���������Ϣ�����ڸ����ӵĳ���
					if (WM_QUIT != msg.message)
					{
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
					else
					{
						bQuit = TRUE;
					}
				}
			}
			break;
			case WAIT_TIMEOUT:
				break;
			default:
				break;
			}
		}

		GRS_PRINTF(_T("%s���̡߳�%d���˳�\n"), pszPreSpace, pThdPms->m_nThreadIndex);
	}
	catch (CGRSCOMException&)
	{

	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
	{

	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
