#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ���ų�����ʹ�õ�����
#include <windows.h>
#include <tchar.h>
#include <fstream>  //for ifstream
using namespace std;
#include <wrl.h> //���WTL֧�� ����ʹ��COM
using namespace Microsoft;
using namespace Microsoft::WRL;
#include <atlcoll.h>  //for atl array
#include <strsafe.h>  //for StringCchxxxxx function

#include <dxgi1_6.h>
#include <d3d12.h> //for d3d12
#include <d3dcompiler.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#include <DirectXMath.h>
#include "..\WindowsCommons\d3dx12.h"
#include "..\WindowsCommons\DDSTextureLoader12.h"
using namespace DirectX;

#define GRS_WND_CLASS_NAME _T("Game Window Class")
#define GRS_WND_TITLE	_T("DirectX12 Texture Sample")

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
	if (StringCchPrintfW(_DebugName, _countof(_DebugName), L"%s[%u]", name, index))
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
	if (StringCchPrintfW(_DebugName, _countof(_DebugName), L"%s[%u]", name, index))
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
	XMFLOAT3 m_vPos;		//Position
	XMFLOAT2 m_vTex;		//Texcoord
	XMFLOAT3 m_vNor;		//Normal
};

// ����������
struct ST_GRS_MVP
{
	XMFLOAT4X4 m_MVP;			//�����Model-view-projection(MVP)����.
};


struct ST_GRS_THREAD_PARAMS
{
	DWORD								dwMainThreadID;
	HANDLE								hMainThread;
	HANDLE								hMainEvent;
	HANDLE								hRenderEvent;
	XMFLOAT4							v4ModelPos;
	const TCHAR*						pszDDSFile;
	const CHAR*							pszMeshFile;
	ID3D12Device4*						pID3DDevice;
	ID3D12CommandQueue*					pID3DCmdQCopy;
	ID3D12CommandAllocator*				pICmdAlloc;
	ID3D12GraphicsCommandList*			pICmdList;
	ID3D12RootSignature*				pIRS;
	ID3D12PipelineState*				pIPSO;
};

UINT g_nCurrentSamplerNO = 1; //��ǰʹ�õĲ��������� ������Ĭ��ʹ�õ�һ��
UINT g_nSampleMaxCnt = 5;		//����������͵Ĳ�����

//��ʼ��Ĭ���������λ��
XMFLOAT3 g_f3EyePos = XMFLOAT3(0.0f, 0.0f, -10.0f); //�۾�λ��
XMFLOAT3 g_f3LockAt = XMFLOAT3(0.0f, 0.0f, 0.0f);    //�۾�������λ��
XMFLOAT3 g_f3HeapUp = XMFLOAT3(0.0f, 1.0f, 0.0f);    //ͷ�����Ϸ�λ��

float g_fYaw = 0.0f;				// ����Z�����ת��.
float g_fPitch = 0.0f;			// ��XZƽ�����ת��

double g_fPalstance = 10.0f * XM_PI / 180.0f;	//������ת�Ľ��ٶȣ���λ������/��

// ȫ���̲߳���
const UINT			 g_nMaxThread = 3;
const UINT			 g_nThdSphere = 0;
const UINT			 g_nThdCube = 1;
const UINT			 g_nThdPlane = 2;
ST_GRS_THREAD_PARAMS g_stThreadParams[g_nMaxThread] = {};

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL LoadMeshVertex(const CHAR*pszMeshFileName, UINT&nVertexCnt, ST_GRS_VERTEX*&ppVertex, UINT*&ppIndices);
UINT __stdcall RenderThread(void* pParam);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR    lpCmdLine, int nCmdShow)
{
	::CoInitialize(nullptr);  //for WIC & COM

	int iWndWidth = 1024;
	int iWndHeight = 768;
	HWND hWnd = nullptr;
	MSG	msg = {};

	const UINT nFrameBackBufCount = 3u;
	UINT nCurrentFrameIndex = 0;

	UINT nDXGIFactoryFlags = 0U;
	UINT nRTVDescriptorSize = 0U;

	ComPtr<IDXGIFactory5>				pIDXGIFactory5;
	ComPtr<IDXGIAdapter1>				pIAdapter;
	ComPtr<ID3D12Device4>				pID3DDevice;
	ComPtr<ID3D12CommandQueue>			pIMainCmdQueue;
	ComPtr<IDXGISwapChain1>				pISwapChain1;
	ComPtr<IDXGISwapChain3>				pISwapChain3;
	ComPtr<ID3D12Resource>				pIARenderTargets[nFrameBackBufCount];
	ComPtr<ID3D12DescriptorHeap>		pIRTVHeap;
	ComPtr<ID3D12DescriptorHeap>		pIDSVHeap;				//��Ȼ�����������
	ComPtr<ID3D12Resource>				pIDepthStencilBuffer;	//������建����

	CD3DX12_VIEWPORT					stViewPort(0.0f, 0.0f, static_cast<float>(iWndWidth), static_cast<float>(iWndHeight));
	CD3DX12_RECT						stScissorRect(0, 0, static_cast<LONG>(iWndWidth), static_cast<LONG>(iWndHeight));

	ComPtr<ID3D12Fence>					pIFence;
	UINT64								n64FenceValue = 0ui64;
	HANDLE								hFenceEvent = nullptr;

	ComPtr<ID3D12CommandAllocator>		pICmdAllocDirect;
	ComPtr<ID3D12GraphicsCommandList>	pICmdListDirect;

	HANDLE								hThread[g_nMaxThread] = {};
	HANDLE								hEventThreadRender[g_nMaxThread] = {};
	CAtlArray<HANDLE>					arHWaited;
	DWORD								dwThreadID[g_nMaxThread] = {};
	ComPtr<ID3D12CommandQueue>			pICmdQueueCopy[g_nMaxThread] = {};
	ComPtr<ID3D12CommandAllocator>		pICmdAlloc[g_nMaxThread] = {};
	ComPtr<ID3D12GraphicsCommandList>   pICmdLists[g_nMaxThread] = {};

	ComPtr<ID3DBlob>					pIVSSphere;
	ComPtr<ID3DBlob>					pIPSSphere;
	ComPtr<ID3D12RootSignature>			pIRootSignature;
	ComPtr<ID3D12PipelineState>			pIPSOSphere;

	ComPtr<ID3D12DescriptorHeap>		pISampleHp;
	UINT								nSamplerDescriptorSize = 0; //��������С

	//======================================================================================================
	//������������С�϶��뵽256Bytes�߽�
	SIZE_T								szMVPBuf = GRS_UPPER(sizeof(ST_GRS_MVP), 256);

	ComPtr<ID3D12Resource>				pITxtSphere;
	ComPtr<ID3D12Resource>				pITxtUpSphere;
	ComPtr<ID3D12Resource>				pIVBSphere;
	ComPtr<ID3D12Resource>				pIIBSphere;
	ComPtr<ID3D12DescriptorHeap>		pISRVCBVHpSphere;
	ComPtr<ID3D12Resource>			    pICBWVPSphere;
	ST_GRS_MVP*							pMVPBufSphere = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			stVBVSphere = {};
	D3D12_INDEX_BUFFER_VIEW				stIBVSphere = {};
	UINT								nSphereIndexCnt = 0;
	XMMATRIX							mxPosSphere = XMMatrixTranslation(2.0f, 2.0f, 0.0f);  //Sphere ��λ��

	ComPtr<ID3D12Resource>				pITxtCube;
	ComPtr<ID3D12Resource>				pITxtUpCube;
	ComPtr<ID3D12Resource>				pIVBCube;
	ComPtr<ID3D12Resource>				pIIBCube;
	ComPtr<ID3D12DescriptorHeap>		pISRVCBVHpCube;
	ComPtr<ID3D12Resource>			    pICBWVPCube;
	ST_GRS_MVP*							pMVPBufCube = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			stVBVCube = {};
	D3D12_INDEX_BUFFER_VIEW				stIBVCube = {};
	UINT								nCubeIndexCnt = 0;
	XMMATRIX							mxPosCube = XMMatrixTranslation(-2.0f, 2.0f, 0.0f);  //Cube ��λ��


	//======================================================================================================

	ComPtr<ID3D12CommandAllocator>		pICmdAllocSphere;
	ComPtr<ID3D12GraphicsCommandList>   pICmdListSphere;

	HANDLE hMainEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr); //�ֶ����õ�Event
	//PulseEvent(hMainEvent);
	//HANDLE ghSemaphore = CreateSemaphore(NULL, 0, 3, NULL);
	//ReleaseSemaphore(ghSemaphore, 3, NULL);

	try
	{
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
			RECT rtWnd = { 0, 0, iWndWidth, iWndHeight };
			AdjustWindowRect(&rtWnd, dwWndStyle, FALSE);

			hWnd = CreateWindowW(GRS_WND_CLASS_NAME, GRS_WND_TITLE, dwWndStyle
				, CW_USEDEFAULT, 0, rtWnd.right - rtWnd.left, rtWnd.bottom - rtWnd.top
				, nullptr, nullptr, hInstance, nullptr);

			if (!hWnd)
			{
				return FALSE;
			}

			ShowWindow(hWnd, nCmdShow);
			UpdateWindow(hWnd);
		}

		//3������ʾ��ϵͳ�ĵ���֧��
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

		//4������DXGI Factory����
		{
			GRS_THROW_IF_FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory5)));
			GRS_SET_DXGI_DEBUGNAME_COMPTR(pIDXGIFactory5);
			// �ر�ALT+ENTER���л�ȫ���Ĺ��ܣ���Ϊ����û��ʵ��OnSize���������ȹر�
			GRS_THROW_IF_FAILED(pIDXGIFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
		}

		//5��ö�������������豸
		{//ѡ��NUMA�ܹ��Ķ���������3D�豸����,��ʱ�Ȳ�֧�ּ����ˣ���Ȼ������޸���Щ��Ϊ
			DXGI_ADAPTER_DESC1 desc = {};
			D3D12_FEATURE_DATA_ARCHITECTURE stArchitecture = {};
			for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pIDXGIFactory5->EnumAdapters1(adapterIndex, &pIAdapter); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc = {};
				pIAdapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
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

		//6������ֱ���������
		{
			D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
			stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&pIMainCmdQueue)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIMainCmdQueue);
		}

		//8������������
		{
			DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
			stSwapChainDesc.BufferCount = nFrameBackBufCount;
			stSwapChainDesc.Width = iWndWidth;
			stSwapChainDesc.Height = iWndHeight;
			stSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			stSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			stSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			stSwapChainDesc.SampleDesc.Count = 1;

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

			nCurrentFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

			//����RTV(��ȾĿ����ͼ)��������(����ѵĺ���Ӧ�����Ϊ������߹̶���СԪ�صĹ̶���С�Դ��)
			D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
			stRTVHeapDesc.NumDescriptors = nFrameBackBufCount;
			stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			GRS_THROW_IF_FAILED(pID3DDevice->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&pIRTVHeap)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIRTVHeap);

			//�õ�ÿ��������Ԫ�صĴ�С
			nRTVDescriptorSize = pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			//---------------------------------------------------------------------------------------------
			CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart());
			for (UINT i = 0; i < nFrameBackBufCount; i++)
			{//���ѭ����©����������ʵ�����Ǹ�����ı���
				GRS_THROW_IF_FAILED(pISwapChain3->GetBuffer(i, IID_PPV_ARGS(&pIARenderTargets[i])));
				GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(pIARenderTargets, i);
				pID3DDevice->CreateRenderTargetView(pIARenderTargets[i].Get(), nullptr, stRTVHandle);
				stRTVHandle.Offset(1, nRTVDescriptorSize);
			}


		}

		//9��������Ȼ��弰��Ȼ�����������
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC stDepthStencilDesc = {};
			stDepthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
			stDepthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			stDepthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
			depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
			depthOptimizedClearValue.DepthStencil.Stencil = 0;

			//ʹ����ʽĬ�϶Ѵ���һ��������建������
			//��Ϊ��������Ȼ�������һֱ��ʹ�ã����õ����岻������ֱ��ʹ����ʽ�ѣ�ͼ����
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)
				, D3D12_HEAP_FLAG_NONE
				, &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT
					, iWndWidth, iWndHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
				, D3D12_RESOURCE_STATE_DEPTH_WRITE
				, &depthOptimizedClearValue
				, IID_PPV_ARGS(&pIDepthStencilBuffer)
			));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIDepthStencilBuffer);

			//==============================================================================================
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			GRS_THROW_IF_FAILED(pID3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&pIDSVHeap)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIDSVHeap);

			pID3DDevice->CreateDepthStencilView(pIDepthStencilBuffer.Get()
				, &stDepthStencilDesc
				, pIDSVHeap->GetCPUDescriptorHandleForHeapStart());
		}

		//7������ֱ�������б������
		{
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
				, IID_PPV_ARGS(&pICmdAllocDirect)));
			//����ֱ�������б������Ͽ���ִ�м������е��������3Dͼ�����桢�������桢��������ȣ�
			//ע���ʼʱ��û��ʹ��PSO���󣬴�ʱ��ʵ��������б���Ȼ���Լ�¼����
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
				, pICmdAllocDirect.Get(), nullptr, IID_PPV_ARGS(&pICmdListDirect)));

			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE
				, IID_PPV_ARGS(&pICmdAllocSphere)));
			GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE
				, pICmdAllocSphere.Get(), nullptr, IID_PPV_ARGS(&pICmdListSphere)));

		}

		//9������Sample�ѣ��������ֲ�����
		{
			D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
			stSamplerHeapDesc.NumDescriptors = g_nSampleMaxCnt;
			stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			GRS_THROW_IF_FAILED(pID3DDevice->CreateDescriptorHeap(&stSamplerHeapDesc, IID_PPV_ARGS(&pISampleHp)));
			GRS_SET_D3D12_DEBUGNAME_COMPTR(pISampleHp);

			CD3DX12_CPU_DESCRIPTOR_HANDLE hSamplerHeap(pISampleHp->GetCPUDescriptorHandleForHeapStart());

			D3D12_SAMPLER_DESC stSamplerDesc = {};
			stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

			stSamplerDesc.MinLOD = 0;
			stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
			stSamplerDesc.MipLODBias = 0.0f;
			stSamplerDesc.MaxAnisotropy = 1;
			stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

			// Sampler 1
			stSamplerDesc.BorderColor[0] = 1.0f;
			stSamplerDesc.BorderColor[1] = 0.0f;
			stSamplerDesc.BorderColor[2] = 1.0f;
			stSamplerDesc.BorderColor[3] = 1.0f;
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			pID3DDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

			hSamplerHeap.Offset(nSamplerDescriptorSize);

			// Sampler 2
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			pID3DDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

			hSamplerHeap.Offset(nSamplerDescriptorSize);

			// Sampler 3
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			pID3DDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

			hSamplerHeap.Offset(nSamplerDescriptorSize);

			// Sampler 4
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			pID3DDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

			hSamplerHeap.Offset(nSamplerDescriptorSize);

			// Sampler 5
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			pID3DDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

		}

		//10��������ǩ��
		{//��������У������Skyboxʹ����ͬ�ĸ�ǩ������Ϊ��Ⱦ��������Ҫ�Ĳ�����һ����
			D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
			// ����Ƿ�֧��V1.1�汾�ĸ�ǩ��
			stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
			if (FAILED(pID3DDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
			{
				stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			// ��GPU��ִ��SetGraphicsRootDescriptorTable�����ǲ��޸������б��е�SRV��������ǿ���ʹ��Ĭ��Rang��Ϊ:
			// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
			CD3DX12_DESCRIPTOR_RANGE1 stDSPRanges[3];
			stDSPRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
			stDSPRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
			stDSPRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

			CD3DX12_ROOT_PARAMETER1 stRootParameters[3];
			stRootParameters[0].InitAsDescriptorTable(1, &stDSPRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);//SRV��PS�ɼ�
			stRootParameters[1].InitAsDescriptorTable(1, &stDSPRanges[1], D3D12_SHADER_VISIBILITY_ALL); //CBV������Shader�ɼ�
			stRootParameters[2].InitAsDescriptorTable(1, &stDSPRanges[2], D3D12_SHADER_VISIBILITY_PIXEL);//SAMPLE��PS�ɼ�

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc;

			stRootSignatureDesc.Init_1_1(_countof(stRootParameters), stRootParameters
				, 0, nullptr
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

		//11������Shader������Ⱦ����״̬����
		{

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif
			//����Ϊ�о�����ʽ	   
			compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

			TCHAR pszShaderFileName[] = _T("D:\\Projects_2018_08\\D3D12 Tutorials\\5-SkyBox\\Shader\\TextureCube.hlsl");

			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "VSMain", "vs_5_0", compileFlags, 0, &pIVSSphere, nullptr));
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "PSMain", "ps_5_0", compileFlags, 0, &pIPSSphere, nullptr));

			// ���Ƕ������һ�����ߵĶ��壬��ĿǰShader�����ǲ�û��ʹ��
			D3D12_INPUT_ELEMENT_DESC stIALayoutSphere[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// ���� graphics pipeline state object (PSO)����
			D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
			stPSODesc.InputLayout = { stIALayoutSphere, _countof(stIALayoutSphere) };
			stPSODesc.pRootSignature = pIRootSignature.Get();
			stPSODesc.VS = CD3DX12_SHADER_BYTECODE(pIVSSphere.Get());
			stPSODesc.PS = CD3DX12_SHADER_BYTECODE(pIPSSphere.Get());
			stPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			stPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			stPSODesc.DepthStencilState.DepthEnable = FALSE;
			stPSODesc.DepthStencilState.StencilEnable = FALSE;
			stPSODesc.SampleMask = UINT_MAX;
			stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			stPSODesc.NumRenderTargets = 1;
			stPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			stPSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			stPSODesc.DepthStencilState.DepthEnable = TRUE;
			stPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//������Ȼ���д�빦��
			stPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;     //��Ȳ��Ժ�������ֵΪ��ͨ����Ȳ��ԣ�
			stPSODesc.SampleDesc.Count = 1;

			GRS_THROW_IF_FAILED(pID3DDevice->CreateGraphicsPipelineState(&stPSODesc
				, IID_PPV_ARGS(&pIPSOSphere)));

			GRS_SET_D3D12_DEBUGNAME_COMPTR(pIPSOSphere);
		}

		// ׼�����������������Ⱦ�߳�
		{
			//struct ST_GRS_THREAD_PARAMS
			//{
			//	DWORD								dwMainThreadID;
			//	HANDLE								hMainThread;
			//	HANDLE								hMainEvent;
			//  HANDLE								hRenderEvent;
			//	XMFLOAT4							v4ModelPos;
			//	const TCHAR*						pszDDSFile;
			//	const CHAR*							pszMeshFile;
			//	ComPtr<ID3D12Device4>				pID3DDevice;
			//	ComPtr<ID3D12CommandQueue>			pID3DCmdQCopy;
			//	ComPtr<ID3D12CommandAllocator>		pICmdAlloc;
			//	ComPtr<ID3D12GraphicsCommandList>	pICmdList;
			//	ComPtr<ID3D12RootSignature>			pIRS;
			//	ComPtr<ID3D12PipelineState>			pIPSO;
			//	ComPtr<ID3D12DescriptorHeap>	    pIHPSample;
			//};


			// ������Բ���
			g_stThreadParams[g_nThdSphere].pszDDSFile = _T("D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\sphere.dds");
			g_stThreadParams[g_nThdSphere].pszMeshFile = "D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\sphere.txt";
			g_stThreadParams[g_nThdSphere].v4ModelPos = XMFLOAT4(2.0f, 2.0f, 0.0f, 1.0f);

			// ��������Բ���
			g_stThreadParams[g_nThdCube].pszDDSFile = _T("D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\Cube.dds");
			g_stThreadParams[g_nThdCube].pszMeshFile = "D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\Cube.txt";
			g_stThreadParams[g_nThdCube].v4ModelPos = XMFLOAT4(-2.0f, 2.0f, 0.0f, 1.0f);

			// ƽ����Բ���
			g_stThreadParams[g_nThdPlane].pszDDSFile = _T("D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\Plane.dds");
			g_stThreadParams[g_nThdPlane].pszMeshFile = "D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\Plane.txt";
			g_stThreadParams[g_nThdPlane].v4ModelPos = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);


			D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
			//ע��������Ȼʹ��ֱ�������������Ϊÿ���̴߳���һ������ִ�и���������������
			//��Ϊ����������еĻ�����Դ����ת��Ȩ��ʱû���Ѹ���Ŀ��Ȩ��ת��ΪShader�ɷ���Ȩ��
			//ʵ����Ҳ��֪������������л���������ɶ�ˡ�����������
			stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //D3D12_COMMAND_LIST_TYPE_COPY


			// ��������Ĺ��Բ�����Ҳ���Ǹ��̵߳Ĺ��Բ���
			for (int i = 0; i < g_nMaxThread; i++)
			{
				//����ÿ���߳���Ҫ�������б�͸����������
				GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
					, IID_PPV_ARGS(&pICmdAlloc[i])));
				GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(pICmdAlloc, i);
				GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
					, pICmdAlloc[i].Get(), nullptr, IID_PPV_ARGS(&pICmdLists[i])));
				GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(pICmdLists, i);
				GRS_THROW_IF_FAILED(pID3DDevice->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&pICmdQueueCopy[i])));
				GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(pICmdQueueCopy, i);

				hEventThreadRender[i] = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

				arHWaited.Add(hEventThreadRender[i]); //��ӵ����ȴ�������

				g_stThreadParams[i].dwMainThreadID = ::GetCurrentThreadId();
				g_stThreadParams[i].hMainThread = ::GetCurrentThread();
				g_stThreadParams[i].hMainEvent = hMainEvent;
				g_stThreadParams[i].hRenderEvent = hEventThreadRender[i];
				g_stThreadParams[i].pID3DDevice = pID3DDevice.Get();
				g_stThreadParams[i].pID3DCmdQCopy = pICmdQueueCopy[i].Get();
				g_stThreadParams[i].pICmdAlloc = pICmdAlloc[i].Get();
				g_stThreadParams[i].pICmdList = pICmdLists[i].Get();
				g_stThreadParams[i].pIRS = pIRootSignature.Get();
				g_stThreadParams[i].pIPSO = pIPSOSphere.Get();

				//����ͣ��ʽ�����߳�
				hThread[i] = (HANDLE)_beginthreadex(nullptr,
					0, RenderThread, (void*)&g_stThreadParams[i],
					CREATE_SUSPENDED, (UINT*)&dwThreadID[i]);

				//Ȼ���ж��̴߳����Ƿ�ɹ�
				if (nullptr == hThread[i]
					|| reinterpret_cast<HANDLE>(-1) == hThread[i])
				{
					throw CGRSCOMException(HRESULT_FROM_WIN32(GetLastError()));
				}
			}

			//��һ�����߳�
			for (int i = 0; i < g_nMaxThread; i++)
			{
				::ResumeThread(hThread[i]);
			}
		}

		//12��������Դ
		//{
		//	TCHAR pszCubeTextureFile[] = _T("D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\Cube.dds");
		//	TCHAR pszSphereTextureFile[] = _T("D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\Sphere.dds");
		//	CHAR pszMeshFileName[] = "D:\\Projects_2018_08\\D3D12 Tutorials\\5-SkyBox\\Mesh\\sphere.txt";
		//	TCHAR pszPlaneTextureFile[] = _T("D:\\Projects_2018_08\\D3D12 Tutorials\\6-MultiThread\\Mesh\\Plane.dds");

		//	std::unique_ptr<uint8_t[]>			pbDDSData;
		//	std::vector<D3D12_SUBRESOURCE_DATA> stArSubResources;
		//	DDS_ALPHA_MODE						emAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
		//	bool								bIsCube = false;

		//	//����DDS
		//	GRS_THROW_IF_FAILED(LoadDDSTextureFromFile(pID3DDevice.Get(), pszSphereTextureFile, pITxtSphere.GetAddressOf()
		//		, pbDDSData, stArSubResources, SIZE_MAX, &emAlphaMode, &bIsCube));

		//	UINT64 n64szUpSphere = GetRequiredIntermediateSize(pITxtSphere.Get(), 0, static_cast<UINT>(stArSubResources.size()));
		//	D3D12_RESOURCE_DESC stTXDesc = pITxtSphere->GetDesc();

		//	//�����ϴ���
		//	GRS_THROW_IF_FAILED(pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
		//		, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(n64szUpSphere), D3D12_RESOURCE_STATE_GENERIC_READ
		//		, nullptr, IID_PPV_ARGS(&pITxtUpSphere)));

		//	//�ϴ�DDS
		//	UpdateSubresources(pICmdListDirect.Get(), pITxtSphere.Get(), pITxtUpSphere.Get()
		//		, 0, 0, static_cast<UINT>(stArSubResources.size()), stArSubResources.data());

		//	//ͬ��
		//	pICmdListDirect->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pITxtSphere.Get()
		//		, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		//	//����SRV CBV��
		//	D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
		//	stSRVHeapDesc.NumDescriptors = 2; //1 SRV + 1 CBV
		//	stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		//	stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		//	GRS_THROW_IF_FAILED(pID3DDevice->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&pISRVCBVHpSphere)));

		//	//����SRV
		//	D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
		//	stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//	stSRVDesc.Format = stTXDesc.Format;
		//	stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		//	stSRVDesc.Texture2D.MipLevels = 1;
		//	pID3DDevice->CreateShaderResourceView(pITxtSphere.Get(), &stSRVDesc, pISRVCBVHpSphere->GetCPUDescriptorHandleForHeapStart());


		//	ST_GRS_VERTEX*						pstSphereVertices = nullptr;
		//	UINT								nSphereVertexCnt = 0;
		//	UINT*								pSphereIndices = nullptr;

		//	//������������
		//	LoadMeshVertex(pszMeshFileName, nSphereVertexCnt, pstSphereVertices, pSphereIndices);
		//	nSphereIndexCnt = nSphereVertexCnt;

		//	//���� Vertex Buffer ��ʹ��Upload��ʽ��
		//	GRS_THROW_IF_FAILED(pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE
		//		, &CD3DX12_RESOURCE_DESC::Buffer(nSphereVertexCnt * sizeof(ST_GRS_VERTEX)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pIVBSphere)));

		//	//ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
		//	UINT8* pVertexDataBegin = nullptr;
		//	CD3DX12_RANGE stReadRange(0, 0);		// We do not intend to read from this resource on the CPU.

		//	GRS_THROW_IF_FAILED(pIVBSphere->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		//	memcpy(pVertexDataBegin, pstSphereVertices, nSphereVertexCnt * sizeof(ST_GRS_VERTEX));
		//	pIVBSphere->Unmap(0, nullptr);

		//	//���� Index Buffer ��ʹ��Upload��ʽ��
		//	GRS_THROW_IF_FAILED(pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE
		//		, &CD3DX12_RESOURCE_DESC::Buffer(nSphereIndexCnt * sizeof(UINT)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pIIBSphere)));

		//	UINT8* pIndexDataBegin = nullptr;
		//	GRS_THROW_IF_FAILED(pIIBSphere->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		//	memcpy(pIndexDataBegin, pSphereIndices, nSphereIndexCnt * sizeof(UINT));
		//	pIIBSphere->Unmap(0, nullptr);

		//	//����Vertex Buffer View
		//	stVBVSphere.BufferLocation = pIVBSphere->GetGPUVirtualAddress();
		//	stVBVSphere.StrideInBytes = sizeof(ST_GRS_VERTEX);
		//	stVBVSphere.SizeInBytes = nSphereVertexCnt * sizeof(ST_GRS_VERTEX);
		//	//����Index Buffer View
		//	stIBVSphere.BufferLocation = pIIBSphere->GetGPUVirtualAddress();
		//	stIBVSphere.Format = DXGI_FORMAT_R32_UINT;
		//	stIBVSphere.SizeInBytes = nSphereIndexCnt * sizeof(UINT);

		//	::HeapFree(::GetProcessHeap(), 0, pstSphereVertices);
		//	::HeapFree(::GetProcessHeap(), 0, pSphereIndices);

		//	// ������������ ע�⻺��ߴ�����Ϊ256�߽�����С
		//	GRS_THROW_IF_FAILED(pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE
		//		, &CD3DX12_RESOURCE_DESC::Buffer(szMVPBuf), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pICBWVPSphere)));

		//	// Map ֮��Ͳ���Unmap�� ֱ�Ӹ������ݽ�ȥ ����ÿ֡������map-copy-unmap�˷�ʱ����
		//	GRS_THROW_IF_FAILED(pICBWVPSphere->Map(0, nullptr, reinterpret_cast<void**>(&pMVPBufSphere)));
		//	//---------------------------------------------------------------------------------------------

		//	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		//	cbvDesc.BufferLocation = pICBWVPSphere->GetGPUVirtualAddress();
		//	cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuf);

		//	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(pISRVCBVHpSphere->GetCPUDescriptorHandleForHeapStart()
		//		, 1, pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		//	pID3DDevice->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
		//	//---------------------------------------------------------------------------------------------
		//}

		////20��ִ�еڶ���Copy����ȴ����е������ϴ�����Ĭ�϶���
		//{
		//	GRS_THROW_IF_FAILED(pICmdListDirect->Close());
		//	ID3D12CommandList* ppCommandLists[] = { pICmdListDirect.Get() };
		//	pIMainCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		//	// ����һ��ͬ�����󡪡�Χ�������ڵȴ��ڶ���Copy���
		//	GRS_THROW_IF_FAILED(pID3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pIFence)));
		//	n64FenceValue = 1;

		//	// ����һ��Eventͬ���������ڵȴ�Χ���¼�֪ͨ
		//	hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		//	if (hFenceEvent == nullptr)
		//	{
		//		GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
		//	}

		//	// �ȴ�������Դ��ʽ���������
		//	const UINT64 fence = n64FenceValue;
		//	GRS_THROW_IF_FAILED(pIMainCmdQueue->Signal(pIFence.Get(), fence));
		//	n64FenceValue++;

		//	// ��������û������ִ�е�Χ����ǵ����û�о������¼�ȥ�ȴ���ע��ʹ�õ���������ж����ָ��
		//	if (pIFence->GetCompletedValue() < fence)
		//	{
		//		GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(fence, hFenceEvent));
		//		WaitForSingleObject(hFenceEvent, INFINITE);
		//	}

		//	//�����������Resetһ�£��ղ��Ѿ�ִ�й���һ���������������
		//	GRS_THROW_IF_FAILED(pICmdAllocDirect->Reset());
		//	//Reset�����б�������ָ�������������PSO����
		//	GRS_THROW_IF_FAILED(pICmdListDirect->Reset(pICmdAllocDirect.Get(), pIPSOSphere.Get()));
		//}

		////27�����������¼�̻�������
		//{
		//	//=================================================================================================
		//	//����������
		//	pICmdListSphere->SetGraphicsRootSignature(pIRootSignature.Get());
		//	pICmdListSphere->SetPipelineState(pIPSOSphere.Get());
		//	ID3D12DescriptorHeap* ppHeapsSphere[] = { pISRVCBVHpSphere.Get(),pISampleHp.Get() };
		//	pICmdListSphere->SetDescriptorHeaps(_countof(ppHeapsSphere), ppHeapsSphere);
		//	//����SRV
		//	pICmdListSphere->SetGraphicsRootDescriptorTable(0, pISRVCBVHpSphere->GetGPUDescriptorHandleForHeapStart());

		//	CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleSphere(pISRVCBVHpSphere->GetGPUDescriptorHandleForHeapStart()
		//		, 1
		//		, pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		//	//����CBV
		//	pICmdListSphere->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleSphere);
		//	CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUSamplerSphere(pISampleHp->GetGPUDescriptorHandleForHeapStart()
		//		, 2
		//		, nSamplerDescriptorSize);
		//	//����Sample
		//	pICmdListSphere->SetGraphicsRootDescriptorTable(2, hGPUSamplerSphere);
		//	//ע������ʹ�õ���Ⱦ�ַ����������б�Ҳ����ͨ����Mesh����
		//	pICmdListSphere->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//	pICmdListSphere->IASetVertexBuffers(0, 1, &stVBVSphere);
		//	pICmdListSphere->IASetIndexBuffer(&stIBVSphere);

		//	//Draw Call������
		//	pICmdListSphere->DrawIndexedInstanced(nSphereIndexCnt, 1, 0, 0, 0);
		//	pICmdListSphere->Close();
		//	//=================================================================================================

		//}

		//---------------------------------------------------------------------------------------------
		//28��������ʱ�������Ա��ڴ�����Ч����Ϣѭ��
		HANDLE phWait = CreateWaitableTimer(NULL, FALSE, NULL);
		LARGE_INTEGER liDueTime = {};
		liDueTime.QuadPart = -1i64;//1���ʼ��ʱ
		SetWaitableTimer(phWait, &liDueTime, 1, NULL, NULL, 0);//40ms������

		//29����¼֡��ʼʱ�䣬�͵�ǰʱ�䣬��ѭ������Ϊ��
		ULONGLONG n64tmFrameStart = ::GetTickCount64();
		ULONGLONG n64tmCurrent = n64tmFrameStart;
		//������ת�Ƕ���Ҫ�ı���
		double dModelRotationYAngle = 0.0f;

		//����һ��Eventͬ���������ڵȴ�Χ���¼�֪ͨ
		//ע���ʼʱ��������Ϊ���ź�״̬
		hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (hFenceEvent == nullptr)
		{
			GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
		}
		arHWaited.Add(hFenceEvent);

		//---------------------------------------------------------------------------------------------
		//30����ʼ��Ϣѭ�����������в�����Ⱦ
		DWORD dwRet = 0;
		BOOL bExit = FALSE;

		CAtlArray<ID3D12CommandList*> arCmdList;

		while (!bExit)
		{//ע���������ǵ�������Ϣѭ�������ȴ�ʱ������Ϊ0��ͬʱ����ʱ�Ե���Ⱦ���ĳ���ÿ��ѭ������Ⱦ
		 //�ر�ע����εȴ���֮ǰ��ͬ
			//---------------------------------------------------------------------------------------------
			// ׼��һ���򵥵���תMVP���� �÷���ת����
			{
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

				//���� �Ӿ��� view * �ü����� projection
				XMMATRIX xmMVP = XMMatrixMultiply(XMMatrixLookAtLH(XMLoadFloat3(&g_f3EyePos)
					, XMLoadFloat3(&g_f3LockAt)
					, XMLoadFloat3(&g_f3HeapUp))
					, XMMatrixPerspectiveFovLH(XM_PIDIV4
						, (FLOAT)iWndWidth / (FLOAT)iWndHeight, 0.1f, 1000.0f));

				//����Skybox��MVP
				//XMStoreFloat4x4(&pMVPBufSkybox->m_MVP, xmMVP);

				//ģ�;��� model �����ǷŴ����ת
				XMMATRIX xmRot = XMMatrixMultiply(mxPosSphere
					, XMMatrixRotationY(static_cast<float>(dModelRotationYAngle)));

				//���������MVP
				xmMVP = XMMatrixMultiply(xmRot, xmMVP);

				//XMStoreFloat4x4(&pMVPBufSphere->m_MVP, xmMVP);
			}
			//---------------------------------------------------------------------------------------------

			//�����������Resetһ��
			GRS_THROW_IF_FAILED(pICmdAllocDirect->Reset());
			//Reset�����б�������ָ�������������PSO����
			GRS_THROW_IF_FAILED(pICmdListDirect->Reset(pICmdAllocDirect.Get(), pIPSOSphere.Get()));

			nCurrentFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

			// ͨ����Դ�����ж��󻺳��Ѿ��л���Ͽ��Կ�ʼ��Ⱦ��
			pICmdListDirect->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pIARenderTargets[nCurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

			//ƫ��������ָ�뵽ָ��֡������ͼλ��
			CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart(), nCurrentFrameIndex, nRTVDescriptorSize);
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(pIDSVHeap->GetCPUDescriptorHandleForHeapStart());
			//������ȾĿ��
			pICmdListDirect->OMSetRenderTargets(1, &stRTVHandle, FALSE, &dsvHandle);

			for (int i = 0; i < g_nMaxThread; i++)
			{
				g_stThreadParams[i].pICmdList->OMSetRenderTargets(1, &stRTVHandle, FALSE, &dsvHandle);
			}
			//---------------------------------------------------------------------------------------------
			pICmdListDirect->RSSetViewports(1, &stViewPort);
			pICmdListDirect->RSSetScissorRects(1, &stScissorRect);

			//---------------------------------------------------------------------------------------------
			// ������¼�����������ʼ��һ֡����Ⱦ
			const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
			pICmdListDirect->ClearRenderTargetView(stRTVHandle, clearColor, 0, nullptr);
			pICmdListDirect->ClearDepthStencilView(pIDSVHeap->GetCPUDescriptorHandleForHeapStart()
				, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			::PulseEvent(hMainEvent); //֪ͨ�������߳̿�ʼ��Ⱦ

			//���߳̽���ȴ�
			dwRet = ::MsgWaitForMultipleObjects(static_cast<DWORD>(arHWaited.GetCount()), arHWaited.GetData(), TRUE, 0, QS_ALLINPUT);
			switch (dwRet - WAIT_OBJECT_0)
			{
			case 0:
			{
				ResetEvent(hMainEvent);//����Ϊ���ź�״̬�������̵߳ȴ�

				//�ر������б�����ȥִ����
				GRS_THROW_IF_FAILED(pICmdListDirect->Close());

				arCmdList.RemoveAll();
				//ִ�������б�
				arCmdList.Add(pICmdListDirect.Get());
				arCmdList.Add(pICmdLists[g_nThdSphere].Get());
				arCmdList.Add(pICmdLists[g_nThdCube].Get());
				arCmdList.Add(pICmdLists[g_nThdPlane].Get());
				pIMainCmdQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

				//��һ����Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
				pICmdListDirect->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pIARenderTargets[nCurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
				//�ύ����
				GRS_THROW_IF_FAILED(pISwapChain3->Present(1, 0));

				//---------------------------------------------------------------------------------------------
				//��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
				const UINT64 fence = n64FenceValue;
				GRS_THROW_IF_FAILED(pIMainCmdQueue->Signal(pIFence.Get(), fence));
				n64FenceValue++;
				GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(fence, hFenceEvent));
				//---------------------------------------------------------------------------------------------
			}
			break;
			case WAIT_TIMEOUT:
			{//��ʱ��ʱ�䵽

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

			//���һ���̵߳Ļ�����������߳��Ѿ��˳��ˣ����˳�ѭ��
			dwRet = WaitForMultipleObjects(g_nMaxThread, hThread, FALSE, 0);
			dwRet -= WAIT_OBJECT_0;
			if (dwRet >= 0 && dwRet <= g_nMaxThread - 1)
			{
				bExit = TRUE;
			}

			//GRS_TRACE(_T("��%u֡��Ⱦ����.\n"), nFrame++);
		}
		//::CoUninitialize();


	}
	catch (CGRSCOMException& e)
	{//������COM�쳣
		e;
	}
	return 0;
}

UINT __stdcall RenderThread(void* pParam)
{
	ST_GRS_THREAD_PARAMS* pThdPms = static_cast<ST_GRS_THREAD_PARAMS*>(pParam);

	try
	{
		if (nullptr == pThdPms)
		{//�����쳣�����쳣��ֹ�߳�
			throw CGRSCOMException(E_INVALIDARG);
		}

		SIZE_T								szMVPBuf = GRS_UPPER(sizeof(ST_GRS_MVP), 256);
		ComPtr<ID3D12Resource>				pITxtSphere;
		ComPtr<ID3D12Resource>				pITxtUpSphere;
		ComPtr<ID3D12Resource>				pIVBSphere;
		ComPtr<ID3D12Resource>				pIIBSphere;
		ComPtr<ID3D12Resource>			    pICBWVPSphere;
		ComPtr<ID3D12DescriptorHeap>		pISRVCBVHpSphere;
		ComPtr<ID3D12DescriptorHeap>		pISampleHp;
		ST_GRS_MVP*							pMVPBufSphere = nullptr;
		D3D12_VERTEX_BUFFER_VIEW			stVBVSphere = {};
		D3D12_INDEX_BUFFER_VIEW				stIBVSphere = {};
		UINT								nSphereIndexCnt = 0;
		XMMATRIX							mxPosSphere = XMMatrixTranslation(2.0f, 2.0f, 0.0f);  //Sphere ��λ��

		std::unique_ptr<uint8_t[]>			pbDDSData;
		std::vector<D3D12_SUBRESOURCE_DATA> stArSubResources;
		DDS_ALPHA_MODE						emAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
		bool								bIsCube = false;

		//����DDS
		GRS_THROW_IF_FAILED(LoadDDSTextureFromFile(pThdPms->pID3DDevice, pThdPms->pszDDSFile, pITxtSphere.GetAddressOf()
			, pbDDSData, stArSubResources, SIZE_MAX, &emAlphaMode, &bIsCube));

		UINT64 n64szUpSphere = GetRequiredIntermediateSize(pITxtSphere.Get(), 0, static_cast<UINT>(stArSubResources.size()));
		D3D12_RESOURCE_DESC stTXDesc = pITxtSphere->GetDesc();

		//�����ϴ���
		GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
			, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(n64szUpSphere), D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr, IID_PPV_ARGS(&pITxtUpSphere)));

		//�ϴ�DDS
		UpdateSubresources(pThdPms->pICmdList, pITxtSphere.Get(), pITxtUpSphere.Get()
			, 0, 0, static_cast<UINT>(stArSubResources.size()), stArSubResources.data());

		//ͬ��
		pThdPms->pICmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pITxtSphere.Get()
			, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		//����SRV CBV��
		D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
		stSRVHeapDesc.NumDescriptors = 2; //1 SRV + 1 CBV
		stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&pISRVCBVHpSphere)));

		//����SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
		stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		stSRVDesc.Format = stTXDesc.Format;
		stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		stSRVDesc.Texture2D.MipLevels = 1;
		pThdPms->pID3DDevice->CreateShaderResourceView(pITxtSphere.Get(), &stSRVDesc, pISRVCBVHpSphere->GetCPUDescriptorHandleForHeapStart());

		// ����Sample
		D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
		stSamplerHeapDesc.NumDescriptors = 1;
		stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateDescriptorHeap(&stSamplerHeapDesc, IID_PPV_ARGS(&pISampleHp)));
		GRS_SET_D3D12_DEBUGNAME_COMPTR(pISampleHp);
		D3D12_SAMPLER_DESC stSamplerDesc = {};
		stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		stSamplerDesc.MinLOD = 0;
		stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		stSamplerDesc.MipLODBias = 0.0f;
		stSamplerDesc.MaxAnisotropy = 1;
		stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		stSamplerDesc.BorderColor[0] = 1.0f;
		stSamplerDesc.BorderColor[1] = 0.0f;
		stSamplerDesc.BorderColor[2] = 1.0f;
		stSamplerDesc.BorderColor[3] = 1.0f;
		stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;

		pThdPms->pID3DDevice->CreateSampler(&stSamplerDesc, pISampleHp->GetCPUDescriptorHandleForHeapStart());

		// Mesh
		ST_GRS_VERTEX*						pstSphereVertices = nullptr;
		UINT								nSphereVertexCnt = 0;
		UINT*								pSphereIndices = nullptr;

		//������������
		LoadMeshVertex(pThdPms->pszMeshFile, nSphereVertexCnt, pstSphereVertices, pSphereIndices);
		nSphereIndexCnt = nSphereVertexCnt;

		//���� Vertex Buffer ��ʹ��Upload��ʽ��
		GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE
			, &CD3DX12_RESOURCE_DESC::Buffer(nSphereVertexCnt * sizeof(ST_GRS_VERTEX)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pIVBSphere)));

		//ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE stReadRange(0, 0);		// We do not intend to read from this resource on the CPU.

		GRS_THROW_IF_FAILED(pIVBSphere->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, pstSphereVertices, nSphereVertexCnt * sizeof(ST_GRS_VERTEX));
		pIVBSphere->Unmap(0, nullptr);

		//���� Index Buffer ��ʹ��Upload��ʽ��
		GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE
			, &CD3DX12_RESOURCE_DESC::Buffer(nSphereIndexCnt * sizeof(UINT)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pIIBSphere)));

		UINT8* pIndexDataBegin = nullptr;
		GRS_THROW_IF_FAILED(pIIBSphere->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, pSphereIndices, nSphereIndexCnt * sizeof(UINT));
		pIIBSphere->Unmap(0, nullptr);

		//����Vertex Buffer View
		stVBVSphere.BufferLocation = pIVBSphere->GetGPUVirtualAddress();
		stVBVSphere.StrideInBytes = sizeof(ST_GRS_VERTEX);
		stVBVSphere.SizeInBytes = nSphereVertexCnt * sizeof(ST_GRS_VERTEX);
		//����Index Buffer View
		stIBVSphere.BufferLocation = pIIBSphere->GetGPUVirtualAddress();
		stIBVSphere.Format = DXGI_FORMAT_R32_UINT;
		stIBVSphere.SizeInBytes = nSphereIndexCnt * sizeof(UINT);

		::HeapFree(::GetProcessHeap(), 0, pstSphereVertices);
		::HeapFree(::GetProcessHeap(), 0, pSphereIndices);

		// ������������ ע�⻺��ߴ�����Ϊ256�߽�����С
		GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE
			, &CD3DX12_RESOURCE_DESC::Buffer(szMVPBuf), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pICBWVPSphere)));

		// Map ֮��Ͳ���Unmap�� ֱ�Ӹ������ݽ�ȥ ����ÿ֡������map-copy-unmap�˷�ʱ����
		GRS_THROW_IF_FAILED(pICBWVPSphere->Map(0, nullptr, reinterpret_cast<void**>(&pMVPBufSphere)));
		//---------------------------------------------------------------------------------------------

		// ����CBV
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = pICBWVPSphere->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuf);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(pISRVCBVHpSphere->GetCPUDescriptorHandleForHeapStart()
			, 1, pThdPms->pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		pThdPms->pID3DDevice->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
		//---------------------------------------------------------------------------------------------

		// ʹ�ø�������ִ�еڶ�����������
		GRS_THROW_IF_FAILED(pThdPms->pICmdList->Close());
		ID3D12CommandList* ppCommandLists[] = { pThdPms->pICmdList };
		pThdPms->pID3DCmdQCopy->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		ComPtr<ID3D12Fence> pIFence;
		// ����һ��ͬ�����󡪡�Χ�������ڵȴ��ڶ���Copy���
		GRS_THROW_IF_FAILED(pThdPms->pID3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pIFence)));
		UINT64 n64FenceValue = 1;

		// ����һ��Eventͬ���������ڵȴ�Χ���¼�֪ͨ
		HANDLE hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (hFenceEvent == nullptr)
		{
			GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
		}

		// �ȴ�������Դ��ʽ���������
		const UINT64 fence = n64FenceValue;
		GRS_THROW_IF_FAILED(pThdPms->pID3DCmdQCopy->Signal(pIFence.Get(), fence));
		n64FenceValue++;

		// ��������û������ִ�е�Χ����ǵ����û�о������¼�ȥ�ȴ���ע��ʹ�õ���������ж����ָ��
		if (pIFence->GetCompletedValue() < fence)
		{
			GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(fence, hFenceEvent));
			WaitForSingleObject(hFenceEvent, INFINITE);
		}

		DWORD dwRet = 0;
		BOOL  bQuit = FALSE;
		MSG   msg = {};
		while (!bQuit)
		{
			//ע��ȴ������� 
			dwRet = ::MsgWaitForMultipleObjects(1, &pThdPms->hMainEvent, FALSE, 0, QS_ALLPOSTMESSAGE);
			switch (dwRet - WAIT_OBJECT_0)
			{
			case 0:
			{
				//�����������Resetһ�£��ղ��Ѿ�ִ�й���һ���������������
				GRS_THROW_IF_FAILED(pThdPms->pICmdAlloc->Reset());
				//Reset�����б�������ָ�������������PSO����
				GRS_THROW_IF_FAILED(pThdPms->pICmdList->Reset(pThdPms->pICmdAlloc, pThdPms->pIPSO));

				pThdPms->pICmdList->SetGraphicsRootSignature(pThdPms->pIRS);
				pThdPms->pICmdList->SetPipelineState(pThdPms->pIPSO);
				ID3D12DescriptorHeap* ppHeapsSphere[] = { pISRVCBVHpSphere.Get(),pISampleHp.Get() };
				pThdPms->pICmdList->SetDescriptorHeaps(_countof(ppHeapsSphere), ppHeapsSphere);
				//����SRV
				pThdPms->pICmdList->SetGraphicsRootDescriptorTable(0, pISRVCBVHpSphere->GetGPUDescriptorHandleForHeapStart());

				CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleSphere(pISRVCBVHpSphere->GetGPUDescriptorHandleForHeapStart()
					, 1
					, pThdPms->pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
				//����CBV
				pThdPms->pICmdList->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleSphere);
				//����Sample
				pThdPms->pICmdList->SetGraphicsRootDescriptorTable(2, pISampleHp->GetGPUDescriptorHandleForHeapStart());
				//ע������ʹ�õ���Ⱦ�ַ����������б�Ҳ����ͨ����Mesh����
				pThdPms->pICmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				pThdPms->pICmdList->IASetVertexBuffers(0, 1, &stVBVSphere);
				pThdPms->pICmdList->IASetIndexBuffer(&stIBVSphere);

				//Draw Call������
				pThdPms->pICmdList->DrawIndexedInstanced(nSphereIndexCnt, 1, 0, 0, 0);
				pThdPms->pICmdList->Close();

				::SetEvent(pThdPms->hMainThread); // �����źţ�֪ͨ���̱߳��߳���Ⱦ���
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
			++g_nCurrentSamplerNO;
			g_nCurrentSamplerNO %= g_nSampleMaxCnt;

			//=================================================================================================
			//������������������
			//pICmdListSphere->Reset(pICmdAllocSphere.Get(), pIPSOSphere.Get());
			//pICmdListSphere->SetGraphicsRootSignature(pIRootSignature.Get());
			//pICmdListSphere->SetPipelineState(pIPSOSphere.Get());
			//ID3D12DescriptorHeap* ppHeapsSphere[] = { pISRVCBVHpSphere.Get(),pISampleHp.Get() };
			//pICmdListSphere->SetDescriptorHeaps(_countof(ppHeapsSphere), ppHeapsSphere);
			////����SRV
			//pICmdListSphere->SetGraphicsRootDescriptorTable(0, pISRVCBVHpSphere->GetGPUDescriptorHandleForHeapStart());

			//CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleSphere(pISRVCBVHpSphere->GetGPUDescriptorHandleForHeapStart()
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
			//pICmdListSphere->IASetVertexBuffers(0, 1, &stVBVSphere);
			//pICmdListSphere->IASetIndexBuffer(&stIBVSphere);

			////Draw Call������
			//pICmdListSphere->DrawIndexedInstanced(nSphereIndexCnt, 1, 0, 0, 0);
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