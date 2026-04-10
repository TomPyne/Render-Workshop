#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <Render/Render.h>
#include <SurfMath.h>
#include <vector>

enum class RenderGraphResourceHandle_t : uint32_t {NONE};
enum class RenderGraphPassHandle_t : uint32_t {NONE};

enum class RenderGraphLoadOp_e : uint8_t
{
	UNKNOWN,
	LOAD,
	CLEAR,
	DONT_CARE
};

// TODO: Change to just read or write, we can infer the specific access type from the pass and supported views on the resource.
enum class RenderGraphResourceAccessType_e : uint8_t
{
	UNKNOWN = 0,
	SRV = 1 << 0,
	UAV = 1 << 1,
	RTV = 1 << 2, 
	DSV = 1 << 3,
	COPYSRC = 1 << 4,
	COPYDST = 1 << 5,
};
IMPLEMENT_FLAGS(RenderGraphResourceAccessType_e, uint8_t)

struct GPUContext_s;
struct RenderGraph_s;
struct RenderGraphBuilder_s;

using RenderGraphCallback_Func = std::function<void(RenderGraph_s&, struct GPUContext_s&)>;

struct RenderGraphExternalTexture_s
{
	rl::ResourceTransitionState State = rl::ResourceTransitionState::COMMON;

	rl::TexturePtr Texture = {};
	rl::RenderTargetViewPtr RTV = {};
	rl::DepthStencilViewPtr DSV = {};
	rl::ShaderResourceViewPtr SRV = {};
	rl::UnorderedAccessViewPtr UAV = {};
};

struct RenderGraphTextureDesc_s
{
	uint32_t Width = 0u;
	uint32_t Height = 0u;
	rl::RenderFormat Format = rl::RenderFormat::UNKNOWN;
	RenderGraphResourceAccessType_e AccessTypes = RenderGraphResourceAccessType_e::UNKNOWN;
};

struct RenderGraphTexture_s
{
	RenderGraphTextureDesc_s Desc = {};
	RenderGraphResourceAccessType_e AccessTypes = RenderGraphResourceAccessType_e::UNKNOWN;
	rl::ResourceTransitionState CurrentState = rl::ResourceTransitionState::COMMON;

	std::wstring DebugName;

	rl::TexturePtr Texture = {};
	rl::RenderTargetViewPtr RTV = {};
	rl::DepthStencilViewPtr DSV = {};
	rl::ShaderResourceViewPtr SRV = {};
	rl::UnorderedAccessViewPtr UAV = {};
};

using RenderGraphTexturePtr_t = std::shared_ptr<RenderGraphTexture_s>;

struct RenderGraphResourcePool_s
{
	std::vector<RenderGraphTexturePtr_t> Textures;
	std::vector<RenderGraphTexturePtr_t> NewTextures;

	RenderGraphTexturePtr_t GetOrCreateTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName);
	RenderGraphTexturePtr_t CreateEmptyTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName);
	void FinishFrame();
};

struct ResourceUsage_s
{
	ResourceUsage_s() = delete;
	ResourceUsage_s(RenderGraphResourceHandle_t InResource, RenderGraphResourceAccessType_e InAccessType, RenderGraphLoadOp_e InLoadOp, rl::ResourceTransitionState InState)
		: Resource(InResource)
		, AccessType(InAccessType)
		, LoadOp(InLoadOp)
		, Producer(RenderGraphPassHandle_t::NONE)
		, DesiredState(InState)
	{
	}

	RenderGraphResourceHandle_t Resource;
	RenderGraphResourceAccessType_e AccessType;
	RenderGraphLoadOp_e LoadOp;
	RenderGraphPassHandle_t Producer;
	rl::ResourceTransitionState DesiredState = rl::ResourceTransitionState::COMMON;
	bool IsExtracted = false;
};

enum class RenderGraphPassType_e : uint8_t
{
	UNKNOWN,
	GRAPHICS, // Bind graphics queues
	COMPUTE, // Bind compute queues
	RAYTRACING, // Bind compute, extra prep for RT
	MISC, // Data manipulation or set up passes
};

struct RenderGraphPass_s
{
	RenderGraphPass_s(RenderGraphBuilder_s& InBuilder, RenderGraphPassType_e InPassType, const wchar_t* InPassName)
		: Builder(InBuilder)
	{
		PassName = InPassName ? InPassName : L"Unknown";
	}

	RenderGraphPass_s& AccessResource(RenderGraphResourceHandle_t Resource, RenderGraphResourceAccessType_e AccessType, RenderGraphLoadOp_e LoadOp);
	RenderGraphPass_s& ExtractResource(RenderGraphResourceHandle_t Resource);
	RenderGraphPass_s& SetExecuteCallback(RenderGraphCallback_Func Func);

	std::vector<ResourceUsage_s> Resources;
	RenderGraphCallback_Func Callback;
	bool WritesToExternal = false;
	RenderGraphPassType_e PassType = RenderGraphPassType_e::UNKNOWN;
	std::wstring PassName = L"Unknown";

private:

	RenderGraphBuilder_s& Builder;
};

struct RenderGraphResourceDesc_s
{
	RenderGraphResourceDesc_s(const wchar_t* InResourceName)
		: ResourceName(InResourceName ? InResourceName : L"Unknown")
	{
	}

	RenderGraphTextureDesc_s Texture = {};
	RenderGraphTexturePtr_t ExternalTextureRef = nullptr;
	bool IsBackBuffer = false;
	bool IsInjected = false;
	std::wstring ResourceName;
};

struct RenderGraphBuilder_s
{
	RenderGraphBuilder_s(RenderGraphResourcePool_s& InResourcePool)
		: ResourcePool(InResourcePool)
	{
	}

	RenderGraphResourceHandle_t CreateTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName);
	RenderGraphResourceHandle_t RefExternalTexture(RenderGraphTexturePtr_t Texture, const wchar_t* ResourceName);
	RenderGraphResourceHandle_t RefBackBufferTexture(rl::Texture_t Texture, rl::RenderTargetView_t RTV, rl::ResourceTransitionState TransitionState);

	// TODO: Support dynamic release and injection, for now use copy
	RenderGraphResourceHandle_t InjectTexture(RenderGraphTexturePtr_t& ExtractedTexture, const wchar_t* ResourceName);
	void QueueTextureExtraction(RenderGraphResourceHandle_t Resource, RenderGraphTexturePtr_t& OutTexture);

	// Resources must have same dimension
	void QueueTextureCopy(RenderGraphResourceHandle_t DstResource, RenderGraphResourceHandle_t SrcResouce);

	RenderGraphPass_s& AddPass(RenderGraphPassType_e PassType, const wchar_t* PassName);

	RenderGraphResourceDesc_s& GetResourceDesc(RenderGraphResourceHandle_t Resource);
	bool IsResourceExternal(RenderGraphResourceHandle_t Resource) const;

	RenderGraph_s Build();

protected:
	RenderGraphResourcePool_s& ResourcePool;
	std::vector<RenderGraphPass_s> Passes;
	std::vector<RenderGraphResourceDesc_s> ResourceDescs;

	rl::Texture_t BackBufferTexture = {};
	rl::RenderTargetView_t BackBufferRTV = {};
	rl::ResourceTransitionState BackBufferTransitionState = rl::ResourceTransitionState::COMMON;

	std::map<RenderGraphResourceHandle_t, RenderGraphTexturePtr_t> ExtractedTextures;

	RenderGraphResourceHandle_t AllocateResourceDesc(RenderGraphResourceDesc_s** NewDesc, const wchar_t* ResourceName);
};

struct RenderGraphResource_s
{
	RenderGraphTexturePtr_t Texture;
	bool IsBackBuffer = false;
	bool Extracted = false;
	std::wstring DebugName;
};

struct RenderGraph_s
{
	std::vector<RenderGraphPass_s> Passes;
	std::vector<RenderGraphResource_s> Resources;

	void Execute(rl::CommandListSubmissionGroup* CLGroup);

	rl::ShaderResourceView_t GetSRV(RenderGraphResourceHandle_t Resource);
	rl::RenderTargetView_t GetRTV(RenderGraphResourceHandle_t Resource);
	rl::DepthStencilView_t GetDSV(RenderGraphResourceHandle_t Resource);
	rl::UnorderedAccessView_t GetUAV(RenderGraphResourceHandle_t Resource);
	rl::Texture_t GetTexture(RenderGraphResourceHandle_t Resource);

	uint32_t GetSRVIndex(RenderGraphResourceHandle_t Resource);
	uint32_t GetUAVIndex(RenderGraphResourceHandle_t Resource);

	uint2 GetTextureDimensions(RenderGraphResourceHandle_t Resource);

	void ExtractTexture(RenderGraphResourceHandle_t Texture);

private:
	RenderGraphResource_s* GetResource(RenderGraphResourceHandle_t Resource);

	rl::Texture_t BackBufferTexture = {};
	rl::RenderTargetView_t BackBufferRTV = {};
	rl::ResourceTransitionState BackBufferTransitionState = rl::ResourceTransitionState::COMMON;

	std::map<RenderGraphResourceHandle_t, RenderGraphTexturePtr_t> ExtractedTextures;

	friend struct RenderGraphBuilder_s;
};

RenderGraphTexturePtr_t CreateRenderGraphTexture(uint32_t Width, uint32_t Height, rl::RenderFormat Format, RenderGraphResourceAccessType_e AccessTypes, const wchar_t* ResourceName);