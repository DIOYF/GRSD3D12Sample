//===================================================================================
//��Ŀ���ƣ�GRS ����ƽ̨ V2.0
//�ļ��������쳣������ �Լ��쳣ģ��ӿڶ����ļ�
//ģ�����ƣ�C/C++ȫ���쳣֧��ģ��
//
//��֯����˾�����ţ����ƣ�One Man Group
//���ߣ�Gamebaby Rock Sun
//���ڣ�2010-5-20 14:29:45
//�޶����ڣ�2011-11-21
//===================================================================================
#include "GRS_Win.h"
#include <eh.h>

//���������GRS_EXPORT�� ����ôʹ��GRS_DLL������ཫ��ɵ�����
#ifdef GRSEXCEPTION_EXPORTS
#define GRS_EXPORT
#endif
#include "GRS_DLL.h"

#pragma once

#define GRS_ERRBUF_MAXLEN	1024

//�쳣�࣬����ཫ�����������Ψһ���쳣��
class GRS_DLL CGRSException
{
	enum EM_GRS_EXCEPTION
	{//�쳣�ľ��������ö��ֵ�����ھ���������ϸ���쳣��Ϣ
		ET_Empty = 0x0,
		ET_SE,				
		ET_LastError,
		ET_COM,
		ET_Customer
	};
protected:
	EM_GRS_EXCEPTION m_EType;		//�쳣����
	UINT m_nSEHCode;
	DWORD m_dwLastError;

	EXCEPTION_POINTERS *m_EP;
	TCHAR m_lpErrMsg[GRS_ERRBUF_MAXLEN];
public:
	CGRSException& operator = ( CGRSException& ) = delete;
public:
	CGRSException()
		: m_nSEHCode(0)	, m_EP(nullptr),m_dwLastError(0),m_lpErrMsg(),m_EType(ET_Empty)
	{
	}

	//CGRSException(CGRSException& e)
	//{
	//}

	CGRSException( LPCTSTR pszSrcFile, INT iLine, HRESULT hr )
		: m_nSEHCode(0), m_EP(nullptr), m_dwLastError(0), m_lpErrMsg(), m_EType(ET_COM)
	{
		_com_error ce(hr);
		StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN, _T("%s(%d):����COM����0x%X��ԭ��%s")
			, pszSrcFile
			, iLine
			, hr
			, ce.ErrorMessage());
	}

	CGRSException( LPCTSTR pszSrcFile, INT iLine )
		:m_nSEHCode(0), m_EP(nullptr), m_dwLastError(0), m_lpErrMsg(), m_EType(ET_LastError)
	{
		ToString( pszSrcFile, iLine, ::GetLastError() );
	}

	CGRSException( UINT nCode,EXCEPTION_POINTERS* ep )
		:m_nSEHCode(nCode),m_EP(ep),m_dwLastError(0),m_lpErrMsg(),m_EType(ET_SE)
	{//�ṹ���쳣
		//���¿�������еĳ���������ntstatus.h�� 
		switch(m_nSEHCode)
		{
		case 0xC0000029://STATUS_INVALID_UNWIND_TARGET:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("չ��(unwind)������������Ч��չ��λ��"));
			}
			break;
		case EXCEPTION_ACCESS_VIOLATION:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�̳߳��Զ����ַ��һ����Ч����."));
			}
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�̳߳��Զ���Ӳ������߽����һ��Խ�����."));
			}
			break;
		case EXCEPTION_BREAKPOINT:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�����ϵ�."));
			}
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�̳߳����ڲ�֧�ֶ����Ӳ���Ϸ���δ���������."));
			}
			break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("������������̫С."));
			}
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�����������."));
			}
			break;
		case EXCEPTION_FLT_INEXACT_RESULT:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("����������޷�����ȷ����."));
			}
			break;
		case EXCEPTION_FLT_INVALID_OPERATION:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("δ֪�ĸ������쳣."));
			}
			break;
		case EXCEPTION_FLT_OVERFLOW:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("������������ָ������ָ�����������������."));
			}
			break;
		case EXCEPTION_FLT_STACK_CHECK:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�����������е��µ�ջ���."));
			}
			break;
		case EXCEPTION_FLT_UNDERFLOW:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("������������ָ������ָ��������Ҫ��������."));
			}
			break;
		case EXCEPTION_GUARD_PAGE:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�̷߳��ʵ���PAGE_GUARD���η�����ڴ�."));
			}
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�߳���ͼִ�в�����ָ��."));
			}
			break;
		case EXCEPTION_IN_PAGE_ERROR:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�߳���ͼ����һ��ϵͳ��ǰ�޷������ȱʧҳ."));
			}
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("���������."));
			}
			break;
		case EXCEPTION_INT_OVERFLOW:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�������������λ���������Чλ."));
			}
			break;
		case EXCEPTION_INVALID_DISPOSITION:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�쳣������̷�����һ�������ÿص���쳣���ַ�����."));
			}
			break;
		case EXCEPTION_INVALID_HANDLE:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�߳�ʹ����һ�������õ��ں˶�����(�þ�������ѹر�)."));
			}
			break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�߳���ͼ�ڲ��ɼ������쳣֮�����ִ��."));
			}
			break;
		case EXCEPTION_PRIV_INSTRUCTION:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�߳���ͼͨ��ִ��һ��ָ����һ���������ڵ�ǰ�����ģʽ�Ĳ���."));
			}
			break;
		case EXCEPTION_SINGLE_STEP:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("ĳָ��ִ������ź�, ��������һ�����������������ָ�����."));
			}
			break;
		case EXCEPTION_STACK_OVERFLOW:
			{
				StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN,_T("�߳�ջ�ռ�ľ�."));
			}
			break;
		default://δ������쳣��
			StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN, _T("CGRSException��δ֪�쳣,code=0x%08X"), 
				m_nSEHCode);
			break;
		};
	}

	//CGRSException( LPCTSTR pszSrcFile, INT iLine,DWORD dwLastError )
	//	:m_nSEHCode(0),m_EP(nullptr),m_dwLastError(dwLastError),m_lpErrMsg(),m_EType(ET_LastError)
	//{
	//	ToString(pszSrcFile,iLine,dwLastError);
	//}

	//CGRSException( EM_GRS_EXCEPTION EType,LPCTSTR pszFmt,... )
	//	:m_nSEHCode(0),m_EP(nullptr),m_dwLastError(0),m_lpErrMsg(),m_EType(EType)
	//{
	//	va_list va;
	//	va_start(va, pszFmt);
	//	if( nullptr != m_lpErrMsg )
	//	{
	//		StringCchVPrintf(m_lpErrMsg,GRS_ERRBUF_MAXLEN,pszFmt,va);
	//	}
	//	va_end(va);
	//}

	CGRSException( LPCTSTR pszFmt , ... )
		:m_nSEHCode(0),m_EP(nullptr),m_dwLastError(0),m_lpErrMsg(),m_EType( ET_Customer )
	{
		va_list va;
		va_start(va, pszFmt);

		if( nullptr != m_lpErrMsg )
		{
			StringCchVPrintf(m_lpErrMsg,GRS_ERRBUF_MAXLEN,pszFmt,va);
		}
		va_end(va);
	}

	virtual ~CGRSException()
	{
	}
public:
	VOID ToString(LPCTSTR pszSrcFile, INT iLine , DWORD dwErrorCode )
	{//���߷��������õ�һ���������Ĵ�����Ϣ 
    //ע����Ϊ������������˶����ڲ����ַ����ڴ棬
    //Ϊ���ܹ��ڶ���ʧЧʱ�����ͷ�����û�а����������Ƴɾ�̬����
		ZeroMemory(m_lpErrMsg, GRS_ERRBUF_MAXLEN * sizeof(TCHAR));

		LPTSTR lpMsgBuf = nullptr;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, nullptr);

		size_t szLen = 0;
		StringCchLength(lpMsgBuf, GRS_ERRBUF_MAXLEN, &szLen);

		if (szLen > 0)
		{
			StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN, _T("%s(%d):��������%d��ԭ��%s")
				, pszSrcFile
				, iLine
				, dwErrorCode
				, lpMsgBuf);

			LocalFree(lpMsgBuf);
		}
		else
		{
			StringCchPrintf(m_lpErrMsg, GRS_ERRBUF_MAXLEN, _T("%s(%d):��������%d��ԭ����")
				, pszSrcFile
				, iLine
				, dwErrorCode);
		}
	}

public:
	LPCTSTR GetReason() const
	{
		return m_lpErrMsg;
	}

	DWORD GetErrCode() const
	{
		return (0 == m_dwLastError)?m_nSEHCode:m_dwLastError;
	}

	EM_GRS_EXCEPTION GetType()
	{
		return m_EType;
	}

    LPCTSTR GetTypeString()
    {
        static const TCHAR* pszTypeString[] = {
            _T("Empty"),
            _T("SE"),
            _T("Thread Last Error"),
            _T("Customer"),
			_T("COM"),
            nullptr
        };

        if( m_EType < sizeof(pszTypeString)/sizeof(pszTypeString[0]))
        {
            return pszTypeString[m_EType];
        }
        return nullptr;
    }
};

__inline void GRS_SEH_Handle(unsigned int code, struct _EXCEPTION_POINTERS *ep)
{//ע��������쳣�׳���ʽ��������֤���е��쳣��ջ�ϲ������Զ�����
	throw CGRSException(code,ep);
}

#ifndef GRS_EH_INIT
#define GRS_EH_INIT() _set_se_translator(GRS_SEH_Handle);
#endif // !GRS_EH_INIT

#ifndef GRS_THROW_LASTERROR
#define GRS_THROW_LASTERROR() throw CGRSException(__WFILE__,__LINE__);
#endif // !GRS_THROW_LASTERROR

#ifndef GRS_THROW_COM_ERROR
#define GRS_THROW_COM_ERROR(hr) throw CGRSException(__WFILE__,__LINE__,(hr));
#endif // !GRS_THROW_COM_ERROR


inline void GRSThrowIfFailed(LPCWSTR pszSrcFile,UINT nLine, HRESULT hr)
{
	if ( FAILED( hr ) )
	{
		// �ڴ��������öϵ㣬�Բ��� Win32 API ����
		throw CGRSException(pszSrcFile,nLine,hr);
	}
}

#define GRS_THROW_IF_FAILED(hr) GRSThrowIfFailed( __WFILE__,__LINE__,(hr) )