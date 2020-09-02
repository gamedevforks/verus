#include "verus.h"

using namespace verus;
using namespace verus::Scene;

CGI::ShaderPwn                      Mesh::s_shader;
CGI::PipelinePwns<Mesh::PIPE_COUNT> Mesh::s_pipe;

Mesh::UB_PerFrame      Mesh::s_ubPerFrame;
Mesh::UB_PerMaterialFS Mesh::s_ubPerMaterialFS;
Mesh::UB_PerMeshVS     Mesh::s_ubPerMeshVS;
Mesh::UB_SkeletonVS    Mesh::s_ubSkeletonVS;
Mesh::UB_PerObject     Mesh::s_ubPerObject;

// Mesh:

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
	Done();
}

void Mesh::InitStatic()
{
	VERUS_QREF_CONST_SETTINGS;

	s_shader.Init("[Shaders]:DS_Mesh.hlsl");
	s_shader->CreateDescriptorSet(0, &s_ubPerFrame, sizeof(s_ubPerFrame), settings.GetLimits()._mesh_ubPerFrameCapacity, {}, CGI::ShaderStageFlags::vs_hs_ds_fs);
	s_shader->CreateDescriptorSet(1, &s_ubPerMaterialFS, sizeof(s_ubPerMaterialFS), settings.GetLimits()._mesh_ubPerMaterialFSCapacity,
		{
			CGI::Sampler::aniso,
			CGI::Sampler::aniso,
			CGI::Sampler::aniso,
			CGI::Sampler::custom
		}, CGI::ShaderStageFlags::fs);
	s_shader->CreateDescriptorSet(2, &s_ubPerMeshVS, sizeof(s_ubPerMeshVS), settings.GetLimits()._mesh_ubPerMeshVSCapacity, {}, CGI::ShaderStageFlags::vs);
	s_shader->CreateDescriptorSet(3, &s_ubSkeletonVS, sizeof(s_ubSkeletonVS), settings.GetLimits()._mesh_ubSkinningVSCapacity, {}, CGI::ShaderStageFlags::vs);
	s_shader->CreateDescriptorSet(4, &s_ubPerObject, sizeof(s_ubPerObject), 0);
	s_shader->CreatePipelineLayout();
}

void Mesh::DoneStatic()
{
	s_pipe.Done();
	s_shader.Done();
}

void Mesh::Init(RcDesc desc)
{
	VERUS_RT_ASSERT(desc._url);

	if (desc._warpUrl)
	{
		_warpUrl = desc._warpUrl;
		Str::ReplaceExtension(_warpUrl, ".xwx");
	}

	_instanceCapacity = desc._instanceCapacity;
	_initShape = desc._initShape;

	BaseMesh::Init(desc._url);
}

void Mesh::Done()
{
	VERUS_DONE(Mesh);
}

void Mesh::Draw(RcDrawDesc dd, CGI::CommandBufferPtr cb)
{
	BindPipeline(cb, dd._allowTess);
	BindGeo(cb);

	UpdateUniformBufferPerFrame();
	cb->BindDescriptors(GetShader(), 0);
	// Material buffer should already be updated. For example call Material::UpdateMeshUniformBuffer.
	cb->BindDescriptors(GetShader(), 1, dd._cshMaterial);
	UpdateUniformBufferPerMeshVS();
	cb->BindDescriptors(GetShader(), 2);
	UpdateUniformBufferSkeletonVS();
	cb->BindDescriptors(GetShader(), 3);
	UpdateUniformBufferPerObject(dd._matW);
	cb->BindDescriptors(GetShader(), 4);

	cb->DrawIndexed(GetIndexCount());
}

void Mesh::BindPipeline(PIPE pipe, CGI::CommandBufferPtr cb)
{
	VERUS_QREF_ATMO;
	VERUS_QREF_CONST_SETTINGS;
	VERUS_QREF_RENDERER;

	if (!settings._gpuTessellation) // Fallback:
	{
		switch (pipe)
		{
		case PIPE_TESS:           pipe = PIPE_MAIN; break;
		case PIPE_TESS_INSTANCED: pipe = PIPE_INSTANCED; break;
		case PIPE_TESS_PLANT:     pipe = PIPE_PLANT; break;
		case PIPE_TESS_ROBOTIC:   pipe = PIPE_ROBOTIC; break;
		case PIPE_TESS_SKINNED:   pipe = PIPE_SKINNED; break;

		case PIPE_DEPTH_TESS:           pipe = PIPE_DEPTH; break;
		case PIPE_DEPTH_TESS_INSTANCED: pipe = PIPE_DEPTH_INSTANCED; break;
		case PIPE_DEPTH_TESS_PLANT:     pipe = PIPE_DEPTH_PLANT; break;
		case PIPE_DEPTH_TESS_ROBOTIC:   pipe = PIPE_DEPTH_ROBOTIC; break;
		case PIPE_DEPTH_TESS_SKINNED:   pipe = PIPE_DEPTH_SKINNED; break;
		};
	}

	if (!s_pipe[pipe])
	{
		static CSZ branches[] =
		{
			"#",
			"#Instanced",
			"#Plant",
			"#Robotic",
			"#Skinned",

			"#Tess",
			"#TessInstanced",
			"#TessPlant",
			"#TessRobotic",
			"#TessSkinned",

			"#Depth",
			"#DepthInstanced",
			"#DepthPlant",
			"#DepthRobotic",
			"#DepthSkinned",

			"#DepthTess",
			"#DepthTessInstanced",
			"#DepthTessPlant",
			"#DepthTessRobotic",
			"#DepthTessSkinned",

			"#SolidColor",
			"#SolidColorInstanced",
			"#SolidColorRobotic",
			"#SolidColorSkinned"
		};

		CGI::PipelineDesc pipeDesc(_geo, s_shader, branches[pipe], renderer.GetDS().GetRenderPassHandle());

		auto SetBlendEqsForDS = [&pipeDesc]()
		{
			pipeDesc._colorAttachBlendEqs[0] = VERUS_COLOR_BLEND_OFF;
			pipeDesc._colorAttachBlendEqs[1] = VERUS_COLOR_BLEND_OFF;
			pipeDesc._colorAttachBlendEqs[2] = VERUS_COLOR_BLEND_OFF;
		};

		switch (pipe)
		{
		case PIPE_MAIN:
		case PIPE_INSTANCED:
		case PIPE_PLANT:
		case PIPE_ROBOTIC:
		case PIPE_SKINNED:
		{
			SetBlendEqsForDS();
		}
		break;
		case PIPE_TESS:
		case PIPE_TESS_INSTANCED:
		case PIPE_TESS_PLANT:
		case PIPE_TESS_ROBOTIC:
		case PIPE_TESS_SKINNED:
		{
			SetBlendEqsForDS();
			if (settings._gpuTessellation)
				pipeDesc._topology = CGI::PrimitiveTopology::patchList3;
		}
		break;
		case PIPE_DEPTH:
		case PIPE_DEPTH_INSTANCED:
		case PIPE_DEPTH_PLANT:
		case PIPE_DEPTH_ROBOTIC:
		case PIPE_DEPTH_SKINNED:
		{
			pipeDesc._colorAttachBlendEqs[0] = "";
			pipeDesc._renderPassHandle = atmo.GetShadowMap().GetRenderPassHandle();
		}
		break;
		case PIPE_DEPTH_TESS:
		case PIPE_DEPTH_TESS_INSTANCED:
		case PIPE_DEPTH_TESS_PLANT:
		case PIPE_DEPTH_TESS_ROBOTIC:
		case PIPE_DEPTH_TESS_SKINNED:
		{
			pipeDesc._colorAttachBlendEqs[0] = "";
			pipeDesc._renderPassHandle = atmo.GetShadowMap().GetRenderPassHandle();
			if (settings._gpuTessellation)
				pipeDesc._topology = CGI::PrimitiveTopology::patchList3;
		}
		break;
		case PIPE_WIREFRAME:
		case PIPE_WIREFRAME_INSTANCED:
		case PIPE_WIREFRAME_ROBOTIC:
		case PIPE_WIREFRAME_SKINNED:
		{
			SetBlendEqsForDS();
			pipeDesc._rasterizationState._polygonMode = CGI::PolygonMode::line;
			pipeDesc._rasterizationState._depthBiasEnable = true;
			pipeDesc._rasterizationState._depthBiasConstantFactor = -1000;
			pipeDesc._depthCompareOp = CGI::CompareOp::lessOrEqual;
		}
		break;
		default:
		{
			SetBlendEqsForDS();
		}
		}
		pipeDesc._vertexInputBindingsFilter = _bindingsMask;
		s_pipe[pipe].Init(pipeDesc);
	}
	cb->BindPipeline(s_pipe[pipe]);
}

void Mesh::BindPipeline(CGI::CommandBufferPtr cb, bool allowTess)
{
	VERUS_QREF_ATMO;
	if (_skeleton.IsInitialized())
	{
		if (atmo.GetShadowMap().IsRendering())
		{
			if (allowTess)
				BindPipeline(PIPE_DEPTH_TESS_SKINNED, cb);
			else
				BindPipeline(PIPE_DEPTH_SKINNED, cb);
		}
		else
		{
			if (allowTess)
				BindPipeline(PIPE_TESS_SKINNED, cb);
			else
				BindPipeline(PIPE_SKINNED, cb);
		}
	}
	else
	{
		if (atmo.GetShadowMap().IsRendering())
		{
			if (allowTess)
				BindPipeline(PIPE_DEPTH_TESS, cb);
			else
				BindPipeline(PIPE_DEPTH, cb);
		}
		else
		{
			if (allowTess)
				BindPipeline(PIPE_TESS, cb);
			else
				BindPipeline(PIPE_MAIN, cb);
		}
	}
}

void Mesh::BindPipelineInstanced(CGI::CommandBufferPtr cb, bool allowTess, bool plant)
{
	VERUS_QREF_ATMO;
	if (atmo.GetShadowMap().IsRendering())
	{
		if (allowTess)
			BindPipeline(plant ? PIPE_DEPTH_TESS_PLANT : PIPE_DEPTH_TESS_INSTANCED, cb);
		else
			BindPipeline(plant ? PIPE_DEPTH_PLANT : PIPE_DEPTH_INSTANCED, cb);
	}
	else
	{
		if (allowTess)
			BindPipeline(plant ? PIPE_TESS_PLANT : PIPE_TESS_INSTANCED, cb);
		else
			BindPipeline(plant ? PIPE_PLANT : PIPE_INSTANCED, cb);
	}
}

void Mesh::BindGeo(CGI::CommandBufferPtr cb)
{
	BindGeo(cb, _bindingsMask);
}

void Mesh::BindGeo(CGI::CommandBufferPtr cb, UINT32 bindingsFilter)
{
	cb->BindVertexBuffers(_geo, bindingsFilter);
	cb->BindIndexBuffer(_geo);
}

void Mesh::UpdateUniformBufferPerFrame(float invTessDist)
{
	VERUS_QREF_RENDERER;
	VERUS_QREF_SM;

	RcPoint3 eyePos = sm.GetMainCamera()->GetEyePosition();
	Point3 eyePosWV = sm.GetCamera()->GetMatrixV() * eyePos;

	s_ubPerFrame._matV = sm.GetCamera()->GetMatrixV().UniformBufferFormat();
	s_ubPerFrame._matVP = sm.GetCamera()->GetMatrixVP().UniformBufferFormat();
	s_ubPerFrame._matP = sm.GetCamera()->GetMatrixP().UniformBufferFormat();
	s_ubPerFrame._viewportSize = renderer.GetCommandBuffer()->GetViewportSize().GLM();
	s_ubPerFrame._eyePosWV_invTessDistSq = float4(eyePosWV.GLM(), invTessDist * invTessDist);
}

void Mesh::UpdateUniformBufferPerMeshVS()
{
	memcpy(&s_ubPerMeshVS._posDeqScale, _posDeq + 0, 12);
	memcpy(&s_ubPerMeshVS._posDeqBias, _posDeq + 3, 12);
	memcpy(&s_ubPerMeshVS._tc0DeqScaleBias, _tc0Deq, 16);
	memcpy(&s_ubPerMeshVS._tc1DeqScaleBias, _tc1Deq, 16);
}

void Mesh::UpdateUniformBufferSkeletonVS()
{
	VERUS_ZERO_MEM(s_ubSkeletonVS._vWarpZones);
	s_ubSkeletonVS._vWarpZones[1].w = FLT_MAX;
	if (_warp.IsInitialized())
	{
		Vector4 warpZones[32];
		VERUS_ZERO_MEM(warpZones);
		_warp.Fill(warpZones);
		memcpy(&s_ubSkeletonVS._vWarpZones, warpZones, sizeof(warpZones));
	}
	_skeleton.UpdateUniformBufferArray(s_ubSkeletonVS._vMatBones);
}

void Mesh::UpdateUniformBufferPerObject(RcTransform3 tr, RcVector4 color)
{
	s_ubPerObject._matW = tr.UniformBufferFormat();
	s_ubPerObject._userColor = color.GLM();
}

void Mesh::UpdateUniformBufferPerObject(Point3 pos, RcVector4 color)
{
	const Transform3 matT = Transform3::translation(Vector3(pos));
	s_ubPerObject._matW = matT.UniformBufferFormat();
	s_ubPerObject._userColor = color.GLM();
}

void Mesh::CreateDeviceBuffers()
{
	_bindingsMask = 0;

	CGI::GeometryDesc geoDesc;
	geoDesc._name = _C(_url);
	const CGI::VertexInputAttrDesc viaDesc[] =
	{
		{0, offsetof(VertexInputBinding0, _pos), CGI::ViaType::shorts, 4, CGI::ViaUsage::position, 0},
		{0, offsetof(VertexInputBinding0, _tc0), CGI::ViaType::shorts, 2, CGI::ViaUsage::texCoord, 0},
		{0, offsetof(VertexInputBinding0, _nrm), CGI::ViaType::ubytes, 4, CGI::ViaUsage::normal, 0},
		{1, offsetof(VertexInputBinding1, _bw),  CGI::ViaType::shorts, 4, CGI::ViaUsage::blendWeights, 0},
		{1, offsetof(VertexInputBinding1, _bi),  CGI::ViaType::shorts, 4, CGI::ViaUsage::blendIndices, 0},
		{2, offsetof(VertexInputBinding2, _tan), CGI::ViaType::shorts, 4, CGI::ViaUsage::tangent, 0},
		{2, offsetof(VertexInputBinding2, _bin), CGI::ViaType::shorts, 4, CGI::ViaUsage::binormal, 0},
		{3, offsetof(VertexInputBinding3, _tc1), CGI::ViaType::shorts, 2, CGI::ViaUsage::texCoord, 1},
		{3, offsetof(VertexInputBinding3, _clr), CGI::ViaType::ubytes, 4, CGI::ViaUsage::color, 0},

		{-4, offsetof(PerInstanceData, _matPart0), CGI::ViaType::floats, 4, CGI::ViaUsage::instData, 0},
		{-4, offsetof(PerInstanceData, _matPart1), CGI::ViaType::floats, 4, CGI::ViaUsage::instData, 1},
		{-4, offsetof(PerInstanceData, _matPart2), CGI::ViaType::floats, 4, CGI::ViaUsage::instData, 2},
		{-4, offsetof(PerInstanceData, _instData), CGI::ViaType::floats, 4, CGI::ViaUsage::instData, 3},
		CGI::VertexInputAttrDesc::End()
	};
	geoDesc._pVertexInputAttrDesc = viaDesc;
	const int strides[] = { sizeof(VertexInputBinding0), sizeof(VertexInputBinding1), sizeof(VertexInputBinding2), sizeof(VertexInputBinding3), sizeof(PerInstanceData), 0 };
	geoDesc._pStrides = strides;
	geoDesc._32BitIndices = _vIndices.empty();
	_geo.Init(geoDesc);

	// Vertex buffer, binding 0:
	_bindingsMask |= (1 << 0);
	_geo->CreateVertexBuffer(Utils::Cast32(_vBinding0.size()), 0);
	_geo->UpdateVertexBuffer(_vBinding0.data(), 0);

	// Vertex buffer, binding 1 (skinning):
	if (!_vBinding1.empty())
	{
		_bindingsMask |= (1 << 1);
		_geo->CreateVertexBuffer(Utils::Cast32(_vBinding1.size()), 1);
		_geo->UpdateVertexBuffer(_vBinding1.data(), 1);
	}

	// Vertex buffer, binding 2 (tangent space):
	if (!_vBinding2.empty())
	{
		_bindingsMask |= (1 << 2);
		_geo->CreateVertexBuffer(Utils::Cast32(_vBinding2.size()), 2);
		_geo->UpdateVertexBuffer(_vBinding2.data(), 2);
	}

	// Vertex buffer, binding 3 (lightmap):
	if (!_vBinding3.empty())
	{
		_bindingsMask |= (1 << 3);
		_geo->CreateVertexBuffer(Utils::Cast32(_vBinding3.size()), 3);
		_geo->UpdateVertexBuffer(_vBinding3.data(), 3);
	}

	// Index buffer:
	if (!_vIndices.empty())
	{
		_geo->CreateIndexBuffer(Utils::Cast32(_vIndices.size()));
		_geo->UpdateIndexBuffer(_vIndices.data());
	}
	else if (!_vIndices32.empty())
	{
		_geo->CreateIndexBuffer(Utils::Cast32(_vIndices32.size()));
		_geo->UpdateIndexBuffer(_vIndices32.data());
	}

	// Instance buffer:
	if (_instanceCapacity > 0)
	{
		_vInstanceBuffer.resize(_instanceCapacity);
		_bindingsMask |= (1 << 4);
		_geo->CreateVertexBuffer(Utils::Cast32(_vInstanceBuffer.size()), 4);
	}
}

void Mesh::UpdateVertexBuffer(const void* p, int binding)
{
	_geo->UpdateVertexBuffer(p, binding);
}

void Mesh::ResetInstanceCount()
{
	_instanceCount = 0;
	_firstInstance = 0;
}

void Mesh::PushInstance(RcTransform3 matW, RcVector4 instData)
{
	VERUS_RT_ASSERT(!_vInstanceBuffer.empty());
	if (!_vertCount)
		return;
	if (IsInstanceBufferFull())
		return;
	matW.InstFormat(&_vInstanceBuffer[_instanceCount]._matPart0);
	_vInstanceBuffer[_instanceCount]._instData = instData;
	_instanceCount++;
}

bool Mesh::IsInstanceBufferFull()
{
	VERUS_RT_ASSERT(!_vInstanceBuffer.empty());
	if (!_vertCount)
		return false;
	return _instanceCount >= _instanceCapacity;
}

bool Mesh::IsInstanceBufferEmpty(bool fromFirstInstance)
{
	VERUS_RT_ASSERT(!_vInstanceBuffer.empty());
	if (!_vertCount)
		return true;
	return GetInstanceCount(fromFirstInstance) <= 0;
}

void Mesh::UpdateInstanceBuffer()
{
	VERUS_RT_ASSERT(!_vInstanceBuffer.empty());
	_geo->UpdateVertexBuffer(&_vInstanceBuffer[_firstInstance], 4, nullptr, GetInstanceCount(true), _firstInstance);
}
