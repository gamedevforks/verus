// Copyright (C) 2021-2022, Dmitry Maluev (dmaluev@gmail.com). All rights reserved.
#pragma once

namespace verus::CGI
{
	class GeometryD3D12 : public BaseGeometry
	{
		struct BufferEx
		{
			D3D12MA::Allocation* _pMaAllocation = nullptr;
			ComPtr<ID3D12Resource> _pBuffer;
			UINT64                 _bufferSize = 0;
			INT64                  _utilization = -1;
		};

		struct VertexBufferEx : BufferEx
		{
			D3D12_VERTEX_BUFFER_VIEW _bufferView[BaseRenderer::s_ringBufferSize] = {};
		};

		struct StructuredBufferEx : BufferEx
		{
			DescriptorHeap _dhDynamicOffsets;
			int            _structSize = 0;
		};

		Vector<VertexBufferEx>           _vVertexBuffers;
		BufferEx                         _indexBuffer;
		Vector<VertexBufferEx>           _vStagingVertexBuffers;
		BufferEx                         _stagingIndexBuffer;
		Vector<StructuredBufferEx>       _vStructuredBuffers;
		D3D12_INDEX_BUFFER_VIEW          _indexBufferView[BaseRenderer::s_ringBufferSize] = {};
		Vector<D3D12_INPUT_ELEMENT_DESC> _vInputElementDescs;
		Vector<int>                      _vStrides;

	public:
		GeometryD3D12();
		virtual ~GeometryD3D12() override;

		virtual void Init(RcGeometryDesc desc) override;
		virtual void Done() override;

		virtual void CreateVertexBuffer(int count, int binding) override;
		virtual void UpdateVertexBuffer(const void* p, int binding, PBaseCommandBuffer pCB, INT64 size, INT64 offset) override;

		virtual void CreateIndexBuffer(int count) override;
		virtual void UpdateIndexBuffer(const void* p, PBaseCommandBuffer pCB, INT64 size, INT64 offset) override;

		virtual void CreateStorageBuffer(int count, int structSize, int sbIndex, ShaderStageFlags stageFlags) override;
		virtual void UpdateStorageBuffer(const void* p, int sbIndex, PBaseCommandBuffer pCB, INT64 size, INT64 offset) override;
		virtual int GetStorageBufferStructSize(int sbIndex) const override;

		virtual Continue Scheduled_Update() override;

		//
		// D3D12
		//

		D3D12_INPUT_LAYOUT_DESC GetD3DInputLayoutDesc(UINT32 bindingsFilter, Vector<D3D12_INPUT_ELEMENT_DESC>& vInputElementDescs) const;

		int GetVertexBufferCount() const { return Utils::Cast32(_vVertexBuffers.size()); }
		const D3D12_VERTEX_BUFFER_VIEW* GetD3DVertexBufferView(int binding) const;
		const D3D12_INDEX_BUFFER_VIEW* GetD3DIndexBufferView() const;
		CD3DX12_GPU_DESCRIPTOR_HANDLE CopyStructuredBufferView(int sbIndex) const;

		void UpdateUtilization() const;
	};
	VERUS_TYPEDEFS(GeometryD3D12);
}
