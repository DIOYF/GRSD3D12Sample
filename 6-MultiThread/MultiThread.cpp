#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ���ų�����ʹ�õ�����
#include <windows.h>
#include <tchar.h>
#include <fstream>  //for ifstream
#include <wrl.h> //���WTL֧�� ����ʹ��COM
#include <atlcoll.h>  //for atl array
#include <atlconv.h>  //for T2A
#include <strsafe.h>  //for StringCchxxxxx function
#include <dxgi1_6.h>
#include <d3d12.h> //for d3d12
#include <d3dcompiler.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#include <DirectXMath.h>
#include "..\WindowsCommons\d3dx12.h"
#include "..\WindowsCommons\DDSTextureLoader12.h"
using namespace std;
using namespace Microsoft;
using namespace Microsoft::WRL;
using namespace DirectX;

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")


#define GRS_WND_CLASS_NAME _T("Game Window Class")
#define GRS_WND_TITLE	_T("DirectX12 MultiThread Sample")

#define GRS_THROW_IF_FAILED(hr) if (FAILED(hr)){ throw CGRSCOMException(hr); }

//�¶���ĺ�������ȡ������
#define GRS_UPPER_DIV(A,B) ((UINT)(((A)+((B)-1))/(B)))
//���������ϱ߽�����㷨 �ڴ�����г��� ���ס
#define GRS_UPPER(A,B) ((UINT)(((A)+((B)-1))&~(B - 1)))

//------------------------------------------------------------------------------------------------------------
// Ϊ�˵��Լ�����������������ͺ궨�壬Ϊÿ���ӿڶ����������ƣ�����鿴�������
#if defined(_DEBUG)
inline void GRS_SetD3D12DebugName(ID3D12Object* pObject, LPCWSTR name)
{
	pObject->SetName(name);
}

inline void GRS_SetD3D12DebugNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
	WCHAR _DebugName[MAX_PATH] = {};
	if ( SUCCEEDED( StringCchPrintfW(_DebugName, _countof(_DebugName), L"%s[%u]", name, index) ) )
	{
		pObject->SetName(_DebugName);
	}
}
#else

inline void GRS_SetD3D12DebugName(ID3D12Object*, LPCWSTR)
{
}
inline void GRS_SetD3D12DebugNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}

#endif

#define GRS_SET_D3D12_DEBUGNAME(x)						GRS_SetD3D12DebugName(x, L#x)
#define GRS_SET_D3D12_DEBUGNAME_INDEXED(x, n)			GRS_SetD3D12DebugNameIndexed(x[n], L#x, n)

#define GRS_SET_D3D12_DEBUGNAME_COMPTR(x)				GRS_SetD3D12DebugName(x.Get(), L#x)
#define GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(x, n)	GRS_SetD3D12DebugNameIndexed(x[n].Get(), L#x, n)


#if defined(_DEBUG)
inline void GRS_SetDXGIDebugName(IDXGIObject* pObject, LPCWSTR name)
{
	size_t szLen = 0;
	StringCchLengthW(name, 50, &szLen);
	pObject->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(szLen - 1), name);
}

inline void GRS_SetDXGIDebugNameIndexed(IDXGIObject* pObject, LPCWSTR name, UINT index)
{
	size_t szLen = 0;
	WCHAR _DebugName[MAX_PATH] = {};
	if (SUCCEEDED(StringCchPrintfW(_DebugName, _countof(_DebugName), L"%s[%u]", name, index)))
	{
		StringCchLengthW(_DebugName, _countof(_DebugName), &szLen);
		pObject->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(szLen), _DebugName);
	}
}
#else

inline void GRS_SetDXGIDebugName(ID3D12Object*, LPCWSTR)
{
}
inline void GRS_SetDXGIDebugNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}

#endif

#define GRS_SET_DXGI_DEBUGNAME(x)						GRS_SetDXGIDebugName(x, L#x)
#define GRS_SET_DXGI_DEBUGNAME_INDEXED(x, n)			GRS_SetDXGIDebugNameIndexed(x[n], L#x, n)

#define GRS_SET_DXGI_DEBUGNAME_COMPTR(x)				GRS_SetDXGIDebugName(x.Get(), L#x)
#define GRS_SET_DXGI_DEBUGNAME_INDEXED_COMPTR(x, n)		GRS_SetDXGIDebugNameIndexed(x[n].Get(), L#x, n)
//------------------------------------------------------------------------------------------------------------

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

// ����ṹ
struct ST_GRS_VERTEX
{
	XMFLOAT4 m_vPos;		//Position
	XMFLOAT2 m_vTex;		//Texcoord
	XMFLOAT3 m_vNor;		//Normal
};

// ����������
struct ST_GRS_MVP
{
	XMFLOAT4X4 m_MVP;			//�����Model-view-projection(MVP)����.
};

// ��Ⱦ���̲߳���
struct ST_GRS_THREAD_PARAMS
{
	DWORD								dwThisThreadID;
	HANDLE								hThisThread;
	DWORD								dwMainThreadID;
	HANDLE								hMainThread;
	HANDLE								hRunEvent;
	HANDLE								hEventRenderOver;
	UINT								nCurrentFrameIndex;
	XMFLOAT4							v4ModelPos;
	TCHAR								pszDDSFile[MAX_PATH];
	CHAR								pszMeshFile[MAX_PATH];
	ID3D12Device4*						pID3DDevice;
	ID3D12CommandAllocator*				pICmdAlloc;
	ID3D12GraphicsCommandList*			pICmdList;
	ID3D12RootSignature*				pIRS;
	ID3D12PipelineState*				pIPSO;
};

UINT __stdcall RenderThread(void* pParam);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL LoadMeshVertex(const CHAR*pszMeshFileName, UINT&nVertexCnt, ST_GRS_VERTEX*&ppVertex, UINT*&ppIndices);

int									g_iWndWidth = 1024;
int									g_iWndHeight = 768;

CD3DX12_VIEWPORT					g_stViewPort(0.0f, 0.0f, static_cast<float>(g_iWndWidth), static_cast<float>(g_iWndHeight));
CD3DX12_RECT						g_stScissorRect(0, 0, static_cast<LONG>(g_iWndWidth), static_cast<LONG>(g_iWndHeight));

//��ʼ��Ĭ���������λ��
XMFLOAT3							g_f3EyePos = XMFLOAT3(0.0f, 5.0f, -10.0f);  //�۾�λ��
XMFLOAT3							g_f3LockAt = XMFLOAT3(0.0f, 0.0f, 0.0f);    //�۾�������λ��
XMFLOAT3							g_f3HeapUp = XMFLOAT3(0.0f, 1.0f, 0.0f);    //ͷ�����Ϸ�λ��

float								g_fYaw = 0.0f;			// ����Z�����ת��.
float								g_fPitch = 0.0f;			// ��XZƽ�����ת��

double								g_fPalstance = 10.0f * XM_PI / 180.0f;	//������ת�Ľ��ٶȣ���λ������/��

XMFLOAT4X4							g_mxWorld = {}; //World Matrix
XMFLOAT4X4							g_mxVP = {};    //View Projection Matrix

// ȫ���̲߳���
const UINT							g_nMaxThread					= 3;
const UINT							g_nThdSphere					= 0;
const UINT							g_nThdCube						= 1;
const UINT							g_nThdPlane						= 2;
ST_GRS_THREAD_PARAMS				g_stThreadParams[g_nMaxThread] = {};

const UINT							g_nFrameBackBufCount = 3u;
UINT								g_nRTVDescriptorSize = 0U;
ComPtr<ID3D12Resource>				g_pIARenderTargets[g_nFrameBackBufCount];
ComPtr<ID3D12DescriptorHeap>		g_pIRTVHeap;
ComPtr<ID3D12DescriptorHeap>		g_pIDSVHeap;				//��Ȼ�����������

TCHAR								g_pszAppPath[MAX_PATH] = {};

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR    lpCmdLine, int nCmdShow)
{
	::CoInitialize(nullptr);  //for WIC & COM

	HWND								hWnd = nullptr;
	MSG									msg = {};

	UINT								nDXGIFactoryFlags = 0U;

	ComPtr<IDXGIFactory5>				pIDXGIFactory5;
	ComPtr<IDXGIAdapter1>				pIAdapter;
	ComPtr<ID3D12Device4>				pID3DDevice;
	ComPtr<ID3D12CommandQueue>			pIMainCmdQueue;
	ComPtr<IDXGISwapChain1>				pISwapChain1;
	ComPtr<IDXGISwapChain3>				pISwapChain3;
	
	ComPtr<ID3D12Resource>				pIDepthStencilBuffer;	//������建����
	UINT								nCurrentFrameIndex = 0;

	DXGI_FORMAT							emRTFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT							emDSFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ComPtr<ID3D12Fence>					pIFence;
	UINT64								n64FenceValue = 1ui64;
	HANDLE								hFenceEvent = nullptr;

	ComPtr<ID3D12CommandAllocator>		pICmdAllocPre;
	ComPtr<ID3D12GraphicsCommandList>	pICmdListPre;

	ComPtr<ID3D12CommandAllocator>		pICmdAllocPost;
	ComPtr<ID3D12GraphicsCommandList>	pICmdListPost;


	CAtlArray<HANDLE>					arHWaited;
	CAtlArray<HANDLE>					arHSubThread;

	ComPtr<ID3DBlob>					pIVSSphere;
	ComPtr<ID3DBlob>					pIPSSphere;
	ComPtr<ID3D12RootSignature>			pIRootSignature;
	ComPtr<ID3D12PipelineState>			pIPSOSphere;

	try
	{
		// �õ���ǰ�Ĺ���Ŀ¼����������ʹ�����·�������ʸ�����Դ�ļ�
		{
			UINT nBytes = GetCurrentDirectory(MAX_PATH, g_pszAppPath);
			if (MAX_PATH == nBytes)
			{
				GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
			}
		}

		//1����������
		{
			//---------------------------------------------------------------------------------------------
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
			RECT rtWnd = { 0, 0, g_iWndWidth, g_iWndHeight };
			AdjustWindowRect(&rtWnd, dwWndStyle, FALSE);

			hWnd = CreateWindowW(GRS_WND_CLASS_NAME, GRS_WND_TITLE, dwWndStyle
				, CW_USEDEFAULT, 0, rtWnd.right - rtWnd.left, rtWnd.bottom - rtWnd.top
				, nullptr, nullptr, hInstance, nullptr);

			if (!hWnd)
			{
				throw CGRSCOMException(HRESULT_FROM_WIN32(GetLastError()));
			}

			ShowWindow(hWnd, nCmdShow);
			UpdateWindow(hWnd);
		}

		//2������ʾ��ϵͳ�ĵ���֧��
		{
#if defined(_DEBUG)
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				// �򿪸��ӵĵ���֧��
				nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
#endif
		}

		//3������DXGI Factory����
		{
			GRS_THROW_IF_FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory5)));
			GRS_SET_DXGI_DEBUGNAME_COMPTR(pIDXGIFactory5);
			// �ر�ALT+ENTER���л�ȫ���Ĺ��ܣ���Ϊ����û��ʵ��OnSize���������ȹر�
			GRS_THROW_IF_FAILED(pIDXGIFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
		}

		//4��ö�������������豸
		{//ѡ��NUMA�ܹ��Ķ���������3D�豸����,��ʱ�Ȳ�֧�ּ����ˣ���Ȼ������޸���Щ��Ϊ
			D3D12_FEATURE_DATA_ARCHITECTURE stArchitecture = {};
			for (UINT nAdapterIndex = 0; DXGI_ERROR_NOT_FOUND != pIDXGIFactory5->EnumAdapters1(nAdapterIndex, &pIAdapter); ++ nAdapterIndex)
			{
				DXGI_ADAPTER_DESC1 stAdapterDesc = {};
				pIAdapter->GetDesc1(&stAdapterDesc);

				if (stAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{//������������������豸
					continue;
				}

				GRS_THROW_IF_FAILED(D3D12CreateDevice(pIAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pID3DDevice)));
				GRS_THROW_IF_FAILED(pID3DDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE
					, &stArchitecture, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE)));

				if (!stArchitecture.UMA)
				{
					break;
				}

				pID3DDevice.Reset();
			}

			//---------------------------------------------------------------------------------------------
			if (nullptr == pID3DDevice.Get())
			{// �����Ļ����Ͼ�Ȼû�ж��� �������˳����� 
				throw CGRSCOMException(E_FAIL);
			}

			GRS_SET_D3D12_DEBUGNAME_COMPTR(pID3DDevice);
		}

		//5������ֱ���������
		{
			D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
			stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&pIMainCmdQueue)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIMainCmdQueue);
		}

		//6������������
		{
			DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
			stSwapChainDesc.BufferCount			= g_nFrameBackBufCount;
			stSwapChainDesc.Width				= g_iWndWidth;
			stSwapChainDesc.Height				= g_iWndHeight;
			stSwapChainDesc.Format				= emRTFormat;
			stSwapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
			stSwapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
			stSwapChainDesc.SampleDesc.Count	= 1;

			GRS_THROW_IF_FAILED(pIDXGIFactory5->CreateSwapChainForHwnd(
				pIMainCmdQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
				hWnd,
				&stSwapChainDesc,
				nullptr,
				nullptr,
				&pISwapChain1
			));
			GRS_SET_DXGI_DEBUGNAME_COMPTR(pISwapChain1);

			//ע��˴�ʹ���˸߰汾��SwapChain�ӿڵĺ���
			GRS_THROW_IF_FAILED(pISwapChain1.As(&pISwapChain3));
			GRS_SET_DXGI_DEBUGNAME_COMPTR(pISwapChain3);

			// ��ȡ��ǰ��һ�������Ƶĺ󻺳����
			nCurrentFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

			//����RTV(��ȾĿ����ͼ)��������(����ѵĺ���Ӧ�����Ϊ������߹̶���СԪ�صĹ̶���С�Դ��)
			D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
			stRTVHeapDesc.NumDescriptors = g_nFrameBackBufCount;
			stRTVHeapDesc.Type			 = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			stRTVHeapDesc.Flags			 = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			GRS_THROW_IF_FAILED(pID3DDevice->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&g_pIRTVHeap)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(g_pIRTVHeap);

			//�õ�ÿ��������Ԫ�صĴ�С
			g_nRTVDescriptorSize = pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			//---------------------------------------------------------------------------------------------
			CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(g_pIRTVHeap->GetCPUDescriptorHandleForHeapStart());
			for (UINT i = 0; i < g_nFrameBackBufCount; i++)
			{//���ѭ����©����������ʵ�����Ǹ�����ı���
				GRS_THROW_IF_FAILED(pISwapChain3->GetBuffer(i, IID_PPV_ARGS(&g_pIARenderTargets[i])));
				GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(g_pIARenderTargets, i);
				pID3DDevice->CreateRenderTargetView(g_pIARenderTargets[i].Get(), nullptr, stRTVHandle);
				stRTVHandle.Offset(1, g_nRTVDescriptorSize);
			}
		}

		//7��������Ȼ��弰��Ȼ�����������
		{
			D3D12_CLEAR_VALUE stDepthOptimizedClearValue		= {};
			stDepthOptimizedClearValue.Format					= emDSFormat;
			stDepthOptimizedClearValue.DepthStencil.Depth		= 1.0f;
			stDepthOptimizedClearValue.DepthStencil.Stencil		= 0;

			//ʹ����ʽĬ�϶Ѵ���һ��������建������
			//��Ϊ��������Ȼ�������һֱ��ʹ�ã����õ����岻������ֱ��ʹ����ʽ�ѣ�ͼ����
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)
				, D3D12_HEAP_FLAG_NONE
				, &CD3DX12_RESOURCE_DESC::Tex2D(emDSFormat
					, g_iWndWidth
					, g_iWndHeight
					, 1
					, 0
					, 1
					, 0
					, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
				, D3D12_RESOURCE_STATE_DEPTH_WRITE
				, &stDepthOptimizedClearValue
				, IID_PPV_ARGS(&pIDepthStencilBuffer)
			));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIDepthStencilBuffer);

			D3D12_DEPTH_STENCIL_VIEW_DESC stDepthStencilDesc = {};
			stDepthStencilDesc.Format			= emDSFormat;
			stDepthStencilDesc.ViewDimension	= D3D12_DSV_DIMENSION_TEXTURE2D;
			stDepthStencilDesc.Flags			= D3D12_DSV_FLAG_NONE;

			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors	= 1;
			dsvHeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			GRS_THROW_IF_FAILED(pID3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_pIDSVHeap)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(g_pIDSVHeap);

			pID3DDevice->CreateDepthStencilView(pIDepthStencilBuffer.Get()
				, &stDepthStencilDesc
				, g_pIDSVHeap->GetCPUDescriptorHandleForHeapStart());
		}

		//8������ֱ�������б�
		{
			// Ԥ���������б�
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
				, IID_PPV_ARGS(&pICmdAllocPre)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pICmdAllocPre);
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
				, pICmdAllocPre.Get(), nullptr, IID_PPV_ARGS(&pICmdListPre)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pICmdListPre);

			//���������б�
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
				, IID_PPV_ARGS(&pICmdAllocPost)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pICmdAllocPost);
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
				, pICmdAllocPost.Get(), nullptr, IID_PPV_ARGS(&pICmdListPost)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pICmdListPost);
		}

		//9��������ǩ��
		{//��������У���������ʹ����ͬ�ĸ�ǩ������Ϊ��Ⱦ��������Ҫ�Ĳ�����һ����
			D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
			// ����Ƿ�֧��V1.1�汾�ĸ�ǩ��
			stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
			if (FAILED(pID3DDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
			{
				stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			CD3DX12_DESCRIPTOR_RANGE1 stDSPRanges[3];
			stDSPRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
			stDSPRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
			stDSPRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

			CD3DX12_ROOT_PARAMETER1 stRootParameters[3];
			stRootParameters[0].InitAsDescriptorTable(1, &stDSPRanges[0], D3D12_SHADER_VISIBILITY_ALL); //CBV������Shader�ɼ�
			stRootParameters[1].InitAsDescriptorTable(1, &stDSPRanges[1], D3D12_SHADER_VISIBILITY_PIXEL);//SRV��PS�ɼ�
			stRootParameters[2].InitAsDescriptorTable(1, &stDSPRanges[2], D3D12_SHADER_VISIBILITY_PIXEL);//SAMPLE��PS�ɼ�

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc;
			stRootSignatureDesc.Init_1_1(_countof(stRootParameters)
				, stRootParameters
				, 0
				, nullptr
				, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> pISignatureBlob;
			ComPtr<ID3DBlob> pIErrorBlob;
			GRS_THROW_IF_FAILED(D3DX12SerializeVersionedRootSignature(&stRootSignatureDesc
				, stFeatureData.HighestVersion
				, &pISignatureBlob
				, &pIErrorBlob));

			GRS_THROW_IF_FAILED(pID3DDevice->CreateRootSignature(0
				, pISignatureBlob->GetBufferPointer()
				, pISignatureBlob->GetBufferSize()
				, IID_PPV_ARGS(&pIRootSignature)));

			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIRootSignature);
		}

		//10������Shader������Ⱦ����״̬����
		{
#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif
			//����Ϊ�о�����ʽ	   
			compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

			TCHAR pszShaderFileName[MAX_PATH] = {};
			StringCchPrintf(pszShaderFileName, MAX_PATH, _T("%s\\Shader\\TextureCube.hlsl"), g_pszAppPath);
			
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "VSMain", "vs_5_0", compileFlags, 0, &pIVSSphere, nullptr));
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "PSMain", "ps_5_0", compileFlags, 0, &pIPSSphere, nullptr));

			// ���Ƕ������һ�����ߵĶ��壬��ĿǰShader�����ǲ�û��ʹ��
			D3D12_INPUT_ELEMENT_DESC stIALayoutSphere[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// ���� graphics pipeline state object (PSO)����
			D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
			stPSODesc.InputLayout = { stIALayoutSphere, _countof(stIALayoutSphere) };
			stPSODesc.pRootSignature = pIRootSignature.Get();
			stPSODesc.VS = CD3DX12_SHADER_BYTECODE(pIVSSphere.Get());
			stPSODesc.PS = CD3DX12_SHADER_BYTECODE(pIPSSphere.Get());
			stPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			stPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			stPSODesc.SampleMask = UINT_MAX;
			stPSODesc.SampleDesc.Count = 1;
			stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			stPSODesc.NumRenderTargets = 1;
			stPSODesc.RTVFormats[0] = emRTFormat;
			stPSODesc.DSVFormat = emDSFormat;
			stPSODesc.DepthStencilState.StencilEnable = FALSE;
			stPSODesc.DepthStencilState.DepthEnable = TRUE;			//����Ȼ���				
			stPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//������Ȼ���д�빦��
			stPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;     //��Ȳ��Ժ�������ֵΪ��ͨ����Ȳ��ԣ�����Сֵд�룩
			
			GRS_THROW_IF_FAILED(pID3DDevice->CreateGraphicsPipelineState(&stPSODesc
				, IID_PPV_ARGS(&pIPSOSphere)));

			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIPSOSphere);
		}

		//11��׼�����������������Ⱦ�߳�
		{
			USES_CONVERSION;
			// ������Բ���
			StringCchPrintf(g_stThreadParams[g_nThdSphere].pszDDSFile, MAX_PATH, _T("%s\\Mesh\\sphere.dds"), g_pszAppPath);
			StringCchPrintfA(g_stThreadParams[g_nThdSphere].pszMeshFile, MAX_PATH, "%s\\Mesh\\sphere.txt", T2A(g_pszAppPath));
			g_stThreadParams[g_nThdSphere].v4ModelPos = XMFLOAT4(2.0f, 2.0f, 0.0f, 1.0f);

			// ��������Բ���
			StringCchPrintf(g_stThreadParams[g_nThdCube].pszDDSFile, MAX_PATH, _T("%s\\Mesh\\Cube.dds"), g_pszAppPath);
			StringCchPrintfA(g_stThreadParams[g_nThdCube].pszMeshFile, MAX_PATH, "%s\\Mesh\\Cube.txt", T2A(g_pszAppPath));
			g_stThreadParams[g_nThdCube].v4ModelPos = XMFLOAT4(-2.0f, 2.0f, 0.0f, 1.0f);

			// ƽ����Բ���
			StringCchPrintf(g_stThreadParams[g_nThdPlane].pszDDSFile, MAX_PATH, _T("%s\\Mesh\\Plane.dds"), g_pszAppPath);
			StringCchPrintfA(g_stThreadParams[g_nThdPlane].pszMeshFile, MAX_PATH, "%s\\Mesh\\Plane.txt", T2A(g_pszAppPath));
			g_stThreadParams[g_nThdPlane].v4ModelPos = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

		
			// ����Ĺ��Բ�����Ҳ���Ǹ��̵߳Ĺ��Բ���
			for (int i = 0; i < g_nMaxThread; i++)
			{
				//����ÿ���߳���Ҫ�������б�͸����������
				GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
					, IID_PPV_ARGS(&g_stThreadParams[i].pICmdAlloc)));
				GRS_SetD3D12DebugNameIndexed(g_stThreadParams[i].pICmdAlloc, _T("pIThreadCmdAlloc"), i);
			
				GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
					, g_stThreadParams[i].pICmdAlloc, nullptr, IID_PPV_ARGS(&g_stThreadParams[i].pICmdList)));
				GRS_SetD3D12DebugNameIndexed(g_stThreadParams[i].pICmdList, _T("pIThreadCmdList"), i);

				g_stThreadParams[i].dwMainThreadID	= ::GetCurrentThreadId();
				g_stThreadParams[i].hMainThread		= ::GetCurrentThread();
				g_stThreadParams[i].hRunEvent		= ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
				g_stThreadParams[i].hEventRenderOver= ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
				g_stThreadParams[i].pID3DDevice		= pID3DDevice.Get();
				g_stThreadParams[i].pIRS			= pIRootSignature.Get();
				g_stThreadParams[i].pIPSO			= pIPSOSphere.Get();
				
				arHWaited.Add(g_stThreadParams[i].hEventRenderOver); //��ӵ����ȴ�������

				//����ͣ��ʽ�����߳�
				g_stThreadParams[i].hThisThread = (HANDLE)_beginthreadex(nullptr,
					0, RenderThread, (void*)&g_stThreadParams[i],
					CREATE_SUSPENDED, (UINT*)&g_stThreadParams[i].dwThisThreadID);

				//Ȼ���ж��̴߳����Ƿ�ɹ�
				if (nullptr == g_stThreadParams[i].hThisThread
					|| reinterpret_cast<HANDLE>(-1) == g_stThreadParams[i].hThisThread)
				{
					throw CGRSCOMException(HRESULT_FROM_WIN32(GetLastError()));
				}

				arHSubThread.Add(g_stThreadParams[i].hThisThread);
			}

			//��һ�����߳�
			for (int i = 0; i < g_nMaxThread; i++)
			{
				::ResumeThread(g_stThreadParams[i].hThisThread);
			}
		}

		//12������Χ������
		{
			GRS_THROW_IF_FAILED(pID3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pIFence)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIFence);
			
			//����һ��Eventͬ���������ڵȴ�Χ���¼�֪ͨ
			hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (hFenceEvent == nullptr)
			{
				GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
			}
		}
	
		DWORD dwRet = 0;
		CAtlArray<ID3D12CommandList*> arCmdList;
		UINT64 n64fence = 0;
		
		//13���ȴ����̵߳�һ������ͬ���ź� ����ʾҪִ�еڶ���COPY��������Դ�ϴ���Ĭ�϶�
		{
			dwRet = WaitForMultipleObjects(static_cast<DWORD>(arHWaited.GetCount()), arHWaited.GetData(), TRUE, INFINITE);
			dwRet -= WAIT_OBJECT_0;
			if (0 == dwRet)
			{
				arCmdList.RemoveAll();
				//ִ�������б�
				arCmdList.Add(g_stThreadParams[g_nThdSphere].pICmdList);
				arCmdList.Add(g_stThreadParams[g_nThdCube].pICmdList);
				arCmdList.Add(g_stThreadParams[g_nThdPlane].pICmdList);

				pIMainCmdQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

				//---------------------------------------------------------------------------------------------
				//��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
				n64fence = n64FenceValue;
				GRS_THROW_IF_FAILED(pIMainCmdQueue->Signal(pIFence.Get(), n64fence));
				n64FenceValue++;
				if (pIFence->GetCompletedValue() < n64fence)
				{
					GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(n64fence, hFenceEvent));
					WaitForSingleObject(hFenceEvent, INFINITE);
				}
			}
			else
			{
				GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
			}
		}

		//---------------------------------------------------------------------------------------------
		BOOL bExit = FALSE;
		HANDLE phWait = CreateWaitableTimer(NULL, FALSE, NULL);
		LARGE_INTEGER liDueTime = {};
		liDueTime.QuadPart = -1i64;//1���ʼ��ʱ
		SetWaitableTimer(phWait, &liDueTime, 1, NULL, NULL, 0);//40ms������

		ULONGLONG n64tmFrameStart = ::GetTickCount64();
		ULONGLONG n64tmCurrent = n64tmFrameStart;
		//������ת�Ƕ���Ҫ�ı���
		double dModelRotationYAngle = 0.0f;


		//14����ʼ��Ϣѭ�����������в�����Ⱦ
		while (!bExit)
		{//ע���������ǵ�������Ϣѭ�������ȴ�ʱ������Ϊ0��ͬʱ����ʱ�Ե���Ⱦ���ĳ���ÿ��ѭ������Ⱦ
		 //�ر�ע����εȴ���֮ǰ��ͬ
			//���߳̽���ȴ�
			dwRet = ::MsgWaitForMultipleObjects(1, &phWait, FALSE, 0, QS_ALLINPUT);
			switch (dwRet - WAIT_OBJECT_0)
			{
			case 0:
			{//��ʱ��ʱ�䵽
			}
			break;
			case WAIT_TIMEOUT:
			{
			}
			break;
			case 1:
			{//������Ϣ
				while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
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
			break;
			default:
				break;
			}
			
			//����ȫ�ֵ�Matrix
			{
				//����ʱ��Ļ������㶼���������߳���
				//��ʵ�����������н���ʱ��ֵҲ��Ϊһ��ÿ֡���µĲ��������̻߳�ȡ�����������߳�
				n64tmCurrent = ::GetTickCount();
				//������ת�ĽǶȣ���ת�Ƕ�(����) = ʱ��(��) * ���ٶ�(����/��)
				//�����������൱�ھ�����Ϸ��Ϣѭ���е�OnUpdate��������Ҫ��������
				dModelRotationYAngle += ((n64tmCurrent - n64tmFrameStart) / 1000.0f) * g_fPalstance;

				n64tmFrameStart = n64tmCurrent;

				//��ת�Ƕ���2PI���ڵı�����ȥ����������ֻ�������0���ȿ�ʼ��С��2PI�Ļ��ȼ���
				if (dModelRotationYAngle > XM_2PI)
				{
					dModelRotationYAngle = fmod(dModelRotationYAngle, XM_2PI);
				}

				//���� World ���� �����Ǹ���ת����
				XMStoreFloat4x4(&g_mxWorld, XMMatrixRotationY(static_cast<float>(dModelRotationYAngle)));
				//���� �Ӿ��� view * �ü����� projection
				XMStoreFloat4x4( &g_mxVP
					,XMMatrixMultiply(XMMatrixLookAtLH(XMLoadFloat3(&g_f3EyePos)
					, XMLoadFloat3(&g_f3LockAt)
					, XMLoadFloat3(&g_f3HeapUp))
					, XMMatrixPerspectiveFovLH(XM_PIDIV4
						, (FLOAT)g_iWndWidth / (FLOAT)g_iWndHeight, 0.1f, 1000.0f)) );
			}

			//��Ⱦǰ����
			{
				pICmdListPre->ResourceBarrier(1
					, &CD3DX12_RESOURCE_BARRIER::Transition(
						g_pIARenderTargets[nCurrentFrameIndex].Get()
						, D3D12_RESOURCE_STATE_PRESENT
						, D3D12_RESOURCE_STATE_RENDER_TARGET)
				);

				//ƫ��������ָ�뵽ָ��֡������ͼλ��
				CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(g_pIRTVHeap->GetCPUDescriptorHandleForHeapStart()
					, nCurrentFrameIndex, g_nRTVDescriptorSize);
				CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(g_pIDSVHeap->GetCPUDescriptorHandleForHeapStart());
				//������ȾĿ��
				pICmdListPre->OMSetRenderTargets(1, &stRTVHandle, FALSE, &dsvHandle);

				pICmdListPre->RSSetViewports(1, &g_stViewPort);
				pICmdListPre->RSSetScissorRects(1, &g_stScissorRect);
				const float clearColor[] = { 0.0f, 0.1f, 0.0f, 1.0f };
				pICmdListPre->ClearRenderTargetView(stRTVHandle, clearColor, 0, nullptr);
				pICmdListPre->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			}

			//֪ͨ���߳̿�ʼ��Ⱦ
			{
				for (int i = 0; i < g_nMaxThread; i++)
				{
					g_stThreadParams[i].nCurrentFrameIndex = nCurrentFrameIndex;
					SetEvent(g_stThreadParams[i].hRunEvent);
				}
			}

			//�ȴ����߳���Ⱦ�꣬��������Ⱦ����
			{
				dwRet = WaitForMultipleObjects(static_cast<DWORD>(arHWaited.GetCount()), arHWaited.GetData(), TRUE, INFINITE);
				dwRet -= WAIT_OBJECT_0;
				if (0 == dwRet)
				{
					pICmdListPost->ResourceBarrier(1
						, &CD3DX12_RESOURCE_BARRIER::Transition(
							g_pIARenderTargets[nCurrentFrameIndex].Get()
							, D3D12_RESOURCE_STATE_RENDER_TARGET
							, D3D12_RESOURCE_STATE_PRESENT));

					//�ر������б�����ȥִ����
					GRS_THROW_IF_FAILED(pICmdListPre->Close());
					GRS_THROW_IF_FAILED(pICmdListPost->Close());

					arCmdList.RemoveAll();
					//ִ�������б�ע�������б��Ŷ���ϵķ�ʽ��
					arCmdList.Add(pICmdListPre.Get());
					arCmdList.Add(g_stThreadParams[g_nThdSphere].pICmdList);
					arCmdList.Add(g_stThreadParams[g_nThdCube].pICmdList);
					arCmdList.Add(g_stThreadParams[g_nThdPlane].pICmdList);
					arCmdList.Add(pICmdListPost.Get());

					pIMainCmdQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

					//�ύ����
					GRS_THROW_IF_FAILED(pISwapChain3->Present(1, 0));

					//��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
					n64fence = n64FenceValue;
					GRS_THROW_IF_FAILED(pIMainCmdQueue->Signal(pIFence.Get(), n64fence));
					n64FenceValue++;
					if (pIFence->GetCompletedValue() < n64fence)
					{
						GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(n64fence, hFenceEvent));
						WaitForSingleObject(hFenceEvent, INFINITE);
					}

					nCurrentFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

					GRS_THROW_IF_FAILED(pICmdAllocPre->Reset());
					GRS_THROW_IF_FAILED(pICmdListPre->Reset(pICmdAllocPre.Get(), pIPSOSphere.Get()));

					GRS_THROW_IF_FAILED(pICmdAllocPost->Reset());
					GRS_THROW_IF_FAILED(pICmdListPost->Reset(pICmdAllocPost.Get(), pIPSOSphere.Get()));
					//==========================================================================================================

				}
				else
				{
					GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
				}

			}

			//---------------------------------------------------------------------------------------------
			//���һ���̵߳Ļ�����������߳��Ѿ��˳��ˣ����˳�ѭ��
			dwRet = WaitForMultipleObjects(static_cast<DWORD>(arHSubThread.GetCount()), arHSubThread.GetData(), FALSE, 0);
			dwRet -= WAIT_OBJECT_0;
			if ( dwRet >= 0 && dwRet < g_nMaxThread )
			{
				bExit = TRUE; 
			}
		}	
		
	}
	catch (CGRSCOMException& e)
	{//������COM�쳣
		e;
	}

	try
	{
		// ֪ͨ���߳��˳�
		for (int i = 0; i < g_nMaxThread; i++)
		{
			::PostThreadMessage(g_stThreadParams[i].dwThisThreadID, WM_QUIT, 0, 0);
		}

		// �ȴ��������߳��˳�
		DWORD dwRet = WaitForMultipleObjects(static_cast<DWORD>(arHSubThread.GetCount()), arHSubThread.GetData(), TRUE, INFINITE);

		// �����������߳���Դ
		for (int i = 0; i < g_nMaxThread; i++)
		{
			::CloseHandle(g_stThreadParams[i].hThisThread);
			::CloseHandle(g_stThreadParams[i].hEventRenderOver);
			g_stThreadParams[i].pICmdList->Release();
			g_stThreadParams[i].pICmdAlloc->Release();
		}

		//::CoUninitialize();
	}
	catch (CGRSCOMException& e)
	{//������COM�쳣
		e;
	}
	::CoUninitialize();
	return 0;
}

UINT __stdcall RenderThread(void* pParam)
{
	ST_GRS_THREAD_PARAMS* pThdPms = static_cast<ST_GRS_THREAD_PARAMS*>(pParam);
	try
	{
		if ( nullptr == pThdPms )
		{//�����쳣�����쳣��ֹ�߳�
			throw CGRSCOMException(E_INVALIDARG);
		}

		SIZE_T								szMVPBuf = GRS_UPPER(sizeof(ST_GRS_MVP), 256);

		ComPtr<ID3D12Resource>				pITexture;
		ComPtr<ID3D12Resource>				pITextureUpload;
		ComPtr<ID3D12Resource>				pIVB;
		ComPtr<ID3D12Resource>				pIIB;
		ComPtr<ID3D12Resource>			    pICBWVP;
		ComPtr<ID3D12DescriptorHeap>		pISRVCBVHp;
		ComPtr<ID3D12DescriptorHeap>		pISampleHp;

		ST_GRS_MVP*							pMVPBufModule = nullptr;
		D3D12_VERTEX_BUFFER_VIEW			stVBV = {};
		D3D12_INDEX_BUFFER_VIEW				stIBV = {};
		UINT								nIndexCnt = 0;
		XMMATRIX							mxPosModule = XMMatrixTranslationFromVector(XMLoadFloat4(&pThdPms->v4ModelPos));  //��ǰ��Ⱦ�����λ��
		// Mesh Value
		ST_GRS_VERTEX*						pstVertices = nullptr;
		UINT*								pnIndices = nullptr;
		UINT								nVertexCnt = 0;
		// DDS Value
		std::unique_ptr<uint8_t[]>			pbDDSData;
		std::vector<D3D12_SUBRESOURCE_DATA> stArSubResources;
		DDS_ALPHA_MODE						emAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
		bool								bIsCube = false;

		//1������DDS����
		{
			GRS_THROW_IF_FAILED(LoadDDSTextureFromFile(pThdPms->pID3DDevice, pThdPms->pszDDSFile, pITexture.GetAddressOf()
				, pbDDSData, stArSubResources, SIZE_MAX, &emAlphaMode, &bIsCube));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pITexture);

			UINT64 n64szUpSphere = GetRequiredIntermediateSize(pITexture.Get(), 0, static_cast<UINT>(stArSubResources.size()));
			D3D12_RESOURCE_DESC stTXDesc = pITexture->GetDesc();

			GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
				, D3D12_HEAP_FLAG_NONE
				, &CD3DX12_RESOURCE_DESC::Buffer(n64szUpSphere)
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pITextureUpload)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pITextureUpload);

			UpdateSubresources(pThdPms->pICmdList
				, pITexture.Get()
				, pITextureUpload.Get()
				, 0
				, 0
				, static_cast<UINT>(stArSubResources.size())
				, stArSubResources.data());

			//ͬ��
			pThdPms->pICmdList->ResourceBarrier(1
				, &CD3DX12_RESOURCE_BARRIER::Transition(pITexture.Get()
					, D3D12_RESOURCE_STATE_COPY_DEST
					, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			D3D12_DESCRIPTOR_HEAP_DESC stSRVCBVHPDesc = {};
			stSRVCBVHPDesc.NumDescriptors = 2; // 1 CBV + 1 SRV
			stSRVCBVHPDesc.Type			  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			stSRVCBVHPDesc.Flags		  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateDescriptorHeap(&stSRVCBVHPDesc, IID_PPV_ARGS(&pISRVCBVHp)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pISRVCBVHp);

			//����SRV
			D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
			stSRVDesc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			stSRVDesc.Format					= stTXDesc.Format;
			stSRVDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
			stSRVDesc.Texture2D.MipLevels		= 1;

			CD3DX12_CPU_DESCRIPTOR_HANDLE stCbvSrvHandle(pISRVCBVHp->GetCPUDescriptorHandleForHeapStart()
				, 1, pThdPms->pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			pThdPms->pID3DDevice->CreateShaderResourceView(pITexture.Get()
				, &stSRVDesc
				, stCbvSrvHandle);

		}
		
		//2������Sample
		{
			D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
			stSamplerHeapDesc.NumDescriptors = 1;
			stSamplerHeapDesc.Type			 = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			stSamplerHeapDesc.Flags			 = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateDescriptorHeap(&stSamplerHeapDesc, IID_PPV_ARGS(&pISampleHp)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pISampleHp);

			D3D12_SAMPLER_DESC stSamplerDesc = {};
			stSamplerDesc.Filter			= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			stSamplerDesc.MinLOD			= 0;
			stSamplerDesc.MaxLOD			= D3D12_FLOAT32_MAX;
			stSamplerDesc.MipLODBias		= 0.0f;
			stSamplerDesc.MaxAnisotropy		= 1;
			stSamplerDesc.ComparisonFunc	= D3D12_COMPARISON_FUNC_ALWAYS;
			stSamplerDesc.AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			stSamplerDesc.AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			stSamplerDesc.AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;

			pThdPms->pID3DDevice->CreateSampler(&stSamplerDesc, pISampleHp->GetCPUDescriptorHandleForHeapStart());
		}

		//3��������������
		{
			LoadMeshVertex(pThdPms->pszMeshFile, nVertexCnt, pstVertices, pnIndices);
			nIndexCnt = nVertexCnt;

			//���� Vertex Buffer ��ʹ��Upload��ʽ��
			GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
				, D3D12_HEAP_FLAG_NONE
				, &CD3DX12_RESOURCE_DESC::Buffer(nVertexCnt * sizeof(ST_GRS_VERTEX))
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pIVB)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIVB);

			//ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
			UINT8* pVertexDataBegin = nullptr;
			CD3DX12_RANGE stReadRange(0, 0);		// We do not intend to read from this resource on the CPU.

			GRS_THROW_IF_FAILED(pIVB->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, pstVertices, nVertexCnt * sizeof(ST_GRS_VERTEX));
			pIVB->Unmap(0, nullptr);

			//���� Index Buffer ��ʹ��Upload��ʽ��
			GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
				, D3D12_HEAP_FLAG_NONE
				, &CD3DX12_RESOURCE_DESC::Buffer(nIndexCnt * sizeof(UINT))
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pIIB)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIIB);

			UINT8* pIndexDataBegin = nullptr;
			GRS_THROW_IF_FAILED(pIIB->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
			memcpy(pIndexDataBegin, pnIndices, nIndexCnt * sizeof(UINT));
			pIIB->Unmap(0, nullptr);

			//����Vertex Buffer View
			stVBV.BufferLocation = pIVB->GetGPUVirtualAddress();
			stVBV.StrideInBytes	 = sizeof(ST_GRS_VERTEX);
			stVBV.SizeInBytes    = nVertexCnt * sizeof(ST_GRS_VERTEX);

			//����Index Buffer View
			stIBV.BufferLocation = pIIB->GetGPUVirtualAddress();
			stIBV.Format		 = DXGI_FORMAT_R32_UINT;
			stIBV.SizeInBytes    = nIndexCnt * sizeof(UINT);

			::HeapFree(::GetProcessHeap(), 0, pstVertices);
			::HeapFree(::GetProcessHeap(), 0, pnIndices);
		}

		//4�������������� 
		{
			GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
				, D3D12_HEAP_FLAG_NONE
				, &CD3DX12_RESOURCE_DESC::Buffer(szMVPBuf) //ע�⻺��ߴ�����Ϊ256�߽�����С
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pICBWVP)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pICBWVP);

			// Map ֮��Ͳ���Unmap�� ֱ�Ӹ������ݽ�ȥ ����ÿ֡������map-copy-unmap�˷�ʱ����
			GRS_THROW_IF_FAILED(pICBWVP->Map(0, nullptr, reinterpret_cast<void**>(&pMVPBufModule)));
			
			// ����CBV
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = pICBWVP->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes    = static_cast<UINT>(szMVPBuf);

			CD3DX12_CPU_DESCRIPTOR_HANDLE stCbvSrvHandle(pISRVCBVHp->GetCPUDescriptorHandleForHeapStart());
			pThdPms->pID3DDevice->CreateConstantBufferView(&cbvDesc, stCbvSrvHandle);
		}

		//5�������¼����� ֪ͨ���л����߳� �����Դ�ĵڶ���Copy����
		{
			GRS_THROW_IF_FAILED(pThdPms->pICmdList->Close());
			//��һ��֪ͨ���̱߳��̼߳�����Դ���
			::SetEvent(pThdPms->hEventRenderOver); // �����źţ�֪ͨ���̱߳��߳���Դ�������
		}
		
		DWORD dwRet = 0;
		BOOL  bQuit = FALSE;
		MSG   msg = {};

		//6����Ⱦѭ��
		while (!bQuit)
		{
			// �ȴ����߳�֪ͨ��ʼ��Ⱦ��ͬʱ���������߳�Post��������Ϣ��Ŀǰ����Ϊ�˵ȴ�WM_QUIT��Ϣ
			dwRet = ::MsgWaitForMultipleObjects(1, &pThdPms->hRunEvent, FALSE, INFINITE, QS_ALLPOSTMESSAGE);
			switch ( dwRet - WAIT_OBJECT_0 )
			{
			case 0:
			{
				//�����������Resetһ�£��ղ��Ѿ�ִ�й���һ���������������
				GRS_THROW_IF_FAILED(pThdPms->pICmdAlloc->Reset());
				//Reset�����б�������ָ�������������PSO����
				GRS_THROW_IF_FAILED(pThdPms->pICmdList->Reset(pThdPms->pICmdAlloc, pThdPms->pIPSO));

				// ׼��MWVP����
				{
					// Module * World
					XMMATRIX xmMWVP = XMMatrixMultiply( mxPosModule,XMLoadFloat4x4(&g_mxWorld) );

					// (Module * World) * View * Projection
					xmMWVP = XMMatrixMultiply(xmMWVP, XMLoadFloat4x4(&g_mxVP));

					XMStoreFloat4x4(&pMVPBufModule->m_MVP, xmMWVP);
				}
				
				//---------------------------------------------------------------------------------------------
				//���ö�Ӧ����ȾĿ����Ӳü���(������Ⱦ���̱߳���Ҫ���Ĳ��裬����Ҳ������ν���߳���Ⱦ�ĺ�������������)
				{
					CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(g_pIRTVHeap->GetCPUDescriptorHandleForHeapStart()
						, pThdPms->nCurrentFrameIndex
						, g_nRTVDescriptorSize);
					CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(g_pIDSVHeap->GetCPUDescriptorHandleForHeapStart());
					//������ȾĿ��
					pThdPms->pICmdList->OMSetRenderTargets(1, &stRTVHandle, FALSE, &dsvHandle);
					pThdPms->pICmdList->RSSetViewports(1, &g_stViewPort);
					pThdPms->pICmdList->RSSetScissorRects(1, &g_stScissorRect);
				}
				//---------------------------------------------------------------------------------------------

				//��Ⱦ��ʵ�ʾ��Ǽ�¼��Ⱦ�����б�
				{
					pThdPms->pICmdList->SetGraphicsRootSignature(pThdPms->pIRS);
					pThdPms->pICmdList->SetPipelineState(pThdPms->pIPSO);
					ID3D12DescriptorHeap* ppHeapsSphere[] = { pISRVCBVHp.Get(),pISampleHp.Get() };
					pThdPms->pICmdList->SetDescriptorHeaps(_countof(ppHeapsSphere), ppHeapsSphere);

					CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleSphere(pISRVCBVHp->GetGPUDescriptorHandleForHeapStart());
					//����CBV
					pThdPms->pICmdList->SetGraphicsRootDescriptorTable(0, stGPUCBVHandleSphere);

					stGPUCBVHandleSphere.Offset(1, pThdPms->pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
					//����SRV
					pThdPms->pICmdList->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleSphere);

					//����Sample
					pThdPms->pICmdList->SetGraphicsRootDescriptorTable(2, pISampleHp->GetGPUDescriptorHandleForHeapStart());

					//ע������ʹ�õ���Ⱦ�ַ����������б�Ҳ����ͨ����Mesh����
					pThdPms->pICmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					pThdPms->pICmdList->IASetVertexBuffers(0, 1, &stVBV);
					pThdPms->pICmdList->IASetIndexBuffer(&stIBV);

					//Draw Call������
					pThdPms->pICmdList->DrawIndexedInstanced(nIndexCnt, 1, 0, 0, 0);
				}

				//�����Ⱦ�����ر������б�������ͬ������֪ͨ���߳̿�ʼִ�У�
				{
					GRS_THROW_IF_FAILED(pThdPms->pICmdList->Close());

					::SetEvent(pThdPms->hEventRenderOver); // �����źţ�֪ͨ���̱߳��߳���Ⱦ���

				}
			}
			break;
			case 1:
			{//������Ϣ
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
		USHORT n16KeyCode = (wParam & 0xFF);
		if (VK_SPACE == n16KeyCode)
		{//���ո���л���ͬ�Ĳ�������Ч����������ÿ�ֲ���������ĺ���
			//UINT g_nCurrentSamplerNO = 0; //��ǰʹ�õĲ���������
			//UINT g_nSampleMaxCnt = 5;		//����������͵Ĳ�����
			//++g_nCurrentSamplerNO;
			//g_nCurrentSamplerNO %= g_nSampleMaxCnt;

			//=================================================================================================
			//������������������
			//pICmdListSphere->Reset(pICmdAllocSphere.Get(), pIPSOSphere.Get());
			//pICmdListSphere->SetGraphicsRootSignature(pIRootSignature.Get());
			//pICmdListSphere->SetPipelineState(pIPSOSphere.Get());
			//ID3D12DescriptorHeap* ppHeapsSphere[] = { pISRVCBVHp.Get(),pISampleHp.Get() };
			//pICmdListSphere->SetDescriptorHeaps(_countof(ppHeapsSphere), ppHeapsSphere);
			////����SRV
			//pICmdListSphere->SetGraphicsRootDescriptorTable(0, pISRVCBVHp->GetGPUDescriptorHandleForHeapStart());

			//CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleSphere(pISRVCBVHp->GetGPUDescriptorHandleForHeapStart()
			//	, 1
			//	, pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			////����CBV
			//pICmdListSphere->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleSphere);
			//CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUSamplerSphere(pISampleHp->GetGPUDescriptorHandleForHeapStart()
			//	, g_nCurrentSamplerNO
			//	, nSamplerDescriptorSize);
			////����Sample
			//pICmdListSphere->SetGraphicsRootDescriptorTable(2, hGPUSamplerSphere);
			////ע������ʹ�õ���Ⱦ�ַ����������б�Ҳ����ͨ����Mesh����
			//pICmdListSphere->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//pICmdListSphere->IASetVertexBuffers(0, 1, &stVBV);
			//pICmdListSphere->IASetIndexBuffer(&stIBV);

			////Draw Call������
			//pICmdListSphere->DrawIndexedInstanced(nIndexCnt, 1, 0, 0, 0);
			//pICmdListSphere->Close();
			//=================================================================================================
		}
		if (VK_ADD == n16KeyCode || VK_OEM_PLUS == n16KeyCode)
		{
			//double g_fPalstance = 10.0f * XM_PI / 180.0f;	//������ת�Ľ��ٶȣ���λ������/��
			g_fPalstance += 10 * XM_PI / 180.0f;
			if (g_fPalstance > XM_PI)
			{
				g_fPalstance = XM_PI;
			}
			//XMMatrixOrthographicOffCenterLH()
		}

		if (VK_SUBTRACT == n16KeyCode || VK_OEM_MINUS == n16KeyCode)
		{
			g_fPalstance -= 10 * XM_PI / 180.0f;
			if (g_fPalstance < 0.0f)
			{
				g_fPalstance = XM_PI / 180.0f;
			}
		}

		//�����û�����任
		//XMVECTOR g_f3EyePos = XMVectorSet(0.0f, 5.0f, -10.0f, 0.0f); //�۾�λ��
		//XMVECTOR g_f3LockAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  //�۾�������λ��
		//XMVECTOR g_f3HeapUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);  //ͷ�����Ϸ�λ��
		XMFLOAT3 move(0, 0, 0);
		float fMoveSpeed = 2.0f;
		float fTurnSpeed = XM_PIDIV2 * 0.005f;

		if ('w' == n16KeyCode || 'W' == n16KeyCode)
		{
			move.z -= 1.0f;
		}

		if ('s' == n16KeyCode || 'S' == n16KeyCode)
		{
			move.z += 1.0f;
		}

		if ('d' == n16KeyCode || 'D' == n16KeyCode)
		{
			move.x += 1.0f;
		}

		if ('a' == n16KeyCode || 'A' == n16KeyCode)
		{
			move.x -= 1.0f;
		}

		if (fabs(move.x) > 0.1f && fabs(move.z) > 0.1f)
		{
			XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
			move.x = XMVectorGetX(vector);
			move.z = XMVectorGetZ(vector);
		}

		if (VK_UP == n16KeyCode)
		{
			g_fPitch += fTurnSpeed;
		}

		if (VK_DOWN == n16KeyCode)
		{
			g_fPitch -= fTurnSpeed;
		}

		if (VK_RIGHT == n16KeyCode)
		{
			g_fYaw -= fTurnSpeed;
		}

		if (VK_LEFT == n16KeyCode)
		{
			g_fYaw += fTurnSpeed;
		}

		// Prevent looking too far up or down.
		g_fPitch = min(g_fPitch, XM_PIDIV4);
		g_fPitch = max(-XM_PIDIV4, g_fPitch);

		// Move the camera in model space.
		float x = move.x * -cosf(g_fYaw) - move.z * sinf(g_fYaw);
		float z = move.x * sinf(g_fYaw) - move.z * cosf(g_fYaw);
		g_f3EyePos.x += x * fMoveSpeed;
		g_f3EyePos.z += z * fMoveSpeed;

		// Determine the look direction.
		float r = cosf(g_fPitch);
		g_f3LockAt.x = r * sinf(g_fYaw);
		g_f3LockAt.y = sinf(g_fPitch);
		g_f3LockAt.z = r * cosf(g_fYaw);

		if (VK_TAB == n16KeyCode)
		{//��Tab����ԭ�����λ��
			g_f3EyePos = XMFLOAT3(0.0f, 0.0f, -10.0f); //�۾�λ��
			g_f3LockAt = XMFLOAT3(0.0f, 0.0f, 0.0f);    //�۾�������λ��
			g_f3HeapUp = XMFLOAT3(0.0f, 1.0f, 0.0f);    //ͷ�����Ϸ�λ��
		}

	}

	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

BOOL LoadMeshVertex(const CHAR*pszMeshFileName, UINT&nVertexCnt, ST_GRS_VERTEX*&ppVertex, UINT*&ppIndices)
{
	ifstream fin;
	char input;
	BOOL bRet = TRUE;
	try
	{
		fin.open(pszMeshFileName);
		if (fin.fail())
		{
			throw CGRSCOMException(E_FAIL);
		}
		fin.get(input);
		while (input != ':')
		{
			fin.get(input);
		}
		fin >> nVertexCnt;

		fin.get(input);
		while (input != ':')
		{
			fin.get(input);
		}
		fin.get(input);
		fin.get(input);

		ppVertex = (ST_GRS_VERTEX*)HeapAlloc(::GetProcessHeap()
			, HEAP_ZERO_MEMORY
			, nVertexCnt * sizeof(ST_GRS_VERTEX));
		ppIndices = (UINT*)HeapAlloc(::GetProcessHeap()
			, HEAP_ZERO_MEMORY
			, nVertexCnt * sizeof(UINT));

		for (UINT i = 0; i < nVertexCnt; i++)
		{
			fin >> ppVertex[i].m_vPos.x >> ppVertex[i].m_vPos.y >> ppVertex[i].m_vPos.z;
			ppVertex[i].m_vPos.w = 1.0f;
			fin >> ppVertex[i].m_vTex.x >> ppVertex[i].m_vTex.y;
			fin >> ppVertex[i].m_vNor.x >> ppVertex[i].m_vNor.y >> ppVertex[i].m_vNor.z;

			ppIndices[i] = i;
		}
	}
	catch (CGRSCOMException& e)
	{
		e;
		bRet = FALSE;
	}
	return bRet;
}
