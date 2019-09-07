//===================================================================================
//��Ŀ���ƣ�GRS ����ƽ̨ V2.0
//�ļ�������DirectX�ļ��ٷ�װͷ�ļ�������Ҫ�ļ򵥶���Ҳ���ϵ�һ��
//ģ�����ƣ�GRSLib V2.0
//
//��֯����˾�����ţ����ƣ�One Man Group
//���ߣ�Gamebaby Rock Sun
//���ڣ�2017-11-30 14:27:19
//�޶����ڣ�2017-11-30
//===================================================================================
#pragma once
//for DirectX Math
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <DirectXPackedVector.h>

using namespace DirectX;

//for Direct2D & DirectWrite
#include <d2d1_3.h>
#include <d2d1_3helper.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>				//for WIC

//for Shader Compiler
#include <D3Dcompiler.h>

//for Dxgi
//#include <dxgi1_4.h>
#include <dxgi1_6.h>

//for d3d11
#include <d3d11_4.h>
#include <pix.h>
#include <d3d11shader.h>
//#include <d3d11shadertracing.h>

//for d3d12
#include <d3d12.h>
#include <d3d12shader.h>

//Xaudio
#include <Xaudio2.h>

//linker
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwrite.lib")

#pragma comment(lib, "Xaudio2.lib")

#include "d3dx12.h"

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#ifndef D3D_COMPILE_STANDARD_FILE_INCLUDE
#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)
#endif

//#include <ppltasks.h>	// ���� create_task

#if defined(_DEBUG)
// �����Ŀ���ڵ������ɽ׶Σ���ͨ�� SDK �����õ��ԡ�
#define GRS_OPEN_D3D12_DEBUG() \
{\
	ID3D12Debug* pID3D12Debug = nullptr;\
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pID3D12Debug))))\
	{\
		pID3D12Debug->EnableDebugLayer();\
	}\
	GRS_SAFE_RELEASE(pID3D12Debug);\
}
#else
#define GRS_OPEN_D3D12_DEBUG()
#endif

#if defined(_DEBUG)
inline void GRS_SetD3D12DebugName(ID3D12Object* pObject, LPCWSTR name)
{
	pObject->SetName(name);
}

inline void GRS_SetD3D12DebugNameIndexed( ID3D12Object* pObject, LPCWSTR name, UINT index )
{
	WCHAR _DebugName[50] = {};
	if ( StringCchPrintfW( _DebugName ,GRS_COUNTOF(_DebugName), L"%s[%u]", name, index ) )
	{
		pObject->SetName( _DebugName );
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

#define GRS_SET__D3D12_DEBUGNAME(x)						GRS_SetD3D12DebugName(x, L#x)
#define GRS_SET__D3D12_DEBUGNAME_INDEXED(x, n)			GRS_SetD3D12DebugNameIndexed(x[n], L#x, n)

#define GRS_SET__D3D12_DEBUGNAME_COMPTR(x)				GRS_SetD3D12DebugName(x.Get(), L#x)
#define GRS_SET__D3D12_DEBUGNAME_INDEXED_COMPTR(x, n)	GRS_SetD3D12DebugNameIndexed(x[n].Get(), L#x, n)


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
	WCHAR _DebugName[50] = {};
	if (StringCchPrintfW(_DebugName, GRS_COUNTOF(_DebugName), L"%s[%u]", name, index))
	{
		StringCchLengthW(_DebugName, 50, &szLen);
		pObject->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(szLen),_DebugName);
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

#define GRS_SET__DXGI_DEBUGNAME(x)								GRS_SetDXGIDebugName(x, L#x)
#define GRS_SET__DXGI_DEBUGNAME_INDEXED(x, n)			GRS_SetDXGIDebugNameIndexed(x[n], L#x, n)

#define GRS_SET__DXGI_DEBUGNAME_COMPTR(x)						GRS_SetDXGIDebugName(x.Get(), L#x)
#define GRS_SET__DXGI_DEBUGNAME_INDEXED_COMPTR(x, n)	GRS_SetDXGIDebugNameIndexed(x[n].Get(), L#x, n)



#if defined(PROFILE) || defined(DEBUG)
GRS_INLINE void GRS_SetD3D11DebugName(IDXGIObject* pObj, const CHAR* pstrName)
{
	if (pObj)
	{
		pObj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(pstrName), pstrName);
	}
}
GRS_INLINE void GRS_SetD3D11DebugName(ID3D11Device* pObj, const CHAR* pstrName)
{
	if (pObj)
	{
		pObj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(pstrName), pstrName);
	}
}
GRS_INLINE void GRS_SetD3D11DebugName(ID3D11DeviceChild* pObj, const CHAR* pstrName)
{
	if (pObj)
	{
		pObj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(pstrName), pstrName);
	}
}
#else
#define GRS_SetD3D11DebugName( pObj, pstrName )
#endif



//Ϊ�ɿ���ʱ�����ѯ�����ȶ��ĵ�Դ״̬�� 
//if (debugLayerEnabled)
//{
//	// Set stable power state for reliable timestamp queries.
//	ThrowIfFailed(m_devices[i]->SetStablePowerState(TRUE));
//}

//#if defined(_DEBUG)
//// ���� SDK ��֧�֡�
//inline bool SdkLayersAvailable()
//{
//	HRESULT hr = D3D11CreateDevice(
//		nullptr,
//		D3D_DRIVER_TYPE_NULL,       // ���贴��ʵ��Ӳ���豸��
//		0,
//		D3D11_CREATE_DEVICE_DEBUG,  // ���� SDK �㡣
//		nullptr,                    // �κι��ܼ��𶼻�������
//		0,
//		D3D11_SDK_VERSION,          // ���� Windows Ӧ���̵�Ӧ�ã�ʼ�ս���ֵ����Ϊ D3D11_SDK_VERSION��
//		nullptr,                    // ���豣�� D3D �豸���á�
//		nullptr,                    // ����֪�����ܼ���
//		nullptr                     // ���豣�� D3D �豸���������á�
//	);
//
//	return SUCCEEDED(hr);
//}
//#endif
