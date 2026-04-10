#include "GPUContext.h"

#include <Logging/Logging.h>
#include <Render/Render.h>
#include <SurfMath.h>
#include <thread>

class GPUCommand_c
{
public:
	virtual void Execute(rl::CommandList* CL) = 0;
};

class GPUCommand_SetRootSignature_c : public GPUCommand_c
{
	rl::RootSignature_t RootSig;

public:

	GPUCommand_SetRootSignature_c(rl::RootSignature_t InRootSig = rl::RootSignature_t::INVALID)
		: RootSig(InRootSig)
	{}

	void Execute(rl::CommandList* CL)
	{
		if (rl::IsValid(RootSig))
		{
			CL->SetRootSignature(RootSig);
		}
		else
		{
			CL->SetRootSignature();
		}		
	}
};

class GPUCommand_SetComputeRootSignature_c : public GPUCommand_c
{
	rl::RootSignature_t RootSig;

public:
	GPUCommand_SetComputeRootSignature_c(rl::RootSignature_t InRootSig)
		: RootSig(InRootSig)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->SetComputeRootSignature(RootSig);
	}
};

class GPUCommand_SetRenderTargets_c : public GPUCommand_c
{
	rl::RenderTargetView_t RTVs[8];
	size_t NumRTVs;
	rl::DepthStencilView_t DSV;

public:
	GPUCommand_SetRenderTargets_c(rl::RenderTargetView_t* InRTVs, size_t InNumRTVs, rl::DepthStencilView_t InDsv)
		: NumRTVs(InNumRTVs)
		, DSV(InDsv)
	{
		memcpy(RTVs, InRTVs, sizeof(rl::RenderTargetView_t) * InNumRTVs);
	}

	void Execute(rl::CommandList* CL)
	{
		CL->SetRenderTargets(RTVs, NumRTVs, DSV);
	}
};

class GPUCommand_SetViewports_c : public GPUCommand_c
{
	rl::Viewport Viewports[8];
	size_t NumViewports;

public:
	GPUCommand_SetViewports_c(rl::Viewport* InViewports, size_t InNumViewports)
		: NumViewports(InNumViewports)
	{
		memcpy(Viewports, InViewports, sizeof(rl::Viewport) * InNumViewports);
	}

	void Execute(rl::CommandList* CL)
	{
		CL->SetViewports(Viewports, NumViewports);
	}
};

class GPUCommand_SetScissorRects_c : public GPUCommand_c
{
	rl::ScissorRect ScissorRects[8];
	size_t NumScissorRects;

public:
	GPUCommand_SetScissorRects_c(rl::ScissorRect* InScissorRects, size_t InNumScissorRects)
		: NumScissorRects(InNumScissorRects)
	{
		memcpy(ScissorRects, InScissorRects, sizeof(rl::Viewport) * InNumScissorRects);
	}

	void Execute(rl::CommandList* CL)
	{
		CL->SetScissors(ScissorRects, NumScissorRects);
	}
};

class GPUCommand_SetGraphicsRootDescriptorTable_c : public GPUCommand_c
{
	uint32_t RootParameterIndex;

public:
	GPUCommand_SetGraphicsRootDescriptorTable_c(uint32_t InRootParameterIndex)
		: RootParameterIndex(InRootParameterIndex)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->SetGraphicsRootDescriptorTable(RootParameterIndex);
	}
};

class GPUCommand_SetComputeRootDescriptorTable_c : public GPUCommand_c
{
	uint32_t RootParameterIndex;

public:
	GPUCommand_SetComputeRootDescriptorTable_c(uint32_t InRootParameterIndex)
		: RootParameterIndex(InRootParameterIndex)
	{
	}

	void Execute(rl::CommandList* CL)
	{
		CL->SetComputeRootDescriptorTable(RootParameterIndex);
	}
};

template<typename BufferType>
class GPUCommand_SetGraphicsRootCBV_c : public GPUCommand_c
{
	uint32_t RootParameterIndex;
	BufferType CBV;

public:
	GPUCommand_SetGraphicsRootCBV_c(uint32_t InRootParameterIndex, BufferType InCBV)
		: RootParameterIndex(InRootParameterIndex)
		, CBV(InCBV)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->SetGraphicsRootCBV(RootParameterIndex, CBV);
	}
};

template<typename BufferType>
class GPUCommand_SetComputeRootCBV_c : public GPUCommand_c
{
	uint32_t RootParameterIndex;
	BufferType CBV;

public:
	GPUCommand_SetComputeRootCBV_c(uint32_t InRootParameterIndex, BufferType InCBV)
		: RootParameterIndex(InRootParameterIndex)
		, CBV(InCBV)
	{
	}

	void Execute(rl::CommandList* CL)
	{
		CL->SetComputeRootCBV(RootParameterIndex, CBV);
	}
};

template<typename SRVType>
class GPUCommand_SetComputeRootSRV_c : public GPUCommand_c
{
	uint32_t RootParameterIndex;
	SRVType SRV;

public:
	GPUCommand_SetComputeRootSRV_c(uint32_t InRootParameterIndex, SRVType InSRV)
		: RootParameterIndex(InRootParameterIndex)
		, SRV(InSRV)
	{}
	void Execute(rl::CommandList* CL)
	{
		CL->SetComputeRootSRV(RootParameterIndex, SRV);
	}
};

class GPUCommand_SetGraphicsRootValue : public GPUCommand_c
{
	uint32_t RootParameterIndex;
	uint32_t DestOffsetIn32BitValues;
	uint32_t Value;

public:
	GPUCommand_SetGraphicsRootValue(uint32_t InRootParameterIndex, uint32_t InDestOffsetIn32BitValues, uint32_t InValue)
		: RootParameterIndex(InRootParameterIndex)
		, DestOffsetIn32BitValues(InDestOffsetIn32BitValues)
		, Value(InValue)
	{}
	void Execute(rl::CommandList* CL)
	{
		CL->SetGraphicsRootValue(RootParameterIndex, DestOffsetIn32BitValues, Value);
	}
};

template<typename PSOType>
class GPUCommand_SetPipelineState_c : public GPUCommand_c
{
	PSOType PSO;

public:
	GPUCommand_SetPipelineState_c(PSOType InPSO)
		: PSO(InPSO)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->SetPipelineState(PSO);
	}
};

class GPUCommand_DrawInstanced_c : public GPUCommand_c
{
	uint32_t VertexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartVertexLocation;
	uint32_t StartInstanceLocation;

public:
	GPUCommand_DrawInstanced_c(uint32_t InVertexCountPerInstance, uint32_t InInstanceCount, uint32_t InStartVertexLocation, uint32_t InStartInstanceLocation)
		: VertexCountPerInstance(InVertexCountPerInstance)
		, InstanceCount(InInstanceCount)
		, StartVertexLocation(InStartVertexLocation)
		, StartInstanceLocation(InStartInstanceLocation)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}
};

class GPUCommand_DrawIndexedInstanced_c : public GPUCommand_c
{
	uint32_t IndexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartIndexLocation;
	int32_t BaseVertexLocation;
	uint32_t StartInstanceLocation;

public:
	GPUCommand_DrawIndexedInstanced_c(uint32_t InIndexCountPerInstance, uint32_t InInstanceCount, uint32_t InStartIndexLocation, int32_t InBaseVertexLocation, uint32_t InStartInstanceLocation)
		: IndexCountPerInstance(InIndexCountPerInstance)
		, InstanceCount(InInstanceCount)
		, StartIndexLocation(InStartIndexLocation)
		, BaseVertexLocation(InBaseVertexLocation)
		, StartInstanceLocation(InStartInstanceLocation)
	{}
	void Execute(rl::CommandList* CL)
	{
		CL->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}
};

class GPUCommand_Dispatch_c : public GPUCommand_c
{
	uint32_t ThreadGroupCountX;
	uint32_t ThreadGroupCountY;
	uint32_t ThreadGroupCountZ;

public:
	GPUCommand_Dispatch_c(uint32_t InThreadGroupCountX, uint32_t InThreadGroupCountY, uint32_t InThreadGroupCountZ)
		: ThreadGroupCountX(InThreadGroupCountX)
		, ThreadGroupCountY(InThreadGroupCountY)
		, ThreadGroupCountZ(InThreadGroupCountZ)
	{
	}

	void Execute(rl::CommandList* CL)
	{
		CL->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}
};

class GPUCommand_DispatchMesh_c : public GPUCommand_c
{
	uint32_t ThreadGroupCountX;
	uint32_t ThreadGroupCountY;
	uint32_t ThreadGroupCountZ;

public:
	GPUCommand_DispatchMesh_c(uint32_t InThreadGroupCountX, uint32_t InThreadGroupCountY, uint32_t InThreadGroupCountZ)
		: ThreadGroupCountX(InThreadGroupCountX)
		, ThreadGroupCountY(InThreadGroupCountY)
		, ThreadGroupCountZ(InThreadGroupCountZ)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}
};

class GPUCommand_DispatchRays_c : public GPUCommand_c
{
	rl::RaytracingShaderTable_t ShaderTable;
	uint32_t ThreadGroupCountX;
	uint32_t ThreadGroupCountY;
	uint32_t ThreadGroupCountZ;

public:
	GPUCommand_DispatchRays_c(rl::RaytracingShaderTable_t InShaderTable, uint32_t InThreadGroupCountX, uint32_t InThreadGroupCountY, uint32_t InThreadGroupCountZ)
		: ShaderTable(InShaderTable)
		, ThreadGroupCountX(InThreadGroupCountX)
		, ThreadGroupCountY(InThreadGroupCountY)
		, ThreadGroupCountZ(InThreadGroupCountZ)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->DispatchRays(ShaderTable, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}
};

class GPUCommand_CopyTexture_c : public GPUCommand_c
{
	rl::Texture_t Dst;
	rl::Texture_t Src;

public:
	GPUCommand_CopyTexture_c(rl::Texture_t InDst, rl::Texture_t InSrc)
		: Dst(InDst)
		, Src(InSrc)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->CopyTexture(Dst, Src);
	}
};

class GPUCommand_TransitionResource_c : public GPUCommand_c
{
	rl::Texture_t Texture;
	rl::ResourceTransitionState BeforeState;
	rl::ResourceTransitionState AfterState;
public:
	GPUCommand_TransitionResource_c(rl::Texture_t InTexture, rl::ResourceTransitionState InBeforeState, rl::ResourceTransitionState InAfterState)
		: Texture(InTexture)
		, BeforeState(InBeforeState)
		, AfterState(InAfterState)
	{}
	void Execute(rl::CommandList* CL)
	{
		CL->TransitionResource(Texture, BeforeState, AfterState);
	}
};

class GPUCommand_ClearRenderTarget_c : public GPUCommand_c
{
	rl::RenderTargetView_t RTV;
	float Color[4];

public:
	GPUCommand_ClearRenderTarget_c(rl::RenderTargetView_t InRTV, const float InColor[4])
		: RTV(InRTV)
	{
		memcpy(Color, InColor, sizeof(float) * 4);
	}

	void Execute(rl::CommandList* CL)
	{
		CL->ClearRenderTarget(RTV, Color);
	}
};

class GPUCommand_ClearDepth_c : public GPUCommand_c
{
	rl::DepthStencilView_t DSV;
	float Depth;

public:
	GPUCommand_ClearDepth_c(rl::DepthStencilView_t InDSV, float InDepth)
		: DSV(InDSV)
		, Depth(InDepth)
	{}

	void Execute(rl::CommandList* CL)
	{
		CL->ClearDepth(DSV, Depth);
	}
};

class GPUCommand_SetVertexBuffers_c : public GPUCommand_c
{
	uint32_t StartSlot;
	uint32_t NumBuffers;
	rl::VertexBuffer_t VertexBuffers[8];
	uint32_t Strides[8];
	uint32_t Offsets[8];

public:
	GPUCommand_SetVertexBuffers_c(uint32_t InStartSlot, uint32_t InNumBuffers, const rl::VertexBuffer_t* InVertexBuffers, const uint32_t* InStrides, const uint32_t* InOffsets)
		: StartSlot(InStartSlot)
		, NumBuffers(InNumBuffers)
	{
		memcpy(VertexBuffers, InVertexBuffers, sizeof(rl::VertexBuffer_t) * InNumBuffers);
		memcpy(Strides, InStrides, sizeof(uint32_t) * InNumBuffers);
		memcpy(Offsets, InOffsets, sizeof(uint32_t) * InNumBuffers);
	}
	void Execute(rl::CommandList* CL)
	{
		CL->SetVertexBuffers(StartSlot, NumBuffers, VertexBuffers, Strides, Offsets);
	}
};

class GPUCommand_SetIndexBuffer_c : public GPUCommand_c
{
	rl::IndexBuffer_t Buffer;
	rl::RenderFormat Format;
	uint32_t IndexCount;

public:
	GPUCommand_SetIndexBuffer_c(rl::IndexBuffer_t InBuffer, rl::RenderFormat InFormat, uint32_t InIndexCount)
		: Buffer(InBuffer)
		, Format(InFormat)
		, IndexCount(InIndexCount)
	{}
	void Execute(rl::CommandList* CL)
	{
		CL->SetIndexBuffer(Buffer, Format, IndexCount);
	}
};

class GPUPass_c
{
	uint32_t StartCommandIndex;
	uint32_t CommandCount;
public:

	GPUPass_c(uint32_t InStartCommandIndex, uint32_t InCommandCount)
		: StartCommandIndex(InStartCommandIndex)
		, CommandCount(InCommandCount)
	{
	}

	uint32_t GetStartCommandIndex() const { return StartCommandIndex; }
	uint32_t GetCommandCount() const { return CommandCount; }
};

void GPUContext_s::AddPass(uint32_t StartCommandIndex, uint32_t CommandCount)
{
	Passes.push_back(new GPUPass_c(StartCommandIndex, CommandCount));
}

GPUContext_s::~GPUContext_s()
{
	for (GPUCommand_c* Command : Commands)
	{
		delete Command;
	}

	for(GPUPass_c * Pass : Passes)
	{
		delete Pass;
	}
}

void GPUContext_s::Execute(rl::CommandListSubmissionGroup* CLGroup)
{
	const size_t CommandCount = Commands.size();
	const size_t CommandsPerList = 100;

	struct ThreadInfo
	{
		size_t StartCommandIndex;
		size_t CommandCount;
	};
	std::vector<ThreadInfo> ThreadInfos;

	// Theoretical upper bound if all passes fit perfectly.
	ThreadInfos.reserve(DivideRoundUp(CommandCount, CommandsPerList));

	size_t CurrentStartCommandIndex = 0;
	size_t CurrentCommandCount = 0;
	for(GPUPass_c* Pass : Passes)
	{
		const size_t PassStartCommandIndex = Pass->GetStartCommandIndex();
		const size_t PassCommandCount = Pass->GetCommandCount();

		CurrentCommandCount += PassCommandCount;

		if (CurrentCommandCount >= CommandsPerList)
		{
			ThreadInfos.emplace_back(CurrentStartCommandIndex, CurrentCommandCount);
			CurrentStartCommandIndex += CurrentCommandCount;
			CurrentCommandCount = 0;
		}
	}
	
	// Handle any leftover commands that didn't fill a full command list
	if (CurrentCommandCount > 0)
	{
		ThreadInfos.emplace_back(CurrentStartCommandIndex, CurrentCommandCount);
	}

	const size_t NumCommandLists = ThreadInfos.size();

	std::vector<std::thread> CommandListThreads;
	CommandListThreads.reserve(NumCommandLists);

	for (size_t ThreadIt = 0; ThreadIt < ThreadInfos.size(); ++ThreadIt)
	{
		rl::CommandList* CL = CLGroup->CreateCommandList();
		CommandListThreads.emplace_back([&ThreadInfos, ThreadIt, CL, this]()
		{
			const size_t StartCommandIndex = ThreadInfos[ThreadIt].StartCommandIndex;
			const size_t EndCommandIndex = StartCommandIndex + ThreadInfos[ThreadIt].CommandCount;

			for (size_t CommandIt = StartCommandIndex; CommandIt < EndCommandIndex; ++CommandIt)
			{
				Commands[CommandIt]->Execute(CL);
			}
		});
	}

	// Wait for command list recording to finish
	for(std::thread& CommandListThread : CommandListThreads)
	{
		CommandListThread.join();
	}
}

void GPUContext_s::BeginPass()
{
	ASSERTMSG(CurrentPassIndex == -1, "Cannot begin a new pass before ending the current one.");
	CurrentPassIndex = static_cast<int32_t>(Commands.size());
}

void GPUContext_s::EndPass()
{
	ASSERTMSG(CurrentPassIndex != -1, "Cannot end a pass before beginning one.");
	const uint32_t StartCommandIndex = static_cast<uint32_t>(CurrentPassIndex);
	const uint32_t EndCommandIndex = static_cast<uint32_t>(Commands.size());
	AddPass(StartCommandIndex, EndCommandIndex - StartCommandIndex);
	CurrentPassIndex = -1;
}

void GPUContext_s::SetRootSignature()
{
	AddCommand<GPUCommand_SetRootSignature_c>();
}

void GPUContext_s::SetRootSignature(rl::RootSignature_t RootSignature)
{
	AddCommand<GPUCommand_SetRootSignature_c>(RootSignature);
}

void GPUContext_s::SetComputeRootSignature(rl::RootSignature_t RootSignature)
{
	AddCommand<GPUCommand_SetComputeRootSignature_c>(RootSignature);
}

void GPUContext_s::SetRenderTargets(rl::RenderTargetView_t* RTVs, size_t NumRTVs, rl::DepthStencilView_t DSV)
{
	ASSERTMSG(NumRTVs <= 8, "NumRTVs must be 8 max");
	AddCommand<GPUCommand_SetRenderTargets_c>(RTVs, NumRTVs, DSV);
}

void GPUContext_s::SetViewports(rl::Viewport* Viewports, size_t NumViewports)
{
	ASSERTMSG(NumViewports <= 8, "NumViewports must be 8 max");
	AddCommand<GPUCommand_SetViewports_c>(Viewports, NumViewports);
}

void GPUContext_s::SetDefaultScissor()
{
	rl::ScissorRect DefaultScissor = {};
	DefaultScissor.left = 0;
	DefaultScissor.top = 0;	
	DefaultScissor.right = LONG_MAX;	
	DefaultScissor.bottom = LONG_MAX;
	AddCommand<GPUCommand_SetScissorRects_c>(&DefaultScissor, 1);	
}

void GPUContext_s::SetScissorRects(rl::ScissorRect* ScissorRects, size_t NumScissorRects)
{
	ASSERTMSG(NumScissorRects <= 8, "NumScissorRects must be 8 max");
	AddCommand<GPUCommand_SetScissorRects_c>(ScissorRects, NumScissorRects);
}

void GPUContext_s::SetGraphicsRootCBV(uint32_t RootParameterIndex, rl::ConstantBuffer_t CBV)
{
	AddCommand<GPUCommand_SetGraphicsRootCBV_c<rl::ConstantBuffer_t>>(RootParameterIndex, CBV);
}

void GPUContext_s::SetGraphicsRootCBV(uint32_t RootParameterIndex, rl::DynamicBuffer_t CBV)
{
	AddCommand<GPUCommand_SetGraphicsRootCBV_c<rl::DynamicBuffer_t>>(RootParameterIndex, CBV);
}

void GPUContext_s::SetComputeRootCBV(uint32_t RootParameterIndex, rl::ConstantBuffer_t CBV)
{
	AddCommand<GPUCommand_SetComputeRootCBV_c<rl::ConstantBuffer_t>>(RootParameterIndex, CBV);
}

void GPUContext_s::SetComputeRootCBV(uint32_t RootParameterIndex, rl::DynamicBuffer_t CBV)
{
	AddCommand<GPUCommand_SetComputeRootCBV_c<rl::DynamicBuffer_t>>(RootParameterIndex, CBV);
}

void GPUContext_s::SetComputeRootSRV(uint32_t RootParameterIndex, rl::RaytracingScene_t SRV)
{
	AddCommand<GPUCommand_SetComputeRootSRV_c<rl::RaytracingScene_t>>(RootParameterIndex, SRV);
}

void GPUContext_s::SetGraphicsRootValue(uint32_t RootParameterIndex, uint32_t OffsetIn32BitValues, uint32_t Value)
{
	AddCommand<GPUCommand_SetGraphicsRootValue>(RootParameterIndex, OffsetIn32BitValues, Value);
}

void GPUContext_s::SetGraphicsRootDescriptorTable(uint32_t RootParameterIndex)
{
	AddCommand<GPUCommand_SetGraphicsRootDescriptorTable_c>(RootParameterIndex);
}

void GPUContext_s::SetComputeRootDescriptorTable(uint32_t RootParameterIndex)
{
	AddCommand<GPUCommand_SetComputeRootDescriptorTable_c>(RootParameterIndex);
}

void GPUContext_s::SetPipelineState(rl::GraphicsPipelineState_t PSO)
{
	AddCommand<GPUCommand_SetPipelineState_c<rl::GraphicsPipelineState_t>>(PSO);
}

void GPUContext_s::SetPipelineState(rl::ComputePipelineState_t PSO)
{
	AddCommand<GPUCommand_SetPipelineState_c<rl::ComputePipelineState_t>>(PSO);
}

void GPUContext_s::SetPipelineState(rl::RaytracingPipelineState_t PSO)
{
	AddCommand<GPUCommand_SetPipelineState_c<rl::RaytracingPipelineState_t>>(PSO);
}

void GPUContext_s::DrawInstanced(uint32_t VertexCountPerInstance, uint32_t InstanceCount, uint32_t StartVertexLocation, uint32_t StartInstanceLocation)
{
	AddCommand<GPUCommand_DrawInstanced_c>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void GPUContext_s::DrawIndexedInstanced(uint32_t IndexCountPerInstance, uint32_t InstanceCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation, uint32_t StartInstanceLocation)
{
	AddCommand<GPUCommand_DrawIndexedInstanced_c>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void GPUContext_s::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
	AddCommand<GPUCommand_Dispatch_c>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void GPUContext_s::DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
	AddCommand<GPUCommand_DispatchMesh_c>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void GPUContext_s::DispatchRays(rl::RaytracingShaderTable_t ShaderTable, uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
	AddCommand<GPUCommand_DispatchRays_c>(ShaderTable, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void GPUContext_s::CopyTexture(rl::Texture_t Dst, rl::Texture_t Src)
{
	AddCommand<GPUCommand_CopyTexture_c>(Dst, Src);
}

void GPUContext_s::TransitionResource(rl::Texture_t Texture, rl::ResourceTransitionState BeforeState, rl::ResourceTransitionState AfterState)
{
	AddCommand<GPUCommand_TransitionResource_c>(Texture, BeforeState, AfterState);
}

void GPUContext_s::ClearRenderTarget(rl::RenderTargetView_t RTV, const float Color[4])
{
	AddCommand<GPUCommand_ClearRenderTarget_c>(RTV, Color);
}

void GPUContext_s::ClearDepth(rl::DepthStencilView_t DSV, float Depth)
{
	AddCommand<GPUCommand_ClearDepth_c>(DSV, Depth);
}

void GPUContext_s::SetVertexBuffer(uint32_t Slot, rl::VertexBuffer_t VertexBuffer, uint32_t Stride, uint32_t Offset)
{
	AddCommand<GPUCommand_SetVertexBuffers_c>(Slot, 1, &VertexBuffer, &Stride, &Offset);
}

void GPUContext_s::SetVertexBuffers(uint32_t StartSlot, uint32_t NumBuffers, const rl::VertexBuffer_t* VertexBuffers, const uint32_t* Strides, const uint32_t* Offsets)
{
	ASSERTMSG(NumBuffers <= 8, "StartSlot must be 8 max");
	AddCommand<GPUCommand_SetVertexBuffers_c>(StartSlot, NumBuffers, VertexBuffers, Strides, Offsets);
}

void GPUContext_s::SetIndexBuffer(rl::IndexBuffer_t Buffer, rl::RenderFormat Format, uint32_t Indexcount)
{
	AddCommand<GPUCommand_SetIndexBuffer_c>(Buffer, Format, Indexcount);
}
