#include "RenderGraph.h"
#include <Logging/Logging.h>
#include <RenderUtils/GPUContext/GPUContext.h>

static bool ResourceReads(RenderGraphResourceAccessType_e AccessType, RenderGraphLoadOp_e LoadOp)
{
	if (rl::HasEnumFlags(AccessType, RenderGraphResourceAccessType_e::SRV | RenderGraphResourceAccessType_e::COPYSRC))
	{
		return true;
	}
	else // (AccessType == RenderGraphResourceAccessType_e::UAV || RenderGraphResourceAccessType_e::RTV || RenderGraphResourceAccessType_e::DSV)
	{
		return LoadOp == RenderGraphLoadOp_e::LOAD;
	}
}

static bool ResourceWrites(RenderGraphResourceAccessType_e AccessType, RenderGraphLoadOp_e LoadOp)
{
	if (rl::HasEnumFlags(AccessType, RenderGraphResourceAccessType_e::SRV | RenderGraphResourceAccessType_e::COPYSRC))
	{
		return false;
	}
	else if (rl::HasEnumFlags(AccessType, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::DSV | RenderGraphResourceAccessType_e::COPYDST))
	{
		return true;
	}
	else
	{
		return false;
	}
}

RenderGraphPass_s& RenderGraphPass_s::AccessResource(RenderGraphResourceHandle_t Resource, RenderGraphResourceAccessType_e AccessType, RenderGraphLoadOp_e LoadOp)
{
	if (rl::HasEnumFlags(AccessType, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::DSV))
	{
		ASSERTMSG(LoadOp == RenderGraphLoadOp_e::CLEAR || LoadOp == RenderGraphLoadOp_e::DONT_CARE || LoadOp == RenderGraphLoadOp_e::LOAD, "Unuspported RTV/DSV load access");
	}

	if (rl::HasEnumFlags(AccessType, RenderGraphResourceAccessType_e::SRV | RenderGraphResourceAccessType_e::COPYSRC))
	{
		ASSERTMSG(LoadOp == RenderGraphLoadOp_e::LOAD, "Only load supported for SRV or copy access");
	}

	if (rl::HasEnumFlags(AccessType, RenderGraphResourceAccessType_e::UAV))
	{
		ASSERTMSG(LoadOp != RenderGraphLoadOp_e::CLEAR, "Clear not yet supported for UAVs"); //TODO
	}

	if (Builder.GetResourceDesc(Resource).Kind == RenderGraphResourceKind_e::RAYTRACING_SCENE)
	{
		ASSERTMSG(AccessType == RenderGraphResourceAccessType_e::UAV || AccessType == RenderGraphResourceAccessType_e::SRV, "Raytracing scenes can only be built (UAV) or traced (SRV)");
	}

	if (!rl::HasEnumFlags(AccessType, RenderGraphResourceAccessType_e::SRV | RenderGraphResourceAccessType_e::COPYSRC) && Builder.IsResourceExternal(Resource))
	{
		WritesToExternal = true;
	}

	rl::ResourceTransitionState DesiredState = rl::ResourceTransitionState::COMMON;
	switch (AccessType)
	{
		case RenderGraphResourceAccessType_e::SRV:
			// Graphics passes can read SRVs from vertex shaders and dispatches as well as pixel shaders
			DesiredState = PassType == RenderGraphPassType_e::GRAPHICS ? rl::ResourceTransitionState::ALL_SHADER_RESOURCE : rl::ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE;
			break;
		case RenderGraphResourceAccessType_e::UAV:
			DesiredState = rl::ResourceTransitionState::UNORDERED_ACCESS;
			break;
		case RenderGraphResourceAccessType_e::RTV:
			DesiredState = rl::ResourceTransitionState::RENDER_TARGET;
			break;
		case RenderGraphResourceAccessType_e::DSV:
			DesiredState = rl::ResourceTransitionState::DEPTH_WRITE; // TODO Handle read cases
			break;
		case RenderGraphResourceAccessType_e::COPYSRC:
			DesiredState = rl::ResourceTransitionState::COPY_SRC;
			break;
		case RenderGraphResourceAccessType_e::COPYDST:
			DesiredState = rl::ResourceTransitionState::COPY_DEST;
			break;
	}

	Resources.emplace_back(Resource, AccessType, LoadOp, DesiredState);
	return *this;
}

RenderGraphPass_s& RenderGraphPass_s::ExtractResource(RenderGraphResourceHandle_t Resource)
{
	WritesToExternal = true;
	Resources.emplace_back(Resource, RenderGraphResourceAccessType_e::UNKNOWN, RenderGraphLoadOp_e::UNKNOWN, rl::ResourceTransitionState::COMMON);
	Resources.back().IsExtracted = true;
	return *this;
}

RenderGraphPass_s& RenderGraphPass_s::SetExecuteCallback(RenderGraphCallback_Func Func)
{
	Callback = Func;
	return *this;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::CreateTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName)
{
	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, ResourceName);
	Resource->Texture.Width = Width;
	Resource->Texture.Height = Height;
	Resource->Texture.Format = Format;
	Resource->Texture.AccessTypes = AccessTypes;
	return Handle;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::CreateTexture(const RenderGraphTextureDesc_s& Desc, const wchar_t* ResourceName)
{
	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, ResourceName);
	Resource->Texture = Desc;
	return Handle;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::CreateTexture3D(uint32_t Width, uint32_t Height, uint32_t Depth, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName)
{
	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, ResourceName);
	Resource->Texture.Width = Width;
	Resource->Texture.Height = Height;
	Resource->Texture.Depth = Depth;
	Resource->Texture.Dimension = rl::TextureDimension::TEX3D;
	Resource->Texture.Format = Format;
	Resource->Texture.AccessTypes = AccessTypes;
	return Handle;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::RefExternalTexture(RenderGraphTexturePtr_t Texture, const wchar_t* ResourceName)
{
	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, ResourceName);
	Resource->ExternalTextureRef = Texture;
	return Handle;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::RefBackBufferTexture(rl::Texture_t Texture, rl::RenderTargetView_t RTV, rl::ResourceTransitionState TransitionState, uint32_t Width, uint32_t Height)
{
	if (!ENSUREMSG(rl::IsValid(Texture) && rl::IsValid(RTV), "RenderGraphBuilder_s::RefBackBufferTexture Texture or RTV are not valid"))
	{
		return RenderGraphResourceHandle_t::NONE;
	}
	
	if(rl::IsValid(Backbuffer.BackBufferTexture) || rl::IsValid(Backbuffer.BackBufferRTV))
	{
		LOGWARNING("RenderGraphBuilder_s::RefBackBufferTexture Back buffer texture or RTV already set, overwriting previous values");
	}
	Backbuffer.BackBufferTexture = Texture;
	Backbuffer.BackBufferRTV = RTV;
	Backbuffer.BackBufferTransitionState = TransitionState;
	Backbuffer.BackbufferWidth = Width;
	Backbuffer.BackbufferHeight = Height;

	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, L"BackbufferTexture");
	Resource->Kind = RenderGraphResourceKind_e::BACKBUFFER;
	return Handle;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::ImportRaytracingScene(rl::RaytracingScene_t Scene, const wchar_t* ResourceName)
{
	if (!ENSUREMSG(rl::IsValid(Scene), "ImportRaytracingScene passed an invalid scene"))
		return RenderGraphResourceHandle_t::NONE;

	// A second handle for the same scene would split its dependency chain
	for (size_t ResourceIndex = 1; ResourceIndex < ResourceDescs.size(); ResourceIndex++)
	{
		if (ResourceDescs[ResourceIndex].Kind == RenderGraphResourceKind_e::RAYTRACING_SCENE && ResourceDescs[ResourceIndex].RaytracingScene == Scene)
		{
			return static_cast<RenderGraphResourceHandle_t>(ResourceIndex);
		}
	}

	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, ResourceName);
	Resource->Kind = RenderGraphResourceKind_e::RAYTRACING_SCENE;
	Resource->RaytracingScene = Scene;
	Resource->IsInjected = true;	// Persists across frames, so it can be traced on frames that don't build it
	return Handle;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::InjectTexture(RenderGraphTexturePtr_t& Texture, const wchar_t* ResourceName)
{
	if(!ENSUREMSG(Texture.get() != nullptr, "InjectTexture passed an invalid texture"))
		return RenderGraphResourceHandle_t::NONE;

	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, ResourceName);
	Resource->ExternalTextureRef = Texture;
	Resource->IsInjected = true;
	
	return Handle;
}

void RenderGraphBuilder_s::QueueTextureExtraction(RenderGraphResourceHandle_t Resource, RenderGraphTexturePtr_t& OutTexture)
{
	CHECK(!ExtractedTextures.contains(Resource));
	ExtractedTextures[Resource] = OutTexture;

	const std::wstring PassName = L"Texture Extraction - " + GetResourceDesc(Resource).ResourceName;
	RenderGraphPass_s& ExtractionPass = AddPass(RenderGraphPassType_e::MISC, PassName.c_str())
	.ExtractResource(Resource)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		RG.ExtractTexture(Resource);
	});
}

void RenderGraphBuilder_s::QueueTextureCopy(RenderGraphResourceHandle_t DstResource, RenderGraphResourceHandle_t SrcResouce)
{
	const std::wstring PassName = L"Texture Copy " + GetResourceDesc(SrcResouce).ResourceName + L" -> " + GetResourceDesc(DstResource).ResourceName;
	RenderGraphPass_s& CopyPass = AddPass(RenderGraphPassType_e::MISC, PassName.c_str())
	.AccessResource(DstResource, RenderGraphResourceAccessType_e::COPYDST, RenderGraphLoadOp_e::DONT_CARE)
	.AccessResource(SrcResouce, RenderGraphResourceAccessType_e::COPYSRC, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		Ctx.CopyTexture(RG.GetResource(DstResource)->Texture->Texture, RG.GetResource(SrcResouce)->Texture->Texture);
	});
}

RenderGraphPass_s& RenderGraphBuilder_s::AddPass(RenderGraphPassType_e PassType, const wchar_t* PassName)
{
	Passes.emplace_back(*this, PassType, PassName);
	return Passes.back();
}

RenderGraphResourceDesc_s& RenderGraphBuilder_s::GetResourceDesc(RenderGraphResourceHandle_t Resource)
{
	const size_t ResourceIndex = static_cast<size_t>(Resource);
	return ResourceDescs[ResourceIndex];
}

bool RenderGraphBuilder_s::IsResourceExternal(RenderGraphResourceHandle_t Resource) const
{
	const size_t ResourceIndex = static_cast<size_t>(Resource);
	const RenderGraphResourceDesc_s& Desc = ResourceDescs[ResourceIndex];
	return Desc.ExternalTextureRef != nullptr || Desc.Kind == RenderGraphResourceKind_e::BACKBUFFER || Desc.Kind == RenderGraphResourceKind_e::RAYTRACING_SCENE;
}

FrameBuffer_s& RenderGraphBuilder_s::CreateWorkerFrameBuffer()
{
	return *WorkerFrameBuffers.emplace_back(std::make_unique<FrameBuffer_s>());
}

RenderGraph_s RenderGraphBuilder_s::Build()
{
	std::vector<RenderGraphPassHandle_t> LastWrittenBy(ResourceDescs.size());
	std::vector<RenderGraphPassHandle_t> RootPasses;

	for(uint32_t PassIt = 0u; PassIt < static_cast<uint32_t>(Passes.size()); PassIt++)
	{
		RenderGraphPass_s& Pass = Passes[PassIt];
		if (Pass.WritesToExternal)
		{
			RootPasses.push_back(static_cast<RenderGraphPassHandle_t>(PassIt + 1));
		}

		for (ResourceUsage_s& Resource : Pass.Resources)
		{
			if (Resource.Resource != RenderGraphResourceHandle_t::NONE)
			{
				if (ResourceReads(Resource.AccessType, Resource.LoadOp) || Resource.IsExtracted)
				{
					Resource.Producer = LastWrittenBy[static_cast<uint32_t>(Resource.Resource) - 1];

					if (!GetResourceDesc(Resource.Resource).IsInjected)
					{
						ENSUREMSG(Resource.Producer != RenderGraphPassHandle_t::NONE, "Pass failed to find a valid producer for a read");
					}
				}

				if (ResourceWrites(Resource.AccessType, Resource.LoadOp))
				{
					LastWrittenBy[static_cast<uint32_t>(Resource.Resource) - 1] = static_cast<RenderGraphPassHandle_t>(PassIt + 1);
				}
			}
		}
	}

	std::vector<RenderGraphPassHandle_t> SortedPasses;
	std::vector<RenderGraphPassHandle_t> Stack = RootPasses;
	std::vector<uint8_t> Visited;
	struct VisitedResource_s
	{
		RenderGraphResourceAccessType_e AccessTypes = RenderGraphResourceAccessType_e::UNKNOWN;
		bool IsExtracted = false;
	};
	std::vector<VisitedResource_s> SeenResources;
	Visited.resize(Passes.size(), 0);
	SeenResources.resize(ResourceDescs.size(), {});

	while (Stack.size() > 0)
	{
		const RenderGraphPassHandle_t PassHandle = Stack.back();
		uint8_t& VisitedVal = Visited[static_cast<uint32_t>(PassHandle) - 1];
		if (VisitedVal == 2)
		{
			Stack.pop_back();
		}
		else if (VisitedVal == 1)
		{
			VisitedVal = 2;
			SortedPasses.push_back(PassHandle);
			Stack.pop_back();
		}
		else
		{
			VisitedVal = 1;
			const RenderGraphPass_s& Pass = Passes[static_cast<uint32_t>(PassHandle) - 1];
			for(const ResourceUsage_s& Resource : Pass.Resources)
			{
				if (ResourceReads(Resource.AccessType, Resource.LoadOp) && Resource.Producer != RenderGraphPassHandle_t::NONE)
				{
					if (Visited[static_cast<uint32_t>(Resource.Producer) - 1] == 0)
					{
						Stack.push_back(Resource.Producer);
					}
				}

				SeenResources[static_cast<uint32_t>(Resource.Resource) - 1].AccessTypes |= Resource.AccessType;
				SeenResources[static_cast<uint32_t>(Resource.Resource) - 1].IsExtracted = Resource.IsExtracted;
			}
		}
	}

	// Set up passes
	RenderGraph_s RenderGraph = {};

	for (int32_t PassIt = 0; PassIt < SortedPasses.size(); PassIt++)
	{
		RenderGraph.Passes.push_back(std::move(Passes[static_cast<uint32_t>(SortedPasses[PassIt]) - 1]));
	}

	// Set up resources
	RenderGraph.Resources.push_back({}); // Resource handle NONE
	for (size_t ResIt = 0; ResIt < SeenResources.size(); ResIt++)
	{
		if (SeenResources[ResIt].AccessTypes != RenderGraphResourceAccessType_e::UNKNOWN)
		{
			// TODO: Check if seen access types match with texture usage

			RenderGraphResource_s Resource = {};
			const RenderGraphResourceDesc_s& Desc = ResourceDescs[ResIt + 1];

			Resource.DebugName = Desc.ResourceName;
			Resource.Kind = Desc.Kind;

			switch (Desc.Kind)
			{
			case RenderGraphResourceKind_e::TEXTURE:
				if (Desc.ExternalTextureRef == nullptr)
				{
					Resource.Texture = ResourcePool.GetOrCreateTexture(Desc.Texture, Desc.ResourceName.c_str());
				}
				else
				{
					Resource.Texture = Desc.ExternalTextureRef;
				}
				break;
			case RenderGraphResourceKind_e::BACKBUFFER:
				break;
			case RenderGraphResourceKind_e::RAYTRACING_SCENE:
				Resource.RaytracingScene = Desc.RaytracingScene;
				break;
			}

			RenderGraph.Resources.push_back(std::move(Resource));
		}
		else
		{
			RenderGraph.Resources.push_back({});
		}
	}

	RenderGraph.ExtractedTextures = std::move(ExtractedTextures);
	RenderGraph.Backbuffer = Backbuffer;
	RenderGraph.MainFrameBuffer = std::move(MainFrameBuffer);
	RenderGraph.WorkerFrameBuffers = std::move(WorkerFrameBuffers);

	ResourcePool.FinishFrame();

	return RenderGraph;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::AllocateResourceDesc(RenderGraphResourceDesc_s** NewDesc, const wchar_t* ResourceName)
{
	if (ResourceDescs.empty())
	{
		ResourceDescs.emplace_back(L"EmptyResource");
	}

	const RenderGraphResourceHandle_t Handle = static_cast<RenderGraphResourceHandle_t>(ResourceDescs.size());

	ResourceDescs.emplace_back(ResourceName);
	*NewDesc = &ResourceDescs.back();

	return Handle;
}

void RenderGraph_s::Execute(rl::CommandListSubmissionGroup* CLGroup)
{
	CHECK(CLGroup);

	GPUContext_s Ctx;

	Ctx.AddFrameBuffer(MainFrameBuffer.get());
	for (std::unique_ptr<FrameBuffer_s>& FrameBuffer : WorkerFrameBuffers)
	{
		Ctx.AddFrameBuffer(FrameBuffer.get());
	}

	for (RenderGraphPass_s& Pass : Passes)
	{		
		//rl::CommandListEventScope PassEvent(CommandList, Pass.PassName.c_str());

		Ctx.BeginPass();

		for (ResourceUsage_s& ResourceUsage : Pass.Resources)
		{
			RenderGraphResource_s& Resource = Resources[static_cast<uint32_t>(ResourceUsage.Resource)];

			if (Resource.Kind == RenderGraphResourceKind_e::RAYTRACING_SCENE)
			{
				// Acceleration structures never change state, a UAV barrier orders a build before later accesses
				if (Resource.WrittenSinceBarrier)
				{
					Ctx.RWBarrier(Resource.RaytracingScene);
				}
				Resource.WrittenSinceBarrier = rl::HasEnumFlags(ResourceUsage.AccessType, RenderGraphResourceAccessType_e::UAV);
				continue;
			}

			if (Resource.Kind == RenderGraphResourceKind_e::BACKBUFFER)
			{
				if (Backbuffer.BackBufferTransitionState != ResourceUsage.DesiredState)
				{
					Ctx.TransitionResource(Backbuffer.BackBufferTexture, Backbuffer.BackBufferTransitionState, ResourceUsage.DesiredState);
					Backbuffer.BackBufferTransitionState = ResourceUsage.DesiredState;
				}
				if (rl::HasEnumFlags(ResourceUsage.AccessType, RenderGraphResourceAccessType_e::RTV) && ResourceUsage.LoadOp == RenderGraphLoadOp_e::CLEAR)
				{
					const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
					Ctx.ClearRenderTarget(Backbuffer.BackBufferRTV, ClearColor);
				}
				continue;
			}

			if (Resource.Texture->CurrentState == rl::ResourceTransitionState::UNORDERED_ACCESS && ResourceUsage.DesiredState == rl::ResourceTransitionState::UNORDERED_ACCESS)
			{
				Ctx.RWBarrier(Resource.Texture->Texture);
			}

			if(Resource.Texture->CurrentState != ResourceUsage.DesiredState && !ResourceUsage.IsExtracted)
			{
				Ctx.TransitionResource(Resource.Texture->Texture, Resource.Texture->CurrentState, ResourceUsage.DesiredState);
				Resource.Texture->CurrentState = ResourceUsage.DesiredState;
			}

			// Clearing
			if (rl::HasEnumFlags(ResourceUsage.AccessType,RenderGraphResourceAccessType_e::RTV) && ResourceUsage.LoadOp == RenderGraphLoadOp_e::CLEAR)
			{
				Ctx.ClearRenderTarget(Resource.Texture->RTV, &Resource.Texture->Desc.ClearValue.x);
			}
			else if (rl::HasEnumFlags(ResourceUsage.AccessType, RenderGraphResourceAccessType_e::DSV) && ResourceUsage.LoadOp == RenderGraphLoadOp_e::CLEAR)
			{
				Ctx.ClearDepth(Resource.Texture->DSV, Resource.Texture->Desc.ClearValue.x);
			}
		}		

		Pass.Callback(*this, Ctx);

		Ctx.EndPass();
	}

	Ctx.Execute(CLGroup);
}

rl::ShaderResourceView_t RenderGraph_s::GetSRV(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphTexture_s* Texture = GetTextureResource(Resource))
	{
		return Texture->SRV;
	}
	return rl::ShaderResourceView_t::INVALID;
}

rl::RenderTargetView_t RenderGraph_s::GetRTV(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		if (ActiveResource->Kind == RenderGraphResourceKind_e::BACKBUFFER)
		{
			return Backbuffer.BackBufferRTV;
		}
	}

	if (const RenderGraphTexture_s* Texture = GetTextureResource(Resource))
	{
		return Texture->RTV;
	}
	return rl::RenderTargetView_t::INVALID;
}

rl::DepthStencilView_t RenderGraph_s::GetDSV(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphTexture_s* Texture = GetTextureResource(Resource))
	{
		return Texture->DSV;
	}
	return rl::DepthStencilView_t::INVALID;
}

rl::UnorderedAccessView_t RenderGraph_s::GetUAV(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphTexture_s* Texture = GetTextureResource(Resource))
	{
		return Texture->UAV;
	}
	return rl::UnorderedAccessView_t::INVALID;
}

rl::Texture_t RenderGraph_s::GetTexture(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphTexture_s* Texture = GetTextureResource(Resource))
	{
		return Texture->Texture;
	}
	return rl::Texture_t::INVALID;
}

uint32_t RenderGraph_s::GetSRVIndex(RenderGraphResourceHandle_t Resource)
{
	return rl::GetDescriptorIndex(GetSRV(Resource));
}

uint32_t RenderGraph_s::GetUAVIndex(RenderGraphResourceHandle_t Resource)
{
	return rl::GetDescriptorIndex(GetUAV(Resource));
}

uint2 RenderGraph_s::GetTextureDimensions(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		if (ActiveResource->Kind == RenderGraphResourceKind_e::BACKBUFFER)
		{
			return uint2(Backbuffer.BackbufferWidth, Backbuffer.BackbufferHeight);
		}
	}

	if (const RenderGraphTexture_s* Texture = GetTextureResource(Resource))
	{
		return uint2(Texture->Desc.Width, Texture->Desc.Height);
	}
	return uint2(0u, 0u);
}

uint3 RenderGraph_s::GetTextureDimensions3D(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		if (ActiveResource->Kind == RenderGraphResourceKind_e::TEXTURE && ActiveResource->Texture)
		{
			const RenderGraphTextureDesc_s& Desc = ActiveResource->Texture->Desc;
			return uint3(Desc.Width, Desc.Height, Desc.Depth);
		}
	}

	const uint2 Dimensions = GetTextureDimensions(Resource);
	return uint3(Dimensions.x, Dimensions.y, 1u);
}

rl::RaytracingScene_t RenderGraph_s::GetRaytracingScene(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		if (ENSUREMSG(ActiveResource->Kind == RenderGraphResourceKind_e::RAYTRACING_SCENE, "GetRaytracingScene called on a resource that isn't a raytracing scene"))
		{
			return ActiveResource->RaytracingScene;
		}
	}
	return rl::RaytracingScene_t::INVALID;
}

void RenderGraph_s::ExtractTexture(RenderGraphResourceHandle_t Texture)
{
	RenderGraphResource_s* ActiveResource = GetResource(Texture);
	CHECK(ActiveResource);
	CHECK(ExtractedTextures.contains(Texture));
	if (ActiveResource)
	{
		ActiveResource->Extracted = true;

		*ExtractedTextures[Texture] = *ActiveResource->Texture;
	}
}

RenderGraphResource_s* RenderGraph_s::GetResource(RenderGraphResourceHandle_t Resource)
{
	return Resource != RenderGraphResourceHandle_t::NONE ? &Resources[static_cast<uint32_t>(Resource)] : nullptr;
}

RenderGraphTexture_s* RenderGraph_s::GetTextureResource(RenderGraphResourceHandle_t Resource)
{
	if (RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		if (ENSUREMSG(ActiveResource->Kind == RenderGraphResourceKind_e::TEXTURE, "Texture getter called on a resource that isn't a texture"))
		{
			return ActiveResource->Texture.get();
		}
	}
	return nullptr;
}

RenderGraphTexturePtr_t RenderGraphResourcePool_s::GetOrCreateTexture(const RenderGraphTextureDesc_s& Desc, const wchar_t* ResourceName)
{
	size_t TexIt = 0;
	for(; TexIt < Textures.size(); TexIt++)
	{
		const RenderGraphTexture_s& Texture = *Textures[TexIt];
		if (Texture.Desc.Width == Desc.Width && Texture.Desc.Height == Desc.Height && Texture.Desc.Depth == Desc.Depth && Texture.Desc.Dimension == Desc.Dimension
			&& Texture.Desc.Format == Desc.Format && Texture.Desc.AccessTypes == Desc.AccessTypes)
		{
			break;
		}
	}

	RenderGraphTexturePtr_t NewTexture = nullptr;

	if(TexIt != Textures.size())
	{
		std::swap(Textures[TexIt], Textures.back());
		NewTexture = std::move(Textures.back());
		Textures.pop_back();
	}
	else
	{
		LOGINFO("Creating Texture %S - %d x %d x %d - Fmt=%d", ResourceName, Desc.Width, Desc.Height, Desc.Depth, (uint32_t)Desc.Format);
		NewTexture = Desc.Dimension == rl::TextureDimension::TEX3D
			? CreateRenderGraphTexture3D(Desc.Width, Desc.Height, Desc.Depth, Desc.Format, Desc.AccessTypes, ResourceName)
			: CreateRenderGraphTexture(Desc, ResourceName);
	}

	NewTextures.push_back(NewTexture);
	return NewTexture;
}

RenderGraphTexturePtr_t RenderGraphResourcePool_s::CreateEmptyTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName)
{
	return RenderGraphTexturePtr_t();
}

void RenderGraphResourcePool_s::FinishFrame()
{
	std::swap(Textures, NewTextures);
	NewTextures.clear();
}

RenderGraphTexturePtr_t CreateRenderGraphTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName, const void* const Data)
{
	RenderGraphTextureDesc_s Desc = {};
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.Format = Format;
	Desc.AccessTypes = AccessTypes;

	return CreateRenderGraphTexture(Desc, Data, ResourceName);
}

RenderGraphTexturePtr_t CreateRenderGraphTexture(const RenderGraphTextureDesc_s& Desc, const wchar_t* ResourceName)
{
	return CreateRenderGraphTexture(Desc, nullptr, ResourceName);
}

RenderGraphTexturePtr_t CreateRenderGraphTexture(const RenderGraphTextureDesc_s& Desc, const void* const Data, const wchar_t* ResourceName)
{
	if (Desc.Width == 0 || Desc.Width > 16238 || Desc.Height == 0 || Desc.Height > 16238)
	{
		ENSUREMSG(false, "Invalid texture dimensions");
		return nullptr;
	}

	if (Desc.Format == rl::RenderFormat::UNKNOWN)
	{
		ENSUREMSG(false, "Invalid texture format");
		return nullptr;
	}

	if (Desc.AccessTypes == RenderGraphResourceAccessType_e::UNKNOWN)
	{
		ENSUREMSG(false, "Invalid texture access types");
		return nullptr;
	}

	std::shared_ptr<RenderGraphTexture_s> OutTexture = std::make_shared<RenderGraphTexture_s>();

	OutTexture->Desc = Desc;

	rl::TextureCreateDesc TexDesc = {};
	TexDesc.Width = Desc.Width;
	TexDesc.Height = Desc.Height;
	TexDesc.Format = Desc.Format;
	
	rl::MipData mipData{ Data, Desc.Format, Desc.Width, Desc.Height };
	TexDesc.Data = Data ? &mipData : nullptr;

	if (rl::HasEnumFlags(Desc.AccessTypes, RenderGraphResourceAccessType_e::SRV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::SRV;
	}
	if (rl::HasEnumFlags(Desc.AccessTypes, RenderGraphResourceAccessType_e::UAV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::UAV;
	}
	if (rl::HasEnumFlags(Desc.AccessTypes, RenderGraphResourceAccessType_e::RTV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::RTV;
	}
	if (rl::HasEnumFlags(Desc.AccessTypes, RenderGraphResourceAccessType_e::DSV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::DSV;
	}

	TexDesc.DebugName = ResourceName;

	OutTexture->Texture = rl::CreateTexture(TexDesc);

	if (rl::HasEnumFlags(Desc.AccessTypes, RenderGraphResourceAccessType_e::SRV))
	{
		OutTexture->SRV = rl::CreateTextureSRV(OutTexture->Texture, TexDesc.Format, rl::TextureDimension::TEX2D, 1u, 1u);
	}
	if (rl::HasEnumFlags(Desc.AccessTypes, RenderGraphResourceAccessType_e::UAV))
	{
		OutTexture->UAV = rl::CreateTextureUAV(OutTexture->Texture, TexDesc.Format, rl::TextureDimension::TEX2D, 1u);
	}
	if (rl::HasEnumFlags(Desc.AccessTypes, RenderGraphResourceAccessType_e::RTV))
	{
		OutTexture->RTV = rl::CreateTextureRTV(OutTexture->Texture, TexDesc.Format, rl::TextureDimension::TEX2D, 1u);
	}
	if (rl::HasEnumFlags(Desc.AccessTypes, RenderGraphResourceAccessType_e::DSV))
	{
		rl::RenderFormat DepthFormat = Desc.Format == rl::RenderFormat::R32_FLOAT ? rl::RenderFormat::D32_FLOAT : rl::RenderFormat::D16_UNORM;
		OutTexture->DSV = rl::CreateTextureDSV(OutTexture->Texture, DepthFormat, rl::TextureDimension::TEX2D, 1u);
	}

	return OutTexture;
}

RenderGraphTexturePtr_t CreateRenderGraphTexture3D(uint32_t Width, uint32_t Height, uint32_t Depth, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName)
{
	if (Width == 0 || Width > 2048 || Height == 0 || Height > 2048 || Depth == 0 || Depth > 2048)
	{
		ENSUREMSG(false, "Invalid 3D texture dimensions");
		return nullptr;
	}

	if (Format == rl::RenderFormat::UNKNOWN)
	{
		ENSUREMSG(false, "Invalid texture format");
		return nullptr;
	}

	const RenderGraphResourceAccessType_e UnsupportedAccess = RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::DSV | RenderGraphResourceAccessType_e::COPYSRC | RenderGraphResourceAccessType_e::COPYDST;
	if (AccessTypes == RenderGraphResourceAccessType_e::UNKNOWN || rl::HasEnumFlags(AccessTypes, UnsupportedAccess))
	{
		ENSUREMSG(false, "3D textures only support SRV and UAV access");
		return nullptr;
	}

	std::shared_ptr<RenderGraphTexture_s> OutTexture = std::make_shared<RenderGraphTexture_s>();

	OutTexture->Desc.Width = Width;
	OutTexture->Desc.Height = Height;
	OutTexture->Desc.Depth = Depth;
	OutTexture->Desc.Dimension = rl::TextureDimension::TEX3D;
	OutTexture->Desc.Format = Format;
	OutTexture->AccessTypes = AccessTypes;

	rl::TextureCreateDescEx TexDesc = {};
	TexDesc.Width = Width;
	TexDesc.Height = Height;
	TexDesc.DepthOrArraySize = Depth;
	TexDesc.Dimension = rl::TextureDimension::TEX3D;
	TexDesc.ResourceFormat = Format;
	TexDesc.DebugName = ResourceName ? ResourceName : L"";

	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::SRV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::SRV;
	}
	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::UAV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::UAV;
	}

	OutTexture->Texture = rl::CreateTextureEx(TexDesc);

	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::SRV))
	{
		OutTexture->SRV = rl::CreateTextureSRV(OutTexture->Texture, Format, rl::TextureDimension::TEX3D, 1u, Depth);
	}
	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::UAV))
	{
		OutTexture->UAV = rl::CreateTextureUAV(OutTexture->Texture, Format, rl::TextureDimension::TEX3D, Depth);
	}

	return OutTexture;
}
