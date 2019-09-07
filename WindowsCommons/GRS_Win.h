//===================================================================================
//��Ŀ���ƣ�GRS ����ƽ̨ V2.0
//�ļ�������Windowsͷ�ļ��ٷ�װͷ�ļ�������Ҫ�ļ�Windows����Ҳ���ϵ�һ��
//ģ�����ƣ�GRSLib V2.0
//
//��֯����˾�����ţ����ƣ�One Man Group
//���ߣ�Gamebaby Rock Sun
//���ڣ�2010-5-20 14:27:19
//�޶����ڣ�2017-11-25
//===================================================================================
#pragma once
//���ü�������ΪĬ�ϴ���ҳ
#pragma setlocale("chinese-simplified")

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ���ų�����ʹ�õ�����
#include <windows.h>
#include <tchar.h>
#include <process.h> //for _beginthread, _endthread 
#include <locale.h>

//Winsock & Net API
#include <winsock2.h>
#include <ws2ipdef.h>
#include <windns.h>
#include <iphlpapi.h>
#include <MSWSOCK.h> 	

//���WTL֧�� ����ʹ��COM
#include <wrl.h>
using namespace Microsoft;
using namespace Microsoft::WRL;
#include <shellapi.h>

//#include <winerror.h> //for downlist code
//#ifndef __midl
//FORCEINLINE _Translates_Win32_to_HRESULT_(x) HRESULT HRESULT_FROM_WIN32(unsigned long x) { return (HRESULT)(x) <= 0 ? (HRESULT)(x) : (HRESULT)(((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000); }
//#else
//#define HRESULT_FROM_WIN32(x) __HRESULT_FROM_WIN32(x)
//#endif

#pragma comment( lib, "Ws2_32.lib" )
#pragma comment( lib, "Iphlpapi.lib")
#pragma comment( lib, "Mswsock.lib" )

//COM Support
#include <comutil.h> 
#include <comip.h>	//for _com_ptr_t
#include <comdef.h> //for _com_error

#ifdef _DEBUG
#pragma comment (lib,"comsuppwd.lib")
#else
#pragma comment (lib,"comsuppw.lib")
#endif


//ʹ��ϵͳ��ȫ�ַ�������֧��
#include <strsafe.h>
//ʹ��ATL���ַ���ת��֧��
#include <atlconv.h>
//ʹ��ATL������
#include <atlcoll.h>

using namespace ATL;



//XML����֧��
#include <msxml6.h>
#pragma comment (lib,"msxml6.lib")

//����һ�ַ�����ֱ�ӵ���DLL�е�������Ϣ����Ҫʱ���л�ԭ����ע��
//#import "msxml6.dll"
//using namespace MSXML2;

//IXMLDOMParseError*      pXMLErr = nullptr;
//IXMLDOMDocument*		pXMLDoc = nullptr;
//
//hr = pXMLDoc->loadXML(pXMLString, &bSuccess);
//
//GRS_SAFEFREE(pXMLData);
//
//if (VARIANT_TRUE != bSuccess || FAILED(hr))
//{
//	pXMLDoc->get_parseError(&pXMLErr);
//	ReportDocErr(argv[1], pXMLErr);
//}

//VOID ReportDocErr(LPCTSTR pszXMLFileName, IXMLDOMParseError* pIDocErr)
//{
//	GRS_USEPRINTF();
//	long lLine = 0;
//	long lCol = 0;
//	LONG lErrCode = 0;
//	BSTR bstrReason = nullptr;
//
//	pIDocErr->get_line(&lLine);
//	pIDocErr->get_linepos(&lCol);
//	pIDocErr->get_reason(&bstrReason);
//
//	GRS_PRINTF(_T("XML�ĵ�\"%s\"��(��:%ld,��:%ld)�����ִ���(0x%08x),ԭ��:%s\n")
//		, pszXMLFileName, lLine, lCol, lErrCode, bstrReason);
//
//	SysFreeString(bstrReason);
//
//}

//===================================================================================
//Debug ֧����غ궨��

//�������������Ϣʱ�����������͵���Ϣ����ַ����͵�
#ifndef GRS_STRING2
#define GRS_STRING2(x) #x
#endif // !GRS_STRING2

#ifndef GRS_STRING
#define GRS_STRING(x) GRS_STRING2(x)
#endif // !GRS_STRING

#ifndef GRS_WSTRING2
#define GRS_WSTRING2(x) L #x
#endif // !GRS_WSTRING2

#ifndef GRS_WSTRING
#define GRS_WSTRING(x) GRS_WSTRING2(x)
#endif // !GRS_WSTRING

#ifndef GRS_WIDEN2
#define GRS_WIDEN2(x) L ## x
#endif // !GRS_WIDEN2

#ifndef GRS_WIDEN
#define GRS_WIDEN(x) GRS_WIDEN2(x)
#endif // !GRS_WIDEN

#ifndef __WFILE__
#define __WFILE__ GRS_WIDEN(__FILE__)
#endif // !GRS_WIDEN

#ifndef GRS_DBGOUT_BUF_LEN
#define GRS_DBGOUT_BUF_LEN 2048
#endif

//���Զ��壨��GRS_Def.h�ļ��������
#ifdef _DEBUG
#define GRS_ASSERT(s) if(!(s)) { ::DebugBreak(); }
#else
#define GRS_ASSERT(s) 
#endif

__inline void __cdecl GRSDebugOutputA(LPCSTR pszFormat, ...)
{//UNICODE��Ĵ�ʱ���Trace
	va_list va;
	va_start(va, pszFormat);

	CHAR pBuffer[GRS_DBGOUT_BUF_LEN] = {};

	if (S_OK != ::StringCchVPrintfA(pBuffer, sizeof(pBuffer) / sizeof(pBuffer[0]), pszFormat, va))
	{
		va_end(va);
		return;
	}
	va_end(va);
	OutputDebugStringA(pBuffer);
}

__inline void __cdecl GRSDebugOutputW(LPCWSTR pszFormat, ...)
{//UNICODE��Ĵ�ʱ���Trace
	va_list va;
	va_start(va, pszFormat);

	WCHAR pBuffer[GRS_DBGOUT_BUF_LEN] = {};

	if (S_OK != ::StringCchVPrintfW(pBuffer, sizeof(pBuffer) / sizeof(pBuffer[0]), pszFormat, va))
	{
		va_end(va);
		return;
	}
	va_end(va);
	OutputDebugStringW(pBuffer);
}

#ifdef UNICODE
#define GRSDebugOutput GRSDebugOutputW
#else 
#define GRSDebugOutput GRSDebugOutputA
#endif


//��ͨ�ĵ������֧��
#if defined(DEBUG) | defined(_DEBUG)
#define GRS_TRACE(...)		GRSDebugOutput(__VA_ARGS__)
#define GRS_TRACEA(...)		GRSDebugOutputA(__VA_ARGS__)
#define GRS_TRACEW(...)		GRSDebugOutputW(__VA_ARGS__)
#define GRS_TRACE_LINE()	GRSDebugOutputA("%s(%d):",__FILE__, __LINE__)
#else
#define GRS_TRACE(...)
#define GRS_TRACEA(...)
#define GRS_TRACEW(...)
#define GRS_TRACE_LINE()
#endif

#define GRS_TRACE_ON1

#if defined(GRS_DBGOUT_ON1)
#define GRS_TRACE1(...)		GRSDebugOutput(__VA_ARGS__)
#define GRS_TRACEA1(...)	GRSDebugOutputA(__VA_ARGS__)
#define GRS_TRACEW1(...)	GRSDebugOutputW(__VA_ARGS__)
#define GRS_TRACE_LINE1()	GRSDebugOutputA("%s(%d):",__FILE__, __LINE__)
#else
#define GRS_TRACE1(...)
#define GRS_TRACEA1(...)
#define GRS_TRACEW1(...)
#define GRS_TRACE_LINE1()
#endif

//����쳣CGRSException
#if defined(DEBUG) | defined(_DEBUG)
#define GRS_TRACE_EXPW(Fun,e) \
			GRSDebugOutputW(L"%s(%d):%s���������쳣(0x%08X):%s\n"\
			,__WFILE__\
			,__LINE__\
			,L#Fun\
			,(e).GetErrCode()\
			,(e).GetReason())
#else
#define GRS_TRACE_EXPW(Fun,e)
#endif
//===================================================================================

//===================================================================================
//����꣬���ڼ������ʱ���ò����Ƿ���ȷ
#if defined(DEBUG) | defined(_DEBUG)
#define GRS_WARNING_IF(isTrue, ... )\
	{ \
		static bool s_TriggeredWarning = false; \
		if ((bool)(isTrue) && !s_TriggeredWarning)\
		{ \
			s_TriggeredWarning = true; \
			GRSDebugOutputW(L"%s(%d)�г����ж�����(����\'%s\'Ϊ��)��%s\n"\
			,__WFILE__\
			,__LINE__\
			,GRS_WSTRING(isTrue)\
			,__VA_ARGS__);\
		} \
	}

#define GRS_WARNING_IF_NOT( isTrue, ... ) GRS_WARNING_IF(!(isTrue), __VA_ARGS__)

#else

#define GRS_WARNING_IF( isTrue, ... ) 
#define GRS_WARNING_IF_NOT( isTrue, ... )

#endif
//===================================================================================

//===================================================================================
//��C �ڴ����⺯�� �궨�� ֱ�ӵ�����ӦWinAPI ���Ч��
#ifndef GRS_ALLOC
#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#endif // !GRS_ALLOC

#ifndef GRS_CALLOC
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#endif // !GRS_CALLOC

#ifndef GRS_REALLOC
#define GRS_REALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#endif // !GRS_REALLOC

#ifndef GRS_FREE
#define GRS_FREE(p)			HeapFree(GetProcessHeap(),0,p)
#endif // !GRS_FREE

#ifndef GRS_MSIZE
#define GRS_MSIZE(p)		HeapSize(GetProcessHeap(),0,p)
#endif // !GRS_MSIZE

#ifndef GRS_MVALID
#define GRS_MVALID(p)		HeapValidate(GetProcessHeap(),0,p)
#endif // !GRS_MVALID

#ifndef GRS_SAFEFREE
//����������ʽ���ܻ���������,p��������ʹ��ָ�����,�����Ǳ��ʽ
#define GRS_SAFE_FREE(p) if( nullptr != (p) ){ HeapFree(GetProcessHeap(),0,(p));(p)=nullptr; }
#endif // !GRS_SAFEFREE

#ifndef GRS_SAFE_DELETE
//����������ʽ���ܻ���������,p��������ʹ��ָ�����,�����Ǳ��ʽ
#define GRS_SAFE_DELETE(p) if( nullptr != (p) ){ delete p; (p) = nullptr; }
#endif // !GRS_SAFEFREE

#ifndef GRS_SAFE_DELETE_ARRAY
//����������ʽ���ܻ���������,p��������ʹ��ָ�����,�����Ǳ��ʽ
#define GRS_SAFE_DELETE_ARRAY(p) if( nullptr != (p) ){ delete[] p; (p) = nullptr; }
#endif // !GRS_SAFEFREE


#ifndef GRS_OPEN_HEAP_LFH
//������������ڴ򿪶ѵ�LFH����,���������
#define GRS_OPEN_HEAP_LFH(h) \
    ULONG  ulLFH = 2;\
    HeapSetInformation((h),HeapCompatibilityInformation,&ulLFH ,sizeof(ULONG) ) ;
#endif // !GRS_OPEN_HEAP_LFH
//===================================================================================

//===================================================================================
//���õĺ궨�� ͳһ�ռ�������
#ifndef GRS_NOVTABLE
#define GRS_NOVTABLE __declspec(novtable)
#endif // !GRS_NOVTABLE

#ifndef GRS_TLS
//TLS �洢֧��
#define GRS_TLS				__declspec( thread )
#endif // !GRS_TLS

//��������
#ifndef GRS_INLINE
#define GRS_INLINE			__inline
#endif

#ifndef GRS_FORCEINLINE
#define GRS_FORCEINLINE		__forceinline
#endif

#ifndef GRS_NOVTABLE
#define GRS_NOVTABLE		__declspec(novtable)
#endif

#ifndef GRS_CDECL
#define GRS_CDECL			__cdecl
#endif

#ifndef GRS_STDCL 
#define GRS_STDCL			__stdcall
#endif

#ifndef GRS_FASTCL
#define GRS_FASTCL			__fastcall
#endif

#ifndef GRS_PASCAL
#define GRS_PASCAL			__pascal
#endif

//ȡ��̬����Ԫ�صĸ���
#ifndef GRS_COUNTOF
#define GRS_COUNTOF(a)		sizeof(a)/sizeof(a[0])
#endif

//�������������ȡ������3/2 = 2
#ifndef GRS_UPDIV
#define GRS_UPDIV(a,b) ((a) + (b) - 1)/(b)
#endif

//��a���뵽b��������
#ifndef GRS_UPROUND
#define GRS_UPROUND(x,y) (((x)+((y)-1))&~((y)-1))
#endif

//��a������뵽4K����С������
#ifndef GRS_UPROUND4K
#define GRS_UPROUND4K(a) GRS_UPROUND(a,4096)
#endif

//��ʹ�õĲ���
#ifndef GRS_UNUSE
#define GRS_UNUSE(a) a;
#endif

#ifndef GRS_SLEEP
//���õȴ��߳̾���Լ���ȷ��ʱһ���ĺ�����,�����������ȷ��ϵͳ����Sleep
#define GRS_SLEEP(dwMilliseconds) WaitForSingleObject(GetCurrentThread(),dwMilliseconds)
#endif // !GRS_SLEEP

#ifndef GRS_FOREVERLOOP
//�������ȫ���н������ѭ���궨��
#define GRS_FOREVERLOOP for(;;)        
#endif // !GRS_FOREVERLOOP

//���ƾ���ĺ궨��
#ifndef GRS_DUPHANDLE
//����һ��������̳б�־�ر�
#define GRS_DUPHANDLE(h,hDup)	DuplicateHandle(GetCurrentProcess(),(h),GetCurrentProcess(),&(hDup),0,FALSE,DUPLICATE_SAME_ACCESS)
#endif // !GRS_DUPHANDLE

#ifndef GRS_DUPHANDLEI
//����һ��������̳б�־��
#define GRS_DUPHANDLEI(h,hDup)	DuplicateHandle(GetCurrentProcess(),(h),GetCurrentProcess(),&(hDup),0,TRUE,DUPLICATE_SAME_ACCESS)
#endif // !GRS_DUPHANDLEI

#ifndef GRS_SETHANDLE_NI
//��һ���������Ϊ���ܼ̳�
#define GRS_SETHANDLE_NI(h)		SetHandleInformation(h,HANDLE_FLAG_INHERIT,0)
#endif // !GRS_SETHANDLE_NI

#ifndef GRS_SETHANDLE_NC
//��һ���������Ϊ������CloseHandle�ر�
#define GRS_SETHANDLE_NC(h)		SetHandleInformation(h,HANDLE_FLAG_PROTECT_FROM_CLOSE,HANDLE_FLAG_PROTECT_FROM_CLOSE)
#endif // !GRS_SETHANDLE_NC

#ifndef GRS_SETHANDLE_I
//��һ���������Ϊ�ܼ̳�
#define GRS_SETHANDLE_I(h)		SetHandleInformation(h,HANDLE_FLAG_INHERIT,HANDLE_FLAG_INHERIT)
#endif // !GRS_SETHANDLE_I

#ifndef GRS_SETHANDLE_C
//��һ���������Ϊ�ܹ���CloseHandle�ر�
#define GRS_SETHANDLE_C(h)		SetHandleInformation(h,HANDLE_FLAG_PROTECT_FROM_CLOSE,0)
#endif // !GRS_SETHANDLE_C

#ifndef GRS_SAFE_CLOSEHANDLE
//��ȫ�ر�һ�����
#define GRS_SAFE_CLOSEHANDLE(h) if(nullptr != (h)){CloseHandle(h);(h)=nullptr;}
#endif // !GRS_SAFE_CLOSEHANDLE

#ifndef GRS_SAFE_CLOSEFILE
//��ȫ�ر�һ���ļ����
#define GRS_SAFE_CLOSEFILE(h) if(INVALID_HANDLE_VALUE != (h)){CloseHandle(h);(h)=INVALID_HANDLE_VALUE;}
#endif // !GRS_SAFE_CLOSEFILE
//=======================================����========================================

//===================================================================================
//ע��DEBUG���GRS_SAFE_RELEASEҪ�����Release��ʱ�����Ϊ0��ֻ��Ϊ�˵��Է���
//���ݲ�ͬCOM�ӿ��ڲ�������ʵ�ֵĲ�һ������˲��������еĽӿ����ͷ�ʱ���صļ�������0
//�������ֻ������֮�⣬�������Ҫ����������о�ʹ�ò�����ʱ����ע���Ǹ�GRS_ASSERT
#ifndef GRS_SAFE_RELEASE
#define GRS_SAFE_RELEASE(I) \
	if(nullptr != (I))\
	{\
		ULONG ulRefCnt = (I)->Release();\
		(I) = nullptr;\
	}
#endif // !GRS_SAFE_RELEASE

#ifndef GRS_SAFE_RELEASE_ASSERT
#define GRS_SAFE_RELEASE_ASSERT(I) \
	if(nullptr != (I))\
	{\
	ULONG ulRefCnt = (I)->Release();\
	GRS_ASSERT(0 == ulRefCnt);\
	(I) = nullptr;\
	}
#endif // !GRS_SAFE_RELEASE_ASSERT
//===================================================================================