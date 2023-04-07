#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ���ų�����ʹ�õ�����
#include <windows.h>
#include <tchar.h>
#include <commdlg.h>
#include <wrl.h> //���WTL֧�� ����ʹ��COM
#include <strsafe.h>

#include <atlbase.h>
#include <atlcoll.h>
#include <atlchecked.h>
#include <atlstr.h>
#include <atlconv.h>
#include <atlexcept.h>

#include <DirectXMath.h>

#include <dxgi1_6.h>
#include <d3d12.h> //for d3d12
#include <d3dcompiler.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

using namespace DirectX;
using namespace Microsoft;
using namespace Microsoft::WRL;

#include "../Commons/GRS_Def.h"
#include "../Commons/GRS_Mem.h"
#include "../Commons/GRS_D3D12_Utility.h"
#include "../Commons/GRS_Texture_Loader.h"
#include "../Commons/CGRSD3DCompilerInclude.h"
#include "../Commons/GRS_Mesh_Load_Txt.h"
#include "../Commons/GRS_Assimp_Loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"  // for HDR Image File

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define GRS_WND_CLASS _T("GRS Game Window Class")
#define GRS_WND_TITLE _T("GRS DirectX12 Sample: PBR IBL With Texture")

// ����ú� ��ʹ���ļ��򿪶Ի���ѡ����Ҫ���ص���Դ
// �����ʹ��Ĭ�ϵ���ԴչʾЧ��
#define GRS_USE_DIR_DIALOG

const UINT		g_nFrameBackBufCount = 3u;

struct ST_GRS_D3D12_DEVICE
{
    DWORD                               m_dwWndStyle;
    INT                                 m_iWndWidth;
    INT							        m_iWndHeight;
    HWND						        m_hWnd;
    RECT                                m_rtWnd;
    RECT                                m_rtWndClient;

    const float						    m_faClearColor[4] = { 0.17647f, 0.549f, 0.941176f, 1.0f };
    D3D_FEATURE_LEVEL					m_emFeatureLevel = D3D_FEATURE_LEVEL_12_1;
    ComPtr<IDXGIFactory6>				m_pIDXGIFactory6;
    ComPtr<ID3D12Device4>				m_pID3D12Device4;

    ComPtr<ID3D12CommandQueue>			m_pIMainCMDQueue;
    ComPtr<ID3D12CommandAllocator>		m_pIMainCMDAlloc;
    ComPtr<ID3D12GraphicsCommandList>	m_pIMainCMDList;

    BOOL                                m_bFullScreen;
    BOOL                                m_bSupportTearing;

    UINT			                    m_nFrameIndex;
    DXGI_FORMAT		                    m_emRenderTargetFormat;
    DXGI_FORMAT                         m_emDepthStencilFormat;

    UINT                                m_nRTVDescriptorSize;
    UINT                                m_nDSVDescriptorSize;
    UINT                                m_nSamplerDescriptorSize;
    UINT                                m_nCBVSRVDescriptorSize;

    UINT64						        m_n64FenceValue;
    HANDLE						        m_hEventFence;

    D3D12_VIEWPORT                      m_stViewPort;
    D3D12_RECT                          m_stScissorRect;

    ComPtr<ID3D12Fence>			        m_pIFence;
    ComPtr<IDXGISwapChain3>		        m_pISwapChain3;
    ComPtr<ID3D12DescriptorHeap>        m_pIRTVHeap;
    ComPtr<ID3D12Resource>		        m_pIARenderTargets[g_nFrameBackBufCount];
    ComPtr<ID3D12DescriptorHeap>        m_pIDSVHeap;
    ComPtr<ID3D12Resource>		        m_pIDepthStencilBuffer;
};

// ����GS һ����ȾCube Map �� 6 ���� ������Ҫʹ�� GS Render Target Array ����
#define GRS_CUBE_MAP_FACE_CNT           6u
#define GRS_LIGHT_COUNT                 4u

#define GRS_RTA_DEFAULT_SIZE_W          2048
#define GRS_RTA_DEFAULT_SIZE_H          2048

#define GRS_RTA_IBL_DIFFUSE_MAP_SIZE_W  256
#define GRS_RTA_IBL_DIFFUSE_MAP_SIZE_H  256

#define GRS_RTA_IBL_SPECULAR_MAP_SIZE_W 512
#define GRS_RTA_IBL_SPECULAR_MAP_SIZE_H 512
#define GRS_RTA_IBL_SPECULAR_MAP_MIP    5

#define GRS_RTA_IBL_LUT_SIZE_W          512
#define GRS_RTA_IBL_LUT_SIZE_H          512
#define GRS_RTA_IBL_LUT_FORMAT          DXGI_FORMAT_R32G32_FLOAT

// Geometry Shader Render Target Array Struct
struct ST_GRS_RENDER_TARGET_ARRAY_DATA
{
    UINT                                m_nRTWidth;
    UINT                                m_nRTHeight;
    D3D12_VIEWPORT                      m_stViewPort;
    D3D12_RECT                          m_stScissorRect;

    DXGI_FORMAT                         m_emRTAFormat;

    D3D12_RESOURCE_BARRIER              m_stRTABarriers;

    ComPtr<ID3D12DescriptorHeap>        m_pIRTAHeap;
    ComPtr<ID3D12Resource>              m_pITextureRTA;
    ComPtr<ID3D12Resource>              m_pITextureRTADownload;
};

struct ST_GRS_PSO
{
    UINT                                m_nCnt;
    UINT                                m_nCBVCnt;
    UINT                                m_nSRVCnt;
    UINT                                m_nSmpCnt;

    ComPtr<ID3D12RootSignature>         m_pIRS;
    ComPtr<ID3D12PipelineState>         m_pIPSO;

    ComPtr<ID3D12CommandAllocator>		m_pICmdAlloc;
    ComPtr<ID3D12GraphicsCommandList>   m_pICmdBundles;

    ComPtr<ID3D12CommandSignature>      m_pICmdSignature;
};

struct ST_GRS_DESCRIPTOR_HEAP
{
    UINT                                m_nSRVOffset;       // SRV Offset
    //UINT                                m_nCnt;
    //UINT                                m_nCBVCnt;
    //UINT                                m_nSRVCnt;
    //UINT                                m_nSmpCnt;

    ComPtr<ID3D12DescriptorHeap>		m_pICBVSRVHeap;     // CBV SRV Heap
    ComPtr<ID3D12DescriptorHeap>		m_pISAPHeap;        // Sample Heap
};

//#pragma pack() 
//#pragma pack(push,4)
//#pragma pack(show)
// Scene Matrix
struct ST_GRS_CB_SCENE_MATRIX
{
    XMMATRIX m_mxWorld;
    XMMATRIX m_mxView;
    XMMATRIX m_mxProj;
    XMMATRIX m_mxWorldView;
    XMMATRIX m_mxWVP;
    XMMATRIX m_mxInvWVP;
    XMMATRIX m_mxVP;
    XMMATRIX m_mxInvVP;
};

// GS 6 Direction Views
struct ST_GRS_CB_GS_VIEW_MATRIX
{
    XMMATRIX m_mxGSCubeView[GRS_CUBE_MAP_FACE_CNT]; // Cube Map 6�� �ӷ���ľ���
};

// Material
struct ST_GRS_PBR_MATERIAL
{
    float       m_fMetallic;   // ������
    float       m_fRoughness;  // �ֲڶ�
    float       m_fAO;		   // �����ڵ�ϵ��
    XMVECTOR    m_v4Albedo;    // ������
};

// Camera
struct ST_GRS_SCENE_CAMERA
{
    XMVECTOR m_v4EyePos;
    XMVECTOR m_v4LookAt;
    XMVECTOR m_v4UpDir;
};

// lights
struct ST_GRS_SCENE_LIGHTS
{
    XMVECTOR m_v4LightPos[GRS_LIGHT_COUNT];
    XMVECTOR m_v4LightColors[GRS_LIGHT_COUNT];
};

struct ST_GRS_VERTEX_CUBE_BOX
{
    XMFLOAT4 m_v4Position;
};

struct ST_GRS_VERTEX_QUAD
{// ע���������ṹ����Color����˼
 // ��Ϊ�˷���UI��Ⱦʱֱ����Ⱦһ����ɫ�������
 // �����Ϳ��Լ�����ɫ���Texture���������ٶ������Ľṹ��
 // Ҳ������ϵ�HGE�����¾�ѧϰ
    XMFLOAT4 m_v4Position;	//Position
    XMFLOAT4 m_v4Color;		//Color
    XMFLOAT2 m_v2UV;		//Texcoord
};

struct ST_GRS_VERTEX_QUAD_SIMPLE
{
    XMFLOAT4 m_v4Position;	//Position
    XMFLOAT2 m_v2UV;		//Texcoord
};

//#pragma pack(pop)
//#pragma pack(show)

struct ST_GRS_SCENE_CONST_DATA
{
    ST_GRS_CB_SCENE_MATRIX* m_pstSceneMatrix;
    ST_GRS_CB_GS_VIEW_MATRIX* m_pstGSViewMatrix;
    ST_GRS_SCENE_CAMERA* m_pstSceneCamera;
    ST_GRS_SCENE_LIGHTS* m_pstSceneLights;
    ST_GRS_PBR_MATERIAL* m_pstPBRMaterial;

    ComPtr<ID3D12Resource>      m_pISceneMatrixBuffer;
    ComPtr<ID3D12Resource>      m_pISceneCameraBuffer;
    ComPtr<ID3D12Resource>      m_pISceneLightsBuffer;
    ComPtr<ID3D12Resource>      m_pIGSMatrixBuffer;
    ComPtr<ID3D12Resource>      m_pIPBRMaterialBuffer;
};

struct ST_GRS_TRIANGLE_LIST_DATA
{
    UINT                        m_nVertexCount;
    UINT                        m_nIndexCount;

    D3D12_VERTEX_BUFFER_VIEW    m_stVBV = {};
    D3D12_INDEX_BUFFER_VIEW     m_stIBV = {};

    ComPtr<ID3D12Resource>      m_pIVB;
    ComPtr<ID3D12Resource>      m_pIIB;

    ComPtr<ID3D12Resource>      m_pITexture;
};

struct ST_GRS_TRIANGLE_STRIP_DATA
{
    UINT                        m_nVertexCount;
    D3D12_VERTEX_BUFFER_VIEW    m_stVertexBufferView;
    ComPtr<ID3D12Resource>      m_pIVertexBuffer;

    ComPtr<ID3D12Resource>      m_pITexture;
    ComPtr<ID3D12Resource>      m_pITextureUpload;
};

#define GRS_INSTANCE_DATA_SLOT_CNT 2

struct ST_GRS_TRIANGLE_STRIP_DATA_WITH_INSTANCE
{
    UINT                                m_nVertexCount;
    UINT                                m_nInstanceCount;
    D3D12_VERTEX_BUFFER_VIEW            m_pstVertexBufferView[GRS_INSTANCE_DATA_SLOT_CNT];
    ComPtr<ID3D12Resource>              m_ppIVertexBuffer[GRS_INSTANCE_DATA_SLOT_CNT];
    CAtlArray<ComPtr<ID3D12Resource>>   m_ppITexture;
};

struct ST_GRS_TRIANGLE_LIST_DATA_WITH_INSTANCE
{
    UINT                                m_nIndexCount;
    UINT                                m_nInstanceCount;
    D3D12_INDEX_BUFFER_VIEW             m_stIBV;
    ComPtr<ID3D12Resource>              m_pIIB;
    D3D12_VERTEX_BUFFER_VIEW            m_pstVBV[GRS_INSTANCE_DATA_SLOT_CNT];
    ComPtr<ID3D12Resource>              m_ppIVB[GRS_INSTANCE_DATA_SLOT_CNT];

    CAtlArray<ComPtr<ID3D12Resource>>   m_ppITexture;
};

struct ST_GRS_MESH_DATA_MULTI_SLOT
{
    UINT                                m_nIndexCount;
    DXGI_FORMAT                         m_emIndexType;

    D3D12_INDEX_BUFFER_VIEW             m_stIBV;
    ComPtr<ID3D12Resource>              m_pIIB;

    CAtlArray<D3D12_VERTEX_BUFFER_VIEW> m_arVBV;
    CAtlArray<ComPtr<ID3D12Resource>>   m_arIVB;
};

struct ST_GRS_PER_QUAD_INSTANCE_DATA
{
    XMMATRIX m_mxQuad2World;
    UINT     m_nTextureIndex;
};

struct ST_GRS_HDR_TEXTURE_DATA
{
    TCHAR                           m_pszHDRFile[MAX_PATH];
    INT                             m_iHDRWidth;
    INT                             m_iHDRHeight;
    INT                             m_iHDRPXComponents;
    FLOAT* m_pfHDRData;

    DXGI_FORMAT                     m_emHDRFormat;
    ComPtr<ID3D12Resource>          m_pIHDRTextureRaw;
    ComPtr<ID3D12Resource>          m_pIHDRTextureRawUpload;
};

#define GRS_MODEL_DATA_CB_CNT 1
#define GRS_PBR_MATERIAL_TEXTURE_CNT 6

struct ST_GRS_MODEL_DATA
{
    XMMATRIX m_mxModel2World;
};

struct ST_GRS_SCENE_PBR_MATERIAL_MODEL_DATA
{
    union
    {
        struct
        {
            TCHAR m_pszAlbedo[MAX_PATH];
            TCHAR m_pszNormal[MAX_PATH];
            TCHAR m_pszDisplacement[MAX_PATH];
            TCHAR m_pszMetallic[MAX_PATH];
            TCHAR m_pszRoughness[MAX_PATH];
            TCHAR m_pszAO[MAX_PATH];
        };
        TCHAR m_pszPBRMaterialPathName[GRS_PBR_MATERIAL_TEXTURE_CNT][MAX_PATH];
    };
    ST_GRS_MODEL_DATA* m_pstModelData;
    ComPtr<ID3D12DescriptorHeap> m_pIMaterialHeap;
    CAtlArray<ComPtr<ID3D12Resource>> m_arTexMaterial;
    CAtlArray<ComPtr<ID3D12Resource>> m_arTexMaterialUpload;
    ComPtr<ID3D12Resource> m_pICBModelData;
};

struct ST_GRS_CAMERA
{
    XMVECTOR m_v4EyePos;
    XMVECTOR m_v4LookDir;
    XMVECTOR m_v4UpDir;

    float    m_fNear;
    float    m_fFar;
};

struct ST_GRS_CAMERA_CONTROL
{
    float    m_fRadius;

    XMVECTOR m_qCameraStart;
    XMVECTOR m_qCameraMoves;
    XMVECTOR m_vCameraPosStart;
    XMVECTOR m_vCameraPosMoves;
};

ST_GRS_CAMERA g_stCamera = {};
ST_GRS_CAMERA_CONTROL g_stCameraControl = {};

BOOL     g_bWorldRotate = TRUE;
// ��������ת���ٶȣ�����/s��
float    g_fPalstance = 15.0f;
// ������ת�ĽǶȣ���ת��
float    g_fWorldRotateAngle = 0;

BOOL g_bShowRTA = TRUE;
BOOL g_bUseLight = FALSE;

// �������ʵ����Է���������
XMFLOAT4 g_v4Albedo[]
= { {0.02f, 0.02f, 0.02f,1.0f},  //ˮ
    {0.03f, 0.03f, 0.03f,1.0f},  //���� / �������ͣ�
    {0.05f, 0.05f, 0.05f,1.0f},  //���ϣ��ߣ�
    {0.08f, 0.08f, 0.08f,1.0f},  //�������ߣ� / �챦ʯ
    {0.17f, 0.17f, 0.17f,1.0f},  //��ʯ
    {0.56f, 0.57f, 0.58f,1.0f},  //��
    {0.95f, 0.64f, 0.54f,1.0f},  //ͭ
    {1.00f, 0.71f, 0.29f,1.0f},  //��
    {0.91f, 0.92f, 0.92f,1.0f},  //��
    {0.95f, 0.93f, 0.88f,1.0f}   //��
};

float g_fScale = 1.0f;                      // ������������ű��� Z���Ŵ� C����С

D3D12_HEAP_PROPERTIES                       g_stDefaultHeapProps = {};
D3D12_HEAP_PROPERTIES                       g_stUploadHeapProps = {};
D3D12_RESOURCE_DESC                         g_stBufferResSesc = {};

ST_GRS_D3D12_DEVICE                         g_stD3DDevice = {};

ST_GRS_SCENE_CONST_DATA                     g_stWorldConstData = {};
ST_GRS_SCENE_CONST_DATA                     g_stQuadConstData = {};

ST_GRS_HDR_TEXTURE_DATA                     g_stHDRData = {};

ST_GRS_RENDER_TARGET_ARRAY_DATA             g_st1TimesGSRTAStatus = {};
ST_GRS_DESCRIPTOR_HEAP                      g_st1TimesGSHeap = {};
ST_GRS_PSO                                  g_st1TimesGSPSO = {};
ST_GRS_TRIANGLE_STRIP_DATA                  g_stBoxData_Strip = {};

ST_GRS_RENDER_TARGET_ARRAY_DATA             g_st6TimesRTAStatus = {};
ST_GRS_DESCRIPTOR_HEAP                      g_stHDR2CubemapHeap = {};
ST_GRS_PSO                                  g_stHDR2CubemapPSO = {};
ST_GRS_TRIANGLE_LIST_DATA                   g_stBoxData_Index = {};

// IBL ������Ԥ���ּ�����Ҫ�����ݽṹ
ST_GRS_RENDER_TARGET_ARRAY_DATA             g_stIBLDiffusePreIntRTA = {};
ST_GRS_PSO                                  g_stIBLDiffusePreIntPSO = {};

// IBL ���淴��Ԥ���ּ�����Ҫ�����ݽṹ
ST_GRS_RENDER_TARGET_ARRAY_DATA             g_stIBLSpecularPreIntRTA = {};
ST_GRS_PSO                                  g_stIBLSpecularPreIntPSO = {};

// IBL ���淴��LUT������Ҫ�����ݽṹ
ST_GRS_RENDER_TARGET_ARRAY_DATA             g_stIBLLutRTA = {};
ST_GRS_PSO                                  g_stIBLLutPSO = {};
ST_GRS_TRIANGLE_STRIP_DATA                  g_stIBLLutData = {};

ST_GRS_DESCRIPTOR_HEAP                      g_stIBLHeap = {};
ST_GRS_PSO                                  g_stIBLPSO = {};
ST_GRS_MESH_DATA_MULTI_SLOT                 g_stMultiMeshData = {};
CAtlArray<ST_GRS_SCENE_PBR_MATERIAL_MODEL_DATA>   g_arPBRMaterial;

ST_GRS_DESCRIPTOR_HEAP                      g_stSkyBoxHeap = {};
ST_GRS_PSO                                  g_stSkyBoxPSO = {};
ST_GRS_TRIANGLE_STRIP_DATA                  g_stSkyBoxData = {};

ST_GRS_DESCRIPTOR_HEAP                      g_stQuadHeap = {};
ST_GRS_PSO                                  g_stQuadPSO = {};
ST_GRS_TRIANGLE_STRIP_DATA_WITH_INSTANCE    g_stQuadData = {};



TCHAR    g_pszAppPath[MAX_PATH] = {};
TCHAR    g_pszShaderPath[MAX_PATH] = {};
TCHAR    g_pszHDRFilePath[MAX_PATH] = {};

void OnSize(UINT width, UINT height, bool minimized);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR    lpCmdLine, int nCmdShow)
{
    USES_CONVERSION;
    try
    {
        ::CoInitialize(nullptr);  //for WIC & COM

        // 0-1����ʼ��ȫ�ֲ���
        {
            g_stD3DDevice.m_iWndWidth = 1280;
            g_stD3DDevice.m_iWndHeight = 800;
            g_stD3DDevice.m_hWnd = nullptr;
            g_stD3DDevice.m_nFrameIndex = 0;
            g_stD3DDevice.m_emRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            g_stD3DDevice.m_emDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
            g_stD3DDevice.m_nRTVDescriptorSize = 0U;
            g_stD3DDevice.m_nDSVDescriptorSize = 0U;
            g_stD3DDevice.m_nSamplerDescriptorSize = 0u;
            g_stD3DDevice.m_nCBVSRVDescriptorSize = 0u;
            g_stD3DDevice.m_stViewPort = { 0.0f, 0.0f, static_cast<float>(g_stD3DDevice.m_iWndWidth), static_cast<float>(g_stD3DDevice.m_iWndHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
            g_stD3DDevice.m_stScissorRect = { 0, 0, static_cast<LONG>(g_stD3DDevice.m_iWndWidth), static_cast<LONG>(g_stD3DDevice.m_iWndHeight) };
            g_stD3DDevice.m_n64FenceValue = 0ui64;
            g_stD3DDevice.m_hEventFence = nullptr;

            g_stDefaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
            g_stDefaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            g_stDefaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            g_stDefaultHeapProps.CreationNodeMask = 0;
            g_stDefaultHeapProps.VisibleNodeMask = 0;

            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            g_stUploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            g_stUploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            g_stUploadHeapProps.CreationNodeMask = 0;
            g_stUploadHeapProps.VisibleNodeMask = 0;

            g_stBufferResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            g_stBufferResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            g_stBufferResSesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            g_stBufferResSesc.Format = DXGI_FORMAT_UNKNOWN;
            g_stBufferResSesc.Width = 0;
            g_stBufferResSesc.Height = 1;
            g_stBufferResSesc.DepthOrArraySize = 1;
            g_stBufferResSesc.MipLevels = 1;
            g_stBufferResSesc.SampleDesc.Count = 1;
            g_stBufferResSesc.SampleDesc.Quality = 0;
        }

        // 0-2���õ���ǰ�Ĺ���Ŀ¼����������ʹ�����·�������ʸ�����Դ�ļ�
        {
            if (0 == ::GetModuleFileName(nullptr, g_pszAppPath, MAX_PATH))
            {
                GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
            }

            WCHAR* lastSlash = _tcsrchr(g_pszAppPath, _T('\\'));
            if (lastSlash)
            {//ɾ��Exe�ļ���
                *(lastSlash) = _T('\0');
            }

            lastSlash = _tcsrchr(g_pszAppPath, _T('\\'));
            if (lastSlash)
            {//ɾ��x64·��
                *(lastSlash) = _T('\0');
            }

            lastSlash = _tcsrchr(g_pszAppPath, _T('\\'));
            if (lastSlash)
            {//ɾ��Debug �� Release·��
                *(lastSlash + 1) = _T('\0');
            }

            ::StringCchPrintf(g_pszShaderPath
                , MAX_PATH
                , _T("%s27-IBL-With-Material-Texture\\Shader")
                , g_pszAppPath);
        }

        // 0-3��׼�����Shaderʱ��������ļ����༰·��        
        TCHAR pszPublicShaderPath[MAX_PATH] = {};
        ::StringCchPrintf(pszPublicShaderPath
            , MAX_PATH
            , _T("%sShader")
            , g_pszAppPath);
        CGRSD3DCompilerInclude grsD3DCompilerInclude(pszPublicShaderPath);
        grsD3DCompilerInclude.AddDir(g_pszShaderPath);

        // 0-4����ʼ�����������
        {
            g_stCamera.m_v4EyePos = XMVectorSet(0.0f, 1.0f, -10.0f, 1.0f); // λ��
            g_stCamera.m_v4LookDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  // ����
            g_stCamera.m_v4UpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    // �Ϸ�
            g_stCamera.m_fNear = 0.1f;
            g_stCamera.m_fFar = 200000.0f;  //��������ֵ��Ϊ�� skybox 

            // �������������뾶
            g_stCameraControl.m_fRadius = 100.0f;
            // ��ʼ�������ת������Ԫ��
            g_stCameraControl.m_qCameraStart = XMQuaternionIdentity();
            g_stCameraControl.m_qCameraMoves = g_stCameraControl.m_qCameraStart;
        }

        // 1-1����������
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
            wcex.lpszClassName = GRS_WND_CLASS;

            ::RegisterClassEx(&wcex);

            g_stD3DDevice.m_dwWndStyle = WS_OVERLAPPEDWINDOW;

            RECT rtWnd = { 0, 0, g_stD3DDevice.m_iWndWidth, g_stD3DDevice.m_iWndHeight };
            ::AdjustWindowRect(&rtWnd, g_stD3DDevice.m_dwWndStyle, FALSE);

            // ���㴰�ھ��е���Ļ����
            INT posX = (::GetSystemMetrics(SM_CXSCREEN) - rtWnd.right - rtWnd.left) / 2;
            INT posY = (::GetSystemMetrics(SM_CYSCREEN) - rtWnd.bottom - rtWnd.top) / 2;

            g_stD3DDevice.m_hWnd = ::CreateWindowW(GRS_WND_CLASS
                , GRS_WND_TITLE
                , g_stD3DDevice.m_dwWndStyle
                , posX
                , posY
                , rtWnd.right - rtWnd.left
                , rtWnd.bottom - rtWnd.top
                , nullptr
                , nullptr
                , hInstance
                , nullptr);

            if (!g_stD3DDevice.m_hWnd)
            {
                GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        // 2-1������DXGI Factory����
        {
            UINT nDXGIFactoryFlags = 0U;
#if defined(_DEBUG)
            // ����ʾ��ϵͳ�ĵ���֧��
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
                // �򿪸��ӵĵ���֧��
                nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
#endif
            ComPtr<IDXGIFactory5> pIDXGIFactory5;
            GRS_THROW_IF_FAILED(::CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory5)));
            GRS_SET_DXGI_DEBUGNAME_COMPTR(pIDXGIFactory5);

            //��ȡIDXGIFactory6�ӿ�
            GRS_THROW_IF_FAILED(pIDXGIFactory5.As(&g_stD3DDevice.m_pIDXGIFactory6));
            GRS_SET_DXGI_DEBUGNAME_COMPTR(g_stD3DDevice.m_pIDXGIFactory6);

            // ���Tearing֧��
            HRESULT hr = g_stD3DDevice.m_pIDXGIFactory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING
                , &g_stD3DDevice.m_bSupportTearing
                , sizeof(g_stD3DDevice.m_bSupportTearing));
            g_stD3DDevice.m_bSupportTearing = SUCCEEDED(hr) && g_stD3DDevice.m_bSupportTearing;
        }

        // 2-2��ö�������������豸
        { //ѡ������ܵ��Կ�������3D�豸����
            ComPtr<IDXGIAdapter1> pIAdapter1;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIDXGIFactory6->EnumAdapterByGpuPreference(0
                , DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE
                , IID_PPV_ARGS(&pIAdapter1)));
            GRS_SET_DXGI_DEBUGNAME_COMPTR(pIAdapter1);

            // ����D3D12.1���豸
            GRS_THROW_IF_FAILED(D3D12CreateDevice(pIAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&g_stD3DDevice.m_pID3D12Device4)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stD3DDevice.m_pID3D12Device4);


            DXGI_ADAPTER_DESC1 stAdapterDesc = {};
            TCHAR pszWndTitle[MAX_PATH] = {};
            GRS_THROW_IF_FAILED(pIAdapter1->GetDesc1(&stAdapterDesc));
            ::GetWindowText(g_stD3DDevice.m_hWnd, pszWndTitle, MAX_PATH);
            StringCchPrintf(pszWndTitle
                , MAX_PATH
                , _T("%s(GPU[0]:%s)")
                , pszWndTitle
                , stAdapterDesc.Description);
            ::SetWindowText(g_stD3DDevice.m_hWnd, pszWndTitle);
        }

        // 2-3���õ�ÿ��������Ԫ�صĴ�С
        {
            g_stD3DDevice.m_nRTVDescriptorSize
                = g_stD3DDevice.m_pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            g_stD3DDevice.m_nDSVDescriptorSize
                = g_stD3DDevice.m_pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            g_stD3DDevice.m_nSamplerDescriptorSize
                = g_stD3DDevice.m_pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            g_stD3DDevice.m_nCBVSRVDescriptorSize
                = g_stD3DDevice.m_pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        // 2-4�������������&�����б�
        {
            D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
            stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&g_stD3DDevice.m_pIMainCMDQueue)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stD3DDevice.m_pIMainCMDQueue);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT
                , IID_PPV_ARGS(&g_stD3DDevice.m_pIMainCMDAlloc)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stD3DDevice.m_pIMainCMDAlloc);

            // ����ͼ�������б�
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandList(
                0
                , D3D12_COMMAND_LIST_TYPE_DIRECT
                , g_stD3DDevice.m_pIMainCMDAlloc.Get()
                , nullptr
                , IID_PPV_ARGS(&g_stD3DDevice.m_pIMainCMDList)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stD3DDevice.m_pIMainCMDList);
        }

        // 2-5������������
        {
            DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
            stSwapChainDesc.BufferCount = g_nFrameBackBufCount;
            stSwapChainDesc.Width = g_stD3DDevice.m_iWndWidth;
            stSwapChainDesc.Height = g_stD3DDevice.m_iWndHeight;
            stSwapChainDesc.Format = g_stD3DDevice.m_emRenderTargetFormat;
            stSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            stSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            stSwapChainDesc.SampleDesc.Count = 1;
            stSwapChainDesc.Scaling = DXGI_SCALING_NONE;

            // ��Ⲣ����Tearing֧��
            stSwapChainDesc.Flags = g_stD3DDevice.m_bSupportTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

            ComPtr<IDXGISwapChain1> pISwapChain1;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIDXGIFactory6->CreateSwapChainForHwnd(
                g_stD3DDevice.m_pIMainCMDQueue.Get(),		// ��������Ҫ������У�Present����Ҫִ��
                g_stD3DDevice.m_hWnd,
                &stSwapChainDesc,
                nullptr,
                nullptr,
                &pISwapChain1
            ));
            GRS_SET_DXGI_DEBUGNAME_COMPTR(pISwapChain1);

            GRS_THROW_IF_FAILED(pISwapChain1.As(&g_stD3DDevice.m_pISwapChain3));
            GRS_SET_DXGI_DEBUGNAME_COMPTR(g_stD3DDevice.m_pISwapChain3);

            //�õ���ǰ�󻺳�������ţ�Ҳ������һ����Ҫ������ʾ�Ļ����������
            //ע��˴�ʹ���˸߰汾��SwapChain�ӿڵĺ���
            g_stD3DDevice.m_nFrameIndex = g_stD3DDevice.m_pISwapChain3->GetCurrentBackBufferIndex();

            //����RTV(��ȾĿ����ͼ)��������(����ѵĺ���Ӧ�����Ϊ������߹̶���СԪ�صĹ̶���С�Դ��)
            {
                D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
                stRTVHeapDesc.NumDescriptors = g_nFrameBackBufCount;
                stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(
                    &stRTVHeapDesc
                    , IID_PPV_ARGS(&g_stD3DDevice.m_pIRTVHeap)));
                GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stD3DDevice.m_pIRTVHeap);

            }

            //����RTV��������
            {
                D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                    = g_stD3DDevice.m_pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
                for (UINT i = 0; i < g_nFrameBackBufCount; i++)
                {
                    GRS_THROW_IF_FAILED(g_stD3DDevice.m_pISwapChain3->GetBuffer(i
                        , IID_PPV_ARGS(&g_stD3DDevice.m_pIARenderTargets[i])));

                    GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(g_stD3DDevice.m_pIARenderTargets, i);

                    g_stD3DDevice.m_pID3D12Device4->CreateRenderTargetView(g_stD3DDevice.m_pIARenderTargets[i].Get()
                        , nullptr
                        , stRTVHandle);

                    stRTVHandle.ptr += g_stD3DDevice.m_nRTVDescriptorSize;
                }
            }

            if (g_stD3DDevice.m_bSupportTearing)
            {// ���֧��Tearing����ô�͹ر�ϵͳ��ALT + Enteryȫ����֧�֣������������Լ�����
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIDXGIFactory6->MakeWindowAssociation(g_stD3DDevice.m_hWnd, DXGI_MWA_NO_ALT_ENTER));
            }

        }

        // 2-6��������Ȼ��弰��Ȼ�����������
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC   stDepthStencilDesc = {};
            D3D12_CLEAR_VALUE               stDepthOptimizedClearValue = {};
            D3D12_RESOURCE_DESC             stDSTex2DDesc = {};
            D3D12_DESCRIPTOR_HEAP_DESC      stDSVHeapDesc = {};

            stDepthStencilDesc.Format = g_stD3DDevice.m_emDepthStencilFormat;
            stDepthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            stDepthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

            stDepthOptimizedClearValue.Format = g_stD3DDevice.m_emDepthStencilFormat;
            stDepthOptimizedClearValue.DepthStencil.Depth = 1.0f;
            stDepthOptimizedClearValue.DepthStencil.Stencil = 0;

            stDSTex2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            stDSTex2DDesc.Alignment = 0;
            stDSTex2DDesc.Width = g_stD3DDevice.m_iWndWidth;
            stDSTex2DDesc.Height = g_stD3DDevice.m_iWndHeight;
            stDSTex2DDesc.DepthOrArraySize = 1;
            stDSTex2DDesc.MipLevels = 0;
            stDSTex2DDesc.Format = g_stD3DDevice.m_emDepthStencilFormat;
            stDSTex2DDesc.SampleDesc.Count = 1;
            stDSTex2DDesc.SampleDesc.Quality = 0;
            stDSTex2DDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            stDSTex2DDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            stDSVHeapDesc.NumDescriptors = 1;
            stDSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            stDSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            //ʹ����ʽĬ�϶Ѵ���һ��������建������
            //��Ϊ��������Ȼ�������һֱ��ʹ�ã����õ����岻������ֱ��ʹ����ʽ�ѣ�ͼ����
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stDefaultHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &stDSTex2DDesc
                , D3D12_RESOURCE_STATE_DEPTH_WRITE
                , &stDepthOptimizedClearValue
                , IID_PPV_ARGS(&g_stD3DDevice.m_pIDepthStencilBuffer)
            ));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stD3DDevice.m_pIDepthStencilBuffer);
            //==============================================================================================

            // ���� DSV Heap
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stDSVHeapDesc, IID_PPV_ARGS(&g_stD3DDevice.m_pIDSVHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stD3DDevice.m_pIDSVHeap);
            // ���� DSV
            g_stD3DDevice.m_pID3D12Device4->CreateDepthStencilView(g_stD3DDevice.m_pIDepthStencilBuffer.Get()
                , &stDepthStencilDesc
                , g_stD3DDevice.m_pIDSVHeap->GetCPUDescriptorHandleForHeapStart());
        }

        // 2-7������Χ��
        {
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateFence(0
                , D3D12_FENCE_FLAG_NONE
                , IID_PPV_ARGS(&g_stD3DDevice.m_pIFence)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stD3DDevice.m_pIFence);

            // ��ʼ�� Fence ����Ϊ 1
            g_stD3DDevice.m_n64FenceValue = 1;

            // ����һ��Eventͬ���������ڵȴ�Χ���¼�֪ͨ
            // ��������Ϊ�� CPU �� GPU ��ͬ��
            g_stD3DDevice.m_hEventFence = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (g_stD3DDevice.m_hEventFence == nullptr)
            {
                GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        // 2-8������ǩ���汾������ֻ֧��1.1���ϵģ��Ͱ汾�ľͶ����˳��ˣ���Ȼ���ݵĴ���̫������Ҳ����Ӫ��
        {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
            // ����Ƿ�֧��V1.1�汾�ĸ�ǩ��
            stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
            if (FAILED(g_stD3DDevice.m_pID3D12Device4->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE
                , &stFeatureData
                , sizeof(stFeatureData))))
            { // 1.0�� ֱ�Ӷ��쳣�˳���
                ATLTRACE("ϵͳ��֧��V1.1�ĸ�ǩ����ֱ���˳��ˣ���Ҫ���������޸Ĵ�����ǩ���Ĵ����1.0�档");
                GRS_THROW_IF_FAILED(E_NOTIMPL);
            }
        }

        //---------------------------------------------------------------------------------------
        // 3��ʹ��HDR �Ⱦ����滷����ͼ���� ��������ͼ��������
        // ��һ�ַ�ʽ�� ʹ��GS Render Target Array����һ����Ⱦ��������        
        {}
        // 3-1-0����ʼ��Render Target����
        {
            g_st1TimesGSRTAStatus.m_nRTWidth = GRS_RTA_DEFAULT_SIZE_W;
            g_st1TimesGSRTAStatus.m_nRTHeight = GRS_RTA_DEFAULT_SIZE_H;
            g_st1TimesGSRTAStatus.m_stViewPort = { 0.0f
                , 0.0f
                , static_cast<float>(g_st1TimesGSRTAStatus.m_nRTWidth)
                , static_cast<float>(g_st1TimesGSRTAStatus.m_nRTHeight)
                , D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
            g_st1TimesGSRTAStatus.m_stScissorRect = { 0
                , 0
                , static_cast<LONG>(g_st1TimesGSRTAStatus.m_nRTWidth)
                , static_cast<LONG>(g_st1TimesGSRTAStatus.m_nRTHeight) };
            g_st1TimesGSRTAStatus.m_emRTAFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        // 3-1-1���������� Geometry shader ��Ⱦ��Cube Map��PSO
        {
            g_st1TimesGSPSO.m_nCBVCnt = 2;
            g_st1TimesGSPSO.m_nSRVCnt = 1;
            g_st1TimesGSPSO.m_nSmpCnt = 1;
            g_st1TimesGSPSO.m_nCnt = g_st1TimesGSPSO.m_nCBVCnt + g_st1TimesGSPSO.m_nSRVCnt;

            D3D12_DESCRIPTOR_RANGE1 stDSPRanges[3] = {};
            // 2 Const Buffer ( MVP Matrix + Cube Map 6 View Matrix)
            stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            stDSPRanges[0].NumDescriptors = g_st1TimesGSPSO.m_nCBVCnt;
            stDSPRanges[0].BaseShaderRegister = 0;
            stDSPRanges[0].RegisterSpace = 0;
            stDSPRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

            // 1 Texture ( HDR Texture ) 
            stDSPRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges[1].NumDescriptors = g_st1TimesGSPSO.m_nSRVCnt;
            stDSPRanges[1].BaseShaderRegister = 0;
            stDSPRanges[1].RegisterSpace = 0;
            stDSPRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[1].OffsetInDescriptorsFromTableStart = 0;

            // 1 Sampler ( Linear Sampler )
            stDSPRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            stDSPRanges[2].NumDescriptors = g_st1TimesGSPSO.m_nSmpCnt;
            stDSPRanges[2].BaseShaderRegister = 0;
            stDSPRanges[2].RegisterSpace = 0;
            stDSPRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[2].OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 stRootParameters[3] = {};
            stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//CBV������Shader�ɼ�
            stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

            stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SRV��PS�ɼ�
            stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

            stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SAMPLE��PS�ɼ�
            stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
            stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
            stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
            stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&g_st1TimesGSPSO.m_pIRS)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st1TimesGSPSO.m_pIRS);

            //--------------------------------------------------------------------------------------------------------------------------------
            UINT nCompileFlags = 0;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            //����Ϊ�о�����ʽ	   
            nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

            TCHAR pszShaderFileName[MAX_PATH] = {};

            ComPtr<ID3DBlob> pIVSCode;
            ComPtr<ID3DBlob> pIGSCode;
            ComPtr<ID3DBlob> pIPSCode;
            ComPtr<ID3DBlob> pIErrorMsg;

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\1-2 GRS_1Times_GS_HDR_2_CubeMap_VS_GS.hlsl"), g_pszShaderPath);

            HRESULT hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "VSMain", "vs_5_0", nCompileFlags, 0, &pIVSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Vertex Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            hr = ::D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "GSMain", "gs_5_0", nCompileFlags, 0, &pIGSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Geometry Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\1-3 GRS_HDR_Spherical_Map_2_Cubemap_PS.hlsl")
                , g_pszShaderPath);

            hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "PSMain", "ps_5_0", nCompileFlags, 0, &pIPSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Pixel Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // �������붥������ݽṹ
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };

            stPSODesc.pRootSignature = g_st1TimesGSPSO.m_pIRS.Get();

            // VS -> GS -> PS
            stPSODesc.VS.BytecodeLength = pIVSCode->GetBufferSize();
            stPSODesc.VS.pShaderBytecode = pIVSCode->GetBufferPointer();
            stPSODesc.GS.BytecodeLength = pIGSCode->GetBufferSize();
            stPSODesc.GS.pShaderBytecode = pIGSCode->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIPSCode->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIPSCode->GetBufferPointer();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            // ע��رձ����޳�����Ϊ�����Triangle Strip Box�Ķ����з����
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.SampleDesc.Count = 1;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = g_st1TimesGSRTAStatus.m_emRTAFormat;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;
            stPSODesc.DepthStencilState.DepthEnable = FALSE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&g_st1TimesGSPSO.m_pIPSO)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st1TimesGSPSO.m_pIPSO);

            // ���� CBV SRV Heap
            D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
            stSRVHeapDesc.NumDescriptors = 6; // 5 CBV + 1 SRV
            stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSRVHeapDesc
                , IID_PPV_ARGS(&g_st1TimesGSHeap.m_pICBVSRVHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st1TimesGSHeap.m_pICBVSRVHeap);

            g_st1TimesGSHeap.m_nSRVOffset = 5;      //��6����SRV

            // ���� Sample Heap
            D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
            stSamplerHeapDesc.NumDescriptors = g_st1TimesGSPSO.m_nSmpCnt;   // 1 Sample
            stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSamplerHeapDesc
                , IID_PPV_ARGS(&g_st1TimesGSHeap.m_pISAPHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st1TimesGSHeap.m_pISAPHeap);
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 3-1-2����һ��HDR�ļ� ������������
        {
            // ������Ĭ�ϵ� HDR File Path
            StringCchPrintf(g_pszHDRFilePath
                , MAX_PATH
                , _T("%sAssets\\HDR Light\\Tropical_Beach")
                , g_pszAppPath);

            StringCchPrintf(g_stHDRData.m_pszHDRFile
                , MAX_PATH
                , _T("%s\\Tropical_Beach_3k.hdr")
                , g_pszHDRFilePath);

#ifdef GRS_USE_DIR_DIALOG
            OPENFILENAME stOfn = {};

            RtlZeroMemory(&stOfn, sizeof(stOfn));
            stOfn.lStructSize = sizeof(stOfn);
            stOfn.hwndOwner = ::GetDesktopWindow(); //g_stD3DDevice.m_hWnd;
            stOfn.lpstrFilter = _T("HDR �ļ� \0*.hdr\0�����ļ�\0*.*\0");
            stOfn.lpstrFile = g_stHDRData.m_pszHDRFile;
            stOfn.nMaxFile = MAX_PATH;
            stOfn.lpstrTitle = _T("��ѡ��һ�� HDR �ļ�...");
            stOfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (!::GetOpenFileName(&stOfn))
            {
                StringCchPrintf(g_stHDRData.m_pszHDRFile
                    , MAX_PATH
                    , _T("%sAssets\\PBR HDR Lights\\Stadium_Center\\Stadium_Center_3k.hdr")
                    , g_pszAppPath);
            }
            else
            {// �ݴ�һ��·�� �������ѡ����պд�ͼ
                StringCchCopy(g_pszHDRFilePath, MAX_PATH, g_stHDRData.m_pszHDRFile);

                WCHAR* lastSlash = _tcsrchr(g_pszHDRFilePath, _T('\\'));

                if (lastSlash)
                {//ɾ��Exe�ļ���
                    *(lastSlash) = _T('\0');
                }
            }
#endif

            // ���������˼��Ҫ��Ҫ���µߵ�һ��ͼƬ
            // ע����OpenGL�����������ݷ����Ǵ��µ���������Ҫ�ߵ�һ��
            // ������D3D �Ͳ����� ���������ã�
            //stbi_set_flip_vertically_on_load(true);

            g_stHDRData.m_pfHDRData = stbi_loadf(T2A(g_stHDRData.m_pszHDRFile)
                , &g_stHDRData.m_iHDRWidth
                , &g_stHDRData.m_iHDRHeight
                , &g_stHDRData.m_iHDRPXComponents
                , 0);
            if (nullptr == g_stHDRData.m_pfHDRData)
            {
                ATLTRACE(_T("�޷�����HDR�ļ���\"%s\"\n"), g_stHDRData.m_pszHDRFile);
                AtlThrowLastWin32();
            }

            if (3 == g_stHDRData.m_iHDRPXComponents)
            {
                g_stHDRData.m_emHDRFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            }
            else
            {
                ATLTRACE(_T("HDR�ļ���\"%s\"������3����Float��ʽ���޷�ʹ�ã�\n"), g_stHDRData.m_pszHDRFile);
                AtlThrowLastWin32();
            }

            UINT nRowAlign = 8u;
            UINT nBPP = (UINT)g_stHDRData.m_iHDRPXComponents * sizeof(float);

            //UINT nPicRowPitch = (  iHDRWidth  *  nBPP  + 7u ) / 8u;
            UINT nPicRowPitch = GRS_UPPER(g_stHDRData.m_iHDRWidth * nBPP, nRowAlign);

            ID3D12Resource* pITextureUpload = nullptr;
            ID3D12Resource* pITexture = nullptr;
            BOOL bRet = LoadTextureFromMem(g_stD3DDevice.m_pIMainCMDList.Get()
                , (BYTE*)g_stHDRData.m_pfHDRData
                , (size_t)g_stHDRData.m_iHDRWidth * nBPP * g_stHDRData.m_iHDRHeight
                , g_stHDRData.m_emHDRFormat
                , g_stHDRData.m_iHDRWidth
                , g_stHDRData.m_iHDRHeight
                , nPicRowPitch
                , pITextureUpload
                , pITexture
            );

            if (!bRet)
            {
                AtlThrowLastWin32();
            }

            g_stHDRData.m_pIHDRTextureRaw.Attach(pITexture);
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stHDRData.m_pIHDRTextureRaw);
            g_stHDRData.m_pIHDRTextureRawUpload.Attach(pITextureUpload);
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stHDRData.m_pIHDRTextureRawUpload);

            D3D12_CPU_DESCRIPTOR_HANDLE stHeapHandle
                = g_st1TimesGSHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
            stHeapHandle.ptr
                += (g_st1TimesGSHeap.m_nSRVOffset * g_stD3DDevice.m_nCBVSRVDescriptorSize); // ���չ��ߵ�Ҫ��ƫ�Ƶ���ȷ�� SRV ���

            // ���� HDR Texture SRV
            D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
            stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            stSRVDesc.Format = g_stHDRData.m_emHDRFormat;
            stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            stSRVDesc.Texture2D.MipLevels = 1;
            g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(g_stHDRData.m_pIHDRTextureRaw.Get(), &stSRVDesc, stHeapHandle);

            // ����һ�����Թ��˵ı�Ե���еĲ�����
            D3D12_SAMPLER_DESC stSamplerDesc = {};
            stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

            stSamplerDesc.MinLOD = 0;
            stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            stSamplerDesc.MipLODBias = 0.0f;
            stSamplerDesc.MaxAnisotropy = 1;
            stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            stSamplerDesc.BorderColor[0] = 0.0f;
            stSamplerDesc.BorderColor[1] = 0.0f;
            stSamplerDesc.BorderColor[2] = 0.0f;
            stSamplerDesc.BorderColor[3] = 0.0f;
            stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

            g_stD3DDevice.m_pID3D12Device4->CreateSampler(&stSamplerDesc
                , g_st1TimesGSHeap.m_pISAPHeap->GetCPUDescriptorHandleForHeapStart());
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 3-1-3������GS Render Target Array ��Texture2DArray����RTV
        {
            //����RTV(��ȾĿ����ͼ)��������( ����������������������� )
            {
                D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
                stRTVHeapDesc.NumDescriptors = 1;   // 1 Render Target Array RTV
                stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&g_st1TimesGSRTAStatus.m_pIRTAHeap)));
                GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st1TimesGSRTAStatus.m_pIRTAHeap);
            }

            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = g_st1TimesGSRTAStatus.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();

            // ע�����ﴴ����������ȾĿ����Դ��������Texture2DArray���ͣ�������6��SubResource
            // �����ʺ� Geometry Shader һ����Ⱦ��� Render Target Array �ķ�ʽ��
            // �� Geometry Shader ��ʹ��ϵͳ���� SV_RenderTargetArrayIndex ��ʶ Render Target��������
            // ������� Texture Render Target �ķ������������� MRT �� GBuffer �У�
            // һ���� Pixel Shader ��ʹ�� SV_Target(n) ����ʶ����� Render Target������ n Ϊ����
            // �磺SV_Target0��SV_Target1��SV_Target2 ��

            // ����������ȾĿ��(Render Target Array Texture2DArray)
            {
                D3D12_RESOURCE_DESC stRenderTargetDesc = g_stD3DDevice.m_pIARenderTargets[0]->GetDesc();
                stRenderTargetDesc.Format = g_st1TimesGSRTAStatus.m_emRTAFormat;
                stRenderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                stRenderTargetDesc.Width = g_st1TimesGSRTAStatus.m_nRTWidth;
                stRenderTargetDesc.Height = g_st1TimesGSRTAStatus.m_nRTHeight;
                stRenderTargetDesc.DepthOrArraySize = GRS_CUBE_MAP_FACE_CNT;

                D3D12_CLEAR_VALUE stClearValue = {};
                stClearValue.Format = g_st1TimesGSRTAStatus.m_emRTAFormat;
                stClearValue.Color[0] = g_stD3DDevice.m_faClearColor[0];
                stClearValue.Color[1] = g_stD3DDevice.m_faClearColor[1];
                stClearValue.Color[2] = g_stD3DDevice.m_faClearColor[2];
                stClearValue.Color[3] = g_stD3DDevice.m_faClearColor[3];

                // ��������������Դ
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                    &g_stDefaultHeapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &stRenderTargetDesc,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    &stClearValue,
                    IID_PPV_ARGS(&g_st1TimesGSRTAStatus.m_pITextureRTA)));
                GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st1TimesGSRTAStatus.m_pITextureRTA);

                // ������Ӧ��RTV��������
                // �����D3D12_RENDER_TARGET_VIEW_DESC�ṹ����Ҫ��
                // ����ʹ��Texture2DArray��ΪRender Target Arrayʱ����Ҫ��ȷ���Ľṹ�壬
                // ���� Geometry Shader û��ʶ�����Array
                // һ�㴴�� Render Target ʱ���������ṹ��
                // ֱ���� CreateRenderTargetView �ĵڶ�����������nullptr

                D3D12_RENDER_TARGET_VIEW_DESC stRTADesc = {};
                stRTADesc.Format = g_st1TimesGSRTAStatus.m_emRTAFormat;
                stRTADesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                stRTADesc.Texture2DArray.ArraySize = GRS_CUBE_MAP_FACE_CNT;

                g_stD3DDevice.m_pID3D12Device4->CreateRenderTargetView(g_st1TimesGSRTAStatus.m_pITextureRTA.Get(), &stRTADesc, stRTVHandle);

                D3D12_RESOURCE_DESC stDownloadResDesc = {};
                stDownloadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                stDownloadResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
                stDownloadResDesc.Width = 0;
                stDownloadResDesc.Height = 1;
                stDownloadResDesc.DepthOrArraySize = 1;
                stDownloadResDesc.MipLevels = 1;
                stDownloadResDesc.Format = DXGI_FORMAT_UNKNOWN;
                stDownloadResDesc.SampleDesc.Count = 1;
                stDownloadResDesc.SampleDesc.Quality = 0;
                stDownloadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                stDownloadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

                UINT64 n64ResBufferSize = 0;

                g_stD3DDevice.m_pID3D12Device4->GetCopyableFootprints(&stRenderTargetDesc
                    , 0, GRS_CUBE_MAP_FACE_CNT, 0, nullptr, nullptr, nullptr, &n64ResBufferSize);

                stDownloadResDesc.Width = n64ResBufferSize;

                // �����������Ҫ����Ⱦ�Ľ�����Ƴ�������Ҫ�Ļ���Ҫ�洢��ָ�����ļ��������ȱ��Read Back��
                g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_READBACK;
                // Download Heap == Upload Heap ���ǹ����ڴ棡
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                    &g_stUploadHeapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &stDownloadResDesc,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_PPV_ARGS(&g_st1TimesGSRTAStatus.m_pITextureRTADownload)));
                GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st1TimesGSRTAStatus.m_pITextureRTADownload);

                // ��ԭΪ�ϴ������ԣ���������Ҫ��
                g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            }
        }

        // 3-1-4��׼��������
        {
            // ���������ε�3D���ݽṹ
            ST_GRS_VERTEX_CUBE_BOX pstVertices[] = {
                { { -1.0f,  1.0f, -1.0f, 1.0f} },
                { { -1.0f, -1.0f, -1.0f, 1.0f} },

                { {  1.0f,  1.0f, -1.0f, 1.0f} },
                { {  1.0f, -1.0f, -1.0f, 1.0f} },

                { {  1.0f,  1.0f,  1.0f, 1.0f} },
                { {  1.0f, -1.0f,  1.0f, 1.0f} },

                { { -1.0f,  1.0f,  1.0f, 1.0f} },
                { { -1.0f, -1.0f,  1.0f, 1.0f} },

                { { -1.0f, 1.0f, -1.0f, 1.0f} },
                { { -1.0f, -1.0f, -1.0f, 1.0f} },

                { { -1.0f, -1.0f,  1.0f, 1.0f} },
                { {  1.0f, -1.0f, -1.0f, 1.0f} },

                { {  1.0f, -1.0f,  1.0f, 1.0f} },
                { {  1.0f,  1.0f,  1.0f, 1.0f} },

                { { -1.0f,  1.0f,  1.0f, 1.0f} },
                { {  1.0f,  1.0f, -1.0f, 1.0f} },

                { { -1.0f,  1.0f, -1.0f, 1.0f} },
            };

            g_stBoxData_Strip.m_nVertexCount = _countof(pstVertices);

            const UINT64 nVertexBufferSize = sizeof(pstVertices);

            g_stBufferResSesc.Width = GRS_UPPER(nVertexBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            // �������㻺�� �������ڴ��У�
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&g_stBoxData_Strip.m_pIVertexBuffer)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stBoxData_Strip.m_pIVertexBuffer);

            UINT8* pBufferData = nullptr;
            // map �� Copy �������ݵ����㻺����
            GRS_THROW_IF_FAILED(g_stBoxData_Strip.m_pIVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pBufferData)));
            memcpy(pBufferData, pstVertices, nVertexBufferSize);
            g_stBoxData_Strip.m_pIVertexBuffer->Unmap(0, nullptr);

            // �������㻺����ͼ
            g_stBoxData_Strip.m_stVertexBufferView.BufferLocation = g_stBoxData_Strip.m_pIVertexBuffer->GetGPUVirtualAddress();
            g_stBoxData_Strip.m_stVertexBufferView.StrideInBytes = sizeof(ST_GRS_VERTEX_CUBE_BOX);
            g_stBoxData_Strip.m_stVertexBufferView.SizeInBytes = nVertexBufferSize;
        }

        // 3-1-5����������������
        {
            // ����Scene Matrix ��������
            g_stBufferResSesc.Width = GRS_UPPER(sizeof(ST_GRS_CB_SCENE_MATRIX), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &g_stBufferResSesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&g_stWorldConstData.m_pISceneMatrixBuffer)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stWorldConstData.m_pISceneMatrixBuffer);
            // ӳ��� Scene �������� ���� Unmap�� ÿ��ֱ��д�뼴��
            GRS_THROW_IF_FAILED(g_stWorldConstData.m_pISceneMatrixBuffer->Map(0, nullptr, reinterpret_cast<void**>(&g_stWorldConstData.m_pstSceneMatrix)));

            // ����GS Matrix ��������
            g_stBufferResSesc.Width = GRS_UPPER(sizeof(ST_GRS_CB_GS_VIEW_MATRIX), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &g_stBufferResSesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&g_stWorldConstData.m_pIGSMatrixBuffer)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stWorldConstData.m_pIGSMatrixBuffer);

            // ӳ��� GS Matrix �������� ���� Unmap�� ÿ��ֱ��д�뼴��
            GRS_THROW_IF_FAILED(g_stWorldConstData.m_pIGSMatrixBuffer->Map(0, nullptr, reinterpret_cast<void**>(&g_stWorldConstData.m_pstGSViewMatrix)));

            // ���� PBR Material ��������
            g_stBufferResSesc.Width = GRS_UPPER(sizeof(ST_GRS_PBR_MATERIAL)
                , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &g_stBufferResSesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&g_stWorldConstData.m_pIPBRMaterialBuffer)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stWorldConstData.m_pIPBRMaterialBuffer);

            // ӳ��� PBR Material �������� ���� Unmap�� ÿ��ֱ��д�뼴��
            GRS_THROW_IF_FAILED(g_stWorldConstData.m_pIPBRMaterialBuffer->Map(0
                , nullptr
                , reinterpret_cast<void**>(&g_stWorldConstData.m_pstPBRMaterial)));

            // ���� Scene Camera ��������
            g_stBufferResSesc.Width = GRS_UPPER(sizeof(ST_GRS_SCENE_CAMERA)
                , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &g_stBufferResSesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&g_stWorldConstData.m_pISceneCameraBuffer)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stWorldConstData.m_pISceneCameraBuffer);

            // ӳ��� Scene Camera �������� ���� Unmap�� ÿ��ֱ��д�뼴��
            GRS_THROW_IF_FAILED(g_stWorldConstData.m_pISceneCameraBuffer->Map(0
                , nullptr
                , reinterpret_cast<void**>(&g_stWorldConstData.m_pstSceneCamera)));

            // ���� Scene Lights ��������
            g_stBufferResSesc.Width = GRS_UPPER(sizeof(ST_GRS_SCENE_LIGHTS)
                , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &g_stBufferResSesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&g_stWorldConstData.m_pISceneLightsBuffer)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stWorldConstData.m_pISceneLightsBuffer);

            // ӳ��� Scene Lights �������� ���� Unmap�� ÿ��ֱ��д�뼴��
            GRS_THROW_IF_FAILED(g_stWorldConstData.m_pISceneLightsBuffer->Map(0
                , nullptr
                , reinterpret_cast<void**>(&g_stWorldConstData.m_pstSceneLights)));

            // ���� ����� CBV
            D3D12_CPU_DESCRIPTOR_HANDLE stCBVHandle
                = g_st1TimesGSHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart();

            D3D12_CONSTANT_BUFFER_VIEW_DESC stCBVDesc = {};
            stCBVDesc.BufferLocation = g_stWorldConstData.m_pISceneMatrixBuffer->GetGPUVirtualAddress();
            stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_CB_SCENE_MATRIX)
                , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            //1��Scene Matrix
            g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVHandle);

            stCBVDesc.BufferLocation = g_stWorldConstData.m_pIGSMatrixBuffer->GetGPUVirtualAddress();
            stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_CB_GS_VIEW_MATRIX)
                , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            stCBVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
            //2��GS Views
            g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVHandle);

            stCBVDesc.BufferLocation = g_stWorldConstData.m_pISceneCameraBuffer->GetGPUVirtualAddress();
            stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_SCENE_CAMERA)
                , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            stCBVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
            //3��Camera
            g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVHandle);

            stCBVDesc.BufferLocation = g_stWorldConstData.m_pISceneLightsBuffer->GetGPUVirtualAddress();
            stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_SCENE_LIGHTS)
                , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            stCBVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
            //4��Lights
            g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVHandle);

            stCBVDesc.BufferLocation = g_stWorldConstData.m_pIPBRMaterialBuffer->GetGPUVirtualAddress();
            stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_PBR_MATERIAL)
                , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            stCBVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
            //5��PBR Material
            g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVHandle);
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 3-1-5�����ó�����������е�����
        {
            // ��䳡������
            g_stWorldConstData.m_pstSceneMatrix->m_mxWorld
                = XMMatrixIdentity();
            g_stWorldConstData.m_pstSceneMatrix->m_mxView
                = XMMatrixLookToLH(g_stCamera.m_v4EyePos, g_stCamera.m_v4LookDir, g_stCamera.m_v4UpDir);

            // ע�������ͶӰ������ӽ� �� �ݺ�ȣ��ӽ��� 90�ȣ��ݺ���� 1.0f Ҳ���Ƿ��ε�ͶӰ 
            g_stWorldConstData.m_pstSceneMatrix->m_mxProj
                = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, g_stCamera.m_fNear, g_stCamera.m_fFar);

            g_stWorldConstData.m_pstSceneMatrix->m_mxWorldView
                = XMMatrixMultiply(g_stWorldConstData.m_pstSceneMatrix->m_mxWorld
                    , g_stWorldConstData.m_pstSceneMatrix->m_mxView);
            g_stWorldConstData.m_pstSceneMatrix->m_mxWVP
                = XMMatrixMultiply(g_stWorldConstData.m_pstSceneMatrix->m_mxWorldView
                    , g_stWorldConstData.m_pstSceneMatrix->m_mxProj);
            g_stWorldConstData.m_pstSceneMatrix->m_mxInvWVP
                = XMMatrixInverse(nullptr, g_stWorldConstData.m_pstSceneMatrix->m_mxWVP);
            g_stWorldConstData.m_pstSceneMatrix->m_mxVP
                = XMMatrixMultiply(g_stWorldConstData.m_pstSceneMatrix->m_mxView
                    , g_stWorldConstData.m_pstSceneMatrix->m_mxProj);
            g_stWorldConstData.m_pstSceneMatrix->m_mxInvVP
                = XMMatrixInverse(nullptr
                    , g_stWorldConstData.m_pstSceneMatrix->m_mxVP);

            // ��� GS Cube Map ��Ҫ�� 6�� View Matrix
            float fHeight = 0.0f;
            XMVECTOR vEyePt = { 0.0f, fHeight, 0.0f , 1.0f };
            XMVECTOR vLookDir = {};
            XMVECTOR vUpDir = {};

            vLookDir = XMVectorSet(1.0f, fHeight, 0.0f, 0.0f);
            vUpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            g_stWorldConstData.m_pstGSViewMatrix->m_mxGSCubeView[0] = XMMatrixLookAtLH(vEyePt, vLookDir, vUpDir);
            vLookDir = XMVectorSet(-1.0f, fHeight, 0.0f, 0.0f);
            vUpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            g_stWorldConstData.m_pstGSViewMatrix->m_mxGSCubeView[1] = XMMatrixLookAtLH(vEyePt, vLookDir, vUpDir);
            vLookDir = XMVectorSet(0.0f, fHeight + 1.0f, 0.0f, 0.0f);
            vUpDir = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
            g_stWorldConstData.m_pstGSViewMatrix->m_mxGSCubeView[2] = XMMatrixLookAtLH(vEyePt, vLookDir, vUpDir);
            vLookDir = XMVectorSet(0.0f, fHeight - 1.0f, 0.0f, 0.0f);
            vUpDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
            g_stWorldConstData.m_pstGSViewMatrix->m_mxGSCubeView[3] = XMMatrixLookAtLH(vEyePt, vLookDir, vUpDir);
            vLookDir = XMVectorSet(0.0f, fHeight, 1.0f, 0.0f);
            vUpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            g_stWorldConstData.m_pstGSViewMatrix->m_mxGSCubeView[4] = XMMatrixLookAtLH(vEyePt, vLookDir, vUpDir);
            vLookDir = XMVectorSet(0.0f, fHeight, -1.0f, 0.0f);
            vUpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            g_stWorldConstData.m_pstGSViewMatrix->m_mxGSCubeView[5] = XMMatrixLookAtLH(vEyePt, vLookDir, vUpDir);
        }

        CAtlArray<ID3D12CommandList*> arCmdList;
        // 3-1-6��ִ�еڶ���Copy����ȴ����е������ϴ�����Ĭ�϶���
        {
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());
            arCmdList.RemoveAll();
            arCmdList.Add(g_stD3DDevice.m_pIMainCMDList.Get());
            g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

            // �ȴ�������Դ��ʽ���������
            const UINT64 fence = g_stD3DDevice.m_n64FenceValue;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), fence));
            g_stD3DDevice.m_n64FenceValue++;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(fence, g_stD3DDevice.m_hEventFence));
        }

        CAtlArray<ID3D12DescriptorHeap*> arHeaps;
        // 3-1-7��ִ��1 Times GS Cube Map ��Ⱦ���������Ҳֻ��Ҫִ��һ�ξ�������Cube Map ��6����
        {
            // �ȴ�ǰ���Copy����ִ�н���
            if (WAIT_OBJECT_0 != ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE))
            {
                ::AtlThrowLastWin32();
            }
            // ��ʼִ�� GS Cube Map��Ⱦ����HDR�Ⱦ��������� һ������Ⱦ��6��RTV
            //�����������Resetһ��
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDAlloc->Reset());
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Reset(g_stD3DDevice.m_pIMainCMDAlloc.Get(), nullptr));

            g_stD3DDevice.m_pIMainCMDList->RSSetViewports(1, &g_st1TimesGSRTAStatus.m_stViewPort);
            g_stD3DDevice.m_pIMainCMDList->RSSetScissorRects(1, &g_st1TimesGSRTAStatus.m_stScissorRect);

            // ������ȾĿ�� ע��һ�ξͰ����е�������ȾĿ��Ž�ȥ
            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = g_st1TimesGSRTAStatus.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();
            g_stD3DDevice.m_pIMainCMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);

            // ������¼�����������ʼ��Ⱦ
            g_stD3DDevice.m_pIMainCMDList->ClearRenderTargetView(stRTVHandle, g_stD3DDevice.m_faClearColor, 0, nullptr);

            //-----------------------------------------------------------------------------------------------------
            // Draw !
            //-----------------------------------------------------------------------------------------------------
            // ��ȾCube
            // ������Ⱦ����״̬����
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootSignature(g_st1TimesGSPSO.m_pIRS.Get());
            g_stD3DDevice.m_pIMainCMDList->SetPipelineState(g_st1TimesGSPSO.m_pIPSO.Get());

            arHeaps.RemoveAll();
            arHeaps.Add(g_st1TimesGSHeap.m_pICBVSRVHeap.Get());
            arHeaps.Add(g_st1TimesGSHeap.m_pISAPHeap.Get());

            g_stD3DDevice.m_pIMainCMDList->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());
            D3D12_GPU_DESCRIPTOR_HANDLE stGPUHandle = g_st1TimesGSHeap.m_pICBVSRVHeap->GetGPUDescriptorHandleForHeapStart();
            // ����CBV
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(0, stGPUHandle);
            // ����SRV
            stGPUHandle.ptr += (g_st1TimesGSHeap.m_nSRVOffset * g_stD3DDevice.m_nCBVSRVDescriptorSize);
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(1, stGPUHandle);
            // ����Sample
            stGPUHandle = g_st1TimesGSHeap.m_pISAPHeap->GetGPUDescriptorHandleForHeapStart();
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(2, stGPUHandle);

            // ��Ⱦ�ַ��������δ�
            g_stD3DDevice.m_pIMainCMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            // ����������Ϣ
            g_stD3DDevice.m_pIMainCMDList->IASetVertexBuffers(0, 1, &g_stBoxData_Strip.m_stVertexBufferView);
            // Draw Call
            g_stD3DDevice.m_pIMainCMDList->DrawInstanced(g_stBoxData_Strip.m_nVertexCount, 1, 0, 0);
            //-----------------------------------------------------------------------------------------------------
            // ʹ����Դ�����л���״̬��ʵ���������Ҫʹ���⼸����ȾĿ��Ĺ��̽���ͬ��
            g_st1TimesGSRTAStatus.m_stRTABarriers.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            g_st1TimesGSRTAStatus.m_stRTABarriers.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            g_st1TimesGSRTAStatus.m_stRTABarriers.Transition.pResource = g_st1TimesGSRTAStatus.m_pITextureRTA.Get();
            g_st1TimesGSRTAStatus.m_stRTABarriers.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            //g_st1TimesGSRTAStatus.m_stRTABarriers.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            g_st1TimesGSRTAStatus.m_stRTABarriers.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            g_st1TimesGSRTAStatus.m_stRTABarriers.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            //��Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
            g_stD3DDevice.m_pIMainCMDList->ResourceBarrier(1, &g_st1TimesGSRTAStatus.m_stRTABarriers);

            //�ر������б�����ȥִ����
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());
            //ִ�������б�
            g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

            //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
            const UINT64 n64CurrentFenceValue = g_stD3DDevice.m_n64FenceValue;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), n64CurrentFenceValue));
            g_stD3DDevice.m_n64FenceValue++;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(n64CurrentFenceValue, g_stD3DDevice.m_hEventFence));
        }
        //---------------------------------------------------------------------------------------

        //---------------------------------------------------------------------------------------
        // �ڶ��ַ�ʽ��ִ��������Ⱦ�õ�Cube map               
        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 3-2-0����ʼ��Render Target����
        {
            g_st6TimesRTAStatus.m_nRTWidth = GRS_RTA_DEFAULT_SIZE_W;
            g_st6TimesRTAStatus.m_nRTHeight = GRS_RTA_DEFAULT_SIZE_H;
            g_st6TimesRTAStatus.m_stViewPort = { 0.0f, 0.0f, static_cast<float>(g_st6TimesRTAStatus.m_nRTWidth), static_cast<float>(g_st6TimesRTAStatus.m_nRTHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
            g_st6TimesRTAStatus.m_stScissorRect = { 0, 0, static_cast<LONG>(g_st6TimesRTAStatus.m_nRTWidth), static_cast<LONG>(g_st6TimesRTAStatus.m_nRTHeight) };
            g_st6TimesRTAStatus.m_emRTAFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 3-2-1���������ý�HDR��Ⱦ��Cube Map����ͨPSO ��Ҫִ��6��
        {
            D3D12_DESCRIPTOR_RANGE1 stDSPRanges[3] = {};
            // 1 Const Buffer ( MVP Matrix )
            stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            stDSPRanges[0].NumDescriptors = 1;
            stDSPRanges[0].BaseShaderRegister = 0;
            stDSPRanges[0].RegisterSpace = 0;
            stDSPRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

            // 1 Texture ( HDR Texture ) 
            stDSPRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges[1].NumDescriptors = 1;
            stDSPRanges[1].BaseShaderRegister = 0;
            stDSPRanges[1].RegisterSpace = 0;
            stDSPRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[1].OffsetInDescriptorsFromTableStart = 0;

            // 1 Sampler ( Linear Sampler )
            stDSPRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            stDSPRanges[2].NumDescriptors = 1;
            stDSPRanges[2].BaseShaderRegister = 0;
            stDSPRanges[2].RegisterSpace = 0;
            stDSPRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[2].OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 stRootParameters[3] = {};
            stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//CBV������Shader�ɼ�
            stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

            stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SRV��PS�ɼ�
            stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

            stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SAMPLE��PS�ɼ�
            stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
            stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
            stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
            stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&g_stHDR2CubemapPSO.m_pIRS)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stHDR2CubemapPSO.m_pIRS);

            //--------------------------------------------------------------------------------------------------------------------------------
            UINT nCompileFlags = 0;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            //����Ϊ�о�����ʽ	   
            nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

            TCHAR pszShaderFileName[MAX_PATH] = {};

            ComPtr<ID3DBlob> pIVSCode;
            ComPtr<ID3DBlob> pIPSCode;
            ComPtr<ID3DBlob> pIErrorMsg;

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\1-1 GRS_6Times_HDR_2_Cubemap_VS.hlsl")
                , g_pszShaderPath);

            HRESULT hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "VSMain", "vs_5_0", nCompileFlags, 0, &pIVSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Vertex Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\1-3 GRS_HDR_Spherical_Map_2_Cubemap_PS.hlsl")
                , g_pszShaderPath);

            hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "PSMain", "ps_5_0", nCompileFlags, 0, &pIPSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Pixel Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // �������붥������ݽṹ
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };
            stPSODesc.pRootSignature = g_stHDR2CubemapPSO.m_pIRS.Get();

            // VS -> PS
            stPSODesc.VS.BytecodeLength = pIVSCode->GetBufferSize();
            stPSODesc.VS.pShaderBytecode = pIVSCode->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIPSCode->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIPSCode->GetBufferPointer();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;

            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.SampleDesc.Count = 1;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = g_st6TimesRTAStatus.m_emRTAFormat;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;
            stPSODesc.DepthStencilState.DepthEnable = FALSE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&g_stHDR2CubemapPSO.m_pIPSO)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stHDR2CubemapPSO.m_pIPSO);

            // ���� CBV SRV Heap
            D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
            stSRVHeapDesc.NumDescriptors = 2; // 1 CBV + 1 SRV
            stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSRVHeapDesc
                , IID_PPV_ARGS(&g_stHDR2CubemapHeap.m_pICBVSRVHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stHDR2CubemapHeap.m_pICBVSRVHeap);

            // ���� Sample Heap
            D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
            stSamplerHeapDesc.NumDescriptors = 1;   // 1 Sample
            stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSamplerHeapDesc
                , IID_PPV_ARGS(&g_stHDR2CubemapHeap.m_pISAPHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stHDR2CubemapHeap.m_pISAPHeap);
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 3-2-2������ SRV �� Sample
        {
            D3D12_CPU_DESCRIPTOR_HANDLE stHeapHandle
                = g_stHDR2CubemapHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
            stHeapHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize; // ���չ��ߵ�Ҫ�� ��2������SRV

            // ���� HDR Texture SRV
            D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
            stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            stSRVDesc.Format = g_stHDRData.m_emHDRFormat;
            stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            stSRVDesc.Texture2D.MipLevels = 1;
            g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(g_stHDRData.m_pIHDRTextureRaw.Get(), &stSRVDesc, stHeapHandle);

            // ����һ�����Թ��˵ı�Ե���еĲ�����
            D3D12_SAMPLER_DESC stSamplerDesc = {};
            stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            stSamplerDesc.MinLOD = 0;
            stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            stSamplerDesc.MipLODBias = 0.0f;
            stSamplerDesc.MaxAnisotropy = 1;
            stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            stSamplerDesc.BorderColor[0] = 0.0f;
            stSamplerDesc.BorderColor[1] = 0.0f;
            stSamplerDesc.BorderColor[2] = 0.0f;
            stSamplerDesc.BorderColor[3] = 0.0f;
            stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

            g_stD3DDevice.m_pID3D12Device4->CreateSampler(&stSamplerDesc, g_stHDR2CubemapHeap.m_pISAPHeap->GetCPUDescriptorHandleForHeapStart());
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 3-2-3������Render Target Array ��Texture2DArray����RTV
        {
            // ����������ȾĿ��(Render Target Array Texture2DArray)
            D3D12_RESOURCE_DESC stRenderTargetDesc = g_stD3DDevice.m_pIARenderTargets[0]->GetDesc();
            stRenderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            stRenderTargetDesc.Format = g_st6TimesRTAStatus.m_emRTAFormat;
            stRenderTargetDesc.Width = g_st6TimesRTAStatus.m_nRTWidth;
            stRenderTargetDesc.Height = g_st6TimesRTAStatus.m_nRTHeight;
            stRenderTargetDesc.DepthOrArraySize = GRS_CUBE_MAP_FACE_CNT;

            D3D12_CLEAR_VALUE stClearValue = {};
            stClearValue.Format = g_st6TimesRTAStatus.m_emRTAFormat;
            stClearValue.Color[0] = g_stD3DDevice.m_faClearColor[0];
            stClearValue.Color[1] = g_stD3DDevice.m_faClearColor[1];
            stClearValue.Color[2] = g_stD3DDevice.m_faClearColor[2];
            stClearValue.Color[3] = g_stD3DDevice.m_faClearColor[3];

            // ��������������Դ
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stDefaultHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &stRenderTargetDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &stClearValue,
                IID_PPV_ARGS(&g_st6TimesRTAStatus.m_pITextureRTA)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st6TimesRTAStatus.m_pITextureRTA);


            //����RTV(��ȾĿ����ͼ)��������( ����������������������� )            
            D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
            stRTVHeapDesc.NumDescriptors = GRS_CUBE_MAP_FACE_CNT;   // 6 Render Target Array RTV
            stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&g_st6TimesRTAStatus.m_pIRTAHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st6TimesRTAStatus.m_pIRTAHeap);

            // ������Ӧ��RTV��������
            D3D12_RENDER_TARGET_VIEW_DESC stRTADesc = {};
            stRTADesc.Format = g_st6TimesRTAStatus.m_emRTAFormat;
            stRTADesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            stRTADesc.Texture2DArray.ArraySize = 1;

            // Ϊÿ������Դ����һ��RTV
            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = g_st6TimesRTAStatus.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();
            for (UINT i = 0; i < GRS_CUBE_MAP_FACE_CNT; i++)
            {
                stRTADesc.Texture2DArray.FirstArraySlice = i;
                g_stD3DDevice.m_pID3D12Device4->CreateRenderTargetView(g_st6TimesRTAStatus.m_pITextureRTA.Get(), &stRTADesc, stRTVHandle);
                stRTVHandle.ptr += g_stD3DDevice.m_nRTVDescriptorSize;
            }

            D3D12_RESOURCE_DESC stDownloadResDesc = {};
            stDownloadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            stDownloadResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            stDownloadResDesc.Width = 0;
            stDownloadResDesc.Height = 1;
            stDownloadResDesc.DepthOrArraySize = 1;
            stDownloadResDesc.MipLevels = 1;
            stDownloadResDesc.Format = DXGI_FORMAT_UNKNOWN;
            stDownloadResDesc.SampleDesc.Count = 1;
            stDownloadResDesc.SampleDesc.Quality = 0;
            stDownloadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            stDownloadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            UINT64 n64ResBufferSize = 0;

            g_stD3DDevice.m_pID3D12Device4->GetCopyableFootprints(&stRenderTargetDesc
                , 0, GRS_CUBE_MAP_FACE_CNT, 0, nullptr, nullptr, nullptr, &n64ResBufferSize);

            stDownloadResDesc.Width = n64ResBufferSize;

            // �����������Ҫ����Ⱦ�Ľ�����Ƴ�������Ҫ�Ļ���Ҫ�洢��ָ�����ļ��������ȱ��Read Back��
            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_READBACK;
            // Download Heap == Upload Heap ���ǹ����ڴ棡
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &stDownloadResDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&g_st6TimesRTAStatus.m_pITextureRTADownload)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_st6TimesRTAStatus.m_pITextureRTADownload);

            // ��ԭΪ�ϴ������ԣ���������Ҫ��
            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        }

        // 3-2-4��׼��������ͳ�������
        {
            ST_GRS_VERTEX_CUBE_BOX pstVertices[] = {
                {{-1.0f, -1.0f, -1.0f,1.0f}},
                {{-1.0f, +1.0f, -1.0f,1.0f}},
                {{+1.0f, +1.0f, -1.0f,1.0f}},
                {{+1.0f, -1.0f, -1.0f,1.0f}},
                {{-1.0f, -1.0f, +1.0f,1.0f}},
                {{-1.0f, +1.0f, +1.0f,1.0f}},
                {{+1.0f, +1.0f, +1.0f,1.0f}},
                {{+1.0f, -1.0f, +1.0f,1.0f}},
            };

            UINT16 arIndex[] = {
                 0, 2, 1,
                 0, 3, 2,

                 4, 5, 6,
                 4, 6, 7,

                 4, 1, 5,
                 4, 0, 1,

                 3, 6, 2,
                 3, 7, 6,

                 1, 6, 5,
                 1, 2, 6,

                 4, 3, 0,
                 4, 7, 3
            };

            g_stBoxData_Index.m_nIndexCount = _countof(arIndex);

            const UINT64 nVertexBufferSize = sizeof(pstVertices);

            g_stBufferResSesc.Width = GRS_UPPER(nVertexBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            // �������㻺�� �������ڴ��У�
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&g_stBoxData_Index.m_pIVB)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stBoxData_Index.m_pIVB);

            UINT8* pBufferData = nullptr;
            // map �� Copy �������ݵ����㻺����
            GRS_THROW_IF_FAILED(g_stBoxData_Index.m_pIVB->Map(0, nullptr, reinterpret_cast<void**>(&pBufferData)));
            memcpy(pBufferData, pstVertices, nVertexBufferSize);
            g_stBoxData_Index.m_pIVB->Unmap(0, nullptr);

            // �������㻺����ͼ
            g_stBoxData_Index.m_stVBV.BufferLocation = g_stBoxData_Index.m_pIVB->GetGPUVirtualAddress();
            g_stBoxData_Index.m_stVBV.StrideInBytes = sizeof(ST_GRS_VERTEX_CUBE_BOX);
            g_stBoxData_Index.m_stVBV.SizeInBytes = nVertexBufferSize;

            // ��������
            const UINT64 nIndexBufferSize = sizeof(arIndex);

            g_stBufferResSesc.Width = GRS_UPPER(nIndexBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            // �������㻺�� �������ڴ��У�
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&g_stBoxData_Index.m_pIIB)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stBoxData_Index.m_pIIB);

            // map �� Copy �������ݵ����㻺����
            GRS_THROW_IF_FAILED(g_stBoxData_Index.m_pIIB->Map(0, nullptr, reinterpret_cast<void**>(&pBufferData)));
            memcpy(pBufferData, arIndex, nIndexBufferSize);
            g_stBoxData_Index.m_pIIB->Unmap(0, nullptr);

            // �������㻺����ͼ
            g_stBoxData_Index.m_stIBV.BufferLocation = g_stBoxData_Index.m_pIIB->GetGPUVirtualAddress();
            g_stBoxData_Index.m_stIBV.Format = DXGI_FORMAT_R16_UINT;
            g_stBoxData_Index.m_stIBV.SizeInBytes = nIndexBufferSize;

            // ������������� CBV
            D3D12_CPU_DESCRIPTOR_HANDLE stCBVHandle = g_stHDR2CubemapHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
            D3D12_CONSTANT_BUFFER_VIEW_DESC stCBVDesc = {};
            stCBVDesc.BufferLocation = g_stWorldConstData.m_pISceneMatrixBuffer->GetGPUVirtualAddress();
            stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_CB_SCENE_MATRIX), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVHandle);
        }

        // 3-2-5��ִ��6��Cube Map ��Ⱦ����������Ҳֻ��Ҫִ��һ�ξ�������Cube Map6����
        {
            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                = g_st6TimesRTAStatus.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();

            arHeaps.RemoveAll();
            arHeaps.Add(g_stHDR2CubemapHeap.m_pICBVSRVHeap.Get());
            arHeaps.Add(g_stHDR2CubemapHeap.m_pISAPHeap.Get());

            for (UINT i = 0; i < GRS_CUBE_MAP_FACE_CNT; i++)
            {
                // �ȴ�ǰһ������ִ�н���
                if (WAIT_OBJECT_0 != ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE))
                {
                    ::AtlThrowLastWin32();
                }

                //---------------------------------------------------------------------------
                // ����View���� ����Const Buffer
                g_stWorldConstData.m_pstSceneMatrix->m_mxWorld = XMMatrixIdentity();

                g_stWorldConstData.m_pstSceneMatrix->m_mxView
                    = g_stWorldConstData.m_pstGSViewMatrix->m_mxGSCubeView[i];

                g_stWorldConstData.m_pstSceneMatrix->m_mxWorldView
                    = XMMatrixMultiply(g_stWorldConstData.m_pstSceneMatrix->m_mxWorld
                        , g_stWorldConstData.m_pstSceneMatrix->m_mxView);

                g_stWorldConstData.m_pstSceneMatrix->m_mxWVP
                    = XMMatrixMultiply(g_stWorldConstData.m_pstSceneMatrix->m_mxWorldView
                        , g_stWorldConstData.m_pstSceneMatrix->m_mxProj);
                //---------------------------------------------------------------------------

                //��ʼִ����Ⱦ
                //�����������Resetһ��
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDAlloc->Reset());
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Reset(g_stD3DDevice.m_pIMainCMDAlloc.Get(), nullptr));

                g_stD3DDevice.m_pIMainCMDList->RSSetViewports(1, &g_st6TimesRTAStatus.m_stViewPort);
                g_stD3DDevice.m_pIMainCMDList->RSSetScissorRects(1, &g_st6TimesRTAStatus.m_stScissorRect);

                // ������ȾĿ�� ע��һ�ξͰ����е�������ȾĿ��Ž�ȥ
                g_stD3DDevice.m_pIMainCMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);
                // ����
                g_stD3DDevice.m_pIMainCMDList->ClearRenderTargetView(stRTVHandle, g_stD3DDevice.m_faClearColor, 0, nullptr);

                stRTVHandle.ptr += g_stD3DDevice.m_nRTVDescriptorSize;
                //-----------------------------------------------------------------------------------------------------
                // Draw !
                //-----------------------------------------------------------------------------------------------------
                // ��ȾCube
                // ������Ⱦ����״̬����
                g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootSignature(g_stHDR2CubemapPSO.m_pIRS.Get());
                g_stD3DDevice.m_pIMainCMDList->SetPipelineState(g_stHDR2CubemapPSO.m_pIPSO.Get());

                g_stD3DDevice.m_pIMainCMDList->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());
                D3D12_GPU_DESCRIPTOR_HANDLE stGPUHandle = g_stHDR2CubemapHeap.m_pICBVSRVHeap->GetGPUDescriptorHandleForHeapStart();
                // ����CBV
                g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(0, stGPUHandle);
                // ����SRV
                stGPUHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
                g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(1, stGPUHandle);
                // ����Sample
                stGPUHandle = g_stHDR2CubemapHeap.m_pISAPHeap->GetGPUDescriptorHandleForHeapStart();
                g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(2, stGPUHandle);

                // ��Ⱦ�ַ�������������
                //g_stD3DDevice.m_pIMainCMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                g_stD3DDevice.m_pIMainCMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                // ����������Ϣ
                g_stD3DDevice.m_pIMainCMDList->IASetVertexBuffers(0, 1, &g_stBoxData_Index.m_stVBV);
                g_stD3DDevice.m_pIMainCMDList->IASetIndexBuffer(&g_stBoxData_Index.m_stIBV);
                // Draw Call
                g_stD3DDevice.m_pIMainCMDList->DrawIndexedInstanced(g_stBoxData_Index.m_nIndexCount, 1, 0, 0, 0);
                //-----------------------------------------------------------------------------------------------------

                // ʹ����Դ�����л���״̬��ʵ���������Ҫʹ���⼸����ȾĿ��Ĺ��̽���ͬ��
                // ����Ϊ�˼��ֱ�ӱ��Shader��Դ
                g_st6TimesRTAStatus.m_stRTABarriers.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                g_st6TimesRTAStatus.m_stRTABarriers.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                g_st6TimesRTAStatus.m_stRTABarriers.Transition.pResource = g_st6TimesRTAStatus.m_pITextureRTA.Get();
                g_st6TimesRTAStatus.m_stRTABarriers.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                g_st6TimesRTAStatus.m_stRTABarriers.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                g_st6TimesRTAStatus.m_stRTABarriers.Transition.Subresource = i;
                //��Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
                g_stD3DDevice.m_pIMainCMDList->ResourceBarrier(1, &g_st6TimesRTAStatus.m_stRTABarriers);

                //�ر������б�����ȥִ����
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());
                //ִ�������б�
                g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

                //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
                const UINT64 n64CurrentFenceValue = g_stD3DDevice.m_n64FenceValue;
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), n64CurrentFenceValue));
                g_stD3DDevice.m_n64FenceValue++;
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(n64CurrentFenceValue, g_stD3DDevice.m_hEventFence));
            }
        }

        //---------------------------------------------------------------------------------------
        // ׼����պб���
        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 4-1��������պе���Ⱦ����״̬����
        {
            D3D12_DESCRIPTOR_RANGE1 stDSPRanges[3] = {};

            stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            stDSPRanges[0].NumDescriptors = 1;
            stDSPRanges[0].BaseShaderRegister = 0;
            stDSPRanges[0].RegisterSpace = 0;
            stDSPRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

            stDSPRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges[1].NumDescriptors = 1;
            stDSPRanges[1].BaseShaderRegister = 0;
            stDSPRanges[1].RegisterSpace = 0;
            // ��GPU��ִ��SetGraphicsRootDescriptorTable�����ǲ��޸������б��е�SRV��������ǿ���ʹ��Ĭ��Rang��Ϊ:
            // D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
            stDSPRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[1].OffsetInDescriptorsFromTableStart = 0;

            stDSPRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            stDSPRanges[2].NumDescriptors = 1;
            stDSPRanges[2].BaseShaderRegister = 0;
            stDSPRanges[2].RegisterSpace = 0;
            stDSPRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[2].OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 stRootParameters[3] = {};

            stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

            stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

            stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
            stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
            stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
            stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&g_stSkyBoxPSO.m_pIRS)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxPSO.m_pIRS);

            UINT nCompileFlags = 0;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            nCompileFlags = 0;
#endif
            //����Ϊ�о�����ʽ	   
            nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

            TCHAR pszSMFileSkybox[MAX_PATH] = {};

            StringCchPrintf(pszSMFileSkybox
                , MAX_PATH
                , _T("%s\\5-1 GRS_SkyBox_With_Spherical_Map.hlsl")
                , g_pszShaderPath);

            ComPtr<ID3DBlob> pIVSSkybox;
            ComPtr<ID3DBlob> pIPSSkybox;

            ComPtr<ID3DBlob> pIErrorMsg;
            // VS
            HRESULT hr = D3DCompileFromFile(pszSMFileSkybox, nullptr, &grsD3DCompilerInclude
                , "SkyboxVS", "vs_5_0", nCompileFlags, 0, &pIVSSkybox, &pIErrorBlob);
            if (FAILED(hr))
            {
                ATLTRACE("���� Vertex Shader��\"%s\" ��������%s\n"
                    , T2A(pszSMFileSkybox)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // PS
            pIErrorBlob.Reset();
            hr = D3DCompileFromFile(pszSMFileSkybox, nullptr, &grsD3DCompilerInclude
                , "SkyboxPS", "ps_5_0", nCompileFlags, 0, &pIPSSkybox, &pIErrorBlob);
            if (FAILED(hr))
            {
                ATLTRACE("���� Pixel Shader��\"%s\" ��������%s\n"
                    , T2A(pszSMFileSkybox)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // ��պ���ֻ�ж���ֻ��λ�ò���
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };
            stPSODesc.pRootSignature = g_stSkyBoxPSO.m_pIRS.Get();

            stPSODesc.VS.BytecodeLength = pIVSSkybox->GetBufferSize();
            stPSODesc.VS.pShaderBytecode = pIVSSkybox->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIPSSkybox->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIPSSkybox->GetBufferPointer();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

            stPSODesc.DepthStencilState.DepthEnable = FALSE;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = g_stD3DDevice.m_emRenderTargetFormat;
            stPSODesc.DSVFormat = g_stD3DDevice.m_emDepthStencilFormat;
            stPSODesc.SampleDesc.Count = 1;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc
                , IID_PPV_ARGS(&g_stSkyBoxPSO.m_pIPSO)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxPSO.m_pIPSO);

            // ���� CBV SRV Heap
            D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
            stSRVHeapDesc.NumDescriptors = 2; // 1 CBV + 1 SRV
            stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSRVHeapDesc
                , IID_PPV_ARGS(&g_stSkyBoxHeap.m_pICBVSRVHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxHeap.m_pICBVSRVHeap);

            // ���� Sample Heap
            D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
            stSamplerHeapDesc.NumDescriptors = 1;   // 1 Sample
            stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSamplerHeapDesc
                , IID_PPV_ARGS(&g_stSkyBoxHeap.m_pISAPHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxHeap.m_pISAPHeap);
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 4-2��׼����պл�������
        {
            // ��OnSize��ʱ����Ҫ���¼�����պд�С
            float fHighW = -1.0f - (1.0f / (float)g_stD3DDevice.m_iWndWidth);
            float fHighH = -1.0f - (1.0f / (float)g_stD3DDevice.m_iWndHeight);
            float fLowW = 1.0f + (1.0f / (float)g_stD3DDevice.m_iWndWidth);
            float fLowH = 1.0f + (1.0f / (float)g_stD3DDevice.m_iWndHeight);

            ST_GRS_VERTEX_CUBE_BOX stSkyboxVertices[4] = {};

            stSkyboxVertices[0].m_v4Position = XMFLOAT4(fLowW, fLowH, 1.0f, 1.0f);
            stSkyboxVertices[1].m_v4Position = XMFLOAT4(fLowW, fHighH, 1.0f, 1.0f);
            stSkyboxVertices[2].m_v4Position = XMFLOAT4(fHighW, fLowH, 1.0f, 1.0f);
            stSkyboxVertices[3].m_v4Position = XMFLOAT4(fHighW, fHighH, 1.0f, 1.0f);

            g_stSkyBoxData.m_nVertexCount = _countof(stSkyboxVertices);
            //---------------------------------------------------------------------------------------------
            //������պ��ӵ�����
            g_stBufferResSesc.Width = GRS_UPPER(sizeof(stSkyboxVertices), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&g_stSkyBoxData.m_pIVertexBuffer)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxData.m_pIVertexBuffer);

            //ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
            //ע�ⶥ�㻺��ʹ���Ǻ��ϴ��������ݻ�����ͬ��һ���ѣ��������
            ST_GRS_VERTEX_CUBE_BOX* pBufferData = nullptr;

            GRS_THROW_IF_FAILED(g_stSkyBoxData.m_pIVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pBufferData)));
            memcpy(pBufferData, stSkyboxVertices, sizeof(stSkyboxVertices));
            g_stSkyBoxData.m_pIVertexBuffer->Unmap(0, nullptr);

            // ������Դ��ͼ��ʵ�ʿ��Լ����Ϊָ�򶥵㻺����Դ�ָ��
            g_stSkyBoxData.m_stVertexBufferView.BufferLocation = g_stSkyBoxData.m_pIVertexBuffer->GetGPUVirtualAddress();
            g_stSkyBoxData.m_stVertexBufferView.StrideInBytes = sizeof(ST_GRS_VERTEX_CUBE_BOX);
            g_stSkyBoxData.m_stVertexBufferView.SizeInBytes = sizeof(stSkyboxVertices);

            // ʹ��ȫ�ֳ�������������CBV
            D3D12_CONSTANT_BUFFER_VIEW_DESC stCBVDesc = {};
            stCBVDesc.BufferLocation = g_stWorldConstData.m_pISceneMatrixBuffer->GetGPUVirtualAddress();
            stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_CB_SCENE_MATRIX), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvHandleSkybox = { g_stSkyBoxHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart() };
            g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, cbvSrvHandleSkybox);
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 4-3��ѡ����պеĸ�����ͼ�����������Ⱦ
        {
            TCHAR pszBigSkyImageFile[MAX_PATH] = {};
            StringCchPrintf(pszBigSkyImageFile
                , MAX_PATH
                , _T("%s\\Tropical_Beach_8k.jpg")
                , g_pszHDRFilePath);

#ifdef GRS_USE_DIR_DIALOG
            OPENFILENAME stOfn = {};

            RtlZeroMemory(&stOfn, sizeof(stOfn));
            stOfn.lStructSize = sizeof(stOfn);
            stOfn.hwndOwner = ::GetDesktopWindow(); //g_stD3DDevice.m_hWnd;
            stOfn.lpstrFilter = _T("jpgͼƬ�ļ�\0*.jpg\0HDR�ļ�\0*.hdr\0�����ļ�\0*.*\0");
            stOfn.lpstrFile = pszBigSkyImageFile;
            stOfn.nMaxFile = MAX_PATH;
            stOfn.lpstrTitle = _T("��ѡ��һ���������պ�ͼƬ�ļ�(��ѡĬ����֮ǰ��HDR�ļ�)...");
            stOfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (!::GetOpenFileName(&stOfn))
            {
                StringCchCopy(pszBigSkyImageFile, MAX_PATH, g_stHDRData.m_pszHDRFile);
            }
#endif
            INT iImageWidth = 0;
            INT iImageHeight = 0;
            INT iImagePXComponents = 0;

            // ���������˼��Ҫ��Ҫ���µߵ�һ��ͼƬ
            // ע����OpenGL�����������ݷ����Ǵ��µ���������Ҫ�ߵ�һ��
            // ������D3D �Ͳ����� ���������ã�
            //stbi_set_flip_vertically_on_load(true);

            // ������Ҫע����ǣ���Ȼ����滷����ͼ��jpgͼƬ������Ȼʹ�� STB ��������
            // ��ΪͼƬ����ʱ WIC ��Ͳ����ˣ���û�������ԭ��
            // ��Ҳ��Ӧ�� STB ��ǿ�������
            float* pfImageData = stbi_loadf(T2A(pszBigSkyImageFile)
                , &iImageWidth
                , &iImageHeight
                , &iImagePXComponents
                , 0);
            if (nullptr == pfImageData)
            {
                ATLTRACE(_T("�޷������ļ���\"%s\"\n"), pszBigSkyImageFile);
                AtlThrowLastWin32();
            }

            if (3 != iImagePXComponents)
            {
                ATLTRACE(_T("�ļ���\"%s\"������3����Float��ʽ���޷�ʹ�ã�\n"), pszBigSkyImageFile);
                AtlThrowLastWin32();
            }

            UINT nRowAlign = 8u;
            UINT nBPP = (UINT)iImagePXComponents * sizeof(float);

            //UINT nPicRowPitch = (  iHDRWidth  *  nBPP  + 7u ) / 8u;
            UINT nPicRowPitch = GRS_UPPER(iImageWidth * nBPP, nRowAlign);

            ID3D12Resource* pITextureUpload = nullptr;
            ID3D12Resource* pITexture = nullptr;

            // �ȴ�ǰ�����Ⱦ����ִ�н���
            if (WAIT_OBJECT_0 != ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE))
            {
                ::AtlThrowLastWin32();
            }

            //�����������Resetһ��
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDAlloc->Reset());
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Reset(g_stD3DDevice.m_pIMainCMDAlloc.Get(), nullptr));

            // ����ͼƬ ��ɵ�һ��Copy���� ���� �����ڶ���Copy����
            BOOL bRet = LoadTextureFromMem(g_stD3DDevice.m_pIMainCMDList.Get()
                , (BYTE*)pfImageData
                , (size_t)iImageWidth * nBPP * iImageHeight
                , DXGI_FORMAT_R32G32B32_FLOAT
                , iImageWidth
                , iImageHeight
                , nPicRowPitch
                , pITextureUpload
                , pITexture
            );

            if (!bRet)
            {
                AtlThrowLastWin32();
            }

            g_stSkyBoxData.m_pITexture.Attach(pITexture);
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxData.m_pITexture);
            g_stSkyBoxData.m_pITextureUpload.Attach(pITextureUpload);
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxData.m_pITextureUpload);

            //�ر������б�ִ��һ�� Copy ����
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());
            //ִ�������б�
            g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

            //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
            const UINT64 n64CurrentFenceValue = g_stD3DDevice.m_n64FenceValue;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), n64CurrentFenceValue));
            g_stD3DDevice.m_n64FenceValue++;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(n64CurrentFenceValue, g_stD3DDevice.m_hEventFence));


            D3D12_CPU_DESCRIPTOR_HANDLE stHeapHandle
                = g_stSkyBoxHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
            stHeapHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize; // ���չ��ߵ�Ҫ�� ��2����SRV

            D3D12_RESOURCE_DESC stResDesc = g_stSkyBoxData.m_pITexture->GetDesc();
            // ���� Skybox Texture SRV
            D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
            stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            stSRVDesc.Format = stResDesc.Format;
            stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            stSRVDesc.Texture2D.MipLevels = 1;
            g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(
                g_stSkyBoxData.m_pITexture.Get()
                , &stSRVDesc
                , stHeapHandle);

            // ����һ�����Թ��˵ı�Ե���еĲ�����
            D3D12_SAMPLER_DESC stSamplerDesc = {};
            stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

            stSamplerDesc.MinLOD = 0;
            stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            stSamplerDesc.MipLODBias = 0.0f;
            stSamplerDesc.MaxAnisotropy = 1;
            stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            stSamplerDesc.BorderColor[0] = 0.0f;
            stSamplerDesc.BorderColor[1] = 0.0f;
            stSamplerDesc.BorderColor[2] = 0.0f;
            stSamplerDesc.BorderColor[3] = 0.0f;
            stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

            g_stD3DDevice.m_pID3D12Device4->CreateSampler(&stSamplerDesc
                , g_stSkyBoxHeap.m_pISAPHeap->GetCPUDescriptorHandleForHeapStart());
        }

        {}
        // 4-5��������պе������        
        {
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE
                , IID_PPV_ARGS(&g_stSkyBoxPSO.m_pICmdAlloc)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxPSO.m_pICmdAlloc);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE
                , g_stSkyBoxPSO.m_pICmdAlloc.Get(), nullptr, IID_PPV_ARGS(&g_stSkyBoxPSO.m_pICmdBundles)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stSkyBoxPSO.m_pICmdBundles);

            //Skybox�������
            g_stSkyBoxPSO.m_pICmdBundles->SetGraphicsRootSignature(g_stSkyBoxPSO.m_pIRS.Get());
            g_stSkyBoxPSO.m_pICmdBundles->SetPipelineState(g_stSkyBoxPSO.m_pIPSO.Get());

            arHeaps.RemoveAll();
            arHeaps.Add(g_stSkyBoxHeap.m_pICBVSRVHeap.Get());
            arHeaps.Add(g_stSkyBoxHeap.m_pISAPHeap.Get());

            g_stSkyBoxPSO.m_pICmdBundles->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());

            D3D12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleSkybox
                = g_stSkyBoxHeap.m_pICBVSRVHeap->GetGPUDescriptorHandleForHeapStart();
            //����SRV
            g_stSkyBoxPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(0, stGPUCBVHandleSkybox);
            stGPUCBVHandleSkybox.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
            //����CBV
            g_stSkyBoxPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleSkybox);
            g_stSkyBoxPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(2, g_stSkyBoxHeap.m_pISAPHeap->GetGPUDescriptorHandleForHeapStart());

            g_stSkyBoxPSO.m_pICmdBundles->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            g_stSkyBoxPSO.m_pICmdBundles->IASetVertexBuffers(0, 1, &g_stSkyBoxData.m_stVertexBufferView);

            //Draw Call������
            g_stSkyBoxPSO.m_pICmdBundles->DrawInstanced(g_stSkyBoxData.m_nVertexCount, 1, 0, 0);
            g_stSkyBoxPSO.m_pICmdBundles->Close();
        }

        //--------------------------------------------------------------------
        // ����HDR Image �� PBR ��Ⱦ IBL ����
        //--------------------------------------------------------------------
        // ��һ���� �����䲿��
        //--------------------------------------------------------------------
        // 5-1-0����ʼ��Render Target�Ĳ���
        {
            g_stIBLDiffusePreIntRTA.m_nRTWidth = GRS_RTA_IBL_DIFFUSE_MAP_SIZE_W;
            g_stIBLDiffusePreIntRTA.m_nRTHeight = GRS_RTA_IBL_DIFFUSE_MAP_SIZE_H;
            g_stIBLDiffusePreIntRTA.m_stViewPort
                = { 0.0f
                , 0.0f
                , static_cast<float>(g_stIBLDiffusePreIntRTA.m_nRTWidth)
                , static_cast<float>(g_stIBLDiffusePreIntRTA.m_nRTHeight)
                , D3D12_MIN_DEPTH
                , D3D12_MAX_DEPTH };
            g_stIBLDiffusePreIntRTA.m_stScissorRect
                = { 0
                , 0
                , static_cast<LONG>(g_stIBLDiffusePreIntRTA.m_nRTWidth)
                , static_cast<LONG>(g_stIBLDiffusePreIntRTA.m_nRTHeight) };

            g_stIBLDiffusePreIntRTA.m_emRTAFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        // 5-1-1���������� Geometry shader 1Times ��Ⱦ�� Cube Map �� IBL Diffuse Pre Integration PSO
        {
            D3D12_DESCRIPTOR_RANGE1 stDSPRanges[3] = {};
            // 2 Const Buffer ( MVP Matrix + Cube Map 6 View Matrix)
            stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            stDSPRanges[0].NumDescriptors = 2;
            stDSPRanges[0].BaseShaderRegister = 0;
            stDSPRanges[0].RegisterSpace = 0;
            stDSPRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

            // 1 Texture ( HDR Texture ) 
            stDSPRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges[1].NumDescriptors = 1;
            stDSPRanges[1].BaseShaderRegister = 0;
            stDSPRanges[1].RegisterSpace = 0;
            stDSPRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[1].OffsetInDescriptorsFromTableStart = 0;

            // 1 Sampler ( Linear Sampler )
            stDSPRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            stDSPRanges[2].NumDescriptors = 1;
            stDSPRanges[2].BaseShaderRegister = 0;
            stDSPRanges[2].RegisterSpace = 0;
            stDSPRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[2].OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 stRootParameters[3] = {};
            stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//CBV������Shader�ɼ�
            stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

            stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SRV��PS�ɼ�
            stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

            stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SAMPLE��PS�ɼ�
            stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
            stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
            stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
            stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&g_stIBLDiffusePreIntPSO.m_pIRS)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLDiffusePreIntPSO.m_pIRS);

            //--------------------------------------------------------------------------------------------------------------------------------
            UINT nCompileFlags = 0;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            //����Ϊ�о�����ʽ	   
            nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

            TCHAR pszShaderFileName[MAX_PATH] = {};

            ComPtr<ID3DBlob> pIVSCode;
            ComPtr<ID3DBlob> pIGSCode;
            ComPtr<ID3DBlob> pIPSCode;
            ComPtr<ID3DBlob> pIErrorMsg;

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\1-2 GRS_1Times_GS_HDR_2_CubeMap_VS_GS.hlsl")
                , g_pszShaderPath);

            HRESULT hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "VSMain", "vs_5_0", nCompileFlags, 0, &pIVSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Vertex Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "GSMain", "gs_5_0", nCompileFlags, 0, &pIGSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Geometry Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // ����Ҫע�����ǹ��ϵ�ѡ��ʹ�ø������ ���ؿ��� ��� �����������ͼ�㷨�İ汾
            // �����ǻ������صĻ�����ֵ���ֵİ汾
            // ��Ҫ����Ϊ�������ְ汾�ļ�����̫���ˣ�һ�����ػ�������Ҫ�ϰ��������ļ���
            // һ����Կ���������ס�������ջٵĿ���
            // ���ػ������ַ����İ汾������Ϊ���ô���˽Ⲣ����IBL��������ͼ���ɵĻ���ԭ�����
            // ���������ģ�����������ִ�еģ��м��мǣ�
            // ����֣�ؾ���ǧ��Ҫʹ�����ػ��ֵ�����汾�������Կ��ջٻ�����������ܻ������ʧ
            // ���˶�һ�к��������
            //::StringCchPrintf(pszShaderFileName
            //    , MAX_PATH
            //    , _T("%s\\2-1 GRS_IBL_Diffuse_Irradiance_Convolution_With_Integration_PS.hlsl")
            //    , g_pszShaderPath);

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\2-2 GRS_IBL_Diffuse_Irradiance_Convolution_With_Monte_Carlo_PS.hlsl")
                , g_pszShaderPath);

            //::StringCchPrintf(pszShaderFileName
            //    , MAX_PATH
            //    , _T("%s\\GRS_IBL_Diffuse_Irradiance_Convolution_With_Integration_PS.hlsl")
            //    , g_pszShaderPath);

            hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "PSMain", "ps_5_0", nCompileFlags, 0, &pIPSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Pixel Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // �������붥������ݽṹ
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };

            stPSODesc.pRootSignature = g_stIBLDiffusePreIntPSO.m_pIRS.Get();

            // VS -> GS -> PS
            stPSODesc.VS.BytecodeLength = pIVSCode->GetBufferSize();
            stPSODesc.VS.pShaderBytecode = pIVSCode->GetBufferPointer();
            stPSODesc.GS.BytecodeLength = pIGSCode->GetBufferSize();
            stPSODesc.GS.pShaderBytecode = pIGSCode->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIPSCode->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIPSCode->GetBufferPointer();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.SampleDesc.Count = 1;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = g_stIBLDiffusePreIntRTA.m_emRTAFormat;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;
            stPSODesc.DepthStencilState.DepthEnable = FALSE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&g_stIBLDiffusePreIntPSO.m_pIPSO)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLDiffusePreIntPSO.m_pIPSO);

            // IBL Diffuse Pre Integration ��Ⱦʹ����HDR to Cubemap GS��Ⱦ��������ͬ��CBV��Sample ��VB��Const Buffer
            // ����ʹ������Ⱦ�����Ϊ�����SRV
            D3D12_CPU_DESCRIPTOR_HANDLE stHeapHandle = g_st1TimesGSHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
            stHeapHandle.ptr += (g_st1TimesGSHeap.m_nSRVOffset * g_stD3DDevice.m_nCBVSRVDescriptorSize); // ���չ��ߵ�Ҫ��ƫ�Ƶ���ȷ�� SRV ���

            // ���� HDR Texture SRV
            D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
            stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            stSRVDesc.Format = g_st1TimesGSRTAStatus.m_emRTAFormat;
            stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            stSRVDesc.TextureCube.MipLevels = 1;

            g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(g_st1TimesGSRTAStatus.m_pITextureRTA.Get()
                , &stSRVDesc, stHeapHandle);
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 5-1-2������ IBL Diffuse Pre Integration Render Target
        {
            // ע�����ﴴ����������ȾĿ����Դ��������Texture2DArray���ͣ�������6��SubResource
            // �����ʺ� Geometry Shader һ����Ⱦ��� Render Target Array �ķ�ʽ��
            // �� Geometry Shader ��ʹ��ϵͳ���� SV_RenderTargetArrayIndex ��ʶ Render Target��������
            // ������� Texture Render Target �ķ������������� MRT �� GBuffer �У�
            // һ���� Pixel Shader ��ʹ�� SV_Target(n) ����ʶ����� Render Target������ n Ϊ����
            // �磺SV_Target0��SV_Target1��SV_Target2 ��

            // ����������ȾĿ��(Render Target Array Texture2DArray)

            D3D12_RESOURCE_DESC stRenderTargetDesc = g_stD3DDevice.m_pIARenderTargets[0]->GetDesc();
            stRenderTargetDesc.Format = g_stIBLDiffusePreIntRTA.m_emRTAFormat;
            stRenderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            stRenderTargetDesc.Width = g_stIBLDiffusePreIntRTA.m_nRTWidth;
            stRenderTargetDesc.Height = g_stIBLDiffusePreIntRTA.m_nRTHeight;
            stRenderTargetDesc.DepthOrArraySize = GRS_CUBE_MAP_FACE_CNT;

            D3D12_CLEAR_VALUE stClearValue = {};
            stClearValue.Format = g_stIBLDiffusePreIntRTA.m_emRTAFormat;
            stClearValue.Color[0] = g_stD3DDevice.m_faClearColor[0];
            stClearValue.Color[1] = g_stD3DDevice.m_faClearColor[1];
            stClearValue.Color[2] = g_stD3DDevice.m_faClearColor[2];
            stClearValue.Color[3] = g_stD3DDevice.m_faClearColor[3];

            // ��������������Դ
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stDefaultHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &stRenderTargetDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &stClearValue,
                IID_PPV_ARGS(&g_stIBLDiffusePreIntRTA.m_pITextureRTA)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLDiffusePreIntRTA.m_pITextureRTA);


            //����RTV(��ȾĿ����ͼ)��������( ����������������������� )            
            D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
            stRTVHeapDesc.NumDescriptors = 1;   // 1 Render Target Array RTV
            stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stRTVHeapDesc
                , IID_PPV_ARGS(&g_stIBLDiffusePreIntRTA.m_pIRTAHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLDiffusePreIntRTA.m_pIRTAHeap);


            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                = g_stIBLDiffusePreIntRTA.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();

            D3D12_RENDER_TARGET_VIEW_DESC stRTADesc = {};
            stRTADesc.Format = g_stIBLDiffusePreIntRTA.m_emRTAFormat;
            stRTADesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            stRTADesc.Texture2DArray.ArraySize = GRS_CUBE_MAP_FACE_CNT;

            // ������Ӧ��RTV��������
            // �����D3D12_RENDER_TARGET_VIEW_DESC�ṹ����Ҫ��
            // ����ʹ��Texture2DArray��ΪRender Target Arrayʱ����Ҫ��ȷ���Ľṹ�壬
            // ���� Geometry Shader û��ʶ�����Array
            // һ�㴴�� Render Target ʱ���������ṹ��
            // ֱ���� CreateRenderTargetView �ĵڶ�����������nullptr

            g_stD3DDevice.m_pID3D12Device4->CreateRenderTargetView(g_stIBLDiffusePreIntRTA.m_pITextureRTA.Get(), &stRTADesc, stRTVHandle);

            D3D12_RESOURCE_DESC stDownloadResDesc = {};
            stDownloadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            stDownloadResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            stDownloadResDesc.Width = 0;
            stDownloadResDesc.Height = 1;
            stDownloadResDesc.DepthOrArraySize = 1;
            stDownloadResDesc.MipLevels = 1;
            stDownloadResDesc.Format = DXGI_FORMAT_UNKNOWN;
            stDownloadResDesc.SampleDesc.Count = 1;
            stDownloadResDesc.SampleDesc.Quality = 0;
            stDownloadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            stDownloadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            UINT64 n64ResBufferSize = 0;

            g_stD3DDevice.m_pID3D12Device4->GetCopyableFootprints(&stRenderTargetDesc
                , 0, GRS_CUBE_MAP_FACE_CNT, 0, nullptr, nullptr, nullptr, &n64ResBufferSize);

            stDownloadResDesc.Width = n64ResBufferSize;

            // �����������Ҫ����Ⱦ�Ľ�����Ƴ�������Ҫ�Ļ���Ҫ�洢��ָ�����ļ��������ȱ��Read Back��
            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_READBACK;
            // Download Heap == Upload Heap ���ǹ����ڴ棡
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &stDownloadResDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&g_stIBLDiffusePreIntRTA.m_pITextureRTADownload)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLDiffusePreIntRTA.m_pITextureRTADownload);

            // ��ԭΪ�ϴ������ԣ���������Ҫ��
            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        }

        // 5-1-3����Ⱦ����IBL Diffuse Map��Ԥ���ּ���
        {}
        {
            // �ȴ�ǰ���Copy����ִ�н���
            if (WAIT_OBJECT_0 != ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE))
            {
                ::AtlThrowLastWin32();
            }
            // ��ʼִ�� GS Cube Map��Ⱦ����HDR�Ⱦ��������� һ������Ⱦ��6��RTV
            //�����������Resetһ��
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDAlloc->Reset());
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Reset(g_stD3DDevice.m_pIMainCMDAlloc.Get(), nullptr));

            g_stD3DDevice.m_pIMainCMDList->RSSetViewports(1, &g_stIBLDiffusePreIntRTA.m_stViewPort);
            g_stD3DDevice.m_pIMainCMDList->RSSetScissorRects(1, &g_stIBLDiffusePreIntRTA.m_stScissorRect);

            // ������ȾĿ�� ע��һ�ξͰ����е�������ȾĿ��Ž�ȥ
            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                = g_stIBLDiffusePreIntRTA.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();
            g_stD3DDevice.m_pIMainCMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);

            // ������¼�����������ʼ��Ⱦ
            g_stD3DDevice.m_pIMainCMDList->ClearRenderTargetView(stRTVHandle, g_stD3DDevice.m_faClearColor, 0, nullptr);

            //-----------------------------------------------------------------------------------------------------
            // Draw !
            //-----------------------------------------------------------------------------------------------------
            // ��ȾCube
            // ������Ⱦ����״̬����
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootSignature(g_stIBLDiffusePreIntPSO.m_pIRS.Get());
            g_stD3DDevice.m_pIMainCMDList->SetPipelineState(g_stIBLDiffusePreIntPSO.m_pIPSO.Get());

            // IBL Diffuse Pre Integration ��Ⱦʹ����HDR to Cubemap GS��Ⱦ������
            // ��ͬ��CBV��Sample ��VB��Const Buffer
            // ����ʹ������Ⱦ�����Ϊ�����SRV
            arHeaps.RemoveAll();
            arHeaps.Add(g_st1TimesGSHeap.m_pICBVSRVHeap.Get());
            arHeaps.Add(g_st1TimesGSHeap.m_pISAPHeap.Get());

            g_stD3DDevice.m_pIMainCMDList->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());
            D3D12_GPU_DESCRIPTOR_HANDLE stGPUHandle = g_st1TimesGSHeap.m_pICBVSRVHeap->GetGPUDescriptorHandleForHeapStart();
            // ����CBV
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(0, stGPUHandle);
            // ����SRV
            stGPUHandle.ptr += (g_st1TimesGSHeap.m_nSRVOffset * g_stD3DDevice.m_nCBVSRVDescriptorSize);
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(1, stGPUHandle);
            // ����Sample
            stGPUHandle = g_st1TimesGSHeap.m_pISAPHeap->GetGPUDescriptorHandleForHeapStart();
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(2, stGPUHandle);

            // ��Ⱦ�ַ��������δ�
            g_stD3DDevice.m_pIMainCMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            // ����������Ϣ
            g_stD3DDevice.m_pIMainCMDList->IASetVertexBuffers(0, 1, &g_stBoxData_Strip.m_stVertexBufferView);
            // Draw Call
            g_stD3DDevice.m_pIMainCMDList->DrawInstanced(g_stBoxData_Strip.m_nVertexCount, 1, 0, 0);
            //-----------------------------------------------------------------------------------------------------
            // ʹ����Դ�����л���״̬��ʵ���������Ҫʹ���⼸����ȾĿ��Ĺ��̽���ͬ��
            g_stIBLDiffusePreIntRTA.m_stRTABarriers.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            g_stIBLDiffusePreIntRTA.m_stRTABarriers.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            g_stIBLDiffusePreIntRTA.m_stRTABarriers.Transition.pResource = g_stIBLDiffusePreIntRTA.m_pITextureRTA.Get();
            g_stIBLDiffusePreIntRTA.m_stRTABarriers.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            g_stIBLDiffusePreIntRTA.m_stRTABarriers.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            g_stIBLDiffusePreIntRTA.m_stRTABarriers.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            //��Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
            g_stD3DDevice.m_pIMainCMDList->ResourceBarrier(1, &g_stIBLDiffusePreIntRTA.m_stRTABarriers);

            //�ر������б�����ȥִ����
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());
            //ִ�������б�
            g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

            //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
            const UINT64 n64CurrentFenceValue = g_stD3DDevice.m_n64FenceValue;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), n64CurrentFenceValue));
            g_stD3DDevice.m_n64FenceValue++;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(n64CurrentFenceValue, g_stD3DDevice.m_hEventFence));
        }
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        // �ڶ����� ���淴�䲿��
        //--------------------------------------------------------------------
        // 5-2-0����ʼ�� Render Target ����
        {
            g_stIBLSpecularPreIntRTA.m_nRTWidth = GRS_RTA_IBL_SPECULAR_MAP_SIZE_W;
            g_stIBLSpecularPreIntRTA.m_nRTHeight = GRS_RTA_IBL_SPECULAR_MAP_SIZE_H;
            g_stIBLSpecularPreIntRTA.m_stViewPort = { 0.0f, 0.0f, static_cast<float>(g_stIBLSpecularPreIntRTA.m_nRTWidth), static_cast<float>(g_stIBLDiffusePreIntRTA.m_nRTHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
            g_stIBLSpecularPreIntRTA.m_stScissorRect = { 0, 0, static_cast<LONG>(g_stIBLSpecularPreIntRTA.m_nRTWidth), static_cast<LONG>(g_stIBLDiffusePreIntRTA.m_nRTHeight) };
            g_stIBLSpecularPreIntRTA.m_emRTAFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        // 5-2-1���������� Geometry shader 1Times ��Ⱦ�� Cube Map �� IBL Specular Pre Integration PSO
        {
            // ע�� ���淴��Ԥ���ֵ�ʱ�� �õ��� PBR Material �е� �ֲڶȲ��� ����Ҫ����5 ����������
            // ʵ��������ֻʹ������һ������

            D3D12_DESCRIPTOR_RANGE1 stDSPRanges[3] = {};
            // 5 Const Buffer ( MVP Matrix + Cube Map 6 View Matrix)
            stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            stDSPRanges[0].NumDescriptors = 5;
            stDSPRanges[0].BaseShaderRegister = 0;
            stDSPRanges[0].RegisterSpace = 0;
            stDSPRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
            stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

            // 1 Texture ( HDR Texture ) 
            stDSPRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges[1].NumDescriptors = 1;
            stDSPRanges[1].BaseShaderRegister = 0;
            stDSPRanges[1].RegisterSpace = 0;
            stDSPRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[1].OffsetInDescriptorsFromTableStart = 0;

            // 1 Sampler ( Linear Sampler )
            stDSPRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            stDSPRanges[2].NumDescriptors = 1;
            stDSPRanges[2].BaseShaderRegister = 0;
            stDSPRanges[2].RegisterSpace = 0;
            stDSPRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[2].OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 stRootParameters[3] = {};
            stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//CBV������Shader�ɼ�
            stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

            stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SRV��PS�ɼ�
            stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

            stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SAMPLE��PS�ɼ�
            stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
            stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
            stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
            stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&g_stIBLSpecularPreIntPSO.m_pIRS)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLSpecularPreIntPSO.m_pIRS);

            //--------------------------------------------------------------------------------------------------------------------------------
            UINT nCompileFlags = 0;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            //����Ϊ�о�����ʽ	   
            nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

            TCHAR pszShaderFileName[MAX_PATH] = {};

            ComPtr<ID3DBlob> pIVSCode;
            ComPtr<ID3DBlob> pIGSCode;
            ComPtr<ID3DBlob> pIPSCode;
            ComPtr<ID3DBlob> pIErrorMsg;

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\1-2 GRS_1Times_GS_HDR_2_CubeMap_VS_GS.hlsl"), g_pszShaderPath);

            HRESULT hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "VSMain", "vs_5_0", nCompileFlags, 0, &pIVSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Vertex Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            hr = ::D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "GSMain", "gs_5_0", nCompileFlags, 0, &pIGSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Geometry Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\3-1 GRS_IBL_Specular_Pre_Integration_PS.hlsl")
                , g_pszShaderPath);

            hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "PSMain", "ps_5_0", nCompileFlags, 0, &pIPSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Pixel Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // �������붥������ݽṹ
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };

            stPSODesc.pRootSignature = g_stIBLSpecularPreIntPSO.m_pIRS.Get();

            // VS -> GS -> PS
            stPSODesc.VS.BytecodeLength = pIVSCode->GetBufferSize();
            stPSODesc.VS.pShaderBytecode = pIVSCode->GetBufferPointer();
            stPSODesc.GS.BytecodeLength = pIGSCode->GetBufferSize();
            stPSODesc.GS.pShaderBytecode = pIGSCode->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIPSCode->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIPSCode->GetBufferPointer();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            //stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
            // �ص������޳�
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.SampleDesc.Count = 1;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = g_stIBLSpecularPreIntRTA.m_emRTAFormat;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;
            stPSODesc.DepthStencilState.DepthEnable = FALSE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&g_stIBLSpecularPreIntPSO.m_pIPSO)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLSpecularPreIntPSO.m_pIPSO);

            // IBL Specular Pre Integration ��Ⱦ
            // ʹ����HDR to Cubemap GS��Ⱦ��������ͬ��CBV��Sample ��VB��Const Buffer
            // ����ʹ������Ⱦ�����Ϊ�����SRV
            // ֮ǰ Diffuse ��Ⱦ�Ѿ��������� ����Ͳ����ٴ����� ֱ�Ӹ��ü���
        }

        {}//�����������Ŀջ������Ǹ���ģ���Ϊ�����ʾʧЧ�ˣ�ǰ������Ϳ��Իָ������ʾ VS2019��BUG VS2022���ɡ�����������
        // 5-2-2������ IBL Specular Pre Integration Render Target����Ҫ�Բ��������ؿ�����֣�
        {
            // ע�����ﴴ����������ȾĿ����Դ��������Texture2DArray���ͣ�������6��SubResource
            // �����ʺ� Geometry Shader һ����Ⱦ��� Render Target Array �ķ�ʽ��
            // �� Geometry Shader ��ʹ��ϵͳ���� SV_RenderTargetArrayIndex ��ʶ Render Target��������
            // ������� Texture Render Target �ķ������������� MRT �� GBuffer �У�
            // һ���� Pixel Shader ��ʹ�� SV_Target(n) ����ʶ����� Render Target������ n Ϊ����
            // �磺SV_Target0��SV_Target1��SV_Target2 ��

            // ����������ȾĿ��(Render Target Array Texture2DArray)

            D3D12_RESOURCE_DESC stRenderTargetDesc = g_stD3DDevice.m_pIARenderTargets[0]->GetDesc();
            stRenderTargetDesc.Format = g_stIBLSpecularPreIntRTA.m_emRTAFormat;
            stRenderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            stRenderTargetDesc.Width = g_stIBLSpecularPreIntRTA.m_nRTWidth;
            stRenderTargetDesc.Height = g_stIBLSpecularPreIntRTA.m_nRTHeight;
            // 6���桢ÿ����5��Mip���ܹ�5*6 = 30��SubResource
            stRenderTargetDesc.DepthOrArraySize = GRS_CUBE_MAP_FACE_CNT;
            stRenderTargetDesc.MipLevels = GRS_RTA_IBL_SPECULAR_MAP_MIP;


            D3D12_CLEAR_VALUE stClearValue = {};
            stClearValue.Format = g_stIBLSpecularPreIntRTA.m_emRTAFormat;
            stClearValue.Color[0] = g_stD3DDevice.m_faClearColor[0];
            stClearValue.Color[1] = g_stD3DDevice.m_faClearColor[1];
            stClearValue.Color[2] = g_stD3DDevice.m_faClearColor[2];
            stClearValue.Color[3] = g_stD3DDevice.m_faClearColor[3];

            // ��������������Դ
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stDefaultHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &stRenderTargetDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &stClearValue,
                IID_PPV_ARGS(&g_stIBLSpecularPreIntRTA.m_pITextureRTA)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLSpecularPreIntRTA.m_pITextureRTA);

            //����RTV(��ȾĿ����ͼ)��������( ����������������������� )
            D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
            // GRS_RTA_IBL_SPECULAR_MAP_MIP �� Render Target Array RTV
            stRTVHeapDesc.NumDescriptors = GRS_RTA_IBL_SPECULAR_MAP_MIP;
            stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stRTVHeapDesc
                , IID_PPV_ARGS(&g_stIBLSpecularPreIntRTA.m_pIRTAHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLSpecularPreIntRTA.m_pIRTAHeap);

            // ������Ӧ��RTV��������
            // �����D3D12_RENDER_TARGET_VIEW_DESC�ṹ����Ҫ��
            // ����ʹ��Texture2DArray��ΪRender Target Arrayʱ����Ҫ��ȷ���Ľṹ�壬
            // ���� Geometry Shader û��ʶ�����Array
            // һ�㴴�� Render Target ʱ���������ṹ��
            // ֱ���� CreateRenderTargetView �ĵڶ�����������nullptr

            D3D12_RENDER_TARGET_VIEW_DESC stRTADesc = {};
            stRTADesc.Format = g_stIBLSpecularPreIntRTA.m_emRTAFormat;
            stRTADesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            stRTADesc.Texture2DArray.ArraySize = GRS_CUBE_MAP_FACE_CNT;

            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                = g_stIBLSpecularPreIntRTA.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();

            for (UINT i = 0; i < GRS_RTA_IBL_SPECULAR_MAP_MIP; i++)
            {
                stRTADesc.Texture2DArray.MipSlice = i;

                g_stD3DDevice.m_pID3D12Device4->CreateRenderTargetView(
                    g_stIBLSpecularPreIntRTA.m_pITextureRTA.Get()
                    , &stRTADesc
                    , stRTVHandle);

                stRTVHandle.ptr += g_stD3DDevice.m_nRTVDescriptorSize;
            }

            D3D12_RESOURCE_DESC stDownloadResDesc = {};
            stDownloadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            stDownloadResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            stDownloadResDesc.Width = 0;
            stDownloadResDesc.Height = 1;
            stDownloadResDesc.DepthOrArraySize = 1;
            stDownloadResDesc.MipLevels = 1;
            stDownloadResDesc.Format = DXGI_FORMAT_UNKNOWN;
            stDownloadResDesc.SampleDesc.Count = 1;
            stDownloadResDesc.SampleDesc.Quality = 0;
            stDownloadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            stDownloadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            UINT64 n64ResBufferSize = 0;

            stRenderTargetDesc = g_stIBLSpecularPreIntRTA.m_pITextureRTA->GetDesc();

            g_stD3DDevice.m_pID3D12Device4->GetCopyableFootprints(&stRenderTargetDesc
                , 0, GRS_CUBE_MAP_FACE_CNT, 0, nullptr, nullptr, nullptr, &n64ResBufferSize);

            stDownloadResDesc.Width = n64ResBufferSize;

            // �����������Ҫ����Ⱦ�Ľ�����Ƴ�������Ҫ�Ļ���Ҫ�洢��ָ�����ļ��������ȱ��Read Back��
            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_READBACK;
            // Download Heap == Upload Heap ���ǹ����ڴ棡
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &stDownloadResDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&g_stIBLSpecularPreIntRTA.m_pITextureRTADownload)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLSpecularPreIntRTA.m_pITextureRTADownload);

            // ��ԭΪ�ϴ������ԣ���������Ҫ��
            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        }

        // 5-2-3����Ⱦ����IBL Specular Map��Ԥ���ּ��㣨��Ҫ�Բ��������ؿ�����֣�
        {}
        {
            // IBL Specular Pre Integration ��Ⱦʹ����HDR to Cubemap GS��Ⱦ������
            // ��ͬ��CBV��Sample ��VB��Const Buffer
            // ����ʹ������Ⱦ�����Ϊ�����SRV
            arHeaps.RemoveAll();
            arHeaps.Add(g_st1TimesGSHeap.m_pICBVSRVHeap.Get());
            arHeaps.Add(g_st1TimesGSHeap.m_pISAPHeap.Get());

            D3D12_RESOURCE_BARRIER stSubResBarrier[GRS_CUBE_MAP_FACE_CNT] = {};
            for (UINT i = 0; i < GRS_CUBE_MAP_FACE_CNT; i++)
            {
                stSubResBarrier[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                stSubResBarrier[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                stSubResBarrier[i].Transition.pResource = g_stIBLSpecularPreIntRTA.m_pITextureRTA.Get();
                stSubResBarrier[i].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                stSubResBarrier[i].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                stSubResBarrier[i].Transition.Subresource = 0;
            }

            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                = g_stIBLSpecularPreIntRTA.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();

            for (UINT i = 0; i < GRS_RTA_IBL_SPECULAR_MAP_MIP; i++)
            {// ����MipLevelѭ�� i �� Mip Slice
                // �ȴ�ǰ�������ִ�н���
                if (WAIT_OBJECT_0 != ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE))
                {
                    ::AtlThrowLastWin32();
                }

                // ����MipLevel
                UINT nRTWidth = g_stIBLSpecularPreIntRTA.m_nRTWidth >> i;
                UINT nRTHeight = g_stIBLSpecularPreIntRTA.m_nRTHeight >> i;

                D3D12_VIEWPORT stViewPort
                    = { 0.0f
                    , 0.0f
                    , static_cast<float>(nRTWidth)
                    , static_cast<float>(nRTHeight)
                    , D3D12_MIN_DEPTH
                    , D3D12_MAX_DEPTH };
                D3D12_RECT     stScissorRect
                    = { 0, 0, static_cast<LONG>(nRTWidth), static_cast<LONG>(nRTHeight) };

                // Ԥ���˵�ʱ�� ÿ��MipLevel ��Ӧһ���ֲڶȵȼ� ����MipMapʱ �������Բ�ֵ�ᱣ֤��ȷ�Ľ��
                g_stWorldConstData.m_pstPBRMaterial->m_fRoughness = (float)i / (float)(GRS_RTA_IBL_SPECULAR_MAP_MIP - 1);
                g_stWorldConstData.m_pstPBRMaterial->m_v4Albedo = { 1.0f,1.0f,1.0f };
                g_stWorldConstData.m_pstPBRMaterial->m_fMetallic = 1.0f;
                g_stWorldConstData.m_pstPBRMaterial->m_fAO = 0.1f;

                // ��ʼִ�� GS Cube Map��Ⱦ��һ������Ⱦ��6��RTV
                //�����������Resetһ��
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDAlloc->Reset());
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Reset(g_stD3DDevice.m_pIMainCMDAlloc.Get(), nullptr));

                g_stD3DDevice.m_pIMainCMDList->RSSetViewports(1, &stViewPort);
                g_stD3DDevice.m_pIMainCMDList->RSSetScissorRects(1, &stScissorRect);

                g_stD3DDevice.m_pIMainCMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);

                // ������¼�����������ʼ��Ⱦ
                g_stD3DDevice.m_pIMainCMDList->ClearRenderTargetView(stRTVHandle, g_stD3DDevice.m_faClearColor, 0, nullptr);
                //-----------------------------------------------------------------------------------------------------
                // Draw !
                //-----------------------------------------------------------------------------------------------------
                // ��ȾCube
                // ������Ⱦ����״̬����
                g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootSignature(g_stIBLSpecularPreIntPSO.m_pIRS.Get());
                g_stD3DDevice.m_pIMainCMDList->SetPipelineState(g_stIBLSpecularPreIntPSO.m_pIPSO.Get());

                g_stD3DDevice.m_pIMainCMDList->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());
                D3D12_GPU_DESCRIPTOR_HANDLE stGPUHandle = g_st1TimesGSHeap.m_pICBVSRVHeap->GetGPUDescriptorHandleForHeapStart();
                // ����CBV
                g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(0, stGPUHandle);
                // ����SRV
                stGPUHandle.ptr += (g_st1TimesGSHeap.m_nSRVOffset * g_stD3DDevice.m_nCBVSRVDescriptorSize);
                g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(1, stGPUHandle);
                // ����Sample
                stGPUHandle = g_st1TimesGSHeap.m_pISAPHeap->GetGPUDescriptorHandleForHeapStart();
                g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(2, stGPUHandle);

                // ��Ⱦ�ַ��������δ�
                g_stD3DDevice.m_pIMainCMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                // ����������Ϣ
                g_stD3DDevice.m_pIMainCMDList->IASetVertexBuffers(0, 1, &g_stBoxData_Strip.m_stVertexBufferView);
                // Draw Call
                g_stD3DDevice.m_pIMainCMDList->DrawInstanced(g_stBoxData_Strip.m_nVertexCount, 1, 0, 0);
                //-----------------------------------------------------------------------------------------------------
                // ʹ����Դ�����л���״̬��ʵ���������Ҫʹ���⼸����ȾĿ��Ĺ��̽���ͬ��
                for (UINT j = 0; j < GRS_CUBE_MAP_FACE_CNT; j++)
                {// ����������Դ
                    stSubResBarrier[j].Transition.Subresource = (j * GRS_RTA_IBL_SPECULAR_MAP_MIP) + i;
                }

                //��Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
                g_stD3DDevice.m_pIMainCMDList->ResourceBarrier(GRS_CUBE_MAP_FACE_CNT, stSubResBarrier);

                //�ر������б�����ȥִ����
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());
                //ִ�������б�
                g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

                //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
                const UINT64 n64CurrentFenceValue = g_stD3DDevice.m_n64FenceValue;
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), n64CurrentFenceValue));
                g_stD3DDevice.m_n64FenceValue++;
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(n64CurrentFenceValue, g_stD3DDevice.m_hEventFence));

                stRTVHandle.ptr += g_stD3DDevice.m_nRTVDescriptorSize;
            }
        }
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        // �������� LUT���� ������ BRDF ������ͼ��
        //--------------------------------------------------------------------        
        {}
        // 5-3-0����ʼ�� BRDF������ͼ ��ز���
        {
            g_stIBLLutRTA.m_nRTWidth = GRS_RTA_IBL_LUT_SIZE_W;
            g_stIBLLutRTA.m_nRTHeight = GRS_RTA_IBL_LUT_SIZE_H;
            g_stIBLLutRTA.m_stViewPort
                = { 0.0f
                , 0.0f
                , static_cast<float>(g_stIBLLutRTA.m_nRTWidth)
                , static_cast<float>(g_stIBLLutRTA.m_nRTHeight)
                , D3D12_MIN_DEPTH
                , D3D12_MAX_DEPTH };
            g_stIBLLutRTA.m_stScissorRect
                = { 0
                , 0
                , static_cast<LONG>(g_stIBLLutRTA.m_nRTWidth)
                , static_cast<LONG>(g_stIBLLutRTA.m_nRTHeight) };

            g_stIBLLutRTA.m_emRTAFormat = GRS_RTA_IBL_LUT_FORMAT;
        }

        // 5-3-1������ BRDF ������ͼ PSO
        {
            // LUT PSO ����Ҫ��������������� 
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
            stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            stRootSignatureDesc.Desc_1_1.NumParameters = 0;
            stRootSignatureDesc.Desc_1_1.pParameters = nullptr;
            stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&g_stIBLLutPSO.m_pIRS)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLLutPSO.m_pIRS);

            //--------------------------------------------------------------------------------------------------------------------------------
            UINT nCompileFlags = 0;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            //����Ϊ�о�����ʽ	   
            nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

            TCHAR pszShaderFileName[MAX_PATH] = {};

            ComPtr<ID3DBlob> pIVSCode;
            ComPtr<ID3DBlob> pIPSCode;
            ComPtr<ID3DBlob> pIErrorMsg;

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\3-2 GRS_IBL_BRDF_Integration_LUT.hlsl"), g_pszShaderPath);

            HRESULT hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "VSMain", "vs_5_0", nCompileFlags, 0, &pIVSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Vertex Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "PSMain", "ps_5_0", nCompileFlags, 0, &pIPSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Pixel Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // �������붥������ݽṹ
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };

            stPSODesc.pRootSignature = g_stIBLLutPSO.m_pIRS.Get();

            // VS -> PS
            stPSODesc.VS.BytecodeLength = pIVSCode->GetBufferSize();
            stPSODesc.VS.pShaderBytecode = pIVSCode->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIPSCode->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIPSCode->GetBufferPointer();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            //stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
            // �ص������޳�
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.SampleDesc.Count = 1;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = g_stIBLLutRTA.m_emRTAFormat;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;
            stPSODesc.DepthStencilState.DepthEnable = FALSE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&g_stIBLLutPSO.m_pIPSO)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLLutPSO.m_pIPSO);
        }

        {}
        // 5-3-2������ BRDF������ͼ Render Target����Ҫ�Բ��������ؿ�����֣�
        {
            // ����������ȾĿ��(Render Target Array Texture2DArray)
            D3D12_RESOURCE_DESC stRenderTargetDesc = g_stD3DDevice.m_pIARenderTargets[0]->GetDesc();
            stRenderTargetDesc.Format = g_stIBLLutRTA.m_emRTAFormat;
            stRenderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            stRenderTargetDesc.Width = g_stIBLLutRTA.m_nRTWidth;
            stRenderTargetDesc.Height = g_stIBLLutRTA.m_nRTHeight;
            stRenderTargetDesc.DepthOrArraySize = 1;
            stRenderTargetDesc.MipLevels = 1;


            D3D12_CLEAR_VALUE stClearValue = {};
            stClearValue.Format = g_stIBLLutRTA.m_emRTAFormat;
            stClearValue.Color[0] = g_stD3DDevice.m_faClearColor[0];
            stClearValue.Color[1] = g_stD3DDevice.m_faClearColor[1];
            stClearValue.Color[2] = g_stD3DDevice.m_faClearColor[2];
            stClearValue.Color[3] = g_stD3DDevice.m_faClearColor[3];

            // ��������������Դ
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stDefaultHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &stRenderTargetDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &stClearValue,
                IID_PPV_ARGS(&g_stIBLLutRTA.m_pITextureRTA)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLLutRTA.m_pITextureRTA);


            //����RTV(��ȾĿ����ͼ)��������( ����������������������� )
            D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
            // GRS_RTA_IBL_SPECULAR_MAP_MIP �� Render Target Array RTV
            stRTVHeapDesc.NumDescriptors = 1;
            stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stRTVHeapDesc
                , IID_PPV_ARGS(&g_stIBLLutRTA.m_pIRTAHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLLutRTA.m_pIRTAHeap);

            // ������Ӧ��RTV��������
            // �����D3D12_RENDER_TARGET_VIEW_DESC�ṹ����Ҫ��
            // ����ʹ��Texture2DArray��ΪRender Target Arrayʱ����Ҫ��ȷ���Ľṹ�壬
            // ���� Geometry Shader û��ʶ�����Array
            // һ�㴴�� Render Target ʱ���������ṹ��
            // ֱ���� CreateRenderTargetView �ĵڶ�����������nullptr

            D3D12_RENDER_TARGET_VIEW_DESC stRTADesc = {};
            stRTADesc.Format = g_stIBLLutRTA.m_emRTAFormat;
            stRTADesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                = g_stIBLLutRTA.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();

            g_stD3DDevice.m_pID3D12Device4->CreateRenderTargetView(
                g_stIBLLutRTA.m_pITextureRTA.Get()
                , &stRTADesc
                , stRTVHandle);

            D3D12_RESOURCE_DESC stDownloadResDesc = {};
            stDownloadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            stDownloadResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            stDownloadResDesc.Width = 0;
            stDownloadResDesc.Height = 1;
            stDownloadResDesc.DepthOrArraySize = 1;
            stDownloadResDesc.MipLevels = 1;
            stDownloadResDesc.Format = DXGI_FORMAT_UNKNOWN;
            stDownloadResDesc.SampleDesc.Count = 1;
            stDownloadResDesc.SampleDesc.Quality = 0;
            stDownloadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            stDownloadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            UINT64 n64ResBufferSize = 0;

            stRenderTargetDesc = g_stIBLLutRTA.m_pITextureRTA->GetDesc();

            g_stD3DDevice.m_pID3D12Device4->GetCopyableFootprints(&stRenderTargetDesc
                , 0, 1, 0, nullptr, nullptr, nullptr, &n64ResBufferSize);

            stDownloadResDesc.Width = n64ResBufferSize;

            // �����������Ҫ����Ⱦ�Ľ�����Ƴ�������Ҫ�Ļ���Ҫ�洢��ָ�����ļ��������ȱ��Read Back��
            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_READBACK;
            // Download Heap == Upload Heap ���ǹ����ڴ棡
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &stDownloadResDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&g_stIBLLutRTA.m_pITextureRTADownload)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLLutRTA.m_pITextureRTADownload);

            // ��ԭΪ�ϴ������ԣ���������Ҫ��
            g_stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        }

        // 5-3-3������ BRDF������ͼ ��Ⱦ��Ҫ�� Quad ����
        {
            ST_GRS_VERTEX_QUAD_SIMPLE stLUTQuadData[] = {
                { { -1.0f , -1.0f , 0.0f, 1.0f }, { 0.0f, 1.0f } },	// Bottom left.
                { { -1.0f ,  1.0f , 0.0f, 1.0f }, { 0.0f, 0.0f } },	// Top left.
                { {  1.0f , -1.0f , 0.0f, 1.0f }, { 1.0f, 1.0f } },	// Bottom right.
                { {  1.0f ,  1.0f , 0.0f, 1.0f }, { 1.0f, 0.0f } }, // Top right.
            };

            g_stIBLLutData.m_nVertexCount = _countof(stLUTQuadData);

            g_stBufferResSesc.Width = GRS_UPPER(sizeof(stLUTQuadData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&g_stIBLLutData.m_pIVertexBuffer)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLLutData.m_pIVertexBuffer);

            UINT8* pBufferData = nullptr;
            GRS_THROW_IF_FAILED(g_stIBLLutData.m_pIVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pBufferData)));
            memcpy(pBufferData, stLUTQuadData, sizeof(stLUTQuadData));
            g_stIBLLutData.m_pIVertexBuffer->Unmap(0, nullptr);

            g_stIBLLutData.m_stVertexBufferView.BufferLocation = g_stIBLLutData.m_pIVertexBuffer->GetGPUVirtualAddress();
            g_stIBLLutData.m_stVertexBufferView.StrideInBytes = sizeof(ST_GRS_VERTEX_QUAD_SIMPLE);
            g_stIBLLutData.m_stVertexBufferView.SizeInBytes = sizeof(stLUTQuadData);
        }

        // 5-3-4����Ⱦ����LUT
        {
            D3D12_RESOURCE_BARRIER stSubResBarrier = {};

            stSubResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            stSubResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            stSubResBarrier.Transition.pResource = g_stIBLLutRTA.m_pITextureRTA.Get();
            stSubResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            stSubResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            stSubResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                = g_stIBLLutRTA.m_pIRTAHeap->GetCPUDescriptorHandleForHeapStart();

            // �ȴ�ǰ�������ִ�н���
            if (WAIT_OBJECT_0 != ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE))
            {
                ::AtlThrowLastWin32();
            }

            // ��ʼִ����Ⱦ
            // �����������Resetһ��
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDAlloc->Reset());
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Reset(g_stD3DDevice.m_pIMainCMDAlloc.Get(), nullptr));

            g_stD3DDevice.m_pIMainCMDList->RSSetViewports(1, &g_stIBLLutRTA.m_stViewPort);
            g_stD3DDevice.m_pIMainCMDList->RSSetScissorRects(1, &g_stIBLLutRTA.m_stScissorRect);

            g_stD3DDevice.m_pIMainCMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);

            // ������¼�����������ʼ��Ⱦ
            g_stD3DDevice.m_pIMainCMDList->ClearRenderTargetView(stRTVHandle, g_stD3DDevice.m_faClearColor, 0, nullptr);
            //-----------------------------------------------------------------------------------------------------
            // Draw Call !
            //-----------------------------------------------------------------------------------------------------
            // ��Ⱦ Quad
            // ������Ⱦ����״̬����
            g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootSignature(g_stIBLLutPSO.m_pIRS.Get());
            g_stD3DDevice.m_pIMainCMDList->SetPipelineState(g_stIBLLutPSO.m_pIPSO.Get());

            // ��Ⱦ�ַ��������δ�
            g_stD3DDevice.m_pIMainCMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            // ����������Ϣ
            g_stD3DDevice.m_pIMainCMDList->IASetVertexBuffers(0, 1, &g_stIBLLutData.m_stVertexBufferView);
            // Draw Call
            g_stD3DDevice.m_pIMainCMDList->DrawInstanced(g_stIBLLutData.m_nVertexCount, 1, 0, 0);

            //-----------------------------------------------------------------------------------------------------
            // ʹ����Դ�����л���״̬��ʵ���������Ҫʹ���⼸����ȾĿ��Ĺ��̽���ͬ��
            //��Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
            g_stD3DDevice.m_pIMainCMDList->ResourceBarrier(1, &stSubResBarrier);

            //�ر������б�����ȥִ����
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());
            //ִ�������б�
            g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

            //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
            const UINT64 n64CurrentFenceValue = g_stD3DDevice.m_n64FenceValue;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), n64CurrentFenceValue));
            g_stD3DDevice.m_n64FenceValue++;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(n64CurrentFenceValue, g_stD3DDevice.m_hEventFence));

        }
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        // PBR IBL Render Resource ��Data ��Multi-Instance
        //--------------------------------------------------------------------   

        // 6-1-1������ PBR IBL Render PSO
        {
            g_stIBLPSO.m_nCBVCnt = 6;
            g_stIBLPSO.m_nSRVCnt = 3 + 6;
            g_stIBLPSO.m_nSmpCnt = 1;
            g_stIBLPSO.m_nCnt = g_stIBLPSO.m_nCBVCnt + g_stIBLPSO.m_nSRVCnt;

            D3D12_DESCRIPTOR_RANGE1 stDSPRanges[4] = {};
            // ������Ⱦ�ϳ���Ҫ ���е�6�� ����������
            stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            stDSPRanges[0].NumDescriptors = g_stIBLPSO.m_nCBVCnt;
            stDSPRanges[0].BaseShaderRegister = 0;
            stDSPRanges[0].RegisterSpace = 0;
            stDSPRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
            stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

            // 3 Texture ( ����Ԥ���� Texture + 1 LUT Texture ) 
            stDSPRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges[1].NumDescriptors = 3;
            stDSPRanges[1].BaseShaderRegister = 0;
            stDSPRanges[1].RegisterSpace = 0;       // register space 0
            stDSPRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[1].OffsetInDescriptorsFromTableStart = 0;

            // 6 Texture (6�� PBR ��������) 
            stDSPRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges[2].NumDescriptors = 6;
            stDSPRanges[2].BaseShaderRegister = 0;
            stDSPRanges[2].RegisterSpace = 1;       // register space 1
            stDSPRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
            stDSPRanges[2].OffsetInDescriptorsFromTableStart = 0;

            // 1 Sampler ( Linear Sampler )
            stDSPRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            stDSPRanges[3].NumDescriptors = g_stIBLPSO.m_nSmpCnt;
            stDSPRanges[3].BaseShaderRegister = 0;
            stDSPRanges[3].RegisterSpace = 0;
            stDSPRanges[3].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[3].OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 stRootParameters[4] = {};
            stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//CBV������Shader�ɼ�
            stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

            stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//SRV��PS�ɼ�
            stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

            stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//SRV��PS�ɼ�
            stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];

            stRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//SAMPLE��PS�ɼ�
            stRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[3].DescriptorTable.pDescriptorRanges = &stDSPRanges[3];

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
            stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
            stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
            stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&g_stIBLPSO.m_pIRS)));

            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLPSO.m_pIRS);
            //--------------------------------------------------------------------------------------------------------------------------------
            UINT nCompileFlags = 0;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            //����Ϊ�о�����ʽ	   
            nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

            TCHAR pszShaderFileName[MAX_PATH] = {};

            ComPtr<ID3DBlob> pIVSCode;
            ComPtr<ID3DBlob> pIPSCode;
            ComPtr<ID3DBlob> pIErrorMsg;

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\4-1 GRS_PBR_IBL_VS_Multi_Instance.hlsl")
                , g_pszShaderPath);

            HRESULT hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "VSMain", "vs_5_1", nCompileFlags, 0, &pIVSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Vertex Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            ::StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\4-2 GRS_PBR_IBL_PS_Texture.hlsl")
                , g_pszShaderPath);

            hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "PSMain", "ps_5_1", nCompileFlags, 0, &pIPSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Pixel Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // �������붥������ݽṹ
            // ��ΪҪʹ�� Assimp �����������Զ��������Ҫ���۴���
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                // ǰ������ÿ�������ݴ�3����۴���
                { "POSITION",0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,      2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };

            stPSODesc.pRootSignature = g_stIBLPSO.m_pIRS.Get();

            // VS -> PS
            stPSODesc.VS.BytecodeLength = pIVSCode->GetBufferSize();
            stPSODesc.VS.pShaderBytecode = pIVSCode->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIPSCode->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIPSCode->GetBufferPointer();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.SampleDesc.Count = 1;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = g_stD3DDevice.m_emRenderTargetFormat; // ���������Ⱦ����Ļ��
            stPSODesc.DepthStencilState.StencilEnable = FALSE;
            stPSODesc.DepthStencilState.DepthEnable = FALSE;

            stPSODesc.DSVFormat = g_stD3DDevice.m_emDepthStencilFormat;
            stPSODesc.DepthStencilState.DepthEnable = TRUE;
            stPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//������Ȼ���д�빦��
            stPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;     //��Ȳ��Ժ�������ֵΪ��ͨ����Ȳ��ԣ�
            stPSODesc.DepthStencilState.StencilEnable = FALSE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc
                , IID_PPV_ARGS(&g_stIBLPSO.m_pIPSO)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLPSO.m_pIPSO);
        }

        {}
        // 6-1-2 ���� Sample Heap & Sample
        {
            // ���� Sample Heap
            D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
            stSamplerHeapDesc.NumDescriptors = g_stIBLPSO.m_nSmpCnt;   // 1 Sample
            stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSamplerHeapDesc
                , IID_PPV_ARGS(&g_stIBLHeap.m_pISAPHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLHeap.m_pISAPHeap);
            // ����һ�����Թ��˵ı�Ե���еĲ�����
            D3D12_SAMPLER_DESC stSamplerDesc = {};
            stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            stSamplerDesc.MinLOD = 0;
            stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            stSamplerDesc.MipLODBias = 0.0f;
            stSamplerDesc.MaxAnisotropy = 1;
            stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            stSamplerDesc.BorderColor[0] = 0.0f;
            stSamplerDesc.BorderColor[1] = 0.0f;
            stSamplerDesc.BorderColor[2] = 0.0f;
            stSamplerDesc.BorderColor[3] = 0.0f;
            stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

            g_stD3DDevice.m_pID3D12Device4->CreateSampler(&stSamplerDesc
                , g_stIBLHeap.m_pISAPHeap->GetCPUDescriptorHandleForHeapStart());
        }

        {}
        // 6-1-3������ PBR Material Texture & Descriptor Heap & CBV��SRV
        {
            g_arPBRMaterial.SetCount(5);
            size_t szIndex = 0;

            // ���� PBR ����������������·����
            {
                TCHAR pszMaterialPath[MAX_PATH] = {};
                ::StringCchPrintf(pszMaterialPath
                    , MAX_PATH
                    , _T("%sAssets\\PBR6PIC")
                    , g_pszAppPath
                );

                // Material 1 : copper rock
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAlbedo
                    , MAX_PATH
                    , _T("%s\\1-copper-rock1\\copper-rock1-alb.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszDisplacement
                    , MAX_PATH
                    , _T("%s\\1-copper-rock1\\copper-rock1-height.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszNormal
                    , MAX_PATH
                    , _T("%s\\1-copper-rock1\\copper-rock1-normal.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszMetallic
                    , MAX_PATH
                    , _T("%s\\1-copper-rock1\\copper-rock1-metal.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszRoughness
                    , MAX_PATH
                    , _T("%s\\1-copper-rock1\\copper-rock1-rough.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAO
                    , MAX_PATH
                    , _T("%s\\1-copper-rock1\\copper-rock1-ao.png")
                    , pszMaterialPath
                );

                ++szIndex;
                // Material 2 : copper rock
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAlbedo
                    , MAX_PATH
                    , _T("%s\\2-fancy-brass-pattern1\\fancy-brass-pattern1_albedo.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszDisplacement
                    , MAX_PATH
                    , _T("%s\\2-fancy-brass-pattern1\\fancy-brass-pattern1_height.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszNormal
                    , MAX_PATH
                    , _T("%s\\2-fancy-brass-pattern1\\fancy-brass-pattern1_normal-ogl.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszMetallic
                    , MAX_PATH
                    , _T("%s\\2-fancy-brass-pattern1\\fancy-brass-pattern1_metallic.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszRoughness
                    , MAX_PATH
                    , _T("%s\\2-fancy-brass-pattern1\\fancy-brass-pattern1_roughness.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAO
                    , MAX_PATH
                    , _T("%s\\2-fancy-brass-pattern1\\fancy-brass-pattern1_ao.png")
                    , pszMaterialPath
                );

                ++szIndex;
                // Material 3 : metal ventilation
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAlbedo
                    , MAX_PATH
                    , _T("%s\\3-metal-ventilation1\\metal-ventilation1-albedo.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszDisplacement
                    , MAX_PATH
                    , _T("%s\\3-metal-ventilation1\\metal-ventilation1-height.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszNormal
                    , MAX_PATH
                    , _T("%s\\3-metal-ventilation1\\metal-ventilation1-normal-ogl.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszMetallic
                    , MAX_PATH
                    , _T("%s\\3-metal-ventilation1\\metal-ventilation1-metallic.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszRoughness
                    , MAX_PATH
                    , _T("%s\\3-metal-ventilation1\\metal-ventilation1-roughness.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAO
                    , MAX_PATH
                    , _T("%s\\3-metal-ventilation1\\metal-ventilation1-ao.png")
                    , pszMaterialPath
                );

                ++szIndex;
                // Material 4 : pirate gold
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAlbedo
                    , MAX_PATH
                    , _T("%s\\4-pirate-gold\\pirate-gold_albedo.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszDisplacement
                    , MAX_PATH
                    , _T("%s\\4-pirate-gold\\pirate-gold_height.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszNormal
                    , MAX_PATH
                    , _T("%s\\4-pirate-gold\\pirate-gold_normal-ogl.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszMetallic
                    , MAX_PATH
                    , _T("%s\\4-pirate-gold\\pirate-gold_metallic.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszRoughness
                    , MAX_PATH
                    , _T("%s\\4-pirate-gold\\pirate-gold_roughness.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAO
                    , MAX_PATH
                    , _T("%s\\4-pirate-gold\\pirate-gold_ao.png")
                    , pszMaterialPath
                );

                ++szIndex;
                // Material 5 : ribbed chipped metal
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAlbedo
                    , MAX_PATH
                    , _T("%s\\5-ribbed-chipped-metal\\ribbed-chipped-metal_albedo.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszDisplacement
                    , MAX_PATH
                    , _T("%s\\5-ribbed-chipped-metal\\ribbed-chipped-metal_height.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszNormal
                    , MAX_PATH
                    , _T("%s\\5-ribbed-chipped-metal\\ribbed-chipped-metal_normal-ogl.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszMetallic
                    , MAX_PATH
                    , _T("%s\\5-ribbed-chipped-metal\\ribbed-chipped-metal_metallic.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszRoughness
                    , MAX_PATH
                    , _T("%s\\5-ribbed-chipped-metal\\ribbed-chipped-metal_roughness.png")
                    , pszMaterialPath
                );
                ::StringCchPrintf(g_arPBRMaterial[szIndex].m_pszAO
                    , MAX_PATH
                    , _T("%s\\5-ribbed-chipped-metal\\ribbed-chipped-metal_ao.png")
                    , pszMaterialPath
                );
            }

            // ��ʾ�����ϵĴ�����Էŵ��ű����棬��Ϊ��������ʾ�̶̳��ѣ���û��������Щ���µĶ����ˣ�ֻ�Ǽ򵥵�����
            ID3D12Resource* pITexture = nullptr;
            ID3D12Resource* pITextureUpload = nullptr;

            // ���� CBV SRV Heap
            D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
            stSRVHeapDesc.NumDescriptors = g_stIBLPSO.m_nCnt;
            stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            D3D12_CPU_DESCRIPTOR_HANDLE stCBVSRVHandle = {};

            D3D12_RESOURCE_DESC stResDesc = {};
            D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
            D3D12_CONSTANT_BUFFER_VIEW_DESC stCBVDesc = {};

            stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

            // �ȴ�ǰ�������ִ�н���
            if (WAIT_OBJECT_0 != ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE))
            {
                ::AtlThrowLastWin32();
            }
            // �����������Resetһ�� , ��Ϊ�������µ���ԴҪ�ϴ����Դ棬��Ҫʹ���������
            // һ����������ר�Ŵ���һ���������������ִ����Щ��Դ�ϴ��Ĺ�����ʾ�������ж���Щ�����˼�
            // ����ĸ���������е��÷�֮ǰ�������ж��У�����ȥ�ο�

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDAlloc->Reset());
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Reset(g_stD3DDevice.m_pIMainCMDAlloc.Get(), nullptr));

            for (szIndex = 0; szIndex < g_arPBRMaterial.GetCount(); ++szIndex)
            {
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSRVHeapDesc
                    , IID_PPV_ARGS(&g_arPBRMaterial[szIndex].m_pIMaterialHeap)));

                GRS_SetD3D12DebugNameIndexed(g_arPBRMaterial[szIndex].m_pIMaterialHeap.Get()
                    , L"g_arPBRMaterial.m_pIMaterialHeap"
                    , (UINT)szIndex);

                // ����ÿ�� Model �� Const Buffer
                g_stBufferResSesc.Width = GRS_UPPER(sizeof(ST_GRS_MODEL_DATA), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                    &g_stUploadHeapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &g_stBufferResSesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&g_arPBRMaterial[szIndex].m_pICBModelData)));
                GRS_SET_D3D12_DEBUGNAME_COMPTR(g_arPBRMaterial[szIndex].m_pICBModelData);
                // ӳ����������� ���� Unmap�� ÿ��ֱ��д�뼴��
                GRS_THROW_IF_FAILED(g_arPBRMaterial[szIndex].m_pICBModelData->Map(0
                    , nullptr
                    , reinterpret_cast<void**>(&g_arPBRMaterial[szIndex].m_pstModelData)));

                stCBVSRVHandle
                    = g_arPBRMaterial[szIndex].m_pIMaterialHeap->GetCPUDescriptorHandleForHeapStart();

                D3D12_CONSTANT_BUFFER_VIEW_DESC stCBVDesc = {};
                stCBVDesc.BufferLocation = g_stWorldConstData.m_pISceneMatrixBuffer->GetGPUVirtualAddress();
                stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_CB_SCENE_MATRIX)
                    , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
                //1��Scene Matrix
                g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVSRVHandle);

                stCBVDesc.BufferLocation = g_stWorldConstData.m_pIGSMatrixBuffer->GetGPUVirtualAddress();
                stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_CB_GS_VIEW_MATRIX)
                    , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
                stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
                //2��GS Views
                g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVSRVHandle);

                stCBVDesc.BufferLocation = g_stWorldConstData.m_pISceneCameraBuffer->GetGPUVirtualAddress();
                stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_SCENE_CAMERA)
                    , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
                stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
                //3��Camera
                g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVSRVHandle);

                stCBVDesc.BufferLocation = g_stWorldConstData.m_pISceneLightsBuffer->GetGPUVirtualAddress();
                stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_SCENE_LIGHTS)
                    , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
                stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
                //4��Lights
                g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVSRVHandle);

                stCBVDesc.BufferLocation = g_stWorldConstData.m_pIPBRMaterialBuffer->GetGPUVirtualAddress();
                stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_PBR_MATERIAL)
                    , D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
                stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;

                //5��PBR Material
                g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVSRVHandle);

                stCBVDesc.BufferLocation = g_arPBRMaterial[szIndex].m_pICBModelData->GetGPUVirtualAddress();
                stCBVDesc.SizeInBytes = (UINT)g_arPBRMaterial[szIndex].m_pICBModelData->GetDesc().Width;
                //����һ�����ڻ���������CBV,֮��ֱ�Ӹ��ƽ�ȥ
                stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;

                // 6��Model Data
                g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, stCBVSRVHandle);

                // ���� SRV
                stResDesc = g_stIBLSpecularPreIntRTA.m_pITextureRTA->GetDesc();

                stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                stSRVDesc.Format = stResDesc.Format;
                stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                stSRVDesc.TextureCube.MipLevels = stResDesc.MipLevels;

                stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
                // 1��Specular Pre Int Texture Srv
                g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(g_stIBLSpecularPreIntRTA.m_pITextureRTA.Get(), &stSRVDesc, stCBVSRVHandle);

                stResDesc = g_stIBLDiffusePreIntRTA.m_pITextureRTA->GetDesc();

                stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                stSRVDesc.Format = stResDesc.Format;
                stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                stSRVDesc.TextureCube.MipLevels = stResDesc.MipLevels;
                stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
                // 2��Diffuse Pre Int Texture Srv
                g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(g_stIBLDiffusePreIntRTA.m_pITextureRTA.Get(), &stSRVDesc, stCBVSRVHandle);

                stResDesc = g_stIBLLutRTA.m_pITextureRTA->GetDesc();

                stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                stSRVDesc.Format = stResDesc.Format;
                stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                stSRVDesc.Texture2D.MipLevels = stResDesc.MipLevels;
                stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
                // 3��BRDF LUT Texture Srv
                g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(g_stIBLLutRTA.m_pITextureRTA.Get(), &stSRVDesc, stCBVSRVHandle);

                // ˳����� PBR �Ĳ�������
                for (size_t szTxNo = 0; szTxNo < GRS_PBR_MATERIAL_TEXTURE_CNT; ++szTxNo)
                {
                    if (!LoadTextureFromFile(g_arPBRMaterial[szIndex].m_pszPBRMaterialPathName[szTxNo]
                        , g_stD3DDevice.m_pIMainCMDList.Get()
                        , pITextureUpload
                        , pITexture))
                    {
                        AtlThrowLastWin32();
                    }

                    g_arPBRMaterial[szIndex].m_arTexMaterial.Add(ComPtr<ID3D12Resource>(pITexture));
                    g_arPBRMaterial[szIndex].m_arTexMaterialUpload.Add(ComPtr<ID3D12Resource>(pITextureUpload));

                    stResDesc = g_arPBRMaterial[szIndex].m_arTexMaterial[szTxNo]->GetDesc();

                    stSRVDesc.Format = stResDesc.Format;
                    stSRVDesc.TextureCube.MipLevels = stResDesc.MipLevels;

                    stCBVSRVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;

                    g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(
                        g_arPBRMaterial[szIndex].m_arTexMaterial[szTxNo].Get()
                        , &stSRVDesc
                        , stCBVSRVHandle);
                }
            }

            // �ر������б�����ȥִ����
            // �����ϴ�֮ǰ��һ��PBR������Դ���Դ��У�����Կ����Դ����Ǹ���ս
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());
            //ִ�������б�
            g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

            //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
            const UINT64 n64CurrentFenceValue = g_stD3DDevice.m_n64FenceValue;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), n64CurrentFenceValue));
            g_stD3DDevice.m_n64FenceValue++;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(n64CurrentFenceValue, g_stD3DDevice.m_hEventFence));
        }

        {}
        // 6-1-4��������������
        {
            TCHAR pszMeshFile[MAX_PATH] = {};
            StringCchPrintf(pszMeshFile, MAX_PATH, _T("%sAssets\\The3DModel\\sphere.x"), g_pszAppPath);
#ifdef GRS_USE_DIR_DIALOG
            OPENFILENAME stOfn = {};

            RtlZeroMemory(&stOfn, sizeof(stOfn));
            stOfn.lStructSize = sizeof(stOfn);
            stOfn.hwndOwner = ::GetDesktopWindow(); //g_stD3DDevice.m_hWnd;
            stOfn.lpstrFilter = _T("x�ļ�\0*.x\0obj�ļ�\0*.obj\0�����ļ�\0*.*\0");
            stOfn.lpstrFile = pszMeshFile;
            stOfn.nMaxFile = MAX_PATH;
            stOfn.lpstrTitle = _T("��ѡ��һ�� Mesh �ļ�...");
            stOfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (!::GetOpenFileName(&stOfn))
            {
                StringCchPrintf(pszMeshFile, MAX_PATH, _T("%sAssets\\misc\\sphere.x"), g_pszAppPath);
            }
#endif

            ST_GRS_MESH_DATA stMeshData = {};
            if (!LoadMesh(T2A(pszMeshFile), stMeshData))
            {
                ::AtlThrowLastWin32();
            }

            // Vertex Data
            size_t szPositions
                = stMeshData.m_arPositions.GetCount()
                * sizeof(stMeshData.m_arPositions[0]);
            size_t szNormals
                = stMeshData.m_arNormals.GetCount()
                * sizeof(stMeshData.m_arNormals[0]);
            size_t szTexCoords
                = stMeshData.m_arTexCoords.GetCount()
                * sizeof(stMeshData.m_arTexCoords[0]);

            // Index Data
            size_t szIndices
                = stMeshData.m_arIndices.GetCount()
                * sizeof(stMeshData.m_arIndices[0]);

            g_stMultiMeshData.m_nIndexCount = (UINT)stMeshData.m_arIndices.GetCount();
            if (2 == sizeof(stMeshData.m_arIndices[0]))
            {// UINT16 ������
                g_stMultiMeshData.m_emIndexType = DXGI_FORMAT_R16_UINT;
            }
            else
            {// ����ͳͳ��Ϊ�� UINT32 �����������õ��ģ�Assimp������Ǳ�֤
                g_stMultiMeshData.m_emIndexType = DXGI_FORMAT_R32_UINT;
            }

            UINT nSlot = 0;
            ComPtr<ID3D12Resource> pIBuffer;
            //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
            // Slot 0 Positions
            g_stBufferResSesc.Width = szPositions;
            //���� Vertex Buffer ��ʹ��Upload��ʽ��
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&pIBuffer)));
            g_stMultiMeshData.m_arIVB.Add(pIBuffer);
            GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(g_stMultiMeshData.m_arIVB, nSlot);

            //ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
            UINT8* pData = nullptr;

            GRS_THROW_IF_FAILED(g_stMultiMeshData.m_arIVB[nSlot]->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
            memcpy(pData, stMeshData.m_arPositions.GetData(), szPositions);
            g_stMultiMeshData.m_arIVB[nSlot]->Unmap(0, nullptr);

            D3D12_VERTEX_BUFFER_VIEW stVBV = {};
            //����Vertex Buffer View
            stVBV.BufferLocation = g_stMultiMeshData.m_arIVB[nSlot]->GetGPUVirtualAddress();
            stVBV.StrideInBytes = sizeof(stMeshData.m_arPositions[0]);
            stVBV.SizeInBytes = (UINT)szPositions;

            g_stMultiMeshData.m_arVBV.Add(stVBV);
            //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

            //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
            // Slot 1 Normals
            nSlot = 1;
            g_stBufferResSesc.Width = szNormals;
            //���� Vertex Buffer ��ʹ��Upload��ʽ��
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&pIBuffer)));
            g_stMultiMeshData.m_arIVB.Add(pIBuffer);
            GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(g_stMultiMeshData.m_arIVB, nSlot);

            //ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
            pData = nullptr;

            GRS_THROW_IF_FAILED(g_stMultiMeshData.m_arIVB[nSlot]->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
            memcpy(pData, stMeshData.m_arNormals.GetData(), szNormals);
            g_stMultiMeshData.m_arIVB[nSlot]->Unmap(0, nullptr);

            //����Vertex Buffer View
            stVBV.BufferLocation = g_stMultiMeshData.m_arIVB[nSlot]->GetGPUVirtualAddress();
            stVBV.StrideInBytes = sizeof(stMeshData.m_arNormals[0]);
            stVBV.SizeInBytes = (UINT)szNormals;

            g_stMultiMeshData.m_arVBV.Add(stVBV);

            //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
            // Slot 2 Texture Coords
            nSlot = 2;
            g_stBufferResSesc.Width = szTexCoords;
            //���� Vertex Buffer ��ʹ��Upload��ʽ��
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&pIBuffer)));
            g_stMultiMeshData.m_arIVB.Add(pIBuffer);
            GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(g_stMultiMeshData.m_arIVB, nSlot);

            //ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
            pData = nullptr;

            GRS_THROW_IF_FAILED(g_stMultiMeshData.m_arIVB[nSlot]->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
            memcpy(pData, stMeshData.m_arTexCoords.GetData(), szTexCoords);
            g_stMultiMeshData.m_arIVB[nSlot]->Unmap(0, nullptr);

            //����Vertex Buffer View
            stVBV.BufferLocation = g_stMultiMeshData.m_arIVB[nSlot]->GetGPUVirtualAddress();
            stVBV.StrideInBytes = sizeof(stMeshData.m_arTexCoords[0]);
            stVBV.SizeInBytes = (UINT)szTexCoords;
            g_stMultiMeshData.m_arVBV.Add(stVBV);
            //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

            // Index Buffer �е�����ר��ͨ�� ����ռ�� Slot
            //���� Index Buffer ��ʹ��Upload��ʽ��
            g_stBufferResSesc.Width = szIndices;
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&g_stMultiMeshData.m_pIIB)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stMultiMeshData.m_pIIB);

            pData = nullptr;
            GRS_THROW_IF_FAILED(g_stMultiMeshData.m_pIIB->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
            memcpy(pData, stMeshData.m_arIndices.GetData(), szIndices);
            g_stMultiMeshData.m_pIIB->Unmap(0, nullptr);

            //����Index Buffer View
            g_stMultiMeshData.m_stIBV.BufferLocation = g_stMultiMeshData.m_pIIB->GetGPUVirtualAddress();
            g_stMultiMeshData.m_stIBV.Format = DXGI_FORMAT_R32_UINT;
            g_stMultiMeshData.m_stIBV.SizeInBytes = g_stMultiMeshData.m_nIndexCount * sizeof(UINT);
        }

        // 6-1-5������PBR��Ⱦ�������
        {}
        {
            //GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE
            //    , IID_PPV_ARGS(&g_stIBLPSO.m_pICmdAlloc)));
            //GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLPSO.m_pICmdAlloc);

            //GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE
            //    , g_stIBLPSO.m_pICmdAlloc.Get(), nullptr, IID_PPV_ARGS(&g_stIBLPSO.m_pICmdBundles)));
            //GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stIBLPSO.m_pICmdBundles);

            //g_stIBLPSO.m_pICmdBundles->SetGraphicsRootSignature(g_stIBLPSO.m_pIRS.Get());
            //g_stIBLPSO.m_pICmdBundles->SetPipelineState(g_stIBLPSO.m_pIPSO.Get());

            //// ����������
            //D3D12_CPU_DESCRIPTOR_HANDLE stDestHandle
            //    = g_stIBLHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
            //
            //stDestHandle.ptr += ((g_stIBLPSO.m_nCBVCnt - 1) * g_stD3DDevice.m_nCBVSRVDescriptorSize);

            //D3D12_CPU_DESCRIPTOR_HANDLE stSrcHandle
            //    = g_arPBRMaterial[0].m_pIMaterialHeap->GetCPUDescriptorHandleForHeapStart();

            //UINT nDesRangeSizes = GRS_MODEL_DATA_CB_CNT;
            //UINT nSrcRangeSizes = GRS_MODEL_DATA_CB_CNT;
            //// Copy 1 CBV
            //g_stD3DDevice.m_pID3D12Device4->CopyDescriptors(
            //    1
            //    , &stDestHandle
            //    , &nDesRangeSizes
            //    , 1
            //    , &stSrcHandle
            //    , &nSrcRangeSizes
            //    , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            //stDestHandle.ptr += ((1 + 3) * g_stD3DDevice.m_nCBVSRVDescriptorSize);

            //stSrcHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;

            //nDesRangeSizes = GRS_PBR_MATERIAL_TEXTURE_CNT;
            //nSrcRangeSizes = GRS_PBR_MATERIAL_TEXTURE_CNT;

            //// Copy 6 SRV
            //g_stD3DDevice.m_pID3D12Device4->CopyDescriptors(
            //    1
            //    , &stDestHandle
            //    , &nDesRangeSizes
            //    , 1
            //    , &stSrcHandle
            //    , &nSrcRangeSizes
            //    , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            //arHeaps.RemoveAll();
            //arHeaps.Add(g_stIBLHeap.m_pICBVSRVHeap.Get());
            //arHeaps.Add(g_stIBLHeap.m_pISAPHeap.Get());

            //g_stIBLPSO.m_pICmdBundles->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());
            //
            //D3D12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandle 
            //    = g_stIBLHeap.m_pICBVSRVHeap->GetGPUDescriptorHandleForHeapStart();
            ////����CBV
            //g_stIBLPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(0, stGPUCBVHandle);

            //// ����SRV (space 0)
            //stGPUCBVHandle.ptr += (g_stIBLHeap.m_nSRVOffset * g_stD3DDevice.m_nCBVSRVDescriptorSize);
            //g_stIBLPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(1, stGPUCBVHandle);

            //// ����SRV (space 1)
            //stGPUCBVHandle.ptr += (3 * g_stD3DDevice.m_nCBVSRVDescriptorSize);
            //g_stIBLPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(2, stGPUCBVHandle);

            //// ����Sample
            //g_stIBLPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(3, g_stIBLHeap.m_pISAPHeap->GetGPUDescriptorHandleForHeapStart());

            //// ������Ⱦ�ַ�
            //g_stIBLPSO.m_pICmdBundles->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            //// ���ö�������
            //g_stIBLPSO.m_pICmdBundles->IASetVertexBuffers(0
            //    , (UINT)g_stMultiMeshData.m_arVBV.GetCount()
            //    , g_stMultiMeshData.m_arVBV.GetData());
            //g_stIBLPSO.m_pICmdBundles->IASetIndexBuffer(&g_stMultiMeshData.m_stIBV);
            //
            //// Draw Call������   
            //g_stIBLPSO.m_pICmdBundles->DrawIndexedInstanced(g_stMultiMeshData.m_nIndexCount, 1, 0, 0, 0);
            //
            //g_stIBLPSO.m_pICmdBundles->Close();
        }

        //--------------------------------------------------------------------
        {}
        // 7-0������Ҫ���ο���ʾ������ȫ����ӵ����ο�����������
        {
            g_stQuadData.m_ppITexture.Add(g_stHDRData.m_pIHDRTextureRaw);
            g_stQuadData.m_ppITexture.Add(g_st1TimesGSRTAStatus.m_pITextureRTA);
            // 6�� ��Ⱦ�Ľ����ʱ�Ȳ���ʾ�� 
            //g_stQuadData.m_ppITexture.Add(g_st6TimesRTAStatus.m_pITextureRTA);
            g_stQuadData.m_ppITexture.Add(g_stIBLDiffusePreIntRTA.m_pITextureRTA);
            g_stQuadData.m_ppITexture.Add(g_stIBLSpecularPreIntRTA.m_pITextureRTA);
            g_stQuadData.m_ppITexture.Add(g_stIBLLutRTA.m_pITextureRTA);

            g_stQuadData.m_nInstanceCount = 0;
            D3D12_RESOURCE_DESC stResDes = {};
            for (size_t i = 0; i < g_stQuadData.m_ppITexture.GetCount(); i++)
            {// ����ÿ�� Array MipLevels Element�����������
                stResDes = g_stQuadData.m_ppITexture[i]->GetDesc();
                g_stQuadData.m_nInstanceCount += stResDes.DepthOrArraySize * stResDes.MipLevels;
            }
        }

        // 7-1��������Ⱦ�����õĹ��ߵĸ�ǩ����״̬����
        {
            //������Ⱦ���εĸ�ǩ������
            D3D12_DESCRIPTOR_RANGE1 stDSPRanges[3] = {};
            stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            stDSPRanges[0].NumDescriptors = 1;
            stDSPRanges[0].BaseShaderRegister = 0;
            stDSPRanges[0].RegisterSpace = 0;
            stDSPRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
            stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

            stDSPRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges[1].NumDescriptors = g_stQuadData.m_nInstanceCount;  // 1 HDR Texture + 6 Render Target Texture
            stDSPRanges[1].BaseShaderRegister = 0;
            stDSPRanges[1].RegisterSpace = 0;
            stDSPRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
            stDSPRanges[1].OffsetInDescriptorsFromTableStart = 0;

            stDSPRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            stDSPRanges[2].NumDescriptors = 1;
            stDSPRanges[2].BaseShaderRegister = 0;
            stDSPRanges[2].RegisterSpace = 0;
            stDSPRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            stDSPRanges[2].OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 stRootParameters[3] = {};
            stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

            stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

            stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
            stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
            stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
            stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
            stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&g_stQuadPSO.m_pIRS)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stQuadPSO.m_pIRS);

            UINT nCompileFlags = 0;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

            TCHAR pszShaderFileName[MAX_PATH] = {};
            StringCchPrintf(pszShaderFileName
                , MAX_PATH
                , _T("%s\\6-1 GRS_Quad_With_Dynamic_Index_Multi_Instance.hlsl")
                , g_pszShaderPath);

            ComPtr<ID3DBlob> pIVSCode;
            ComPtr<ID3DBlob> pIPSCode;
            ComPtr<ID3DBlob> pIErrorMsg;

            // VS
            HRESULT hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "VSMain", "vs_5_1", nCompileFlags, 0, &pIVSCode, &pIErrorMsg);
            if (FAILED(hr))
            {
                ATLTRACE("���� Vertex Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();
            // ����Ŀ�������򿪶�̬������������
            nCompileFlags |= D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
            hr = D3DCompileFromFile(pszShaderFileName, nullptr, &grsD3DCompilerInclude
                , "PSMain", "ps_5_1", nCompileFlags, 0, &pIPSCode, &pIErrorMsg);

            if (FAILED(hr))
            {
                ATLTRACE("���� Pixel Shader��\"%s\" ��������%s\n"
                    , T2A(pszShaderFileName)
                    , pIErrorMsg ? pIErrorMsg->GetBufferPointer() : "�޷���ȡ�ļ���");
                GRS_THROW_IF_FAILED(hr);
            }
            pIErrorMsg.Reset();

            // �������붥������ݽṹ��ע��������ж�ʵ��������
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
                { "COLOR",	  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },

                { "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "COLOR",    1, DXGI_FORMAT_R32_UINT,           1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };
            stPSODesc.pRootSignature = g_stQuadPSO.m_pIRS.Get();

            stPSODesc.VS.BytecodeLength = pIVSCode->GetBufferSize();
            stPSODesc.VS.pShaderBytecode = pIVSCode->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIPSCode->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIPSCode->GetBufferPointer();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.DepthStencilState.DepthEnable = FALSE;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.SampleDesc.Count = 1;

            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = g_stD3DDevice.m_emRenderTargetFormat;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc
                , IID_PPV_ARGS(&g_stQuadPSO.m_pIPSO)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stQuadPSO.m_pIPSO);

            // �������ο���Ⱦ��Ҫ��SRV CBV Sample��
            D3D12_DESCRIPTOR_HEAP_DESC stHeapDesc = {};
            stHeapDesc.NumDescriptors = 1 + g_stQuadData.m_nInstanceCount; // 1 CBV + n SRV
            stHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            stHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            //CBV SRV��
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stHeapDesc
                , IID_PPV_ARGS(&g_stQuadHeap.m_pICBVSRVHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stQuadHeap.m_pICBVSRVHeap);

            //Sample
            D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
            stSamplerHeapDesc.NumDescriptors = 1;  //ֻ��һ��Sample
            stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateDescriptorHeap(&stSamplerHeapDesc
                , IID_PPV_ARGS(&g_stQuadHeap.m_pISAPHeap)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stQuadHeap.m_pISAPHeap);

            // Sample View
            D3D12_SAMPLER_DESC stSamplerDesc = {};
            stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            stSamplerDesc.MinLOD = 0;
            stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            stSamplerDesc.MipLODBias = 0.0f;
            stSamplerDesc.MaxAnisotropy = 1;
            stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

            g_stD3DDevice.m_pID3D12Device4->CreateSampler(&stSamplerDesc
                , g_stQuadHeap.m_pISAPHeap->GetCPUDescriptorHandleForHeapStart());
        }
        {}
        // 7-2���������ο�Ķ��㻺��
        {
            //���ο�Ķ���ṹ����
            //ע�����ﶨ����һ����λ��С�������Σ�����ͨ�����ź�λ�ƾ���任�����ʵĴ�С���������������Spirit��UI��2D��Ⱦ
            // ע�����ﶥ���w���꣬�����趨Ϊ1.0f����������ʱ��ߴ粻�������ش�С���������ųߴ� * 1/w �����Ե�wС��1ʱ�����ַŴ���һ���ߴ�
            ST_GRS_VERTEX_QUAD stTriangleVertices[] =
            {
                { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f },	{ 0.0f, 0.0f }  },
                { { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f },	{ 1.0f, 0.0f }  },
                { { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },	{ 0.0f, 1.0f }  },
                { { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f },	{ 1.0f, 1.0f }  }
            };

            g_stQuadData.m_nVertexCount = _countof(stTriangleVertices);

            UINT nSlotIndex = 0;
            // Slot 0
            g_stBufferResSesc.Width = GRS_UPPER(sizeof(stTriangleVertices), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&g_stQuadData.m_ppIVertexBuffer[nSlotIndex])));
            GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(g_stQuadData.m_ppIVertexBuffer, nSlotIndex);

            UINT8* pBufferData = nullptr;
            GRS_THROW_IF_FAILED(g_stQuadData.m_ppIVertexBuffer[nSlotIndex]->Map(0, nullptr, reinterpret_cast<void**>(&pBufferData)));
            memcpy(pBufferData, stTriangleVertices, sizeof(stTriangleVertices));
            g_stQuadData.m_ppIVertexBuffer[nSlotIndex]->Unmap(0, nullptr);

            g_stQuadData.m_pstVertexBufferView[nSlotIndex].BufferLocation = g_stQuadData.m_ppIVertexBuffer[nSlotIndex]->GetGPUVirtualAddress();
            g_stQuadData.m_pstVertexBufferView[nSlotIndex].StrideInBytes = sizeof(ST_GRS_VERTEX_QUAD);
            g_stQuadData.m_pstVertexBufferView[nSlotIndex].SizeInBytes = sizeof(stTriangleVertices);

            // Slot 1
            nSlotIndex = 1;
            UINT szSlot1Buffer = g_stQuadData.m_nInstanceCount * sizeof(ST_GRS_PER_QUAD_INSTANCE_DATA);
            g_stBufferResSesc.Width = GRS_UPPER(szSlot1Buffer, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &g_stBufferResSesc
                , D3D12_RESOURCE_STATE_GENERIC_READ
                , nullptr
                , IID_PPV_ARGS(&g_stQuadData.m_ppIVertexBuffer[nSlotIndex])));
            GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(g_stQuadData.m_ppIVertexBuffer, nSlotIndex);

            ST_GRS_PER_QUAD_INSTANCE_DATA* pstQuadInstanceData = nullptr;
            GRS_THROW_IF_FAILED(g_stQuadData.m_ppIVertexBuffer[nSlotIndex]->Map(0, nullptr, reinterpret_cast<void**>(&pstQuadInstanceData)));

            float fOffsetX = 2.0f;
            float fOffsetY = fOffsetX;
            float fWidth = 200.f;
            float fHeight = fWidth;
            size_t szCol = 10;

            for (size_t i = 0; i < g_stQuadData.m_nInstanceCount; i++)
            {
                pstQuadInstanceData[i].m_nTextureIndex = (UINT)i;
                // ������
                pstQuadInstanceData[i].m_mxQuad2World = XMMatrixScaling(fWidth, fHeight, 1.0f);
                // ��ƫ�Ƶ����ʵ�λ��
                pstQuadInstanceData[i].m_mxQuad2World = XMMatrixMultiply(
                    pstQuadInstanceData[i].m_mxQuad2World
                    , XMMatrixTranslation(
                        (i % szCol) * (fWidth + fOffsetX) + fOffsetX
                        , (i / szCol) * (fHeight + fOffsetY) + fOffsetY
                        , 0.0f)
                );
            }

            g_stQuadData.m_ppIVertexBuffer[nSlotIndex]->Unmap(0, nullptr);

            g_stQuadData.m_pstVertexBufferView[nSlotIndex].BufferLocation
                = g_stQuadData.m_ppIVertexBuffer[nSlotIndex]->GetGPUVirtualAddress();
            g_stQuadData.m_pstVertexBufferView[nSlotIndex].StrideInBytes
                = sizeof(ST_GRS_PER_QUAD_INSTANCE_DATA);
            g_stQuadData.m_pstVertexBufferView[nSlotIndex].SizeInBytes
                = szSlot1Buffer;

            //--------------------------------------------------------------------
            // ����Scene Matrix ��������
            g_stBufferResSesc.Width = GRS_UPPER(sizeof(ST_GRS_CB_SCENE_MATRIX), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stUploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &g_stBufferResSesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&g_stQuadConstData.m_pISceneMatrixBuffer)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stQuadConstData.m_pISceneMatrixBuffer);
            // ӳ��� Scene �������� ���� Unmap�� ÿ��ֱ��д�뼴��
            GRS_THROW_IF_FAILED(g_stQuadConstData.m_pISceneMatrixBuffer->Map(0, nullptr, reinterpret_cast<void**>(&g_stQuadConstData.m_pstSceneMatrix)));

            // ��ʼ����������
            g_stQuadConstData.m_pstSceneMatrix->m_mxWorld = XMMatrixIdentity();
            g_stQuadConstData.m_pstSceneMatrix->m_mxView = XMMatrixIdentity();

            g_stQuadConstData.m_pstSceneMatrix->m_mxProj = XMMatrixScaling(1.0f, -1.0f, 1.0f); //���·�ת�����֮ǰ��Quad��Vertex����һ��ʹ��
            g_stQuadConstData.m_pstSceneMatrix->m_mxProj = XMMatrixMultiply(g_stQuadConstData.m_pstSceneMatrix->m_mxProj, XMMatrixTranslation(-0.5f, +0.5f, 0.0f)); //΢��ƫ�ƣ���������λ��
            g_stQuadConstData.m_pstSceneMatrix->m_mxProj = XMMatrixMultiply(
                g_stQuadConstData.m_pstSceneMatrix->m_mxProj
                , XMMatrixOrthographicOffCenterLH(g_stD3DDevice.m_stViewPort.TopLeftX
                    , g_stD3DDevice.m_stViewPort.TopLeftX + g_stD3DDevice.m_stViewPort.Width
                    , -(g_stD3DDevice.m_stViewPort.TopLeftY + g_stD3DDevice.m_stViewPort.Height)
                    , -g_stD3DDevice.m_stViewPort.TopLeftY
                    , g_stD3DDevice.m_stViewPort.MinDepth
                    , g_stD3DDevice.m_stViewPort.MaxDepth)
            );
            // View * Orthographic ����ͶӰʱ���Ӿ���ͨ����һ����λ��������֮���Գ�һ���Ա�ʾ����ӰͶӰһ��
            g_stQuadConstData.m_pstSceneMatrix->m_mxWorldView
                = XMMatrixMultiply(g_stQuadConstData.m_pstSceneMatrix->m_mxWorld
                    , g_stQuadConstData.m_pstSceneMatrix->m_mxView);
            g_stQuadConstData.m_pstSceneMatrix->m_mxWVP
                = XMMatrixMultiply(g_stQuadConstData.m_pstSceneMatrix->m_mxWorldView
                    , g_stQuadConstData.m_pstSceneMatrix->m_mxProj);

            // ʹ�ó�������������CBV
            D3D12_CONSTANT_BUFFER_VIEW_DESC stCBVDesc = {};
            stCBVDesc.BufferLocation = g_stQuadConstData.m_pISceneMatrixBuffer->GetGPUVirtualAddress();
            stCBVDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(ST_GRS_CB_SCENE_MATRIX), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvHandleSkybox = { g_stQuadHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart() };
            g_stD3DDevice.m_pID3D12Device4->CreateConstantBufferView(&stCBVDesc, cbvSrvHandleSkybox);

            // ʹ��Ĭ�ϵ�HDR������Ϊ���ο�Ĭ����ʾ�Ļ����ȣ�ͨ����tab���л�Ϊ��Ⱦ�������
            cbvSrvHandleSkybox.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;

            // SRV
            D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
            stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            D3D12_RESOURCE_DESC stResDesc = {};
            // ע�����洴��SRV�ķ�ʽ�������Texture2DArray��ÿһ������Դ��Ϊһ����ͼ������һ�ֳ��õļ���
            for (size_t i = 0; i < g_stQuadData.m_ppITexture.GetCount(); i++)
            {
                stResDesc = g_stQuadData.m_ppITexture[i]->GetDesc();

                stSRVDesc.Format = stResDesc.Format;
                stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;

                for (UINT16 nArraySlice = 0; nArraySlice < stResDesc.DepthOrArraySize; nArraySlice++)
                {
                    for (UINT16 nMipSlice = 0; nMipSlice < stResDesc.MipLevels; nMipSlice++)
                    {
                        stSRVDesc.Texture2DArray.ArraySize = 1;
                        stSRVDesc.Texture2DArray.MipLevels = 1;

                        stSRVDesc.Texture2DArray.FirstArraySlice = nArraySlice;
                        stSRVDesc.Texture2DArray.MostDetailedMip = nMipSlice;

                        g_stD3DDevice.m_pID3D12Device4->CreateShaderResourceView(g_stQuadData.m_ppITexture[i].Get(), &stSRVDesc, cbvSrvHandleSkybox);

                        cbvSrvHandleSkybox.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
                    }
                }
            }
        }
        {}
        // 7-3��������Ⱦ���ο�������
        {
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE
                , IID_PPV_ARGS(&g_stQuadPSO.m_pICmdAlloc)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stQuadPSO.m_pICmdAlloc);

            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE
                , g_stQuadPSO.m_pICmdAlloc.Get(), nullptr, IID_PPV_ARGS(&g_stQuadPSO.m_pICmdBundles)));
            GRS_SET_D3D12_DEBUGNAME_COMPTR(g_stQuadPSO.m_pICmdBundles);

            g_stQuadPSO.m_pICmdBundles->SetGraphicsRootSignature(g_stQuadPSO.m_pIRS.Get());
            g_stQuadPSO.m_pICmdBundles->SetPipelineState(g_stQuadPSO.m_pIPSO.Get());

            arHeaps.RemoveAll();
            arHeaps.Add(g_stQuadHeap.m_pICBVSRVHeap.Get());
            arHeaps.Add(g_stQuadHeap.m_pISAPHeap.Get());

            g_stQuadPSO.m_pICmdBundles->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());
            D3D12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandle = g_stQuadHeap.m_pICBVSRVHeap->GetGPUDescriptorHandleForHeapStart();
            //����CBV
            g_stQuadPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(0, stGPUCBVHandle);

            // ����SRV
            stGPUCBVHandle.ptr += g_stD3DDevice.m_nCBVSRVDescriptorSize;
            g_stQuadPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(1, stGPUCBVHandle);
            // ����Sample
            g_stQuadPSO.m_pICmdBundles->SetGraphicsRootDescriptorTable(2, g_stQuadHeap.m_pISAPHeap->GetGPUDescriptorHandleForHeapStart());
            // ������Ⱦ�ַ�
            g_stQuadPSO.m_pICmdBundles->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            // ���ö�������
            g_stQuadPSO.m_pICmdBundles->IASetVertexBuffers(0, GRS_INSTANCE_DATA_SLOT_CNT, g_stQuadData.m_pstVertexBufferView);

            // Draw Call������
            g_stQuadPSO.m_pICmdBundles->DrawInstanced(g_stQuadData.m_nVertexCount, g_stQuadData.m_nInstanceCount, 0, 0);

            g_stQuadPSO.m_pICmdBundles->Close();
        }
        //--------------------------------------------------------------------

        {}
        // 8-0�����õƹ�����
        {
            float fLightZ = -1.0f;

            g_stWorldConstData.m_pstSceneLights->m_v4LightPos[0] = { 0.0f,0.0f,fLightZ,1.0f };
            g_stWorldConstData.m_pstSceneLights->m_v4LightPos[1] = { 0.0f,1.0f,fLightZ,1.0f };
            g_stWorldConstData.m_pstSceneLights->m_v4LightPos[2] = { 1.0f,-1.0f,fLightZ,1.0f };
            g_stWorldConstData.m_pstSceneLights->m_v4LightPos[3] = { -1.0f,-1.0f,fLightZ,1.0f };

            // ע�� PBR��ʱ�� ��Ҫ�ܸߵĹ�Դ����Ч�� 
            g_stWorldConstData.m_pstSceneLights->m_v4LightColors[0] = { 234.7f,213.1f,207.9f, 1.0f };
            g_stWorldConstData.m_pstSceneLights->m_v4LightColors[1] = { 25.0f,0.0f,0.0f,1.0f };
            g_stWorldConstData.m_pstSceneLights->m_v4LightColors[2] = { 0.0f,25.0f,0.0f,1.0f };
            g_stWorldConstData.m_pstSceneLights->m_v4LightColors[3] = { 0.0f,0.0f,25.0f,1.0f };
        }

        D3D12_RESOURCE_BARRIER              stBeginResBarrier = {};
        D3D12_RESOURCE_BARRIER              stEneResBarrier = {};

        // 8-1�������ȾĿ�����Դ���Ͻṹ
        {
            stBeginResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            stBeginResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            stBeginResBarrier.Transition.pResource = g_stD3DDevice.m_pIARenderTargets[g_stD3DDevice.m_nFrameIndex].Get();
            stBeginResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            stBeginResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            stBeginResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            stEneResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            stEneResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            stEneResBarrier.Transition.pResource = g_stD3DDevice.m_pIARenderTargets[g_stD3DDevice.m_nFrameIndex].Get();
            stEneResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            stEneResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            stEneResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        }

        DWORD dwRet = 0;
        BOOL bExit = FALSE;
        D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = g_stD3DDevice.m_pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE stDSVHandle = g_stD3DDevice.m_pIDSVHeap->GetCPUDescriptorHandleForHeapStart();

        FLOAT fStartTime = (FLOAT)::GetTickCount64();
        FLOAT fCurrentTime = fStartTime;
        FLOAT fTimeInSeconds = (fCurrentTime - fStartTime) / 1000.0f;
        FLOAT fDeltaTime = fCurrentTime - fStartTime;

        // ��ʼ��ʽ��Ϣѭ��ǰ����ʾ������
        ShowWindow(g_stD3DDevice.m_hWnd, nCmdShow);
        UpdateWindow(g_stD3DDevice.m_hWnd);

        MSG msg = {};
        // 9����Ϣѭ��
        while (!bExit)
        {
            dwRet = ::MsgWaitForMultipleObjects(1, &g_stD3DDevice.m_hEventFence, FALSE, INFINITE, QS_ALLINPUT);
            switch (dwRet - WAIT_OBJECT_0)
            {
            case 0:
            {
                //-----------------------------------------------------------------------------------------------------
                // OnUpdate()
                // Animation Calculate etc.
                //-----------------------------------------------------------------------------------------------------
                fCurrentTime = (FLOAT)::GetTickCount64();
                fDeltaTime = fCurrentTime - fStartTime;

                // Update! with fDeltaTime or fCurrentTime & fStartTime in MS

                //������ת�ĽǶȣ���ת�Ƕ�(����) = ʱ��(��) * ���ٶ�(����/��)        
                if (g_bWorldRotate)
                {// ��תʱ �ǶȲ��ۼ� ����͹̶�����
                    g_fWorldRotateAngle += ((fDeltaTime) / 1000.0f) * (g_fPalstance * XM_PI / 180.0f);
                }

                //��ת�Ƕ���2PI���ڵı�����ȥ����������ֻ�������0���ȿ�ʼ��С��2PI�Ļ��ȼ���
                if (g_fWorldRotateAngle > XM_2PI)
                {
                    g_fWorldRotateAngle = fmod(g_fWorldRotateAngle, XM_2PI);
                }

                // ƽ��������
                g_stWorldConstData.m_pstSceneMatrix->m_mxWorld
                    = XMMatrixIdentity();

                g_stWorldConstData.m_pstSceneMatrix->m_mxView
                    = XMMatrixLookToLH(g_stCamera.m_v4EyePos, g_stCamera.m_v4LookDir, g_stCamera.m_v4UpDir);

                g_stWorldConstData.m_pstSceneMatrix->m_mxProj
                    = XMMatrixPerspectiveFovLH(XM_PIDIV4
                        , (FLOAT)g_stD3DDevice.m_iWndWidth / (FLOAT)g_stD3DDevice.m_iWndHeight
                        , g_stCamera.m_fNear
                        , g_stCamera.m_fFar);

                g_stWorldConstData.m_pstSceneMatrix->m_mxWorldView
                    = XMMatrixMultiply(
                        g_stWorldConstData.m_pstSceneMatrix->m_mxWorld
                        , g_stWorldConstData.m_pstSceneMatrix->m_mxView
                    );

                g_stWorldConstData.m_pstSceneMatrix->m_mxWVP
                    = XMMatrixMultiply(
                        g_stWorldConstData.m_pstSceneMatrix->m_mxWorldView
                        , g_stWorldConstData.m_pstSceneMatrix->m_mxProj
                    );

                g_stWorldConstData.m_pstSceneMatrix->m_mxInvWVP
                    = XMMatrixInverse(nullptr
                        , g_stWorldConstData.m_pstSceneMatrix->m_mxWVP);

                g_stWorldConstData.m_pstSceneMatrix->m_mxVP
                    = XMMatrixMultiply(g_stWorldConstData.m_pstSceneMatrix->m_mxView
                        , g_stWorldConstData.m_pstSceneMatrix->m_mxProj);

                g_stWorldConstData.m_pstSceneMatrix->m_mxInvVP
                    = XMMatrixInverse(nullptr
                        , g_stWorldConstData.m_pstSceneMatrix->m_mxVP);

                g_stWorldConstData.m_pstSceneCamera->m_v4EyePos = g_stCamera.m_v4EyePos;
                g_stWorldConstData.m_pstSceneCamera->m_v4LookAt = XMVectorAdd(g_stCamera.m_v4EyePos, g_stCamera.m_v4LookDir);
                g_stWorldConstData.m_pstSceneCamera->m_v4UpDir = g_stCamera.m_v4UpDir;


                if (g_bUseLight)
                {// ʹ�ö����ĵ��Դ
                    // ע�� PBR��ʱ�� ��Ҫ�ܸߵĹ�Դ����Ч�� 
                    g_stWorldConstData.m_pstSceneLights->m_v4LightColors[0] = { 234.7f,213.1f,207.9f, 1.0f };
                    g_stWorldConstData.m_pstSceneLights->m_v4LightColors[1] = { 25.0f,0.0f,0.0f,1.0f };
                    g_stWorldConstData.m_pstSceneLights->m_v4LightColors[2] = { 0.0f,25.0f,0.0f,1.0f };
                    g_stWorldConstData.m_pstSceneLights->m_v4LightColors[3] = { 0.0f,0.0f,25.0f,1.0f };
                }
                else
                { // ��ʹ�ö����Ĺ�Դ��ֻ��IBL
                    g_stWorldConstData.m_pstSceneLights->m_v4LightColors[0] = { 0.0f,0.0f,0.0f, 1.0f };
                    g_stWorldConstData.m_pstSceneLights->m_v4LightColors[1] = { 0.0f,0.0f,0.0f,1.0f };
                    g_stWorldConstData.m_pstSceneLights->m_v4LightColors[2] = { 0.0f,0.0f,0.0f,1.0f };
                    g_stWorldConstData.m_pstSceneLights->m_v4LightColors[3] = { 0.0f,0.0f,0.0f,1.0f };
                }

                fStartTime = fCurrentTime;
                //-----------------------------------------------------------------------------------------------------
                // OnRender()
                //----------------------------------------------------------------------------------------------------- 
                //�����������Resetһ��
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDAlloc->Reset());
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Reset(g_stD3DDevice.m_pIMainCMDAlloc.Get(), nullptr));

                g_stD3DDevice.m_nFrameIndex = g_stD3DDevice.m_pISwapChain3->GetCurrentBackBufferIndex();

                g_stD3DDevice.m_pIMainCMDList->RSSetViewports(1, &g_stD3DDevice.m_stViewPort);
                g_stD3DDevice.m_pIMainCMDList->RSSetScissorRects(1, &g_stD3DDevice.m_stScissorRect);

                // ͨ����Դ�����ж��󻺳��Ѿ��л���Ͽ��Կ�ʼ��Ⱦ��
                stBeginResBarrier.Transition.pResource = g_stD3DDevice.m_pIARenderTargets[g_stD3DDevice.m_nFrameIndex].Get();
                g_stD3DDevice.m_pIMainCMDList->ResourceBarrier(1, &stBeginResBarrier);

                // ������ȾĿ��
                stRTVHandle = g_stD3DDevice.m_pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
                stRTVHandle.ptr += (size_t)g_stD3DDevice.m_nFrameIndex * g_stD3DDevice.m_nRTVDescriptorSize;
                g_stD3DDevice.m_pIMainCMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, &stDSVHandle);

                // ������¼�����������ʼ��һ֡����Ⱦ
                g_stD3DDevice.m_pIMainCMDList->ClearRenderTargetView(stRTVHandle, g_stD3DDevice.m_faClearColor, 0, nullptr);
                g_stD3DDevice.m_pIMainCMDList->ClearDepthStencilView(g_stD3DDevice.m_pIDSVHeap->GetCPUDescriptorHandleForHeapStart()
                    , D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

                //-----------------------------------------------------------------------------------------------------
                // Draw !
                //-----------------------------------------------------------------------------------------------------
                // ִ��Skybox�������
                arHeaps.RemoveAll();
                arHeaps.Add(g_stSkyBoxHeap.m_pICBVSRVHeap.Get());
                arHeaps.Add(g_stSkyBoxHeap.m_pISAPHeap.Get());

                g_stD3DDevice.m_pIMainCMDList->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());
                g_stD3DDevice.m_pIMainCMDList->ExecuteBundle(g_stSkyBoxPSO.m_pICmdBundles.Get());
                //-----------------------------------------------------------------------------------------------------

                //-----------------------------------------------------------------------------------------------------
                // ����IBL ��Ⱦ����С��
                //-----------------------------------------------------------------------------------------------------

                // ����С��������ת �� ����
                // ��ת��ȫ��ģ�����ֻ��Ը���
                //-----------------------------------------------------------------------------------------------------             
                int   iColHalf = (int)g_arPBRMaterial.GetCount() / 2;
                float fDeltaPos = 5.0f;
                float fDeltaX = 0.0f;
                float fDeltaY = 0.0f;

                XMMATRIX mxScale = XMMatrixScaling(g_fScale, g_fScale, g_fScale);


                for (int iCol = 0; iCol < (int)g_arPBRMaterial.GetCount(); iCol++)
                {// ��ѭ��
                    fDeltaX = (iCol - iColHalf) * fDeltaPos;

                    //// �����ţ���ƽ��
                    //g_stWorldConstData.m_pstSceneMatrix->m_mxWorld 
                    //    = XMMatrixMultiply(mxScale
                    //        , XMMatrixTranslation(fDeltaX, fDeltaY, fDeltaPos));
                    //// ��ת
                    //g_stWorldConstData.m_pstSceneMatrix->m_mxWorld
                    //    = XMMatrixMultiply(g_stWorldConstData.m_pstSceneMatrix->m_mxWorld
                    //        ,XMMatrixRotationY(static_cast<float>(g_fWorldRotateAngle)));

                    // ���� ��ת
                    g_arPBRMaterial[iCol].m_pstModelData->m_mxModel2World
                        = XMMatrixMultiply(mxScale
                            , XMMatrixRotationY(static_cast<float>(g_fWorldRotateAngle)));
                    // ƽ��
                    g_arPBRMaterial[iCol].m_pstModelData->m_mxModel2World
                        = XMMatrixMultiply(g_arPBRMaterial[iCol].m_pstModelData->m_mxModel2World
                            , XMMatrixTranslation(fDeltaX, fDeltaY, fDeltaPos));

                    g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootSignature(g_stIBLPSO.m_pIRS.Get());
                    g_stD3DDevice.m_pIMainCMDList->SetPipelineState(g_stIBLPSO.m_pIPSO.Get());

                    //const UINT ncCopyCnt = 2;
                    //// ����������
                    //D3D12_CPU_DESCRIPTOR_HANDLE stDestHandle[ncCopyCnt] = {};
                    //stDestHandle[0] = g_stIBLHeap.m_pICBVSRVHeap->GetCPUDescriptorHandleForHeapStart();
                    //stDestHandle[0].ptr += ((g_stIBLPSO.m_nCBVCnt - 1) * g_stD3DDevice.m_nCBVSRVDescriptorSize);
                    //stDestHandle[1].ptr = stDestHandle[0].ptr + ((1 + 3) * g_stD3DDevice.m_nCBVSRVDescriptorSize);

                    //D3D12_CPU_DESCRIPTOR_HANDLE stSrcHandle[ncCopyCnt] = {};
                    //stSrcHandle[0] = g_arPBRMaterial[iCol].m_pIMaterialHeap->GetCPUDescriptorHandleForHeapStart();
                    //stSrcHandle[1].ptr = stSrcHandle[0].ptr + g_stD3DDevice.m_nCBVSRVDescriptorSize;

                    //UINT naDesRangeSizes[ncCopyCnt] = {};
                    //UINT naSrcRangeSizes[ncCopyCnt] = {};
                    //// Copy 1 CBV + 6 SRV
                    //naDesRangeSizes[0] = GRS_MODEL_DATA_CB_CNT;
                    //naSrcRangeSizes[0] = GRS_MODEL_DATA_CB_CNT;

                    //naDesRangeSizes[1] = GRS_PBR_MATERIAL_TEXTURE_CNT;
                    //naSrcRangeSizes[1] = GRS_PBR_MATERIAL_TEXTURE_CNT;

                    //g_stD3DDevice.m_pID3D12Device4->CopyDescriptors(
                    //    ncCopyCnt
                    //    , stDestHandle
                    //    , naDesRangeSizes
                    //    , ncCopyCnt
                    //    , stSrcHandle
                    //    , naSrcRangeSizes
                    //    , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);  

                    arHeaps.RemoveAll();
                    arHeaps.Add(g_arPBRMaterial[iCol].m_pIMaterialHeap.Get());
                    arHeaps.Add(g_stIBLHeap.m_pISAPHeap.Get());

                    g_stD3DDevice.m_pIMainCMDList->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());

                    D3D12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandle
                        = g_arPBRMaterial[iCol].m_pIMaterialHeap->GetGPUDescriptorHandleForHeapStart();
                    //����CBV
                    g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(0, stGPUCBVHandle);

                    // ����SRV (space 0)
                    stGPUCBVHandle.ptr += (g_stIBLPSO.m_nCBVCnt * g_stD3DDevice.m_nCBVSRVDescriptorSize);
                    g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(1, stGPUCBVHandle);

                    // ����SRV (space 1)
                    stGPUCBVHandle.ptr += (3 * g_stD3DDevice.m_nCBVSRVDescriptorSize);
                    g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(2, stGPUCBVHandle);

                    // ����Sample
                    g_stD3DDevice.m_pIMainCMDList->SetGraphicsRootDescriptorTable(3
                        , g_stIBLHeap.m_pISAPHeap->GetGPUDescriptorHandleForHeapStart());

                    // ������Ⱦ�ַ�
                    g_stD3DDevice.m_pIMainCMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    // ���ö�������
                    g_stD3DDevice.m_pIMainCMDList->IASetVertexBuffers(0, (UINT)g_stMultiMeshData.m_arVBV.GetCount(), g_stMultiMeshData.m_arVBV.GetData());
                    g_stD3DDevice.m_pIMainCMDList->IASetIndexBuffer(&g_stMultiMeshData.m_stIBV);

                    // Draw Call������   
                    g_stD3DDevice.m_pIMainCMDList->DrawIndexedInstanced(g_stMultiMeshData.m_nIndexCount, 1, 0, 0, 0);

                    //D3D12_RESOURCE_BARRIER stRBSwitch = {};
                    //stRBSwitch.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
                    ////stRBSwitch.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
                    //stRBSwitch.Aliasing.pResourceAfter = nullptr;   //g_arPBRMaterial[(iCol+1)% g_arPBRMaterial.GetCount()].m_pICBModelData.Get();
                    //stRBSwitch.Aliasing.pResourceBefore = g_arPBRMaterial[iCol].m_pICBModelData.Get();
                    //g_stD3DDevice.m_pIMainCMDList->ResourceBarrier(1, &stRBSwitch);
                    // ����
                    //g_stD3DDevice.m_pIMainCMDList->ExecuteBundle(g_stIBLPSO.m_pICmdBundles.Get());
                }
                //-----------------------------------------------------------------------------------------------------

                //-----------------------------------------------------------------------------------------------------
                // ִ�о��ο�������
                if (g_bShowRTA)
                {
                    arHeaps.RemoveAll();
                    arHeaps.Add(g_stQuadHeap.m_pICBVSRVHeap.Get());
                    arHeaps.Add(g_stQuadHeap.m_pISAPHeap.Get());

                    // ����SRV
                    g_stD3DDevice.m_pIMainCMDList->SetDescriptorHeaps((UINT)arHeaps.GetCount(), arHeaps.GetData());
                    g_stD3DDevice.m_pIMainCMDList->ExecuteBundle(g_stQuadPSO.m_pICmdBundles.Get());
                }
                //-----------------------------------------------------------------------------------------------------

                //��һ����Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
                stEneResBarrier.Transition.pResource = g_stD3DDevice.m_pIARenderTargets[g_stD3DDevice.m_nFrameIndex].Get();
                g_stD3DDevice.m_pIMainCMDList->ResourceBarrier(1, &stEneResBarrier);

                //�ر������б�����ȥִ����
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDList->Close());

                //ִ�������б�
                arCmdList.RemoveAll();
                arCmdList.Add(g_stD3DDevice.m_pIMainCMDList.Get());
                g_stD3DDevice.m_pIMainCMDQueue->ExecuteCommandLists(static_cast<UINT>(arCmdList.GetCount()), arCmdList.GetData());

                //�ύ����
                // ���֧��Tearing ��ô����Tearing��ʽ���ֻ���
                UINT nPresentFlags = (g_stD3DDevice.m_bSupportTearing && !g_stD3DDevice.m_bFullScreen) ? DXGI_PRESENT_ALLOW_TEARING : 0;
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pISwapChain3->Present(0, nPresentFlags));

                //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
                const UINT64 n64CurrentFenceValue = g_stD3DDevice.m_n64FenceValue;
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIMainCMDQueue->Signal(g_stD3DDevice.m_pIFence.Get(), n64CurrentFenceValue));
                ++g_stD3DDevice.m_n64FenceValue;
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIFence->SetEventOnCompletion(n64CurrentFenceValue, g_stD3DDevice.m_hEventFence));
                //-----------------------------------------------------------------------------------------------------
            }
            break;
            case 1:
            {// ������Ϣ
                while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
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
            default:
                break;
            }
        }

        if (nullptr != g_stHDRData.m_pfHDRData)
        {// �ͷ� HDR ͼƬ����
            stbi_image_free(g_stHDRData.m_pfHDRData);
            g_stHDRData.m_pfHDRData = nullptr;
        }
    }
    catch (CAtlException& e)
    {//������COM�쳣
        e;
    }
    catch (...)
    {

    }
    return 0;
}

void OnSize(UINT width, UINT height, bool minimized)
{
    if ((width == g_stD3DDevice.m_iWndWidth && height == g_stD3DDevice.m_iWndHeight)
        || minimized
        || (nullptr == g_stD3DDevice.m_pISwapChain3)
        || (0 == width)
        || (0 == height))
    {
        return;
    }

    try
    {
        // 1����Ϣѭ����·
        if (WAIT_OBJECT_0 != ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE))
        {
            GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
        }

        // 2���ͷŻ�ȡ�ĺ󻺳����ӿ�
        for (UINT i = 0; i < g_nFrameBackBufCount; i++)
        {
            g_stD3DDevice.m_pIARenderTargets[i].Reset();

        }

        // 3�����������ô�С
        DXGI_SWAP_CHAIN_DESC stSWDesc = {};
        g_stD3DDevice.m_pISwapChain3->GetDesc(&stSWDesc);

        HRESULT hr = g_stD3DDevice.m_pISwapChain3->ResizeBuffers(g_nFrameBackBufCount
            , width
            , height
            , stSWDesc.BufferDesc.Format
            , stSWDesc.Flags);

        GRS_THROW_IF_FAILED(hr);

        // 4���õ���ǰ�󻺳�����
        g_stD3DDevice.m_nFrameIndex = g_stD3DDevice.m_pISwapChain3->GetCurrentBackBufferIndex();

        // 5������任��ĳߴ�
        g_stD3DDevice.m_iWndWidth = width;
        g_stD3DDevice.m_iWndHeight = height;

        // 6������RTV��������
        ComPtr<ID3D12Device> pID3D12Device;
        ComPtr<ID3D12Device4> pID3D12Device4;
        GRS_THROW_IF_FAILED(g_stD3DDevice.m_pIRTVHeap->GetDevice(IID_PPV_ARGS(&pID3D12Device)));
        GRS_THROW_IF_FAILED(pID3D12Device.As(&pID3D12Device4));
        {
            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle
                = g_stD3DDevice.m_pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
            for (UINT i = 0; i < g_nFrameBackBufCount; i++)
            {
                GRS_THROW_IF_FAILED(g_stD3DDevice.m_pISwapChain3->GetBuffer(i
                    , IID_PPV_ARGS(&g_stD3DDevice.m_pIARenderTargets[i])));

                GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(g_stD3DDevice.m_pIARenderTargets, i);

                pID3D12Device4->CreateRenderTargetView(g_stD3DDevice.m_pIARenderTargets[i].Get()
                    , nullptr
                    , stRTVHandle);

                stRTVHandle.ptr += g_stD3DDevice.m_nRTVDescriptorSize;
            }
        }

        // 7��������Ȼ��弰��Ȼ�����������
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC   stDepthStencilDesc = {};
            D3D12_CLEAR_VALUE               stDepthOptimizedClearValue = {};
            D3D12_RESOURCE_DESC             stDSTex2DDesc = {};

            stDepthOptimizedClearValue.Format = g_stD3DDevice.m_emDepthStencilFormat;
            stDepthOptimizedClearValue.DepthStencil.Depth = 1.0f;
            stDepthOptimizedClearValue.DepthStencil.Stencil = 0;

            stDSTex2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            stDSTex2DDesc.Alignment = 0;
            stDSTex2DDesc.Width = g_stD3DDevice.m_iWndWidth;
            stDSTex2DDesc.Height = g_stD3DDevice.m_iWndHeight;
            stDSTex2DDesc.DepthOrArraySize = 1;
            stDSTex2DDesc.MipLevels = 0;
            stDSTex2DDesc.Format = g_stD3DDevice.m_emDepthStencilFormat;
            stDSTex2DDesc.SampleDesc.Count = 1;
            stDSTex2DDesc.SampleDesc.Quality = 0;
            stDSTex2DDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            stDSTex2DDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            // �ͷ���Ȼ�����
            g_stD3DDevice.m_pIDepthStencilBuffer.Reset();

            //ʹ����ʽĬ�϶Ѵ���һ��������建������
            //��Ϊ��������Ȼ�������һֱ��ʹ�ã����õ����岻������ֱ��ʹ����ʽ�ѣ�ͼ����
            GRS_THROW_IF_FAILED(g_stD3DDevice.m_pID3D12Device4->CreateCommittedResource(
                &g_stDefaultHeapProps
                , D3D12_HEAP_FLAG_NONE
                , &stDSTex2DDesc
                , D3D12_RESOURCE_STATE_DEPTH_WRITE
                , &stDepthOptimizedClearValue
                , IID_PPV_ARGS(&g_stD3DDevice.m_pIDepthStencilBuffer)
            ));

            stDepthStencilDesc.Format = g_stD3DDevice.m_emDepthStencilFormat;
            stDepthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            stDepthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

            // Ȼ��ֱ���ؽ�������
            g_stD3DDevice.m_pID3D12Device4->CreateDepthStencilView(g_stD3DDevice.m_pIDepthStencilBuffer.Get()
                , &stDepthStencilDesc
                , g_stD3DDevice.m_pIDSVHeap->GetCPUDescriptorHandleForHeapStart());
        }

        // 8�������ӿڴ�С����ΰ�Χ�д�С
        g_stD3DDevice.m_stViewPort.TopLeftX = 0.0f;
        g_stD3DDevice.m_stViewPort.TopLeftY = 0.0f;
        g_stD3DDevice.m_stViewPort.Width = static_cast<float>(g_stD3DDevice.m_iWndWidth);
        g_stD3DDevice.m_stViewPort.Height = static_cast<float>(g_stD3DDevice.m_iWndHeight);

        g_stD3DDevice.m_stScissorRect.left = static_cast<LONG>(g_stD3DDevice.m_stViewPort.TopLeftX);
        g_stD3DDevice.m_stScissorRect.right = static_cast<LONG>(g_stD3DDevice.m_stViewPort.TopLeftX + g_stD3DDevice.m_stViewPort.Width);
        g_stD3DDevice.m_stScissorRect.top = static_cast<LONG>(g_stD3DDevice.m_stViewPort.TopLeftY);
        g_stD3DDevice.m_stScissorRect.bottom = static_cast<LONG>(g_stD3DDevice.m_stViewPort.TopLeftY + g_stD3DDevice.m_stViewPort.Height);

        // 9�������趨��������
        g_stQuadConstData.m_pstSceneMatrix->m_mxWorld = XMMatrixIdentity();
        g_stQuadConstData.m_pstSceneMatrix->m_mxView = XMMatrixIdentity();

        g_stQuadConstData.m_pstSceneMatrix->m_mxProj = XMMatrixScaling(1.0f, -1.0f, 1.0f); //���·�ת�����֮ǰ��Quad��Vertex����һ��ʹ��
        g_stQuadConstData.m_pstSceneMatrix->m_mxProj = XMMatrixMultiply(g_stQuadConstData.m_pstSceneMatrix->m_mxProj, XMMatrixTranslation(-0.5f, +0.5f, 0.0f)); //΢��ƫ�ƣ���������λ��
        g_stQuadConstData.m_pstSceneMatrix->m_mxProj = XMMatrixMultiply(
            g_stQuadConstData.m_pstSceneMatrix->m_mxProj
            , XMMatrixOrthographicOffCenterLH(g_stD3DDevice.m_stViewPort.TopLeftX
                , g_stD3DDevice.m_stViewPort.TopLeftX + g_stD3DDevice.m_stViewPort.Width
                , -(g_stD3DDevice.m_stViewPort.TopLeftY + g_stD3DDevice.m_stViewPort.Height)
                , -g_stD3DDevice.m_stViewPort.TopLeftY
                , g_stD3DDevice.m_stViewPort.MinDepth
                , g_stD3DDevice.m_stViewPort.MaxDepth)
        );
        // View * Orthographic ����ͶӰʱ���Ӿ���ͨ����һ����λ��������֮���Գ�һ���Ա�ʾ����ӰͶӰһ��
        g_stQuadConstData.m_pstSceneMatrix->m_mxWorldView = XMMatrixMultiply(g_stQuadConstData.m_pstSceneMatrix->m_mxWorld, g_stQuadConstData.m_pstSceneMatrix->m_mxView);
        g_stQuadConstData.m_pstSceneMatrix->m_mxWVP = XMMatrixMultiply(g_stQuadConstData.m_pstSceneMatrix->m_mxWorldView, g_stQuadConstData.m_pstSceneMatrix->m_mxProj);

        //---------------------------------------------------------------------------------------------
        // ������պ�Զƽ��ߴ�
        float fHighW = -1.0f - (1.0f / (float)g_stD3DDevice.m_iWndWidth);
        float fHighH = -1.0f - (1.0f / (float)g_stD3DDevice.m_iWndHeight);
        float fLowW = 1.0f + (1.0f / (float)g_stD3DDevice.m_iWndWidth);
        float fLowH = 1.0f + (1.0f / (float)g_stD3DDevice.m_iWndHeight);

        ST_GRS_VERTEX_CUBE_BOX stSkyboxVertices[4] = {};

        stSkyboxVertices[0].m_v4Position = XMFLOAT4(fLowW, fLowH, 1.0f, 1.0f);
        stSkyboxVertices[1].m_v4Position = XMFLOAT4(fLowW, fHighH, 1.0f, 1.0f);
        stSkyboxVertices[2].m_v4Position = XMFLOAT4(fHighW, fLowH, 1.0f, 1.0f);
        stSkyboxVertices[3].m_v4Position = XMFLOAT4(fHighW, fHighH, 1.0f, 1.0f);

        //������պ��ӵ�����
        g_stBufferResSesc.Width = GRS_UPPER(sizeof(stSkyboxVertices), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        //ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����        
        ST_GRS_VERTEX_CUBE_BOX* pBufferData = nullptr;

        GRS_THROW_IF_FAILED(g_stSkyBoxData.m_pIVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pBufferData)));
        memcpy(pBufferData, stSkyboxVertices, sizeof(stSkyboxVertices));
        g_stSkyBoxData.m_pIVertexBuffer->Unmap(0, nullptr);
        //---------------------------------------------------------------------------------------------

        // 10���������¼�״̬������Ϣ��·��������Ϣѭ��        
        SetEvent(g_stD3DDevice.m_hEventFence);
    }
    catch (CAtlException& e)
    {//������COM�쳣
        e;
    }
    catch (...)
    {

    }

}

void SwitchFullScreenWindow()
{
    if (g_stD3DDevice.m_bFullScreen)
    {// ��ԭΪ���ڻ�
        SetWindowLong(g_stD3DDevice.m_hWnd, GWL_STYLE, g_stD3DDevice.m_dwWndStyle);

        SetWindowPos(
            g_stD3DDevice.m_hWnd,
            HWND_NOTOPMOST,
            g_stD3DDevice.m_rtWnd.left,
            g_stD3DDevice.m_rtWnd.top,
            g_stD3DDevice.m_rtWnd.right - g_stD3DDevice.m_rtWnd.left,
            g_stD3DDevice.m_rtWnd.bottom - g_stD3DDevice.m_rtWnd.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(g_stD3DDevice.m_hWnd, SW_NORMAL);
    }
    else
    {// ȫ����ʾ
        // ���洰�ڵ�ԭʼ�ߴ�
        GetWindowRect(g_stD3DDevice.m_hWnd, &g_stD3DDevice.m_rtWnd);

        // ʹ�����ޱ߿��Ա�ͻ�������������Ļ��  
        SetWindowLong(g_stD3DDevice.m_hWnd
            , GWL_STYLE
            , g_stD3DDevice.m_dwWndStyle
            & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));

        RECT stFullscreenWindowRect = {};
        ComPtr<IDXGIOutput> pIOutput;
        DXGI_OUTPUT_DESC stOutputDesc = {};
        HRESULT _hr = g_stD3DDevice.m_pISwapChain3->GetContainingOutput(&pIOutput);
        if (SUCCEEDED(g_stD3DDevice.m_pISwapChain3->GetContainingOutput(&pIOutput)) &&
            SUCCEEDED(pIOutput->GetDesc(&stOutputDesc)))
        {// ʹ����ʾ�������ʾ�ߴ�
            stFullscreenWindowRect = stOutputDesc.DesktopCoordinates;
        }
        else
        {// ��ȡϵͳ���õķֱ���
            DEVMODE stDevMode = {};
            stDevMode.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &stDevMode);

            stFullscreenWindowRect = {
                stDevMode.dmPosition.x,
                stDevMode.dmPosition.y,
                stDevMode.dmPosition.x + static_cast<LONG>(stDevMode.dmPelsWidth),
                stDevMode.dmPosition.y + static_cast<LONG>(stDevMode.dmPelsHeight)
            };
        }

        // ���ô������
        SetWindowPos(
            g_stD3DDevice.m_hWnd,
            HWND_TOPMOST,
            stFullscreenWindowRect.left,
            stFullscreenWindowRect.top,
            stFullscreenWindowRect.right,
            stFullscreenWindowRect.bottom,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(g_stD3DDevice.m_hWnd, SW_MAXIMIZE);
    }

    g_stD3DDevice.m_bFullScreen = !!!g_stD3DDevice.m_bFullScreen;
}

VOID ScreenPt2Vector(const float& fPtX, const float& fPtY, const float& fScreenWidth, const float& fScreenHeight, const float& fArcBallRaius, XMVECTOR& v)
{
    FLOAT x = -(fPtX - fScreenWidth / 2.0f) / (fArcBallRaius * fScreenWidth / 2.0f);
    FLOAT y = (fPtY - fScreenHeight / 2.0f) / (fArcBallRaius * fScreenHeight / 2.0f);

    FLOAT z = 0.0f;
    FLOAT mag = x * x + y * y;

    if (mag > 1.0f)
    {
        FLOAT scale = 1.0f / sqrtf(mag);
        x *= scale;
        y *= scale;
    }
    else
    {
        z = sqrtf(1.0f - mag);
    }

    v = XMVectorSet(x, y, z, 0.0f);
}

VOID QuaternionWithVector(XMVECTOR& v1, XMVECTOR& v2, XMVECTOR& vQ)
{
    XMVECTOR v1n = XMVector3Normalize(v1);
    XMVECTOR v2n = XMVector3Normalize(v2);

    vQ = XMVector3Cross(v1n, v2n);
    vQ = XMVectorSetW(vQ, XMVectorGetX(XMVector3Dot(v1n, v2n)));
}

BOOL GRSCameraProcessMouseMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (WM_LBUTTONDOWN == message)
    {
        ::SetCapture(hWnd);
        ScreenPt2Vector(
            (float)LOWORD(lParam)
            , (float)HIWORD(lParam)
            , (float)g_stD3DDevice.m_iWndWidth
            , (float)g_stD3DDevice.m_iWndHeight
            , g_stCameraControl.m_fRadius
            , g_stCameraControl.m_vCameraPosStart);

        //g_qCameraStart = g_qCameraMoves;

        return TRUE;
    }

    if ((WM_MOUSEMOVE == message) && (MK_LBUTTON & wParam))
    {
        float fMoveX = (float)(short)LOWORD(lParam);
        float fMoveY = (float)(short)HIWORD(lParam);

        ScreenPt2Vector(fMoveX
            , fMoveY
            , (float)g_stD3DDevice.m_iWndWidth
            , (float)g_stD3DDevice.m_iWndHeight
            , g_stCameraControl.m_fRadius
            , g_stCameraControl.m_vCameraPosMoves);

        QuaternionWithVector(g_stCameraControl.m_vCameraPosStart, g_stCameraControl.m_vCameraPosMoves, g_stCameraControl.m_qCameraMoves);

        g_stCameraControl.m_qCameraMoves = XMQuaternionMultiply(g_stCameraControl.m_qCameraStart, g_stCameraControl.m_qCameraMoves);

        XMMATRIX mxRot = XMMatrixRotationQuaternion(g_stCameraControl.m_qCameraMoves);

        g_stCamera.m_v4LookDir = XMVector3Transform(g_stCamera.m_v4LookDir, mxRot);
        g_stCamera.m_v4UpDir = XMVector3Transform(g_stCamera.m_v4UpDir, mxRot);

        XMVECTOR vRight = XMVector3Cross(g_stCamera.m_v4UpDir, g_stCamera.m_v4LookDir);
        g_stCamera.m_v4UpDir = XMVector3Cross(g_stCamera.m_v4LookDir, vRight);

        return TRUE;
    }

    if (WM_LBUTTONUP == message)
    {
        ::ReleaseCapture();
        return TRUE;
    }

    return FALSE;
}

BOOL GRSCameraProcessKeyMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (WM_KEYDOWN != message)
    {
        return FALSE;
    }
    BOOL bRet = FALSE;
    USHORT n16KeyCode = (wParam & 0xFF);

    //-------------------------------------------------------------------------------
    // ���������λ���ƶ�����
    float fMoveSpeed = 1.0f;
    float fDeltaDisplacement = fMoveSpeed;// fmax(fMoveSpeed * g_stTimer.m_fDeltaTimeInMS, 0.5f);

    XMVECTOR vVelocity = { 0.0f,0.0f,0.0f,0.0f };

    // Z-��
    if (VK_UP == n16KeyCode || 'w' == n16KeyCode || 'W' == n16KeyCode)
    {
        vVelocity = XMVectorSetZ(vVelocity, fDeltaDisplacement);
        bRet = TRUE;
    }

    if (VK_DOWN == n16KeyCode || 's' == n16KeyCode || 'S' == n16KeyCode)
    {
        vVelocity = XMVectorSetZ(vVelocity, -fDeltaDisplacement);
        bRet = TRUE;
    }

    // X-��
    if (VK_LEFT == n16KeyCode || 'a' == n16KeyCode || 'A' == n16KeyCode)
    {
        vVelocity = XMVectorSetX(vVelocity, fDeltaDisplacement);
        bRet = TRUE;
    }

    if (VK_RIGHT == n16KeyCode || 'd' == n16KeyCode || 'D' == n16KeyCode)
    {
        vVelocity = XMVectorSetX(vVelocity, -fDeltaDisplacement);
        bRet = TRUE;
    }

    // Y-��
    if (VK_PRIOR == n16KeyCode || 'r' == n16KeyCode || 'R' == n16KeyCode)
    {
        vVelocity = XMVectorSetY(vVelocity, fDeltaDisplacement);
        bRet = TRUE;
    }

    if (VK_NEXT == n16KeyCode || 'f' == n16KeyCode || 'F' == n16KeyCode)
    {
        vVelocity = XMVectorSetY(vVelocity, -fDeltaDisplacement);
        bRet = TRUE;
    }

    XMVECTOR vRight = XMVector3Cross(g_stCamera.m_v4UpDir, g_stCamera.m_v4LookDir);

    g_stCamera.m_v4EyePos = XMVectorAdd(g_stCamera.m_v4EyePos, XMVectorScale(vRight, XMVectorGetX(vVelocity)));
    g_stCamera.m_v4EyePos = XMVectorAdd(g_stCamera.m_v4EyePos, XMVectorScale(g_stCamera.m_v4UpDir, XMVectorGetY(vVelocity)));
    g_stCamera.m_v4EyePos = XMVectorAdd(g_stCamera.m_v4EyePos, XMVectorScale(g_stCamera.m_v4LookDir, XMVectorGetZ(vVelocity)));

    //-------------------------------------------------------------------------------
    return bRet;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
    {
        // ��·һ����Ϣѭ��
        if (WAIT_OBJECT_0 == ::WaitForSingleObject(g_stD3DDevice.m_hEventFence, INFINITE)
            && g_stD3DDevice.m_bSupportTearing
            && g_stD3DDevice.m_pISwapChain3)
        {// ֧��Tearingʱ���Զ��˳���ȫ��״̬
            g_stD3DDevice.m_pISwapChain3->SetFullscreenState(FALSE, nullptr);
        }
        PostQuitMessage(0);
    }
    break;
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:
    {
        GRSCameraProcessMouseMsg(hWnd, message, wParam, lParam);
    }
    break;

    case WM_KEYDOWN:
    {
        USHORT n16KeyCode = (wParam & 0xFF);
        if (VK_ESCAPE == n16KeyCode)
        {// Esc���˳�
            ::DestroyWindow(hWnd);
        }

        if (VK_TAB == n16KeyCode)
        {// ��Tab�� �����Ƿ���ʾ�м���Ⱦ�����Texture
            g_bShowRTA = !!!g_bShowRTA;
        }

        if (VK_SPACE == n16KeyCode)
        {// ��Space�� ���ƶ�����Դ�Ŀ�����ر�
            g_bUseLight = !!!g_bUseLight;
        }

        if (VK_F2 == n16KeyCode)
        {// F3��
            // ��ԭ���λ��
            g_stCamera.m_v4EyePos = XMVectorSet(0.0f, 10.0f, -10.0f, 1.0f); // λ��
            g_stCamera.m_v4LookDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);   // ����
            g_stCamera.m_v4UpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);     // �Ϸ�            
        }
        //-------------------------------------------------------------------------------
        // ���������λ���ƶ�����
        GRSCameraProcessKeyMsg(message, wParam, lParam);
        //-------------------------------------------------------------------------------

        float fZoom = 0.1f;
        if ('z' == n16KeyCode || 'Z' == n16KeyCode)
        {
            g_fScale *= (1 + fZoom);
        }
        if ('c' == n16KeyCode || 'C' == n16KeyCode)
        {
            g_fScale /= (1 + fZoom);
        }

        if (VK_ADD == n16KeyCode || VK_OEM_PLUS == n16KeyCode)
        {// + ��  

        }

        if (VK_SUBTRACT == n16KeyCode || VK_OEM_MINUS == n16KeyCode)
        {// - �� 

        }

        if (VK_MULTIPLY == n16KeyCode)
        {// * ��  ������ת�ٶ� ����             
            g_fPalstance = ((g_fPalstance >= 180.0f) ? 180.0f : (g_fPalstance + 1.0f));
        }

        if (VK_DIVIDE == n16KeyCode)
        {// / �� ������ת�ٶ� ����      
            g_fPalstance = ((g_fPalstance > 1.0f) ? (g_fPalstance - 1.0f) : 1.0f);
        }

        if (VK_SHIFT == n16KeyCode)
        {// ���Ʋ���������Ƿ���ת
            g_bWorldRotate = !!!g_bWorldRotate;
        }
    }
    break;
    case WM_SIZE:
    {
        ::GetClientRect(hWnd, &g_stD3DDevice.m_rtWndClient);
        OnSize(g_stD3DDevice.m_rtWndClient.right - g_stD3DDevice.m_rtWndClient.left
            , g_stD3DDevice.m_rtWndClient.bottom - g_stD3DDevice.m_rtWndClient.top
            , wParam == SIZE_MINIMIZED);
        return 0; // ������ϵͳ��������
    }
    break;
    case WM_GETMINMAXINFO:
    {// ���ƴ�����С��СΪ1024*768
        MINMAXINFO* pMMInfo = (MINMAXINFO*)lParam;
        pMMInfo->ptMinTrackSize.x = 1024;
        pMMInfo->ptMinTrackSize.y = 768;
        return 0;
    }
    break;
    case WM_SYSKEYDOWN:
    {
        if ((wParam == VK_RETURN) && (lParam & (1 << 29)))
        {// ����Alt + Entry
            if (g_stD3DDevice.m_bSupportTearing)
            {
                SwitchFullScreenWindow();
                return 0;
            }
        }
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

