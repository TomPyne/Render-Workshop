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

	if (!rl::HasEnumFlags(AccessType, RenderGraphResourceAccessType_e::SRV | RenderGraphResourceAccessType_e::COPYSRC) && Builder.IsResourceExternal(Resource))
	{
		WritesToExternal = true;
	}

	rl::ResourceTransitionState DesiredState = rl::ResourceTransitionState::COMMON;
	switch (AccessType)
	{
		case RenderGraphResourceAccessType_e::SRV:
			DesiredState = PassType == RenderGraphPassType_e::GRAPHICS ? rl::ResourceTransitionState::PIXEL_SHADER_RESOURCE : rl::ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE;
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

RenderGraphResourceHandle_t RenderGraphBuilder_s::RefExternalTexture(RenderGraphTexturePtr_t Texture, const wchar_t* ResourceName)
{
	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, ResourceName);
	Resource->ExternalTextureRef = Texture;
	return Handle;
}

RenderGraphResourceHandle_t RenderGraphBuilder_s::RefBackBufferTexture(rl::Texture_t Texture, rl::RenderTargetView_t RTV, rl::ResourceTransitionState TransitionState)
{
	if (!ENSUREMSG(rl::IsValid(Texture) && rl::IsValid(RTV), "RenderGraphBuilder_s::RefBackBufferTexture Texture or RTV are not valid"))
	{
		return RenderGraphResourceHandle_t::NONE;
	}
	
	if(rl::IsValid(BackBufferTexture) || rl::IsValid(BackBufferRTV))
	{
		LOGWARNING("RenderGraphBuilder_s::RefBackBufferTexture Back buffer texture or RTV already set, overwriting previous values");
	}
	BackBufferTexture = Texture;
	BackBufferRTV = RTV;
	BackBufferTransitionState = TransitionState;

	RenderGraphResourceDesc_s* Resource = nullptr;
	const RenderGraphResourceHandle_t Handle = AllocateResourceDesc(&Resource, L"BackbufferTexture");
	Resource->IsBackBuffer = true;
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
	return ResourceDescs[ResourceIndex].ExternalTextureRef != nullptr || ResourceDescs[ResourceIndex].IsBackBuffer;
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
					if (GetResourceDesc(Resource.Resource).IsInjected)
					{
						Resource.Producer = RenderGraphPassHandle_t::NONE;
					}
					else
					{
						Resource.Producer = LastWrittenBy[static_cast<uint32_t>(Resource.Resource) - 1];

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

			if (Desc.IsBackBuffer)
			{
				Resource.IsBackBuffer = true;
			}
			else
			{
				if (Desc.ExternalTextureRef == nullptr)
				{
					Resource.Texture = ResourcePool.GetOrCreateTexture(
						Desc.Texture.Width,
						Desc.Texture.Height,
						Desc.Texture.Format,
						Desc.Texture.AccessTypes,
						Desc.ResourceName.c_str());
				}
				else
				{
					Resource.Texture = Desc.ExternalTextureRef;
				}
			}

			RenderGraph.Resources.push_back(std::move(Resource));
		}
		else
		{
			RenderGraph.Resources.push_back({});
		}
	}

	RenderGraph.ExtractedTextures = std::move(ExtractedTextures);
	RenderGraph.BackBufferRTV = BackBufferRTV;
	RenderGraph.BackBufferTexture = BackBufferTexture;
	RenderGraph.BackBufferTransitionState = BackBufferTransitionState;

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

	for (RenderGraphPass_s& Pass : Passes)
	{		
		//rl::CommandListEventScope PassEvent(CommandList, Pass.PassName.c_str());

		Ctx.BeginPass();

		for (ResourceUsage_s& ResourceUsage : Pass.Resources)
		{
			RenderGraphResource_s& Resource = Resources[static_cast<uint32_t>(ResourceUsage.Resource)];

			if (Resource.IsBackBuffer)
			{
				if (BackBufferTransitionState != ResourceUsage.DesiredState)
				{
					Ctx.TransitionResource(BackBufferTexture, BackBufferTransitionState, ResourceUsage.DesiredState);
					BackBufferTransitionState = ResourceUsage.DesiredState;
				}
				if (rl::HasEnumFlags(ResourceUsage.AccessType, RenderGraphResourceAccessType_e::RTV) && ResourceUsage.LoadOp == RenderGraphLoadOp_e::CLEAR)
				{
					const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
					Ctx.ClearRenderTarget(BackBufferRTV, ClearColor);
				}
				continue;
			}

			// Transitions
			if(Resource.Texture->CurrentState != ResourceUsage.DesiredState && !ResourceUsage.IsExtracted)
			{
				Ctx.TransitionResource(Resource.Texture->Texture, Resource.Texture->CurrentState, ResourceUsage.DesiredState);
				Resource.Texture->CurrentState = ResourceUsage.DesiredState;
			}

			// Clearing
			if (rl::HasEnumFlags(ResourceUsage.AccessType,RenderGraphResourceAccessType_e::RTV) && ResourceUsage.LoadOp == RenderGraphLoadOp_e::CLEAR)
			{
				const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
				Ctx.ClearRenderTarget(Resource.Texture->RTV, ClearColor);
			}
			else if (rl::HasEnumFlags(ResourceUsage.AccessType, RenderGraphResourceAccessType_e::DSV) && ResourceUsage.LoadOp == RenderGraphLoadOp_e::CLEAR)
			{
				Ctx.ClearDepth(Resource.Texture->DSV, 1.0f);
			}
		}		

		Pass.Callback(*this, Ctx);

		Ctx.EndPass();
	}

	Ctx.Execute(CLGroup);
}

rl::ShaderResourceView_t RenderGraph_s::GetSRV(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		return ActiveResource->Texture->SRV;
	}
	return rl::ShaderResourceView_t::INVALID;
}

rl::RenderTargetView_t RenderGraph_s::GetRTV(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		if (ActiveResource->IsBackBuffer)
		{
			return BackBufferRTV;
		}
		else
		{
			return ActiveResource->Texture->RTV;
		}
	}
	return rl::RenderTargetView_t::INVALID;
}

rl::DepthStencilView_t RenderGraph_s::GetDSV(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		return ActiveResource->Texture->DSV;
	}
	return rl::DepthStencilView_t::INVALID;
}

rl::UnorderedAccessView_t RenderGraph_s::GetUAV(RenderGraphResourceHandle_t Resource)
{
	if (const RenderGraphResource_s* ActiveResource = GetResource(Resource))
	{
		return ActiveResource->Texture->UAV;
	}
	return rl::UnorderedAccessView_t::INVALID;
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
		if (ActiveResource->Texture)
		{
			return uint2(ActiveResource->Texture->Desc.Width, ActiveResource->Texture->Desc.Height);
		}
	}
	return uint2(0u, 0u);
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

RenderGraphTexturePtr_t RenderGraphResourcePool_s::GetOrCreateTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName)
{
	size_t TexIt = 0;
	for(; TexIt < Textures.size(); TexIt++)
	{
		if (Textures[TexIt]->Desc.Width == Width && Textures[TexIt]->Desc.Height == Height && Textures[TexIt]->Desc.Format == Format && Textures[TexIt]->AccessTypes == AccessTypes)
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
		LOGINFO("Creating Texture %S - %d x %d - Fmt=%d", ResourceName, Width, Height, (uint32_t)Format);
		NewTexture = CreateRenderGraphTexture(Width, Height, Format, AccessTypes, ResourceName);
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

RenderGraphTexturePtr_t CreateRenderGraphTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName)
{
	if (Width == 0 || Width > 16238 || Height == 0 || Height > 16238)
	{
		ENSUREMSG(false, "Invalid texture dimensions");
		return nullptr;
	}

	if(Format == rl::RenderFormat::UNKNOWN)
	{
		ENSUREMSG(false, "Invalid texture format");
		return nullptr;
	}

	if (AccessTypes == RenderGraphResourceAccessType_e::UNKNOWN)
	{
		ENSUREMSG(false, "Invalid texture access types");
		return nullptr;
	}

	std::shared_ptr<RenderGraphTexture_s> OutTexture = std::make_shared<RenderGraphTexture_s>();

	OutTexture->Desc.Width = Width;
	OutTexture->Desc.Height = Height;
	OutTexture->Desc.Format = Format;
	OutTexture->AccessTypes = AccessTypes;

	rl::TextureCreateDesc TexDesc = {};
	TexDesc.Width = Width;
	TexDesc.Height = Height;
	TexDesc.Format = Format;

	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::SRV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::SRV;
	}
	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::UAV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::UAV;
	}
	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::RTV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::RTV;
	}
	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::DSV))
	{
		TexDesc.Flags |= rl::RenderResourceFlags::DSV;
	}

	TexDesc.DebugName = ResourceName;

	OutTexture->Texture = rl::CreateTexture(TexDesc);

	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::SRV))
	{
		OutTexture->SRV = rl::CreateTextureSRV(OutTexture->Texture, TexDesc.Format, rl::TextureDimension::TEX2D, 1u, 1u);
	}
	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::UAV))
	{
		OutTexture->UAV = rl::CreateTextureUAV(OutTexture->Texture, TexDesc.Format, rl::TextureDimension::TEX2D, 1u);
	}
	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::RTV))
	{
		OutTexture->RTV = rl::CreateTextureRTV(OutTexture->Texture, TexDesc.Format, rl::TextureDimension::TEX2D, 1u);
	}
	if (rl::HasEnumFlags(AccessTypes, RenderGraphResourceAccessType_e::DSV))
	{
		rl::RenderFormat DepthFormat = Format == rl::RenderFormat::R32_FLOAT ? rl::RenderFormat::D32_FLOAT : rl::RenderFormat::D16_UNORM;
		OutTexture->DSV = rl::CreateTextureDSV(OutTexture->Texture, DepthFormat, rl::TextureDimension::TEX2D, 1u);
	}

	return OutTexture;
}
