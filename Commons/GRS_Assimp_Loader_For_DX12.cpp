#include "GRS_Def.h"
#include "GRS_Assimp_Loader_For_DX12.h"
#include "GRS_D3D12_Utility.h"

#pragma comment(lib, "assimp-vc142-mtd.lib")


struct ST_GRS_ARRAY_DATA
{
	size_t m_szESize;
	size_t m_szECount;
	size_t m_szBufLength; // = m_szESize * m_szECount
	VOID*  m_pData; // length = m_szBufLength;
};

typedef CAtlArray< ST_GRS_ARRAY_DATA > CGRSArrayData;

BOOL LoadMesh_DX12(LPCSTR pszFileName
    , ID3D12Device4* pID3D12Device4
    , ST_GRS_MESH_DATA_MULTI_SLOT& stMeshBufferData
    , UINT nFlags)
{
	try
	{
		if (nullptr == pID3D12Device4)
		{
			AtlThrow( E_INVALIDARG );
		}

		if (!LoadMesh(pszFileName, stMeshBufferData.m_stMeshData, nFlags))
		{
			AtlThrowLastWin32();
		}

		CGRSArrayData arArrayData;
		ST_GRS_ARRAY_DATA szArData = {};
		D3D12_VERTEX_BUFFER_VIEW stVBV = {};
		D3D12_INPUT_ELEMENT_DESC stElementDesc = {};
		UINT nSlot = 0;
		//typedef struct D3D12_INPUT_ELEMENT_DESC
		//{
		//	LPCSTR SemanticName;
		//	UINT SemanticIndex;
		//	DXGI_FORMAT Format;
		//	UINT InputSlot;
		//	UINT AlignedByteOffset;
		//	D3D12_INPUT_CLASSIFICATION InputSlotClass;
		//	UINT InstanceDataStepRate;
		//} 	D3D12_INPUT_ELEMENT_DESC;

		if ( stMeshBufferData.m_stMeshData.m_arPositions.GetCount() > 0 )
		{
			szArData.m_szESize = sizeof(stMeshBufferData.m_stMeshData.m_arPositions[0]);
			szArData.m_szECount = stMeshBufferData.m_stMeshData.m_arPositions.GetCount();
			szArData.m_szBufLength = szArData.m_szESize * szArData.m_szECount;
			szArData.m_pData = stMeshBufferData.m_stMeshData.m_arPositions.GetData();

			arArrayData.Add(szArData);

			stVBV.SizeInBytes = (UINT)szArData.m_szBufLength;
			stVBV.StrideInBytes = (UINT)szArData.m_szESize;
			stMeshBufferData.m_arVBV.Add(stVBV);

			size_t szIndex = stMeshBufferData.m_arSemanticName.Add("POSITION");
			stElementDesc.SemanticName = stMeshBufferData.m_arSemanticName[szIndex];
			stElementDesc.SemanticIndex = 0;
			stElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			stElementDesc.InputSlot = nSlot;
			stElementDesc.AlignedByteOffset = 0;
			stElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			stElementDesc.InstanceDataStepRate = 0;
			stMeshBufferData.m_arInputElementDesc.Add(stElementDesc);
			++nSlot;
		}

		if (stMeshBufferData.m_stMeshData.m_arNormals.GetCount() > 0)
		{
			szArData.m_szESize = sizeof(stMeshBufferData.m_stMeshData.m_arNormals[0]);
			szArData.m_szECount = stMeshBufferData.m_stMeshData.m_arNormals.GetCount();
			szArData.m_szBufLength = szArData.m_szESize * szArData.m_szECount;
			szArData.m_pData = stMeshBufferData.m_stMeshData.m_arNormals.GetData();
			arArrayData.Add(szArData);

			stVBV.SizeInBytes = (UINT)szArData.m_szBufLength;
			stVBV.StrideInBytes = (UINT)szArData.m_szESize;
			stMeshBufferData.m_arVBV.Add(stVBV);

			size_t szIndex = stMeshBufferData.m_arSemanticName.Add("NORMAL");
			stElementDesc.SemanticName = stMeshBufferData.m_arSemanticName[szIndex];
			stElementDesc.SemanticIndex = 0;
			stElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			stElementDesc.InputSlot = nSlot;
			stElementDesc.AlignedByteOffset = 0;
			stElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			stElementDesc.InstanceDataStepRate = 0;
			stMeshBufferData.m_arInputElementDesc.Add(stElementDesc);
			++nSlot;
		}

		if (stMeshBufferData.m_stMeshData.m_arTexCoords.GetCount() > 0)
		{
			szArData.m_szESize = sizeof(stMeshBufferData.m_stMeshData.m_arTexCoords[0]);
			szArData.m_szECount = stMeshBufferData.m_stMeshData.m_arTexCoords.GetCount();
			szArData.m_szBufLength = szArData.m_szESize * szArData.m_szECount;
			szArData.m_pData = stMeshBufferData.m_stMeshData.m_arTexCoords.GetData();
			arArrayData.Add(szArData);

			stVBV.SizeInBytes = (UINT)szArData.m_szBufLength;
			stVBV.StrideInBytes = (UINT)szArData.m_szESize;
			stMeshBufferData.m_arVBV.Add(stVBV);

			size_t szIndex = stMeshBufferData.m_arSemanticName.Add("TEXCOORD");
			stElementDesc.SemanticName = stMeshBufferData.m_arSemanticName[szIndex];
			stElementDesc.SemanticIndex = 0;
			stElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			stElementDesc.InputSlot = nSlot;
			stElementDesc.AlignedByteOffset = 0;
			stElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			stElementDesc.InstanceDataStepRate = 0;
			stMeshBufferData.m_arInputElementDesc.Add(stElementDesc);
			++nSlot;
		}

		if (stMeshBufferData.m_stMeshData.m_arTangent.GetCount() > 0)
		{
			szArData.m_szESize = sizeof(stMeshBufferData.m_stMeshData.m_arTangent[0]);
			szArData.m_szECount = stMeshBufferData.m_stMeshData.m_arTangent.GetCount();
			szArData.m_szBufLength = szArData.m_szESize * szArData.m_szECount;
			szArData.m_pData = stMeshBufferData.m_stMeshData.m_arTangent.GetData();
			arArrayData.Add(szArData);

			stVBV.SizeInBytes = (UINT)szArData.m_szBufLength;
			stVBV.StrideInBytes = (UINT)szArData.m_szESize;
			stMeshBufferData.m_arVBV.Add(stVBV);

			size_t szIndex = stMeshBufferData.m_arSemanticName.Add("TANGENT");
			stElementDesc.SemanticName = stMeshBufferData.m_arSemanticName[szIndex];
			stElementDesc.SemanticIndex = 0;
			stElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			stElementDesc.InputSlot = nSlot;
			stElementDesc.AlignedByteOffset = 0;
			stElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			stElementDesc.InstanceDataStepRate = 0;
			stMeshBufferData.m_arInputElementDesc.Add(stElementDesc);
			++nSlot;
		}

		if (stMeshBufferData.m_stMeshData.m_arBitangent.GetCount() > 0)
		{
			szArData.m_szESize = sizeof(stMeshBufferData.m_stMeshData.m_arBitangent[0]);
			szArData.m_szECount = stMeshBufferData.m_stMeshData.m_arBitangent.GetCount();
			szArData.m_szBufLength = szArData.m_szESize * szArData.m_szECount;
			szArData.m_pData = stMeshBufferData.m_stMeshData.m_arBitangent.GetData();
			arArrayData.Add(szArData);

			stVBV.SizeInBytes = (UINT)szArData.m_szBufLength;
			stVBV.StrideInBytes = (UINT)szArData.m_szESize;
			stMeshBufferData.m_arVBV.Add(stVBV);

			size_t szIndex = stMeshBufferData.m_arSemanticName.Add("BITANGENT");
			stElementDesc.SemanticName = stMeshBufferData.m_arSemanticName[szIndex];
			stElementDesc.SemanticIndex = 0;
			stElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			stElementDesc.InputSlot = nSlot;
			stElementDesc.AlignedByteOffset = 0;
			stElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			stElementDesc.InstanceDataStepRate = 0;
			stMeshBufferData.m_arInputElementDesc.Add(stElementDesc);
			++nSlot;
		}

		if (stMeshBufferData.m_stMeshData.m_arBoneIndices.GetCount() > 0)
		{
			szArData.m_szESize = sizeof(stMeshBufferData.m_stMeshData.m_arBoneIndices[0]);
			szArData.m_szECount = stMeshBufferData.m_stMeshData.m_arBoneIndices.GetCount();
			szArData.m_szBufLength = szArData.m_szESize * szArData.m_szECount;
			szArData.m_pData = stMeshBufferData.m_stMeshData.m_arBoneIndices.GetData();
			arArrayData.Add(szArData);

			stVBV.SizeInBytes = (UINT)szArData.m_szBufLength;
			stVBV.StrideInBytes = (UINT)szArData.m_szESize;
			stMeshBufferData.m_arVBV.Add(stVBV);

			size_t szIndex = stMeshBufferData.m_arSemanticName.Add("BLENDINDICES");
			stElementDesc.SemanticName = stMeshBufferData.m_arSemanticName[szIndex];
			stElementDesc.SemanticIndex = 0;
			stElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			stElementDesc.InputSlot = nSlot;
			stElementDesc.AlignedByteOffset = 0;
			stElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			stElementDesc.InstanceDataStepRate = 0;
			stMeshBufferData.m_arInputElementDesc.Add(stElementDesc);

			szIndex = stMeshBufferData.m_arSemanticName.Add("BLENDWEIGHT");
			stElementDesc.SemanticName = stMeshBufferData.m_arSemanticName[szIndex];
			stElementDesc.SemanticIndex = 0;
			stElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			stElementDesc.InputSlot = nSlot;
			stElementDesc.AlignedByteOffset = 16;
			stElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			stElementDesc.InstanceDataStepRate = 0;
			stMeshBufferData.m_arInputElementDesc.Add(stElementDesc);

			++nSlot;
		}
		
		stMeshBufferData.m_nMaxSlot = nSlot;

		ATLASSERT( stMeshBufferData.m_arVBV.GetCount() == arArrayData.GetCount() );

		D3D12_HEAP_PROPERTIES stUploadHeapProps = {};
		stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		stUploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stUploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stUploadHeapProps.CreationNodeMask = 0;
		stUploadHeapProps.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC stBufferResSesc = {};
		stBufferResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stBufferResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stBufferResSesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		stBufferResSesc.Format = DXGI_FORMAT_UNKNOWN;
		stBufferResSesc.Width = 0;
		stBufferResSesc.Height = 1;
		stBufferResSesc.DepthOrArraySize = 1;
		stBufferResSesc.MipLevels = 1;
		stBufferResSesc.SampleDesc.Count = 1;
		stBufferResSesc.SampleDesc.Quality = 0;
		
		stMeshBufferData.m_arIVB.SetCount(arArrayData.GetCount());

		BYTE* pData = nullptr;
		// ���� Vertex Buffer
		for ( size_t i = 0; i < arArrayData.GetCount(); i++ )
		{
			ATLASSERT(arArrayData[i].m_szBufLength > 0);
			stBufferResSesc.Width = arArrayData[i].m_szBufLength;
			// �������㻺�� �������ڴ��У�
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stUploadHeapProps
				, D3D12_HEAP_FLAG_NONE
				, &stBufferResSesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&stMeshBufferData.m_arIVB[i])));

			GRS_SET_D3D12_DEBUGNAME_INDEXED_COMPTR(stMeshBufferData.m_arIVB,(UINT)i);

			GRS_THROW_IF_FAILED(stMeshBufferData.m_arIVB[i]->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
			// ��һ�� Copy��CPU Memory -> Share Memory ʵ���Ͼ������ڴ��е��ڣ�ֻ�������ַ��ͬ
			// ���ƻ�����ôһ�죬���Copy�������ܱ����滻ΪCPU�����ַӳ��ı������˵Vulkan���Ѿ�������������
			memcpy(pData, arArrayData[i].m_pData, arArrayData[i].m_szBufLength);
			stMeshBufferData.m_arIVB[i]->Unmap(0, nullptr);

			stMeshBufferData.m_arVBV[i].BufferLocation = stMeshBufferData.m_arIVB[i]->GetGPUVirtualAddress();
		}

		//���� Index Buffer
		size_t szIndices
			= stMeshBufferData.m_stMeshData.m_arIndices.GetCount()
			* sizeof(stMeshBufferData.m_stMeshData.m_arIndices[0]);

		stBufferResSesc.Width = szIndices;
		GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
			&stUploadHeapProps
			, D3D12_HEAP_FLAG_NONE
			, &stBufferResSesc
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&stMeshBufferData.m_pIIB)));
		GRS_SET_D3D12_DEBUGNAME_COMPTR(stMeshBufferData.m_pIIB);

		GRS_THROW_IF_FAILED(stMeshBufferData.m_pIIB->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
		memcpy(pData, stMeshBufferData.m_stMeshData.m_arIndices.GetData(), szIndices);
		stMeshBufferData.m_pIIB->Unmap(0, nullptr);

		//����Index Buffer View
		stMeshBufferData.m_stIBV.BufferLocation = stMeshBufferData.m_pIIB->GetGPUVirtualAddress();
		stMeshBufferData.m_stIBV.Format = DXGI_FORMAT_R32_UINT;
		stMeshBufferData.m_stIBV.SizeInBytes = (UINT)szIndices;
	}
	catch (CAtlException& e)
	{//������COM�쳣
		e;
	}
	catch (...)
	{

	}
	return TRUE;
}