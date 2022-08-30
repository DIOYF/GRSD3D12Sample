#pragma once
#include <SDKDDKVer.h>
#include <tchar.h>
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ���ų�����ʹ�õ�����
#include <windows.h>
#include <strsafe.h>

#include <atlconv.h>
#include <atlcoll.h>
#include <atlstr.h>
#include <d3dcompiler.h>

#include "GRS_Mem.h"

#define GRS_FILE_IS_EXIST(f) (INVALID_FILE_ATTRIBUTES != ::GetFileAttributes(f))

#ifndef GRS_SAFE_CLOSEFILE
//��ȫ�ر�һ���ļ����
#define GRS_SAFE_CLOSEFILE(h) if(INVALID_HANDLE_VALUE != (h)){::CloseHandle(h);(h)=INVALID_HANDLE_VALUE;}
#endif // !GRS_SAFE_CLOSEFILE

// ǿ��D3DCompiler������׼��IncludeĿ¼��ͨ����Ҳûɶ�ã�ֻ�Ǳ����ǵ�����ô�����ü���
#ifndef D3D_COMPILE_STANDARD_FILE_INCLUDE
#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)
#endif

typedef CAtlArray<CAtlString> CStringList;

class CGRSD3DCompilerInclude final : public ID3DInclude
{
protected:
	CStringList m_DirList;
public:
	CGRSD3DCompilerInclude(LPCTSTR pszDir)
	{
		AddDir(pszDir);
	}

	virtual ~CGRSD3DCompilerInclude()
	{
	}
public:
	VOID AddDir(LPCTSTR pszDir)
	{
		CString strDir(pszDir);
		if ( ( strDir.GetLength() > 0 ) )
		{ //�ǿ�·��
			if (_T('\\') == strDir[strDir.GetLength() - 1])
			{
				strDir.SetAt(strDir.GetLength() - 1, _T('\0'));
			}

			for ( size_t i = 0; i < m_DirList.GetCount(); i++ )
			{
				if ( m_DirList[i] == strDir )
				{// �Ѿ����ڲ����������
					return;
				}
			}

			m_DirList.Add(strDir);
		}
	}
public:
	STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
	{
		HANDLE hFile = INVALID_HANDLE_VALUE;
		VOID* pFileData = nullptr;
		*ppData = nullptr;
		HRESULT _hrRet = S_OK;
		try
		{ 
			CString strFileName(pFileName);
			TCHAR pszFullFileName[MAX_PATH] = {};

			for (size_t i = 0; i < m_DirList.GetCount(); i++)
			{
				HRESULT hr = ::StringCchPrintf(pszFullFileName
					, MAX_PATH
					, _T("%s\\%s")
					, m_DirList[i].GetBuffer()
					, strFileName.GetBuffer() );

				if ( FAILED(hr) )
				{
					AtlThrow(hr);
				}

				if ( !GRS_FILE_IS_EXIST(pszFullFileName) )
				{
					// ��¼���������ֱ��������Ҳ�����ô����Ϊ���Ĵ����뷵��
					_hrRet = HRESULT_FROM_WIN32(::GetLastError());
					continue;
				}

				hFile = ::CreateFile(pszFullFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

				if ( INVALID_HANDLE_VALUE == hFile )
				{
					AtlThrowLastWin32();
				}

				DWORD dwFileSize = ::GetFileSize(hFile, NULL);//Include�ļ�һ�㲻�ᳬ��4G��С,����......

				if (INVALID_FILE_SIZE == dwFileSize)
				{
					AtlThrowLastWin32();
				}

				VOID* pFileData = GRS_CALLOC(dwFileSize);
				if (nullptr == pFileData)
				{
					AtlThrowLastWin32();
				}

				if (!::ReadFile(hFile, pFileData, dwFileSize, (LPDWORD)pBytes, NULL))
				{
					AtlThrowLastWin32();
				}
				GRS_SAFE_CLOSEFILE(hFile);

				*ppData = pFileData;
				_hrRet = S_OK;
				break;				
			}
		}
		catch (CAtlException& e)
		{
			GRS_SAFE_CLOSEFILE(hFile);
			GRS_SAFE_FREE(pFileData);
			_hrRet = e.m_hr;
		}
		GRS_SAFE_CLOSEFILE(hFile);
		return _hrRet;
	}

	STDMETHOD(Close)(THIS_ LPCVOID pData) override
	{
		GRS_FREE((VOID*)pData);
		return S_OK;
	}
};