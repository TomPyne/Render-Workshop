#include "ComputeSeminarApp.h"

#include "Camera/FlyCamera.h"
#include "Noise/Perlin.h"

#include <Render/Render.h>

using namespace rl;

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

struct Ball
{
	float3 Position;
	float Scale;
	float3 Color;
	float Bounciness;
	float3 Velocity;
	float __pad;
};

static struct
{
	uint32_t ScreenWidth = 0;
	uint32_t ScreenHeight = 0;

	TexturePtr DepthTexture = Texture_t::INVALID;
	DepthStencilViewPtr DepthDSV = DepthStencilView_t::INVALID;

	FlyCamera Cam;

	Mesh SphereMesh;
	TerrainTileMesh TerrainMesh;

	GraphicsPipelineStatePtr MeshPSO;
	GraphicsPipelineStatePtr TerrainPSO;
	ComputePipelineStatePtr BallComputePSO;
	ComputePipelineStatePtr ClearArgsPSO;

	TexturePtr HeightTexture;
	ShaderResourceViewPtr HeightTextureSRV;

	TexturePtr NormalTexture;
	ShaderResourceViewPtr NormalTextureSRV;

	StructuredBufferPtr BallDataBuffer;
	ShaderResourceViewPtr BallDataBufferSRV;
	UnorderedAccessViewPtr BallDataBufferUAV;

	StructuredBufferPtr BallIndexBuffer;
	ShaderResourceViewPtr BallIndexBufferSRV;
	UnorderedAccessViewPtr BallIndexBufferUAV;

	StructuredBuffer_t IndirectDrawBuffer;
	UnorderedAccessViewPtr IndirectDrawBufferUAV;

	IndirectCommand_t IndirectDrawIndexedCommand;

	uint32_t BallsToDraw;

	std::vector<float> HeightData;
	std::vector<float3> NormalData;

	std::vector<Ball> Balls;
	std::vector<uint32_t> BallIndices;
	uint32_t BallCount;

	const uint32_t TileMeshDim = 128u;
	const uint32_t NoiseDim = 512u;
	const float TerrainScale = 100.0f;
	const float TerrainHeight = 10.0f;
	const float Gravity = -2.0f;

	const bool UseCompute = true;
} G;

struct ComputeSceneData
{
	uint32_t FrameID;
	float TerrainScale;
	float TerrainHeight;
	float DeltaSeconds;

	float Gravity;
	uint32_t NumBalls;
	float DragCoefficient;
	uint32_t NoiseDim;

	uint32_t NoiseTexSRVIndex;
	uint32_t BallDataUAVIndex;
	uint32_t BallDataSRVIndex;
	uint32_t BallIndexUAVIndex;

	uint32_t BallIndexSRVIndex;
	uint32_t IndirectDrawUAVIndex;
	float __pad[2];

	float4 FrustumPlanes[Frustum::COUNT];
};

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
	mesh.UVBuf.buf = CreateVertexBufferFromArray(UVs.data(), UVs.size());
	mesh.UVBuf.offset = 0;
	mesh.UVBuf.stride = sizeof(float2);
	mesh.IndexBuf.buf = CreateIndexBufferFromArray(Indices.data(), Indices.size());
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
		for (uint32_t j = 0; j < Slices; j++)
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

	Outmesh.positionBuf.buf = CreateVertexBufferFromArray(Positions.data(), Positions.size());
	Outmesh.positionBuf.offset = 0;
	Outmesh.positionBuf.stride = sizeof(float3);

	Outmesh.indexBuf.buf = CreateIndexBufferFromArray(Indices.data(), Indices.size() * sizeof(uint32_t));
	Outmesh.indexBuf.count = (uint32_t)Indices.size();
	Outmesh.indexBuf.format = RenderFormat::R32_UINT;
	Outmesh.indexBuf.offset = 0;

	return Outmesh;
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

ComputePipelineStatePtr CreateClearArgsPSO()
{
	static const ComputeShader_t ClearCS = CreateComputeShader("Shaders/ClearIndirectDrawIndexed.hlsl");
	ComputePipelineStateDesc PSODesc = {};
	PSODesc.Cs = ClearCS;
	PSODesc.DebugName = L"ClearIndirectDrawIndexedCS";

	return CreateComputePipelineState(PSODesc);
}

void GetNoiseData(u32 Dim, float XZScale, float YScale, std::vector<float>& HeightData, std::vector<float3>& NormalData)
{
	HeightData.resize(Dim * Dim);
	NormalData.resize(Dim * Dim);

	auto Height = [&HeightData, Dim](u32 x, u32 y)->float&
	{
		return HeightData[y * Dim + x];
	};

	double rcp = 10.0 / (double)Dim;
	for (u32 y = 0; y < Dim; y++)
	{
		for (u32 x = 0; x < Dim; x++)
		{
			double Noise = PerlinNoise2D((double)x * rcp, (double)y * rcp, 2.0, 2.0, 4);
			Noise += 1.0;
			Noise *= 0.5;
			Height(x, y) = powf(static_cast<float>(Noise), 4.0f);
		}
	}

	for (u32 y = 0; y < Dim; y++)
	{
		for (u32 x = 0; x < Dim; x++)
		{
			float sx = Height(x < Dim - 1 ? x + 1 : x, y) - Height(x > 0 ? x - 1 : x, y);
			if (x == 0 || x == Dim - 1)
				sx *= 2;

			float sy = Height(x, y < Dim - 1 ? y + 1 : y) - Height(x, y > 0 ? y - 1 : y);
			if (y == 0 || y == Dim - 1)
				sy *= 2;

			NormalData[y * Dim + x] = Normalize(float3(-sx * YScale, 2.0f * 0.01f, sy * YScale));
		}
	}
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

void CreateNoiseNormalTexture(u32 Dim, const float3* const DataPtr, TexturePtr& OutTex, ShaderResourceViewPtr& OutSrv)
{
	MipData Mip(DataPtr, RenderFormat::R32G32B32_FLOAT, Dim, Dim);
	TextureCreateDesc Desc = {};
	Desc.Data = &Mip;
	Desc.DebugName = L"NormalTex";
	Desc.Flags = RenderResourceFlags::SRV;
	Desc.Format = RenderFormat::R32G32B32_FLOAT;
	Desc.Height = Dim;
	Desc.Width = Dim;

	OutTex = CreateTexture(Desc);
	OutSrv = CreateTextureSRV(OutTex, RenderFormat::R32G32B32_FLOAT, TextureDimension::TEX2D, 1u, 1u);
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

void InitBallArray()
{
	G.Balls.clear();
	G.BallIndices.clear();

	const uint32_t StartGrid = 5u;
	const uint32_t EndGrid = 100u;
	for (uint32_t y = StartGrid; y < EndGrid; y += 2)
	{
		for (uint32_t x = StartGrid; x < EndGrid; x += 2)
		{
			Ball NewBall;
			NewBall.Position = float3((float)x, 20.0f, (float)y);
			NewBall.Scale = RandFloatInRange(0.1f, 0.4f);
			NewBall.Color = RandFloat3();
			NewBall.Bounciness = RandFloatInRange(0.5f, 0.9f);
			NewBall.Velocity = float3{ 0 };

			G.Balls.push_back(std::move(NewBall));

			G.BallIndices.push_back(static_cast<uint32_t>(G.BallIndices.size()));
		}
	}

	G.BallCount = (uint32_t)G.Balls.size();
}

enum RootSigSlots
{
	RS_VIEW_BUF,
	RS_MESH_BUF,
	RS_SRV_TABLE,
	RS_UAV_TABLE,
	RS_COUNT,
};

rl::RenderInitParams GetAppRenderParams()
{
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
	params.RootSigDesc.Slots[RS_SRV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, rl::RootSignatureDescriptorTableType::SRV);
	params.RootSigDesc.Slots[RS_UAV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, rl::RootSignatureDescriptorTableType::UAV);

	params.RootSigDesc.GlobalSamplers.resize(1);
	params.RootSigDesc.GlobalSamplers[0].AddressModeUVW(SamplerAddressMode::WRAP).FilterModeMinMagMip(SamplerFilterMode::LINEAR);

	return params;
}

void InitializeApp()
{
	G.SphereMesh = CreateSphereMesh(16, 16);
	G.TerrainMesh = CreateTerrainTileMesh(G.TileMeshDim);

	GetNoiseData(G.NoiseDim, G.TerrainScale, G.TerrainHeight, G.HeightData, G.NormalData);
	CreateNoiseTexture(G.NoiseDim, G.HeightData.data(), G.HeightTexture, G.HeightTextureSRV);
	CreateNoiseNormalTexture(G.NoiseDim, G.NormalData.data(), G.NormalTexture, G.NormalTextureSRV);

	G.MeshPSO = CreateMeshPSO();
	G.TerrainPSO = CreateTerrainPSO();
	G.BallComputePSO = CreateBallComputePSO();
	G.ClearArgsPSO = CreateClearArgsPSO();

	InitBallArray();

	RenderResourceFlags StructBufResourceFlags = RenderResourceFlags::SRV;
	if (G.UseCompute)
	{
		StructBufResourceFlags |= RenderResourceFlags::UAV;
	}

	G.BallDataBuffer = CreateStructuredBuffer(G.Balls.data(), G.BallCount * sizeof(Ball), sizeof(Ball), StructBufResourceFlags);
	G.BallDataBufferSRV = CreateStructuredBufferSRV(G.BallDataBuffer, 0u, G.BallCount, (uint32_t)sizeof(Ball));
	G.BallDataBufferUAV = G.UseCompute ? CreateStructuredBufferUAV(G.BallDataBuffer, 0u, G.BallCount, (uint32_t)sizeof(Ball)) : UnorderedAccessView_t{};

	G.BallIndexBuffer = CreateStructuredBuffer(G.BallIndices.data(), G.BallIndices.size() * sizeof(u32), sizeof(u32), StructBufResourceFlags);
	G.BallIndexBufferSRV = CreateStructuredBufferSRV(G.BallIndexBuffer, 0u, G.BallCount, (u32)sizeof(u32));
	G.BallIndexBufferUAV = G.UseCompute ? CreateStructuredBufferUAV(G.BallIndexBuffer, 0u, G.BallCount, (u32)sizeof(u32)) : UnorderedAccessView_t{};

	IndirectDrawIndexedLayout IndirectDrawArgs;
	IndirectDrawArgs.numIndices = G.SphereMesh.indexBuf.count;
	IndirectDrawArgs.numInstances = 0u;
	IndirectDrawArgs.startIndex = 0u;
	IndirectDrawArgs.startVertex = 0u;
	IndirectDrawArgs.startInstance = 0u;
	G.IndirectDrawBuffer = CreateStructuredBuffer(&IndirectDrawArgs, sizeof(IndirectDrawArgs), sizeof(IndirectDrawArgs), RenderResourceFlags::UAV);
	G.IndirectDrawBufferUAV = CreateStructuredBufferUAV(G.IndirectDrawBuffer, 0u, 1u, sizeof(IndirectDrawArgs));

	G.IndirectDrawIndexedCommand = CreateIndirectDrawIndexedCommand();

	G.Cam.SetPosition(float3(-5, 20, 25));
	G.Cam.SetNearFar(0.1f, 1000.0f);
}

void ResizeApp(uint32_t width, uint32_t height)
{
	width = Max(width, 1u);
	height = Max(height, 1u);

	if (G.ScreenWidth == width && G.ScreenHeight == height)
	{
		return;
	}

	G.ScreenWidth = width;
	G.ScreenHeight = height;

	TextureCreateDescEx depthDesc = {};
	depthDesc.DebugName = L"SceneDepth";
	depthDesc.Flags = RenderResourceFlags::DSV;
	depthDesc.ResourceFormat = RenderFormat::D16_UNORM;
	depthDesc.Height = G.ScreenHeight;
	depthDesc.Width = G.ScreenWidth;
	depthDesc.InitialState = ResourceTransitionState::DEPTH_WRITE;
	depthDesc.Dimension = TextureDimension::TEX2D;
	G.DepthTexture = CreateTextureEx(depthDesc);
	G.DepthDSV = CreateTextureDSV(G.DepthTexture, RenderFormat::D16_UNORM, TextureDimension::TEX2D, 1u);

	G.Cam.Resize(G.ScreenWidth, G.ScreenHeight);
}

void Update(float deltaSeconds)
{
	G.Cam.UpdateView(deltaSeconds);

	if (!G.UseCompute)
	{
		G.BallsToDraw = 0u;

		const Frustum viewFrustum = G.Cam.GetWorldFrustum();

		for (u32 i = 0; i < G.Balls.size(); i++)
		{
			Ball& ball = G.Balls[i];

			const float VelocityMagSqr = LengthSqr(ball.Velocity);

			ball.Velocity += float3(0, G.Gravity * deltaSeconds, 0) - Sign(ball.Velocity) * VelocityMagSqr * 0.0001f;

			ball.Position += ball.Velocity * deltaSeconds;

			if (ball.Position.x < 0.0f)
			{
				ball.Position.x += G.TerrainScale;
			}

			if (ball.Position.x > G.TerrainScale)
			{
				ball.Position.x -= G.TerrainScale;
			}

			if (ball.Position.y < -20.0f)
			{
				ball.Position.y += 100.0f;
			}

			if (ball.Position.z < 0.0f)
			{
				ball.Position.z += G.TerrainScale;
			}

			if (ball.Position.z > G.TerrainScale)
			{
				ball.Position.z -= G.TerrainScale;
			}

			auto TerrainCoordForPosition = [&](const float3& Pos)
			{
				float u = Clamp(Pos.x / G.TerrainScale, 0.0f, 1.0f);
				float v = Clamp(Pos.z / G.TerrainScale, 0.0f, 1.0f);

				uint32_t uCoord = Min(static_cast<uint32_t>(u * G.NoiseDim), G.NoiseDim - 1u);
				uint32_t vCoord = Min(static_cast<uint32_t>(v * G.NoiseDim), G.NoiseDim - 1u);

				return uint2(uCoord, vCoord);
			};

			// Gather pixels directly under ball
			float radius = ball.Scale;

			uint2 topLeftCoord = TerrainCoordForPosition(float3(ball.Position.x - radius, 0.0f, ball.Position.z - radius));
			uint2 bottomRightCoord = TerrainCoordForPosition(float3(ball.Position.x + radius, 0.0f, ball.Position.z + radius));

			auto GetHit = [&]()
			{
				float penetration = 0.0f;
				float3 bounceDir = 0;
				u32 samples = 0;
				for (u32 y = topLeftCoord.y; y <= bottomRightCoord.y; y++)
				{
					for (u32 x = topLeftCoord.x; x <= bottomRightCoord.x; x++)
					{
						float height = G.HeightData[y * G.NoiseDim + x] * G.TerrainHeight;
						float3 samplePos = float3(((float)x / (float)G.NoiseDim) * G.TerrainScale, height, ((float)y / (float)G.NoiseDim) * G.TerrainScale);

						float3 direction = ball.Position - samplePos;
						float penetrationTest = ball.Scale * ball.Scale - LengthSqr(direction);
						if (penetrationTest > penetration)
						{
							penetration = penetrationTest;
							bounceDir = direction;
						}

						samples++;
					}
				}

				return Normalize(bounceDir);
			};
			ball.Velocity += GetHit() * Length(ball.Velocity) * ball.Bounciness;

			BoundingSphere bounds = BoundingSphere(ball.Position, ball.Scale);
			if (!CullFrustumSphere(viewFrustum, bounds))
			{
				G.BallIndices[G.BallsToDraw++] = i;
			}
		}

		UpdateStructuredBufferFromArray(G.BallDataBuffer, G.Balls.data(), G.Balls.size());
		UpdateStructuredBufferFromArray(G.BallIndexBuffer, G.BallIndices.data(), G.BallIndices.size());
	}
	else
	{
		G.BallsToDraw = (u32)G.Balls.size();
	}
}

void ImguiUpdate()
{
}

void Render(rl::RenderView* view, rl::CommandListSubmissionGroup* clGroup, float deltaSeconds)
{
	struct
	{
		matrix viewProjection;
		float3 CamPos;
		float __pad;
	} viewConsts;

	viewConsts.viewProjection = G.Cam.GetView() * G.Cam.GetProjection();
	viewConsts.CamPos = G.Cam.GetPosition();

	DynamicBuffer_t viewCbuf = CreateDynamicConstantBuffer(&viewConsts);

	struct
	{
		float2 Offset;
		float2 Scale;
		u32 NoiseTex;
		u32 NormalTex;
		float Height;
		float CellSize;
	} terrainTileConstants;

	terrainTileConstants.Offset = { 0.f };
	terrainTileConstants.Scale = { G.TerrainScale };
	terrainTileConstants.NoiseTex = GetDescriptorIndex(G.HeightTextureSRV);
	terrainTileConstants.NormalTex = GetDescriptorIndex(G.NormalTextureSRV);
	terrainTileConstants.Height = G.TerrainHeight;
	terrainTileConstants.CellSize = (1.0f / (float)G.TileMeshDim);

	DynamicBuffer_t terrainCbuf = CreateDynamicConstantBuffer(&terrainTileConstants);

	ComputeSceneData sceneData;
	sceneData.BallDataSRVIndex = GetDescriptorIndex(G.BallDataBufferSRV);
	sceneData.BallDataUAVIndex = GetDescriptorIndex(G.BallDataBufferUAV);
	sceneData.BallIndexSRVIndex = GetDescriptorIndex(G.BallIndexBufferSRV);
	sceneData.BallIndexUAVIndex = GetDescriptorIndex(G.BallDataBufferUAV);
	sceneData.IndirectDrawUAVIndex = GetDescriptorIndex(G.IndirectDrawBufferUAV);
	sceneData.DeltaSeconds = deltaSeconds;
	sceneData.DragCoefficient = 0.001f;
	sceneData.FrameID = (uint32_t)view->GetFrameID();
	sceneData.Gravity = G.Gravity;
	sceneData.NoiseDim = G.NoiseDim;
	sceneData.NoiseTexSRVIndex = GetDescriptorIndex(G.HeightTextureSRV);
	sceneData.NumBalls = G.BallCount;
	sceneData.TerrainHeight = G.TerrainHeight;
	sceneData.TerrainScale = G.TerrainScale;

	if (G.UseCompute)
	{
		const Frustum viewFrustum = G.Cam.GetWorldFrustum();
		for (u32 i = 0; i < Frustum::COUNT; i++)
		{
			sceneData.FrustumPlanes[i] = float4(viewFrustum.Planes[i].Normal, viewFrustum.Planes[i].Distance);
		}
	}

	DynamicBuffer_t sceneCBuf = CreateDynamicConstantBuffer(&sceneData);

	CommandList* cl = clGroup->CreateCommandList();

	cl->SetRootSignature();

	// Trigger compute work as early as possible
	if (G.UseCompute)
	{
		cl->TransitionResource(G.BallDataBuffer, ResourceTransitionState::READ, ResourceTransitionState::UNORDERED_ACCESS);
		cl->TransitionResource(G.BallIndexBuffer, ResourceTransitionState::READ, ResourceTransitionState::UNORDERED_ACCESS);
		cl->TransitionResource(G.HeightTexture, ResourceTransitionState::ALL_SHADER_RESOURCE, ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE);
		cl->TransitionResource(G.IndirectDrawBuffer, ResourceTransitionState::READ, ResourceTransitionState::UNORDERED_ACCESS);

		cl->SetComputeRootDescriptorTable(RS_SRV_TABLE);
		cl->SetComputeRootDescriptorTable(RS_UAV_TABLE);
		cl->SetComputeRootCBV(RS_VIEW_BUF, sceneCBuf);

		cl->SetPipelineState(G.ClearArgsPSO);
		cl->Dispatch(1u, 1u, 1u);

		cl->UAVBarrier(G.IndirectDrawBuffer);

		cl->SetPipelineState(G.BallComputePSO);

		cl->Dispatch(DivideRoundUp(G.BallCount, 128u), 1u, 1u);

		cl->TransitionResource(G.HeightTexture, ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE, ResourceTransitionState::ALL_SHADER_RESOURCE);
		cl->TransitionResource(G.BallDataBuffer, ResourceTransitionState::UNORDERED_ACCESS, ResourceTransitionState::ALL_SHADER_RESOURCE);
		cl->TransitionResource(G.BallIndexBuffer, ResourceTransitionState::UNORDERED_ACCESS, ResourceTransitionState::ALL_SHADER_RESOURCE);
		cl->TransitionResource(G.IndirectDrawBuffer, ResourceTransitionState::UNORDERED_ACCESS, ResourceTransitionState::INDIRECT_ARGUMENT);
	}

	// Bind and clear targets
	{
		RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();

		constexpr float DefaultClearCol[4] = { 0.3f, 0.3f, 0.6f, 0.0f };

		cl->ClearRenderTarget(backBufferRtv, DefaultClearCol);
		cl->ClearDepth(G.DepthDSV, 1.0f);

		cl->SetRenderTargets(&backBufferRtv, 1, G.DepthDSV);
	}

	// Init viewport and scissor
	{
		Viewport vp{ G.ScreenWidth, G.ScreenHeight };
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
	cl->SetPipelineState(G.MeshPSO);
	cl->SetVertexBuffers(0, 1, &G.SphereMesh.positionBuf.buf, &G.SphereMesh.positionBuf.stride, &G.SphereMesh.positionBuf.offset);
	cl->SetIndexBuffer(G.SphereMesh.indexBuf.buf, G.SphereMesh.indexBuf.format, G.SphereMesh.indexBuf.offset);
	if (G.UseCompute)
	{
		cl->ExecuteIndirect(G.IndirectDrawIndexedCommand, G.IndirectDrawBuffer);
	}
	else
	{
		cl->DrawIndexedInstanced(G.SphereMesh.indexBuf.count, G.BallsToDraw, 0, 0, 0);
	}

	// Draw terrain
	{
		cl->SetPipelineState(G.TerrainPSO);

		if (Render_IsBindless())
		{
			cl->SetGraphicsRootCBV(1, terrainCbuf);
		}
		else
		{
			cl->BindVertexCBVs(1, 1, &terrainCbuf);
			cl->BindVertexSRVs(0, 1, &G.HeightTextureSRV);
			cl->BindPixelSRVs(0, 1, &G.HeightTextureSRV);
		}

		cl->SetVertexBuffers(0, 1, &G.TerrainMesh.UVBuf.buf, &G.TerrainMesh.UVBuf.stride, &G.TerrainMesh.UVBuf.offset);
		cl->SetIndexBuffer(G.TerrainMesh.IndexBuf.buf, G.TerrainMesh.IndexBuf.format, G.TerrainMesh.IndexBuf.offset);
		cl->DrawIndexedInstanced(G.TerrainMesh.IndexBuf.count, 1u, 0u, 0u, 0u);

	}

	if (G.UseCompute)
	{
		cl->TransitionResource(G.BallDataBuffer, ResourceTransitionState::ALL_SHADER_RESOURCE, ResourceTransitionState::READ);
		cl->TransitionResource(G.BallIndexBuffer, ResourceTransitionState::ALL_SHADER_RESOURCE, ResourceTransitionState::READ);
		cl->TransitionResource(G.IndirectDrawBuffer, ResourceTransitionState::INDIRECT_ARGUMENT, ResourceTransitionState::READ);
	}
}

void ShutdownApp()
{
}
