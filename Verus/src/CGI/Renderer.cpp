#include "verus.h"

using namespace verus;
using namespace verus::CGI;

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
	Done();
}

PBaseRenderer Renderer::operator->()
{
	VERUS_RT_ASSERT(_pBaseRenderer);
	return _pBaseRenderer;
}

bool Renderer::IsLoaded()
{
	return IsValidSingleton() && !!I()._pBaseRenderer;
}

void Renderer::Init(PRendererDelegate pDelegate)
{
	VERUS_INIT();
	VERUS_QREF_SETTINGS;

	_pRendererDelegate = pDelegate;

	CSZ dll = "RendererVulkan.dll";
	switch (settings._gapi)
	{
	case 12:
	{
		dll = "RendererDirect3D12.dll";
		VERUS_LOG_INFO("Using Direct3D 12");
	}
	break;
	default:
		VERUS_LOG_INFO("Using Vulkan");
	}
	BaseRendererDesc desc;
	_pBaseRenderer = BaseRenderer::Load(dll, desc);

	_gapi = _pBaseRenderer->GetGapi();

	_commandBuffer.Init();

	_rpSwapChain = _pBaseRenderer->CreateRenderPass(
		{ RP::Attachment("Color", Format::unormB8G8R8A8).LoadOpClear().FinalLayout(ImageLayout::presentSrc) },
		{ RP::Subpass("Sp0").Color({RP::Ref("Color", ImageLayout::colorAttachmentOptimal)}) },
		{});
	_rpDS = _pBaseRenderer->CreateRenderPass(
		{
			RP::Attachment("GBuffer0", Format::unormR8G8B8A8).LoadOpClear().FinalLayout(ImageLayout::presentSrc),
			RP::Attachment("GBuffer1", Format::unormR8G8B8A8).LoadOpClear().FinalLayout(ImageLayout::presentSrc),
			RP::Attachment("GBuffer2", Format::unormR8G8B8A8).LoadOpClear().FinalLayout(ImageLayout::presentSrc),
			RP::Attachment("GBuffer3", Format::unormR8G8B8A8).LoadOpClear().FinalLayout(ImageLayout::presentSrc),
			RP::Attachment("Depth", Format::unormD24uintS8).LoadOpClear().FinalLayout(ImageLayout::presentSrc),
		},
		{
			RP::Subpass("Sp0").Color(
				{
					RP::Ref("GBuffer0", ImageLayout::colorAttachmentOptimal),
					RP::Ref("GBuffer1", ImageLayout::colorAttachmentOptimal),
					RP::Ref("GBuffer2", ImageLayout::colorAttachmentOptimal),
					RP::Ref("GBuffer3", ImageLayout::colorAttachmentOptimal)
				}).DepthStencil(RP::Ref("Depth", ImageLayout::depthStencilAttachmentOptimal))
		},
		{
		});

	_fbSwapChain.resize(_pBaseRenderer->GetNumSwapChainBuffers());
	VERUS_FOR(i, _fbSwapChain.size())
		_fbSwapChain[i] = _pBaseRenderer->CreateFramebuffer(_rpSwapChain, {}, settings._screenSizeWidth, settings._screenSizeHeight, i);
}

void Renderer::Done()
{
	if (_pBaseRenderer)
	{
		_pBaseRenderer->WaitIdle();
		_commandBuffer.Done();
		_pBaseRenderer->ReleaseMe();
		_pBaseRenderer = nullptr;
	}

	VERUS_SMART_DELETE(_pRendererDelegate);

	VERUS_DONE(Renderer);
}

void Renderer::Draw()
{
	if (_pRendererDelegate)
		_pRendererDelegate->Renderer_OnDraw();
}

void Renderer::Present()
{
	if (_pRendererDelegate)
		_pRendererDelegate->Renderer_OnPresent();
	_numFrames++;

	VERUS_QREF_TIMER;
	_fps = _fps * 0.75f + timer.GetDeltaTimeInv()*0.25f;
}

void Renderer::OnShaderError(CSZ s)
{
	throw VERUS_RUNTIME_ERROR << "Shader Error:\n" << s;
}

void Renderer::OnShaderWarning(CSZ s)
{
	VERUS_LOG_WARN("Shader Warning:\n" << s);
}

float Renderer::GetWindowAspectRatio() const
{
	VERUS_QREF_SETTINGS;
	return static_cast<float>(settings._screenSizeWidth) / static_cast<float>(settings._screenSizeHeight);
}
