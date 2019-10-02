#include "stdafx.h"

using namespace verus;
using namespace verus::CGI;

// ShaderInclude:

HRESULT STDMETHODCALLTYPE ShaderInclude::Open(
	D3D_INCLUDE_TYPE IncludeType,
	LPCSTR pFileName,
	LPCVOID pParentData,
	LPCVOID* ppData,
	UINT* pBytes)
{
	const String url = String("[Shaders]:") + pFileName;
	Vector<BYTE> vData;
	IO::FileSystem::LoadResource(_C(url), vData);
	char* p = new char[vData.size()];
	memcpy(p, vData.data(), vData.size());
	*pBytes = Utils::Cast32(vData.size());
	*ppData = p;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ShaderInclude::Close(LPCVOID pData)
{
	delete[] pData;
	return S_OK;
}

// ShaderD3D12:

ShaderD3D12::ShaderD3D12()
{
}

ShaderD3D12::~ShaderD3D12()
{
	Done();
}

void ShaderD3D12::Init(CSZ source, CSZ sourceName, CSZ* branches)
{
	VERUS_INIT();
	VERUS_QREF_CONST_SETTINGS;
	HRESULT hr = 0;

	const size_t len = strlen(source);
	ShaderInclude inc;
	const String version = "5_1";
#ifdef _DEBUG
	const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_OPTIMIZATION_LEVEL1 | D3DCOMPILE_DEBUG;
#else
	const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	ComPtr<ID3DBlob> pErrorMsgs;

	auto CheckErrorMsgs = [this](ComPtr<ID3DBlob>& pErrorMsgs)
	{
		if (pErrorMsgs)
		{
			OnError(static_cast<CSZ>(pErrorMsgs->GetBufferPointer()));
			pErrorMsgs.Reset();
		}
	};

	while (*branches)
	{
		String entryVS, entryHS, entryDS, entryGS, entryFS, entryCS;
		Vector<String> vMacroName;
		Vector<String> vMacroValue;
		const String entry = Parse(*branches, entryVS, entryHS, entryDS, entryGS, entryFS, entryCS, vMacroName, vMacroValue, "DEF_");

		if (IsInIgnoreList(_C(entry)))
		{
			branches++;
			continue;
		}

		// <User defines>
		Vector<D3D_SHADER_MACRO> vDefines;
		vDefines.reserve(20);
		const int num = Utils::Cast32(vMacroName.size());
		VERUS_FOR(i, num)
		{
			D3D_SHADER_MACRO sm;
			sm.Name = _C(vMacroName[i]);
			sm.Definition = _C(vMacroValue[i]);
			vDefines.push_back(sm);
		}
		// </User defines>

		// <System defines>
		char defAnisotropyLevel[64] = {};
		{
			sprintf_s(defAnisotropyLevel, "%d", settings._gpuAnisotropyLevel);
			vDefines.push_back({ "_ANISOTROPY_LEVEL", defAnisotropyLevel });
		}
		char defShadowQuality[64] = {};
		{
			sprintf_s(defShadowQuality, "%d", settings._sceneShadowQuality);
			vDefines.push_back({ "_SHADOW_QUALITY", defShadowQuality });
		}
		char defWaterQuality[64] = {};
		{
			sprintf_s(defWaterQuality, "%d", settings._sceneWaterQuality);
			vDefines.push_back({ "_WATER_QUALITY", defWaterQuality });
		}
		char defMaxNumBones[64] = {};
		{
			sprintf_s(defMaxNumBones, "%d", VERUS_MAX_NUM_BONES);
			vDefines.push_back({ "VERUS_MAX_NUM_BONES", defMaxNumBones });
		}
		vDefines.push_back({ "_DIRECT3D", "1" });
		const int typeIndex = Utils::Cast32(vDefines.size());
		vDefines.push_back({ "_XS", "1" });
		vDefines.push_back({});
		// </System defines>

		Compiled compiled;

		if (strstr(source, " mainVS("))
		{
			compiled._numStages++;
			vDefines[typeIndex].Name = "_VS";
			hr = D3DCompile(source, len, sourceName, vDefines.data(), &inc, _C(entryVS), _C("vs_" + version), flags, 0, &compiled._pBlobs[+Stage::vs], &pErrorMsgs);
			CheckErrorMsgs(pErrorMsgs);
		}

		if (strstr(source, " mainHS("))
		{
			compiled._numStages++;
			vDefines[typeIndex].Name = "_HS";
			hr = D3DCompile(source, len, sourceName, vDefines.data(), &inc, _C(entryHS), _C("hs_" + version), flags, 0, &compiled._pBlobs[+Stage::hs], &pErrorMsgs);
			CheckErrorMsgs(pErrorMsgs);
		}

		if (strstr(source, " mainDS("))
		{
			compiled._numStages++;
			vDefines[typeIndex].Name = "_DS";
			hr = D3DCompile(source, len, sourceName, vDefines.data(), &inc, _C(entryDS), _C("ds_" + version), flags, 0, &compiled._pBlobs[+Stage::ds], &pErrorMsgs);
			CheckErrorMsgs(pErrorMsgs);
		}

		if (strstr(source, " mainGS("))
		{
			compiled._numStages++;
			vDefines[typeIndex].Name = "_GS";
			hr = D3DCompile(source, len, sourceName, vDefines.data(), &inc, _C(entryGS), _C("gs_" + version), flags, 0, &compiled._pBlobs[+Stage::gs], &pErrorMsgs);
			CheckErrorMsgs(pErrorMsgs);
		}

		if (strstr(source, " mainFS("))
		{
			compiled._numStages++;
			vDefines[typeIndex].Name = "_FS";
			hr = D3DCompile(source, len, sourceName, vDefines.data(), &inc, _C(entryFS), _C("ps_" + version), flags, 0, &compiled._pBlobs[+Stage::fs], &pErrorMsgs);
			CheckErrorMsgs(pErrorMsgs);
		}

		if (strstr(source, " mainCS("))
		{
			compiled._numStages++;
			vDefines[typeIndex].Name = "_CS";
			hr = D3DCompile(source, len, sourceName, vDefines.data(), &inc, _C(entryCS), _C("cs_" + version), flags, 0, &compiled._pBlobs[+Stage::cs], &pErrorMsgs);
			CheckErrorMsgs(pErrorMsgs);
			_compute = true;
		}

		_mapCompiled[entry] = compiled;

		branches++;
	}

	_vDescriptorSetDesc.reserve(4);
}

void ShaderD3D12::Done()
{
	for (auto& dsd : _vDescriptorSetDesc)
	{
		VERUS_SMART_RELEASE(dsd._pMaAllocation);
		VERUS_COM_RELEASE_CHECK(dsd._pConstantBuffer.Get());
		dsd._dhDynamicOffsets.Reset();
		dsd._pConstantBuffer.Reset();
	}

	VERUS_COM_RELEASE_CHECK(_pRootSignature.Get());
	_pRootSignature.Reset();

	for (auto& x : _mapCompiled)
	{
		VERUS_FOR(i, +Stage::count)
		{
			VERUS_COM_RELEASE_CHECK(x.second._pBlobs[i].Get());
			x.second._pBlobs[i].Reset();
		}
	}

	VERUS_DONE(ShaderD3D12);
}

void ShaderD3D12::CreateDescriptorSet(int setNumber, const void* pSrc, int size, int capacity, std::initializer_list<Sampler> il, ShaderStageFlags stageFlags)
{
	VERUS_QREF_RENDERER_D3D12;
	HRESULT hr = 0;
	VERUS_RT_ASSERT(_vDescriptorSetDesc.size() == setNumber);
	VERUS_RT_ASSERT(!(reinterpret_cast<intptr_t>(pSrc) & 0xF));

	DescriptorSetDesc dsd;
	dsd._vSamplers.assign(il);
	dsd._pSrc = pSrc;
	dsd._size = size;
	dsd._sizeAligned = Math::AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	dsd._capacity = capacity;
	dsd._capacityInBytes = dsd._sizeAligned*dsd._capacity;
	dsd._stageFlags = stageFlags;

	if (capacity > 0)
	{
		const UINT64 bufferSize = dsd._capacityInBytes * BaseRenderer::s_ringBufferSize;
		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		if (FAILED(hr = pRendererD3D12->GetMaAllocator()->CreateResource(
			&allocDesc,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			&dsd._pMaAllocation,
			IID_PPV_ARGS(&dsd._pConstantBuffer))))
			throw VERUS_RUNTIME_ERROR << "CreateResource(D3D12_HEAP_TYPE_UPLOAD), hr=" << VERUS_HR(hr);

		const int num = dsd._capacity*BaseRenderer::s_ringBufferSize;
		dsd._dhDynamicOffsets.Create(pRendererD3D12->GetD3DDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num);
		VERUS_FOR(i, num)
		{
			const int offset = i * dsd._sizeAligned;
			auto handle = dsd._dhDynamicOffsets.AtCPU(i);
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
			desc.BufferLocation = dsd._pConstantBuffer->GetGPUVirtualAddress() + offset;
			desc.SizeInBytes = dsd._sizeAligned;
			pRendererD3D12->GetD3DDevice()->CreateConstantBufferView(&desc, handle);
		}
	}

	_vDescriptorSetDesc.push_back(dsd);
}

void ShaderD3D12::CreatePipelineLayout()
{
	VERUS_QREF_RENDERER_D3D12;
	HRESULT hr = 0;

	int numAllTextures = 0;
	for (const auto& dsd : _vDescriptorSetDesc)
		numAllTextures += Utils::Cast32(dsd._vSamplers.size());

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureSupportData = {};
	featureSupportData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(pRendererD3D12->GetD3DDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureSupportData, sizeof(featureSupportData))))
		featureSupportData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = _compute ?
		D3D12_ROOT_SIGNATURE_FLAG_NONE : D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Vector<CD3DX12_DESCRIPTOR_RANGE1> vDescRanges;
	vDescRanges.reserve(_vDescriptorSetDesc.size() + numAllTextures); // Must not reallocate.
	Vector<CD3DX12_DESCRIPTOR_RANGE1> vDescRangesSamplers;
	vDescRangesSamplers.reserve(numAllTextures);
	Vector<CD3DX12_ROOT_PARAMETER1> vRootParams;
	vRootParams.reserve(_vDescriptorSetDesc.size() + 1);
	Vector<D3D12_STATIC_SAMPLER_DESC> vStaticSamplers;
	vStaticSamplers.reserve(numAllTextures);
	if (!_vDescriptorSetDesc.empty())
	{
		ShaderStageFlags stageFlags = _vDescriptorSetDesc.front()._stageFlags;
		// Reverse order:
		const int num = Utils::Cast32(_vDescriptorSetDesc.size());
		int space = num - 1;
		for (int i = num - 1; i >= 0; --i)
		{
			auto& dsd = _vDescriptorSetDesc[i];
			stageFlags |= dsd._stageFlags;
			if (dsd._capacity > 0)
			{
				CD3DX12_DESCRIPTOR_RANGE1 descRange;
				descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, space);
				vDescRanges.push_back(descRange);
				const auto pDescriptorRanges = &vDescRanges.back();
				const int numTextures = Utils::Cast32(dsd._vSamplers.size());
				VERUS_FOR(i, numTextures)
				{
					if (Sampler::storage == dsd._vSamplers[i])
					{
						CD3DX12_DESCRIPTOR_RANGE1 descRange;
						descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, i + 1, space);
						vDescRanges.push_back(descRange);
					}
					else
					{
						CD3DX12_DESCRIPTOR_RANGE1 descRange;
						descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i + 1, space);
						vDescRanges.push_back(descRange);
						if (Sampler::custom == dsd._vSamplers[i])
						{
							dsd._staticSamplersOnly = false;
							CD3DX12_DESCRIPTOR_RANGE1 descRangeSampler;
							descRangeSampler.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, i + 1, space);
							vDescRangesSamplers.push_back(descRangeSampler);
						}
						else
						{
							Sampler s = dsd._vSamplers[i];
							if (Sampler::input == s)
								s = Sampler::nearest2D;
							D3D12_STATIC_SAMPLER_DESC samplerDesc = pRendererD3D12->GetStaticSamplerDesc(s);
							samplerDesc.ShaderRegister = i + 1;
							samplerDesc.RegisterSpace = space;
							samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
							vStaticSamplers.push_back(samplerDesc);
						}
					}
				}
				CD3DX12_ROOT_PARAMETER1 rootParam;
				rootParam.InitAsDescriptorTable(1 + numTextures, pDescriptorRanges);
				vRootParams.push_back(rootParam);
			}
			else
			{
				VERUS_RT_ASSERT(&dsd == &_vDescriptorSetDesc.back()); // Only the last set is allowed to use push constants.
				CD3DX12_ROOT_PARAMETER1 rootParam;
				rootParam.InitAsConstants(dsd._size >> 2, 0, space);
				vRootParams.push_back(rootParam);
			}
			space--;
		}

		// Samplers:
		if (!vDescRangesSamplers.empty())
		{
			CD3DX12_ROOT_PARAMETER1 rootParam;
			rootParam.InitAsDescriptorTable(Utils::Cast32(vDescRangesSamplers.size()), vDescRangesSamplers.data());
			vRootParams.push_back(rootParam);
		}

		if (!(stageFlags & ShaderStageFlags::vs))
			rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
		if (!(stageFlags & ShaderStageFlags::hs))
			rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		if (!(stageFlags & ShaderStageFlags::ds))
			rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
		if (!(stageFlags & ShaderStageFlags::gs))
			rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		if (!(stageFlags & ShaderStageFlags::fs))
			rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	}

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(
		Utils::Cast32(vRootParams.size()), vRootParams.data(),
		Utils::Cast32(vStaticSamplers.size()), vStaticSamplers.data(),
		rootSignatureFlags);

	ComPtr<ID3DBlob> pRootSignatureBlob;
	ComPtr<ID3DBlob> pErrorBlob;
	if (FAILED(hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureSupportData.HighestVersion, &pRootSignatureBlob, &pErrorBlob)))
	{
		CSZ errors = pErrorBlob ? static_cast<CSZ>(pErrorBlob->GetBufferPointer()) : "";
		throw VERUS_RUNTIME_ERROR << "D3DX12SerializeVersionedRootSignature(), hr=" << VERUS_HR(hr) << ", errors=" << errors;
	}
	if (FAILED(hr = pRendererD3D12->GetD3DDevice()->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_pRootSignature))))
		throw VERUS_RUNTIME_ERROR << "CreateRootSignature(), hr=" << VERUS_HR(hr);
}

int ShaderD3D12::BindDescriptorSetTextures(int setNumber, std::initializer_list<TexturePtr> il, const int* pMips)
{
	VERUS_QREF_RENDERER_D3D12;

	int complexSetID = -1;
	VERUS_FOR(i, _vComplexSets.size())
	{
		if (_vComplexSets[i]._vTextures.empty())
		{
			complexSetID = i;
			break;
		}
	}
	if (-1 == complexSetID)
	{
		complexSetID = Utils::Cast32(_vComplexSets.size());
		_vComplexSets.resize(complexSetID + 1);
	}

	auto& dsd = _vDescriptorSetDesc[setNumber];
	VERUS_RT_ASSERT(dsd._vSamplers.size() == il.size());

	RComplexSet complexSet = _vComplexSets[complexSetID];
	complexSet._vTextures.reserve(il.size());
	complexSet._dhSrvUav.Create(pRendererD3D12->GetD3DDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Utils::Cast32(il.size()));
	if (!dsd._staticSamplersOnly)
		complexSet._dhSamplers.Create(pRendererD3D12->GetD3DDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, Utils::Cast32(il.size()));
	int index = 0;
	for (auto x : il)
	{
		complexSet._vTextures.push_back(x);
		const int mip = pMips ? pMips[index] : 0;
		auto& texD3D12 = static_cast<RTextureD3D12>(*x);
		if (Sampler::storage == dsd._vSamplers[index])
		{
			pRendererD3D12->GetD3DDevice()->CopyDescriptorsSimple(1,
				complexSet._dhSrvUav.AtCPU(index),
				texD3D12.GetDescriptorHeapUAV().AtCPU(mip),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else
		{
			pRendererD3D12->GetD3DDevice()->CopyDescriptorsSimple(1,
				complexSet._dhSrvUav.AtCPU(index),
				texD3D12.GetDescriptorHeapSRV().AtCPU(mip),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			if (Sampler::custom == dsd._vSamplers[index])
			{
				pRendererD3D12->GetD3DDevice()->CopyDescriptorsSimple(1,
					complexSet._dhSamplers.AtCPU(index),
					texD3D12.GetDescriptorHeapSampler().AtCPU(0),
					D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			}
		}
		index++;
	}

	return complexSetID;
}

void ShaderD3D12::BeginBindDescriptors()
{
	VERUS_QREF_RENDERER;
	VERUS_QREF_RENDERER_D3D12;
	HRESULT hr = 0;

	const bool resetOffset = _currentFrame != renderer.GetNumFrames();
	_currentFrame = renderer.GetNumFrames();

	for (auto& dsd : _vDescriptorSetDesc)
	{
		if (!dsd._capacity)
			continue;
		VERUS_RT_ASSERT(!dsd._pMappedData);
		CD3DX12_RANGE readRange(0, 0);
		void* pData = nullptr;
		if (FAILED(hr = dsd._pConstantBuffer->Map(0, &readRange, &pData)))
			throw VERUS_RUNTIME_ERROR << "Map(), hr=" << VERUS_HR(hr);
		dsd._pMappedData = static_cast<BYTE*>(pData);
		dsd._pMappedData += dsd._capacityInBytes*pRendererD3D12->GetRingBufferIndex(); // Adjust address for this frame.
		if (resetOffset)
		{
			dsd._offset = 0;
			dsd._index = 0;
		}
	}
}

void ShaderD3D12::EndBindDescriptors()
{
	for (auto& dsd : _vDescriptorSetDesc)
	{
		if (!dsd._capacity)
			continue;
		VERUS_RT_ASSERT(dsd._pMappedData);
		dsd._pConstantBuffer->Unmap(0, nullptr);
		dsd._pMappedData = nullptr;
	}
}

UINT ShaderD3D12::ToRootParameterIndex(int setNumber) const
{
	return static_cast<UINT>(_vDescriptorSetDesc.size()) - setNumber - 1;
}

bool ShaderD3D12::TryRootConstants(int setNumber, RBaseCommandBuffer cb)
{
	auto& dsd = _vDescriptorSetDesc[setNumber];
	if (!dsd._capacity)
	{
		auto& cbD3D12 = static_cast<RCommandBufferD3D12>(cb);
		const UINT rootParameterIndex = ToRootParameterIndex(setNumber);
		if (_compute)
			cbD3D12.GetD3DGraphicsCommandList()->SetComputeRoot32BitConstants(rootParameterIndex, dsd._size >> 2, dsd._pSrc, 0);
		else
			cbD3D12.GetD3DGraphicsCommandList()->SetGraphicsRoot32BitConstants(rootParameterIndex, dsd._size >> 2, dsd._pSrc, 0);
		return true;
	}
	return false;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShaderD3D12::UpdateUniformBuffer(int setNumber, int complexSetID, bool copyDescOnly)
{
	VERUS_QREF_RENDERER_D3D12;

	auto& dsd = _vDescriptorSetDesc[setNumber];
	int at = 0;
	if (copyDescOnly)
		at = dsd._capacity * pRendererD3D12->GetRingBufferIndex();
	else
	{
		if (dsd._offset + dsd._sizeAligned > dsd._capacityInBytes)
			return CD3DX12_GPU_DESCRIPTOR_HANDLE();

		VERUS_RT_ASSERT(dsd._pMappedData);
		at = dsd._capacity * pRendererD3D12->GetRingBufferIndex() + dsd._index;
		memcpy(dsd._pMappedData + dsd._offset, dsd._pSrc, dsd._size);
		dsd._offset += dsd._sizeAligned;
		dsd._index++;
	}

	auto hpBase = pRendererD3D12->GetHeapCbvSrvUav().GetNextHandlePair();

	// Copy CBV:
	pRendererD3D12->GetD3DDevice()->CopyDescriptorsSimple(1,
		hpBase._hCPU,
		dsd._dhDynamicOffsets.AtCPU(at),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	if (complexSetID >= 0)
	{
		// Copy SRVs:
		const auto& complexSet = _vComplexSets[complexSetID];
		const int num = Utils::Cast32(complexSet._vTextures.size());
		pRendererD3D12->GetD3DDevice()->CopyDescriptorsSimple(num,
			pRendererD3D12->GetHeapCbvSrvUav().GetNextHandlePair(num)._hCPU,
			complexSet._dhSrvUav.AtCPU(0),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	return hpBase._hGPU;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShaderD3D12::UpdateSamplers(int setNumber, int complexSetID)
{
	VERUS_QREF_RENDERER_D3D12;
	VERUS_RT_ASSERT(complexSetID >= 0);

	auto& dsd = _vDescriptorSetDesc[setNumber];
	if (dsd._staticSamplersOnly)
		return CD3DX12_GPU_DESCRIPTOR_HANDLE();

	const auto& complexSet = _vComplexSets[complexSetID];
	const int num = Utils::Cast32(complexSet._vTextures.size());
	auto hpSampler = pRendererD3D12->GetHeapSampler().GetNextHandlePair(num);
	pRendererD3D12->GetD3DDevice()->CopyDescriptorsSimple(num,
		hpSampler._hCPU,
		complexSet._dhSamplers.AtCPU(0),
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	return hpSampler._hGPU;
}

void ShaderD3D12::OnError(CSZ s)
{
	VERUS_QREF_RENDERER;
	if (strstr(s, "HS': entrypoint not found"))
		return;
	if (strstr(s, "DS': entrypoint not found"))
		return;
	if (strstr(s, "GS': entrypoint not found"))
		return;
	if (strstr(s, "error X"))
		renderer.OnShaderError(s);
	else
		renderer.OnShaderWarning(s);
}
