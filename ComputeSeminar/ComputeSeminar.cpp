#include <Render/Render.h>
#include "imgui.h"
#include "ImGui/imgui_impl_render.h"
#include "backends/imgui_impl_win32.h"
#include <SurfClock.h>
#include <SurfMath.h>

#include "Camera/FlyCamera.h"
#include "Noise/Perlin.h"

using namespace tpr;

static struct
{
	uint32_t screenWidth = 0;
	uint32_t screenHeight = 0;

	TexturePtr depthTexture = Texture_t::INVALID;
	DepthStencilViewPtr depthDsv = DepthStencilView_t::INVALID;

	matrix projectionMatrix;
} G;

struct BindVertexBuffer
{
	VertexBufferPtr buf = VertexBuffer_t::INVALID;
	uint32_t stride = 0;
	uint32_t offset = 0;
};

struct BindIndexBuffer
{
	IndexBufferPtr buf = IndexBuffer_t::INVALID;
	RenderFormat format;
	uint32_t offset = 0;
	uint32_t count = 0;
};

struct Mesh
{
	BindVertexBuffer positionBuf;
	BindVertexBuffer colorBuf;
	BindIndexBuffer indexBuf;
};

struct TerrainTileMesh
{
	BindVertexBuffer UVBuf;
	BindIndexBuffer IndexBuf;
};

FlyCamera GCam;

TerrainTileMesh CreateTerrainTileMesh(u32 Resolution)
{
	u32 XSize = Resolution + 1;
	u32 YSize = Resolution + 1;

	float XSep = 1.0f / Resolution;
	float YSep = 1.0f / Resolution;

	std::vector<float2> UVs;
	UVs.resize(XSize * YSize);

	auto UVIter = UVs.begin();

	for (u32 Y = 0; Y < YSize; Y++)
	{
		for (u32 X = 0; X < XSize; X++)
		{
			*UVIter = float2{ X * XSep, Y * YSep };
			UVIter++;
		}
	}

	u32 IndexCount = Resolution * Resolution * 6;

	std::vector<u32> Indices;
	Indices.resize(IndexCount);

	auto IndexIter = Indices.begin();

	for (u32 Y = 0; Y < Resolution; Y++)
	{
		for (u32 X = 0; X < Resolution; X++)
		{			
			*IndexIter = ((Y + 1) * XSize) + X + 0; IndexIter++;
			*IndexIter = (Y * XSize) + X + 1; IndexIter++;
			*IndexIter = (Y * XSize) + X + 0; IndexIter++;

			*IndexIter = ((Y + 1) * XSize) + X + 0; IndexIter++;
			*IndexIter = ((Y + 1) * XSize) + X + 1; IndexIter++;			
			*IndexIter = (Y * XSize) + X + 1; IndexIter++;
		}
	}

	TerrainTileMesh mesh;
	mesh.UVBuf.buf = CreateVertexBuffer(UVs.data(), sizeof(float2) * UVs.size());
	mesh.UVBuf.offset = 0;
	mesh.UVBuf.stride = sizeof(float2);
	mesh.IndexBuf.buf = CreateIndexBuffer(Indices.data(), sizeof(u32) * Indices.size());
	mesh.IndexBuf.count = (u32)Indices.size();
	mesh.IndexBuf.format = RenderFormat::R32_UINT;
	mesh.IndexBuf.offset = 0;

	return mesh;
}

Mesh CreateCubeMesh()
{
	Mesh mesh;

	constexpr float3 pftl = float3(-0.5f, 0.5f, 0.5f);
	constexpr float3 pftr = float3(0.5f, 0.5f, 0.5f);
	constexpr float3 pfbr = float3(0.5f, -0.5f, 0.5f);
	constexpr float3 pfbl = float3(-0.5f, -0.5f, 0.5f);

	constexpr float3 pbtl = float3(-0.5f, 0.5f, -0.5f);
	constexpr float3 pbtr = float3(0.5f, 0.5f, -0.5f);
	constexpr float3 pbbr = float3(0.5f, -0.5f, -0.5f);
	constexpr float3 pbbl = float3(-0.5f, -0.5f, -0.5f);

	float3 posVerts[6 * 4] =
	{
		pftl, pftr, pfbr, pfbl,
		pbtr, pbtl, pbbl, pbbr,
		pftr, pbtr, pbbr, pfbr,
		pbtl, pftl, pfbl, pbbl,
		pfbl, pfbr, pbbr, pbbl,
		pftl, pbtl, pbtr, pftr,
	};

	mesh.positionBuf.buf = CreateVertexBuffer(posVerts, sizeof(posVerts));
	mesh.positionBuf.offset = 0;
	mesh.positionBuf.stride = sizeof(float3);

	constexpr float3 cftl = (pftl + float3(1.0f)) * 0.5f;
	constexpr float3 cftr = (pftr + float3(1.0f)) * 0.5f;
	constexpr float3 cfbr = (pfbr + float3(1.0f)) * 0.5f;
	constexpr float3 cfbl = (pfbl + float3(1.0f)) * 0.5f;
								 			   	
	constexpr float3 cbtl = (pbtl + float3(1.0f)) * 0.5f;
	constexpr float3 cbtr = (pbtr + float3(1.0f)) * 0.5f;
	constexpr float3 cbbr = (pbbr + float3(1.0f)) * 0.5f;
	constexpr float3 cbbl = (pbbl + float3(1.0f)) * 0.5f;

	float3 colVerts[6 * 4] =
	{
		cftl, cftr, cfbr, cfbl,
		cbtr, cbtl, cbbl, cbbr,
		cftr, cbtr, cbbr, cfbr,
		cbtl, cftl, cfbl, cbbl,
		cfbl, cfbr, cbbr, cbbl,
		cftl, cbtl, cbtr, cftr,
	};

	mesh.colorBuf.buf = CreateVertexBuffer(colVerts, sizeof(colVerts));
	mesh.colorBuf.offset = 0;
	mesh.colorBuf.stride = sizeof(float3);

	u16 Indices[6 * 6] =
	{
		2, 1, 0, 0, 3, 2,
		6, 5, 4, 4, 7, 6,
		10, 9, 8, 8, 11, 10,
		14, 13, 12, 12, 15, 14,
		18, 17, 16, 16, 19, 18,
		22, 21, 20, 20, 23, 22,
	};

	mesh.indexBuf.buf = CreateIndexBuffer(Indices, sizeof(Indices));
	mesh.indexBuf.count = ARRAYSIZE(Indices);
	mesh.indexBuf.offset = 0;
	mesh.indexBuf.format = RenderFormat::R16_UINT;

	return mesh;
}

GraphicsPipelineStatePtr CreateMeshPSO()
{
	VertexShader_t meshVS = CreateVertexShader("Shaders/Mesh.hlsl");
	PixelShader_t meshPS = CreatePixelShader("Shaders/Mesh.hlsl");

	GraphicsPipelineStateDesc psoDesc = {};
	psoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
		.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
		.TargetBlendDesc({ RenderFormat::R8G8B8A8_UNORM }, { BlendMode::Default() }, RenderFormat::D16_UNORM)
		.VertexShader(meshVS)
		.PixelShader(meshPS);

	InputElementDesc inputDesc[] =
	{
		{"POSITION", 0, RenderFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PER_VERTEX, 0 },
		{"COLOR", 0, RenderFormat::R32G32B32_FLOAT, 1, 0, InputClassification::PER_VERTEX, 0 },
	};

	return CreateGraphicsPipelineState(psoDesc, inputDesc, ARRAYSIZE(inputDesc));
}

GraphicsPipelineStatePtr CreateTerrainPSO()
{
	VertexShader_t TerrainVS = CreateVertexShader("Shaders/Terrain.hlsl");
	PixelShader_t TerrainPS = CreatePixelShader("Shaders/Terrain.hlsl");

	GraphicsPipelineStateDesc PSODesc = {};
	PSODesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
		.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
		.TargetBlendDesc({ RenderFormat::R8G8B8A8_UNORM }, { BlendMode::Default() }, RenderFormat::D16_UNORM)
		.VertexShader(TerrainVS)
		.PixelShader(TerrainPS);

	InputElementDesc inputDesc[] =
	{
		{"TEXCOORD", 0, RenderFormat::R32G32_FLOAT, 0, 0, InputClassification::PER_VERTEX, 0 },
	};

	return CreateGraphicsPipelineState(PSODesc, inputDesc, ARRAYSIZE(inputDesc));
}

void CreateNoiseTexture(u32 Dim, TexturePtr& OutTex, ShaderResourceViewPtr& OutSrv)
{
	std::vector<float> Data;
	Data.resize(Dim * Dim);

	double rcp = 10.0 / (double)Dim;
	for (u32 y = 0; y < Dim; y++)
	{
		for (u32 x = 0; x < Dim; x++)
		{
			double Noise = PerlinNoise2D((double)x * rcp, (double)y * rcp, 2.0, 2.0, 4);
			Noise += 1.0;
			Noise *= 0.5;
			Data[y * Dim + x] = Noise;
		}
	}
	
	MipData Mip(Data.data(), RenderFormat::R32_FLOAT, Dim, Dim);
	TextureCreateDesc Desc = {};
	Desc.Data = &Mip;
	Desc.DebugName = L"NoiseTex";
	Desc.Flags = RenderResourceFlags::SRV;
	Desc.Format = RenderFormat::R32_FLOAT;
	Desc.Height = Dim;
	Desc.Width = Dim;

	OutTex = CreateTexture(Desc);
	OutSrv = CreateTextureSRV(OutTex, RenderFormat::R32_FLOAT, TextureDimension::TEX2D, 1u, 1u);
}

void ResizeScreen(uint32_t width, uint32_t height)
{
	width = Max(width, 1u);
	height = Max(height, 1u);

	if (G.screenWidth == width && G.screenHeight == height)
	{
		return;
	}

	G.screenWidth = width;
	G.screenHeight = height;

	RenderRelease(G.depthTexture);
	RenderRelease(G.depthDsv);

	TextureCreateDescEx depthDesc = {};
	depthDesc.DebugName = L"SceneDepth";
	depthDesc.Flags = RenderResourceFlags::DSV;
	depthDesc.ResourceFormat = RenderFormat::D16_UNORM;
	depthDesc.Height = G.screenHeight;
	depthDesc.Width = G.screenWidth;
	depthDesc.InitialState = ResourceTransitionState::DEPTH_WRITE;
	depthDesc.Dimension = TextureDimension::TEX2D;
	G.depthTexture = CreateTextureEx(depthDesc);
	G.depthDsv = CreateTextureDSV(G.depthTexture, RenderFormat::D16_UNORM, TextureDimension::TEX2D, 1u);

	constexpr float fovRad = ConvertToRadians(45.0f);

	float aspectRatio = (float)G.screenWidth / (float)G.screenHeight;
	
	G.projectionMatrix = MakeMatrixPerspectiveFovLH(fovRad, aspectRatio, 0.01f, 100.0f);

	GCam.Resize(G.screenWidth, G.screenHeight);
}

enum RootSigSlots
{
	RS_VIEW_BUF,
	RS_MESH_BUF,
	RS_SRV_TABLE,
	RS_COUNT,
};

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int main()
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "Render Example", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "Render Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	RenderInitParams params;
#ifdef _DEBUG
	params.DebugEnabled = true;
#else
	params.DebugEnabled = false;
#endif
	params.RootSigDesc.Flags = RootSignatureFlags::ALLOW_INPUT_LAYOUT;
	params.RootSigDesc.Slots.resize(RS_COUNT);
	params.RootSigDesc.Slots[RS_VIEW_BUF] = RootSignatureSlot::CBVSlot(0, 0);
	params.RootSigDesc.Slots[RS_MESH_BUF] = RootSignatureSlot::CBVSlot(1, 0);
	params.RootSigDesc.Slots[RS_SRV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, tpr::RootSignatureDescriptorTableType::SRV);

	params.RootSigDesc.GlobalSamplers.resize(1);
	params.RootSigDesc.GlobalSamplers[0].AddressModeUVW(SamplerAddressMode::WRAP).FilterModeMinMagMip(SamplerFilterMode::LINEAR);

	if (!Render_Init(params))
	{
		Render_ShutDown();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	RenderViewPtr view = CreateRenderViewPtr((intptr_t)hwnd);

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplRender_Init(RenderFormat::R8G8B8A8_UNORM);

	Mesh mesh = CreateCubeMesh();

	const u32 TileMeshDim = 128;
	TerrainTileMesh tile = CreateTerrainTileMesh(TileMeshDim);

	TexturePtr NoiseTex = {};
	ShaderResourceViewPtr NoiseSrv = {};

	CreateNoiseTexture(512, NoiseTex, NoiseSrv);

	matrix viewMatrix = MakeMatrixLookAtLH(float3{ 0.0f, 0.0f, -2.0f }, float3{ 0.0f, 0.0f, 0.0f }, float3(0.0f, 1.0f, 0.0f));

	GraphicsPipelineStatePtr meshPSO = CreateMeshPSO();

	GraphicsPipelineStatePtr terrainPSO = CreateTerrainPSO();

	float rot = 0.0f;

	SurfClock Clock = {};

	// Main loop
	bool bQuit = false;
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (bQuit == false && msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		Clock.Tick();

		GCam.UpdateView(Clock.GetDeltaSeconds());

		Render_BeginFrame();

		{
			ImGui_ImplRender_NewFrame();

			ImGui_ImplWin32_NewFrame();

			ImGui::NewFrame();

			ImGui::Render();
		}

		struct
		{
			matrix viewProjection;
		} viewConsts;

		viewConsts.viewProjection = GCam.GetView() * GCam.GetProjection();

		DynamicBuffer_t viewCbuf = CreateDynamicConstantBuffer(&viewConsts, sizeof(viewConsts));

		const bool bDrawMesh = false;

		struct
		{
			matrix transform;
		} meshConsts;

		meshConsts.transform = MakeMatrixRotationAxis(float3(1, 1, 1), rot);

		DynamicBuffer_t meshCbuf = CreateDynamicConstantBuffer(&meshConsts, sizeof(meshConsts));

		rot += 0.01f;

		struct
		{
			float2 Offset;
			float2 Scale;
			u32 NoiseTex;
			float Height;
			float CellSize;
			float _Pad;
		} terrainTileConstants;

		const float Scale = 100.0f;

		terrainTileConstants.Offset = { 0.f };
		terrainTileConstants.Scale = { Scale };
		terrainTileConstants.NoiseTex = GetDescriptorIndex(NoiseSrv);
		terrainTileConstants.Height = 10.0f;
		terrainTileConstants.CellSize = (1.0f / (float)TileMeshDim);

		DynamicBuffer_t terrainCbuf = CreateDynamicConstantBuffer(&terrainTileConstants, sizeof(terrainTileConstants));

		Render_BeginRenderFrame();

		CommandListSubmissionGroup clGroup(CommandListType::GRAPHICS);

		CommandList* cl = clGroup.CreateCommandList();

		UploadBuffers(cl);

		cl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::PRESENT, ResourceTransitionState::RENDER_TARGET);

		cl->SetRootSignature();

		// Bind and clear targets
		{
			RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();

			constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.2f, 0.0f };

			cl->ClearRenderTarget(backBufferRtv, DefaultClearCol);
			cl->ClearDepth(G.depthDsv, 1.0f);

			cl->SetRenderTargets(&backBufferRtv, 1, G.depthDsv);
		}

		// Init viewport and scissor
		{
			Viewport vp{ G.screenWidth, G.screenHeight};
			cl->SetViewports(&vp, 1);
			cl->SetDefaultScissor();
		}

		// Bind draw buffers
		{
			if (Render_IsBindless())
			{
				cl->SetGraphicsRootCBV(0, viewCbuf);

				cl->SetGraphicsRootDescriptorTable(RS_SRV_TABLE);
			}
			else
			{
				cl->BindVertexCBVs(0, 1, &viewCbuf);
			}
		}

		// Draw mesh
		if(bDrawMesh)
		{
			cl->SetPipelineState(meshPSO);

			if (Render_IsBindless())
			{
				cl->SetGraphicsRootCBV(1, meshCbuf);
			}
			else
			{
				cl->BindVertexCBVs(1, 1, &meshCbuf);
			}

			cl->SetVertexBuffers(0, 1, &mesh.positionBuf.buf, &mesh.positionBuf.stride, &mesh.positionBuf.offset);
			cl->SetVertexBuffers(1, 1, &mesh.colorBuf.buf, &mesh.colorBuf.stride, &mesh.colorBuf.offset);
			cl->SetIndexBuffer(mesh.indexBuf.buf, mesh.indexBuf.format, mesh.indexBuf.offset);
			cl->DrawIndexedInstanced(mesh.indexBuf.count, 1, 0, 0, 0);
		}

		// Draw terrain
		{
			cl->SetPipelineState(terrainPSO);
			
			if (Render_IsBindless())
			{
				cl->SetGraphicsRootCBV(1, terrainCbuf);
			}
			else
			{
				cl->BindVertexCBVs(1, 1, &terrainCbuf);
				cl->BindVertexSRVs(0, 1, &NoiseSrv);
				cl->BindPixelSRVs(0, 1, &NoiseSrv);
			}

			cl->SetVertexBuffers(0, 1, &tile.UVBuf.buf, &tile.UVBuf.stride, &tile.UVBuf.offset);
			cl->SetIndexBuffer(tile.IndexBuf.buf, tile.IndexBuf.format, tile.IndexBuf.offset);
			cl->DrawIndexedInstanced(tile.IndexBuf.count, 1u, 0u, 0u, 0u);
		}

		cl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::RENDER_TARGET, ResourceTransitionState::PRESENT);

		clGroup.Submit();

		Render_EndFrame();

		view->Present(true);
	}

	ImGui_ImplRender_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Render_ShutDown();

	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	RenderView* rv = GetRenderViewForHwnd((intptr_t)hWnd);

	switch (msg)
	{
	//case WM_MOVE:
	//{
	//	RECT r;
	//	GetWindowRect(hWnd, &r);
	//	const int x = (int)(r.left);
	//	const int y = (int)(r.top);
	//	break;
	//}
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			const int w = (int)LOWORD(lParam);
			const int h = (int)HIWORD(lParam);

			if (rv)	rv->Resize(w, h);
			ResizeScreen(w, h);
			return 0;
		}

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
