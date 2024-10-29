#include <Render/Render.h>
#include "imgui.h"
#include "ImGui/imgui_impl_render.h"
#include "backends/imgui_impl_win32.h"
#include <SurfClock.h>
#include <SurfMath.h>

#include <cstdlib>

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

Mesh CreateSphereMesh(uint32_t Slices, uint32_t Stacks)
{
	Stacks = Max(Stacks, 1u);

	Mesh Outmesh;

	std::vector<float3> Positions;

	Positions.emplace_back(0.0f, 1.0f, 0.0f);

	for (uint32_t i = 0; i < Stacks - 1; i++)
	{
		float Phi = K_PI * float(i + 1) / float(Stacks);
		for (int j = 0; j < Slices; j++)
		{
			float Theta = 2.0f * K_PI * float(j) / float(Slices);
			float x = sinf(Phi) * cosf(Theta);
			float y = cosf(Phi);
			float z = sinf(Phi) * sinf(Theta);
			Positions.emplace_back(x, y, z);
		}
	}

	uint32_t v0 = 0;
	uint32_t v1 = (uint32_t)Positions.size();

	Positions.emplace_back(0.0f, -1.0f, 0.0f);

	std::vector<uint32_t> Indices;
	for (uint32_t i = 0; i < Slices; i++)
	{
		uint32_t i0 = i + 1;
		uint32_t i1 = (i + 1) % Slices + 1; // 03456 723 723
		Indices.push_back(v0);
		Indices.push_back(i1);
		Indices.push_back(i0);
		i0 = i + Slices * (Stacks - 2) + 1;
		i1 = (i + 1) % Slices + Slices * (Stacks - 2) + 1;
		Indices.push_back(v1);
		Indices.push_back(i0);
		Indices.push_back(i1);
	}

	for (uint32_t j = 0; j < Stacks - 2; j++)
	{
		uint32_t j0 = j * Slices + 1;
		uint32_t j1 = (j + 1) * Slices + 1;
		for (uint32_t i = 0; i < Slices; i++)
		{
			uint32_t i0 = j0 + i;
			uint32_t i1 = j0 + (i + 1) % Slices;
			uint32_t i2 = j1 + (i + 1) % Slices;
			uint32_t i3 = j1 + i;
			Indices.push_back(i0);
			Indices.push_back(i1);
			Indices.push_back(i2);
			Indices.push_back(i2);
			Indices.push_back(i3);
			Indices.push_back(i0);
		}
	}

	Outmesh.positionBuf.buf = CreateVertexBuffer(Positions.data(), Positions.size() * sizeof(float3));
	Outmesh.positionBuf.offset = 0;
	Outmesh.positionBuf.stride = sizeof(float3);

	Outmesh.indexBuf.buf = CreateIndexBuffer(Indices.data(), Indices.size() * sizeof(uint32_t));
	Outmesh.indexBuf.count = Indices.size();
	Outmesh.indexBuf.format = RenderFormat::R32_UINT;
	Outmesh.indexBuf.offset = 0;

	return Outmesh;
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
	};

	return CreateGraphicsPipelineState(psoDesc, inputDesc, ARRAYSIZE(inputDesc));
}

GraphicsPipelineStatePtr CreateTerrainPSO()
{
	static const VertexShader_t TerrainVS = CreateVertexShader("Shaders/Terrain.hlsl");
	static const PixelShader_t TerrainPS = CreatePixelShader("Shaders/Terrain.hlsl");

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

ComputePipelineStatePtr CreateBallComputePSO()
{
	static const ComputeShader_t BallCS = CreateComputeShader("Shaders/BallMovementCompute.hlsl");
	ComputePipelineStateDesc PSODesc = {};
	PSODesc.Cs = BallCS;
	PSODesc.DebugName = L"BallMovementComputeCS";

	return CreateComputePipelineState(PSODesc);
}

std::vector<float> GetNoiseData(u32 Dim)
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
			Data[y * Dim + x] = static_cast<float>(Noise);
		}
	}

	return Data;
}

void CreateNoiseTexture(u32 Dim, const float* const DataPtr, TexturePtr& OutTex, ShaderResourceViewPtr& OutSrv)
{	
	MipData Mip(DataPtr, RenderFormat::R32_FLOAT, Dim, Dim);
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

void RandHash(uint32_t& Seed) {
	Seed ^= 2747636419u;
	Seed *= 2654435769u;
	Seed ^= Seed >> 16;
	Seed *= 2654435769u;
	Seed ^= Seed >> 16;
	Seed *= 2654435769u;
}
uint32_t InitRandomGenerator(const float3& Position, uint32_t FrameID)
{
	return uint32_t(Position.z * 400 + Position.y * 200 + Position.z) + uint32_t(FrameID);
}

float PseudoRand(uint32_t Seed) {
	RandHash(Seed);
	return float(Seed) / 4294967295.0;
}

float RandFloat()
{
	return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float RandFloatInRange(float Min, float Max)
{
	float R = RandFloat();
	return Min + R * (Max - Min);
}

float3 RandFloat3()
{
	return float3{ RandFloat(), RandFloat(), RandFloat() };
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
	RS_UAV_TABLE,
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
	params.RootSigDesc.Slots[RS_UAV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, tpr::RootSignatureDescriptorTableType::UAV);

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

	Mesh mesh = CreateSphereMesh(16, 16);

	const u32 TileMeshDim = 128;
	TerrainTileMesh tile = CreateTerrainTileMesh(TileMeshDim);

	TexturePtr NoiseTex = {};
	ShaderResourceViewPtr NoiseSrv = {};

	const uint32_t NoiseDim = 512;
	std::vector<float> NoiseData = GetNoiseData(NoiseDim);
	CreateNoiseTexture(NoiseDim, NoiseData.data(), NoiseTex, NoiseSrv);

	matrix viewMatrix = MakeMatrixLookAtLH(float3{ 0.0f, 0.0f, -2.0f }, float3{ 0.0f, 0.0f, 0.0f }, float3(0.0f, 1.0f, 0.0f));

	GraphicsPipelineStatePtr meshPSO = CreateMeshPSO();
	GraphicsPipelineStatePtr terrainPSO = CreateTerrainPSO();
	ComputePipelineStatePtr ballComputePSO = CreateBallComputePSO();

	struct Ball
	{
		float3 Position;
		float Scale;
		float3 Color;
		float Bounciness;
		float3 Velocity;
		float __pad;
	};

	std::vector<Ball> Balls;
	for (uint32_t y = 5; y < 100; y += 2)
	{
		for (uint32_t x = 5; x < 100; x += 2)
		{
			Ball NewBall;
			NewBall.Position = float3((float)x, 20.0f, (float)y);
			NewBall.Scale = RandFloatInRange(0.3f, 1.2f);
			NewBall.Color = RandFloat3();
			NewBall.Bounciness = RandFloatInRange(0.5f, 0.9f);
			NewBall.Velocity = float3{ 0 };

			Balls.push_back(std::move(NewBall));
		}
	}

	const uint32_t BallCount = (uint32_t)Balls.size();
	const float TerrainScale = 100.0f;
	const float TerrainHeight = 10.0f;
	const float Gravity = -2.0f;

	const bool UseCompute = true;

	struct
	{
		uint32_t NoiseTexSRVIndex;
		uint32_t BallDataUAVIndex;
		uint32_t BallDataSRVIndex;
		uint32_t FrameID;
		float TerrainScale;
		float TerrainHeight;
		float DeltaSeconds;
		float Gravity;
		uint32_t NumBalls;
		float DragCoefficient;
		uint32_t NoiseDim;
		float __pad;
	} ComputeSceneData;

	StructuredBufferPtr BallDataBuffer = CreateStructuredBuffer(Balls.data(), BallCount * sizeof(Ball), sizeof(Ball), RenderResourceFlags::SRV | RenderResourceFlags::UAV);
	ShaderResourceViewPtr BallDataSRV = CreateStructuredBufferSRV(BallDataBuffer, 0u, BallCount, (uint32_t)sizeof(Ball));
	UnorderedAccessViewPtr BallDataUAV = CreateStructuredBufferUAV(BallDataBuffer, 0u, BallCount, (uint32_t)sizeof(Ball));

	ComputeSceneData.NoiseTexSRVIndex = GetDescriptorIndex(NoiseSrv);
	ComputeSceneData.BallDataUAVIndex = GetDescriptorIndex(BallDataUAV);
	ComputeSceneData.BallDataSRVIndex = GetDescriptorIndex(BallDataSRV);
	ComputeSceneData.TerrainScale = TerrainScale;
	ComputeSceneData.TerrainHeight = TerrainHeight;
	ComputeSceneData.Gravity = Gravity;
	ComputeSceneData.NumBalls = BallCount;
	ComputeSceneData.DragCoefficient = 0.001f;
	ComputeSceneData.NoiseDim = NoiseDim;

	SurfClock Clock = {};

	uint32_t FrameID = 0u;

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

		FrameID++;

		Clock.Tick();

		const float DeltaSeconds = Clock.GetDeltaSeconds();

		GCam.UpdateView(DeltaSeconds);

		if (!UseCompute)
		{
			for (Ball& ball : Balls)
			{
				const float VelocityMagSqr = LengthSqrF3(ball.Velocity);

				ball.Velocity += float3(0, Gravity * DeltaSeconds, 0) + -SignF3(ball.Velocity) * VelocityMagSqr * 0.001f;

				ball.Position += ball.Velocity * DeltaSeconds;

				auto TerrainCoordForPosition = [&](const float3& Pos)
				{
					float u = Clamp(Pos.x / TerrainScale, 0.0f, 1.0f);
					float v = Clamp(Pos.z / TerrainScale, 0.0f, 1.0f);

					uint32_t uCoord = Min(static_cast<uint32_t>(u * NoiseDim), NoiseDim - 1u);
					uint32_t vCoord = Min(static_cast<uint32_t>(v * NoiseDim), NoiseDim - 1u);

					return uint2(uCoord, vCoord);
				};

				// Gather pixels directly under ball
				float radius = ball.Scale;

				uint2 topLeftCoord = TerrainCoordForPosition(float3(ball.Position.x - radius, 0.0f, ball.Position.z - radius));
				uint2 bottomRightCoord = TerrainCoordForPosition(float3(ball.Position.x + radius, 0.0f, ball.Position.z + radius));

				auto GetHit = [&]()
				{
					float3 bounceDir = 0;
					for (u32 y = topLeftCoord.y; y <= bottomRightCoord.y; y++)
					{
						for (u32 x = topLeftCoord.x; x <= bottomRightCoord.x; x++)
						{
							float height = NoiseData[y * NoiseDim + x] * TerrainHeight;
							float3 samplePos = float3(((float)x / (float)NoiseDim) * TerrainScale, height, ((float)y / (float)NoiseDim) * TerrainScale);

							float3 direction = ball.Position - samplePos;
							float penetration = ball.Scale * ball.Scale - LengthSqrF3(direction);
							if (penetration > 0.0f)
							{
								bounceDir += direction * penetration;
							}
						}
					}

					return NormalizeF3(bounceDir);
				};
				ball.Velocity += GetHit() * LengthF3(ball.Velocity) * ball.Bounciness;
			}

			UpdateStructuredBuffer(BallDataBuffer, Balls.data(), Balls.size() * sizeof(Ball));
		}

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

		const bool bDrawMesh = true;

		std::vector<DynamicBuffer_t> MeshBuffers;
		MeshBuffers.reserve(Balls.size());

		for (const Ball& b : Balls)
		{
			struct
			{
				matrix transform;
				float3 Color;
				float __pad;
			} meshConsts;

			meshConsts.transform = MakeMatrixTranslation(b.Position) * MakeMatrixScaling(b.Scale, b.Scale, b.Scale);
			meshConsts.Color = b.Color;

			MeshBuffers.push_back(CreateDynamicConstantBuffer(&meshConsts, sizeof(meshConsts)));
		}

		struct
		{
			float2 Offset;
			float2 Scale;
			u32 NoiseTex;
			float Height;
			float CellSize;
			float _Pad;
		} terrainTileConstants;

		terrainTileConstants.Offset = { 0.f };
		terrainTileConstants.Scale = { TerrainScale };
		terrainTileConstants.NoiseTex = GetDescriptorIndex(NoiseSrv);
		terrainTileConstants.Height = 10.0f;
		terrainTileConstants.CellSize = (1.0f / (float)TileMeshDim);

		DynamicBuffer_t terrainCbuf = CreateDynamicConstantBuffer(&terrainTileConstants, sizeof(terrainTileConstants));

		DynamicBuffer_t sceneCBuf;
		ComputeSceneData.DeltaSeconds = DeltaSeconds;
		ComputeSceneData.FrameID = FrameID;
		sceneCBuf = CreateDynamicConstantBuffer(&ComputeSceneData, sizeof(ComputeSceneData));

		Render_BeginRenderFrame();

		CommandListSubmissionGroup clGroup(CommandListType::GRAPHICS);

		CommandList* cl = clGroup.CreateCommandList();

		UploadBuffers(cl);

		cl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::PRESENT, ResourceTransitionState::RENDER_TARGET);

		cl->SetRootSignature();

		// Trigger compute work as early as possible
		if (UseCompute)
		{
			cl->TransitionResource(BallDataBuffer, ResourceTransitionState::READ, ResourceTransitionState::UNORDERED_ACCESS);
			cl->TransitionResource(NoiseTex, ResourceTransitionState::ALL_SHADER_RESOURCE, ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE);

			cl->SetComputeRootDescriptorTable(RS_SRV_TABLE);
			cl->SetComputeRootDescriptorTable(RS_UAV_TABLE);
			cl->SetComputeRootCBV(RS_VIEW_BUF, sceneCBuf);

			cl->SetPipelineState(ballComputePSO);

			cl->Dispatch(DivideRoundUp(BallCount, 128u), 1u, 1u);

			cl->TransitionResource(NoiseTex, ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE, ResourceTransitionState::ALL_SHADER_RESOURCE);
			cl->TransitionResource(BallDataBuffer, ResourceTransitionState::UNORDERED_ACCESS, ResourceTransitionState::ALL_SHADER_RESOURCE);
		}

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
				cl->SetGraphicsRootCBV(1, sceneCBuf);

				cl->SetGraphicsRootDescriptorTable(RS_SRV_TABLE);
			}
			else
			{
				cl->BindVertexCBVs(0, 1, &viewCbuf);
			}
		}

		// Draw mesh
		cl->SetPipelineState(meshPSO);
		cl->SetVertexBuffers(0, 1, &mesh.positionBuf.buf, &mesh.positionBuf.stride, &mesh.positionBuf.offset);
		cl->SetIndexBuffer(mesh.indexBuf.buf, mesh.indexBuf.format, mesh.indexBuf.offset);
		cl->DrawIndexedInstanced(mesh.indexBuf.count, BallCount, 0, 0, 0);

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

		if (UseCompute)
		{
			cl->TransitionResource(BallDataBuffer, ResourceTransitionState::ALL_SHADER_RESOURCE, ResourceTransitionState::READ);
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
