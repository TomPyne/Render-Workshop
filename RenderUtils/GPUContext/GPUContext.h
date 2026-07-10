#pragma once

#include <Render/RenderTypes.h>

class GPUCommand_c;
class GPUPass_c;

struct GPUContext_s
{
private:
	std::vector<GPUCommand_c*> Commands;
	std::vector<GPUPass_c*> Passes;

	template<class CommandType, class... Args>
	void AddCommand(Args&&... args)
	{
		// Allocation strategy could be better if we alloc in chunks
		Commands.push_back(new CommandType(std::forward<Args>(args)...));
	}

	void AddPass(uint32_t StartCommandIndex, uint32_t CommandCount);

	int32_t CurrentPassIndex = -1;

public:

	~GPUContext_s();

	void Execute(rl::CommandListSubmissionGroup* CLGroup);

	void BeginPass(); // Perhaps pass a name or some sort of identifier for debugging purposes?
	void EndPass();

	void SetRootSignature();
	void SetRootSignature(rl::RootSignature_t RootSignature);
	void SetComputeRootSignature(rl::RootSignature_t RootSignature);
	void SetRenderTargets(rl::RenderTargetView_t* RTVs, size_t NumRTVs, rl::DepthStencilView_t DSV);
	void SetViewports(rl::Viewport* Viewports, size_t NumViewports);
	void SetDefaultScissor();
	void SetScissorRects(rl::ScissorRect* ScissorRects, size_t NumScissorRects);
	void SetGraphicsRootCBV(uint32_t RootParameterIndex, rl::ConstantBuffer_t CBV);
	void SetGraphicsRootCBV(uint32_t RootParameterIndex, rl::DynamicBuffer_t CBV);
	void SetComputeRootCBV(uint32_t RootParameterIndex, rl::ConstantBuffer_t CBV);
	void SetComputeRootCBV(uint32_t RootParameterIndex, rl::DynamicBuffer_t CBV);
	void SetComputeRootSRV(uint32_t RootParameterIndex, rl::RaytracingScene_t SRV);
	void SetGraphicsRootValue(uint32_t RootParameterIndex, uint32_t OffsetIn32BitValues, uint32_t Value);
	void SetGraphicsRootDescriptorTable(uint32_t RootParameterIndex);
	void SetComputeRootDescriptorTable(uint32_t RootParameterIndex);
	void SetPipelineState(rl::GraphicsPipelineState_t PSO);
	void SetPipelineState(rl::ComputePipelineState_t PSO);
	void SetPipelineState(rl::RaytracingPipelineState_t PSO);
	void DrawInstanced(uint32_t VertexCountPerInstance, uint32_t InstanceCount, uint32_t StartVertexLocation, uint32_t StartInstanceLocation);
	void DrawIndexedInstanced(uint32_t IndexCountPerInstance, uint32_t InstanceCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation, uint32_t StartInstanceLocation);
	void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);
	void DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);
	void DispatchRays(rl::RaytracingShaderTable_t ShaderTable, uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);
	void CopyTexture(rl::Texture_t Dst, rl::Texture_t Src);
	void TransitionResource(rl::Texture_t Texture, rl::ResourceTransitionState BeforeState, rl::ResourceTransitionState AfterState);
	void RWBarrier(rl::Texture_t Texture);
	void ClearRenderTarget(rl::RenderTargetView_t RTV, const float Color[4]);
	void ClearDepth(rl::DepthStencilView_t DSV, float Depth);
	void SetVertexBuffer(uint32_t Slot, rl::VertexBuffer_t VertexBuffer, uint32_t Stride, uint32_t Offset);
	void SetVertexBuffers(uint32_t StartSlot, uint32_t NumBuffers, const rl::VertexBuffer_t* VertexBuffers, const uint32_t* Strides, const uint32_t* Offsets);
	void SetIndexBuffer(rl::IndexBuffer_t Buffer, rl::RenderFormat Format, uint32_t Indexcount);
};