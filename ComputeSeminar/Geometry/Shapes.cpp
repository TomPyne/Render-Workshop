#include "Shapes.h"

#include <SurfMath.h>

static std::vector<float3> GetCubePositions(float Scale)
{
	float3 pftl = float3(-0.5f, 0.5f, 0.5f) * Scale;
	float3 pftr = float3(0.5f, 0.5f, 0.5f) * Scale;
	float3 pfbr = float3(0.5f, -0.5f, 0.5f) * Scale;
	float3 pfbl = float3(-0.5f, -0.5f, 0.5f) * Scale;

	float3 pbtl = float3(-0.5f, 0.5f, -0.5f) * Scale;
	float3 pbtr = float3(0.5f, 0.5f, -0.5f) * Scale;
	float3 pbbr = float3(0.5f, -0.5f, -0.5f) * Scale;
	float3 pbbl = float3(-0.5f, -0.5f, -0.5f) * Scale;

	return {
		pftl, pftr, pfbr, pfbl, // Front
		pbtr, pbtl, pbbl, pbbr, // Back
		pftr, pbtr, pbbr, pfbr, // Right
		pbtl, pftl, pfbl, pbbl, // Left
		pfbl, pfbr, pbbr, pbbl, // Bottom
		pftl, pbtl, pbtr, pftr, // Top
	};
}

static std::vector<float3> GetCubeNormals()
{
	float3 Front = float3(0, 0, 1);
	float3 Back = float3(0, 0, 1);
	float3 Right = float3(0, 0, 1);
	float3 Left = float3(0, 0, 1);
	float3 Bottom = float3(0, 0, 1);
	float3 Top = float3(0, 0, 1);

	return {
		Front, Front, Front, Front,
		Back, Back, Back, Back,
		Right, Right, Right, Right,
		Left, Left, Left, Left,
		Bottom, Bottom, Bottom, Bottom,
		Top, Top, Top, Top,
	};
}

static std::vector<uint16_t> GetCubeIndices()
{
	return {
		2, 1, 0, 0, 3, 2,
		6, 5, 4, 4, 7, 6,
		10, 9, 8, 8, 11, 10,
		14, 13, 12, 12, 15, 14,
		18, 17, 16, 16, 19, 18,
		22, 21, 20, 20, 23, 22,
	};
}

ShapeMeshPos CreateCubeMeshPos(float Scale)
{
	ShapeMeshPos Mesh = {};

	{
		std::vector<float3> Positions = GetCubePositions(Scale);
		Mesh.PositionBuffer = tpr::CreateVertexBuffer(Positions.data(), sizeof(float3) * Positions.size());
		Mesh.BindBuffers[0] = Mesh.PositionBuffer;
		Mesh.BindStrides[0] = (uint32_t)sizeof(float3);
		Mesh.BindOffsets[0] = 0u;
	}

	{
		std::vector<uint16_t> Indices = GetCubeIndices();
		Mesh.IndexBuffer = tpr::CreateIndexBuffer(Indices.data(), sizeof(uint16_t) * Indices.size());
		Mesh.IndexCount = Indices.size();
		Mesh.IndexFormat = tpr::RenderFormat::R16_UINT;
	}

	Mesh.InputDesc =
	{
		{"POSITION", 0, tpr::RenderFormat::R32G32B32_FLOAT, 0, 0, tpr::InputClassification::PER_VERTEX, 0 },
	};

	return Mesh;
}

ShapeMeshPosNormal CreateCubeMeshPosNormal(float Scale)
{
	ShapeMeshPosNormal Mesh = {};

	{
		const std::vector<float3> Positions = GetCubePositions(Scale);
		Mesh.PositionBuffer = tpr::CreateVertexBuffer(Positions.data(), sizeof(float3) * Positions.size());
		Mesh.BindBuffers[0] = Mesh.PositionBuffer;
		Mesh.BindStrides[0] = (uint32_t)sizeof(float3);
		Mesh.BindOffsets[0] = 0u;
	}

	{
		const std::vector<float3> Normals = GetCubeNormals();
		Mesh.NormalBuffer = tpr::CreateVertexBuffer(Normals.data(), sizeof(float3) * Normals.size());
		Mesh.BindBuffers[1] = Mesh.NormalBuffer;
		Mesh.BindStrides[1] = (uint32_t)sizeof(float3);
		Mesh.BindOffsets[1] = 0u;
	}

	{
		const std::vector<uint16_t> Indices = GetCubeIndices();
		Mesh.IndexBuffer = tpr::CreateIndexBuffer(Indices.data(), sizeof(uint16_t) * Indices.size());
		Mesh.IndexCount = Indices.size();
		Mesh.IndexFormat = tpr::RenderFormat::R16_UINT;
	}

	Mesh.InputDesc =
	{
		{"POSITION", 0, tpr::RenderFormat::R32G32B32_FLOAT, 0, 0, tpr::InputClassification::PER_VERTEX, 0 },
		{"NORMAL", 0, tpr::RenderFormat::R32G32B32_FLOAT, 1, 0, tpr::InputClassification::PER_VERTEX, 0 },
	};

	return Mesh;
}

ShapeMeshPos CreateTileMeshPos(float Scale, uint32_t Dimension)
{
	ShapeMeshPos Mesh;

	const u32 XSize = Dimension + 1;
	const u32 YSize = Dimension + 1;

	{
		float XSep = 1.0f / Dimension;
		float YSep = 1.0f / Dimension;

		std::vector<float2> Positions;
		Positions.resize(XSize * YSize);

		auto PosIter = Positions.begin();

		for (uint32_t Y = 0; Y < YSize; Y++)
		{
			for (uint32_t X = 0; X < XSize; X++)
			{
				*PosIter = float2{ X * XSep, Y * YSep };
				PosIter++;
			}
		}

		Mesh.PositionBuffer = tpr::CreateVertexBuffer(Positions.data(), sizeof(float2) * Positions.size());
		Mesh.BindBuffers[0] = Mesh.PositionBuffer;
		Mesh.BindStrides[0] = (uint32_t)sizeof(float2);
		Mesh.BindOffsets[0] = 0u;
	}

	{
		uint32_t IndexCount = Dimension * Dimension * 6;

		std::vector<uint32_t> Indices;
		Indices.resize(IndexCount);

		auto IndexIter = Indices.begin();

		for (uint32_t Y = 0; Y < Dimension; Y++)
		{
			for (uint32_t X = 0; X < Dimension; X++)
			{
				*IndexIter = ((Y + 1) * XSize) + X + 0; IndexIter++;
				*IndexIter = (Y * XSize) + X + 1; IndexIter++;
				*IndexIter = (Y * XSize) + X + 0; IndexIter++;

				*IndexIter = ((Y + 1) * XSize) + X + 0; IndexIter++;
				*IndexIter = ((Y + 1) * XSize) + X + 1; IndexIter++;
				*IndexIter = (Y * XSize) + X + 1; IndexIter++;
			}
		}

		Mesh.IndexBuffer = tpr::CreateIndexBuffer(Indices.data(), sizeof(uint32_t) * Indices.size());
		Mesh.IndexCount = Indices.size();
		Mesh.IndexFormat = tpr::RenderFormat::R32_UINT;
	}

	Mesh.InputDesc =
	{
		{"POSITION", 0, tpr::RenderFormat::R32G32_FLOAT, 0, 0, tpr::InputClassification::PER_VERTEX, 0 },
	};

	return Mesh;
}

static std::vector<float3> GetSpherePositions(float Scale, uint32_t Slices, uint32_t Stacks)
{
	std::vector<float3> Positions;

	const uint32_t VertexCount = Slices * (Stacks - 1) + 2u;

	Positions.resize(VertexCount);

	auto PosIter = Positions.begin();

	const float StacksRcp = 1.0f / float(Stacks);
	const float SlicesRcp = 1.0f / float(Slices);

	*PosIter = float3(0.0f, 0.5f, 0.0f) * Scale; PosIter++;

	for (uint32_t i = 0; i < Stacks - 1; i++)
	{
		const float Phi = K_PI * float(i + 1) * StacksRcp;
		const float SinPhi = sinf(Phi);
		const float CosPhi = cosf(Phi);

		for (uint32_t j = 0; j < Slices; j++)
		{
			const float Theta = 2.0f * K_PI * float(j) * SlicesRcp;
			const float SinTheta = sinf(Theta);
			const float CosTheta = cosf(Theta);

			*PosIter = float3(SinPhi * CosTheta, CosPhi, SinPhi * SinTheta) * 0.5f * Scale; PosIter++;
		}
	}

	*PosIter = float3(0.0f, -0.5f, 0.0f) * Scale;

	return Positions;
}

static std::vector<float3> GetSphereNormals(uint32_t Slices, uint32_t Stacks)
{
	std::vector<float3> Normals;

	const uint32_t VertexCount = Slices * (Stacks - 1) + 2u;

	Normals.resize(VertexCount);

	auto NormIter = Normals.begin();

	const float StacksRcp = 1.0f / float(Stacks);
	const float SlicesRcp = 1.0f / float(Slices);

	*NormIter = float3(0.0f, 1.0f, 0.0f); NormIter++;

	for (uint32_t i = 0; i < Stacks - 1; i++)
	{
		const float Phi = K_PI * float(i + 1) * StacksRcp;
		const float SinPhi = sinf(Phi);
		const float CosPhi = cosf(Phi);

		for (uint32_t j = 0; j < Slices; j++)
		{
			const float Theta = 2.0f * K_PI * float(j) * SlicesRcp;
			const float SinTheta = sinf(Theta);
			const float CosTheta = cosf(Theta);

			*NormIter = NormalizeF3(float3(SinPhi * CosTheta, CosPhi, SinPhi * SinTheta)); NormIter++;
		}
	}

	*NormIter = float3(0.0f, -1.0f, 0.0f);

	return Normals;
}

std::vector<uint32_t> GetSphereIndices(uint32_t Slices, uint32_t Stacks)
{
	const uint32_t IndexCount = Slices * 6u + Slices * (Stacks - 2u) * 6u;
	const uint32_t VertexCount = Slices * (Stacks - 1) + 2u;

	std::vector<uint32_t> Indices;
	Indices.resize(IndexCount);

	auto IndexIter = Indices.begin();

	for (uint32_t i = 0; i < Slices; i++)
	{
		*IndexIter = (i + 1) % Slices + 1; IndexIter++;
		*IndexIter = i + 1; IndexIter++;
		*IndexIter = 0; IndexIter++;
		*IndexIter = VertexCount - 1; IndexIter++;
		*IndexIter = i + Slices * (Stacks - 2) + 1; IndexIter++;
		*IndexIter = (i + 1) % Slices + Slices * (Stacks - 2) + 1; IndexIter++;
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

			*IndexIter = i0; IndexIter++;
			*IndexIter = i1; IndexIter++;
			*IndexIter = i2; IndexIter++;
			*IndexIter = i2; IndexIter++;
			*IndexIter = i3; IndexIter++;
			*IndexIter = i0; IndexIter++;
		}
	}

	return Indices;
}

ShapeMeshPos CreateSphereMeshPos(float Scale, uint32_t Slices, uint32_t Stacks)
{
	ShapeMeshPos Mesh = {};

	{
		std::vector<float3> Positions = GetSpherePositions(Scale, Slices, Stacks);
		Mesh.PositionBuffer = tpr::CreateVertexBuffer(Positions.data(), sizeof(float3) * Positions.size());
		Mesh.BindBuffers[0] = Mesh.PositionBuffer;
		Mesh.BindStrides[0] = (uint32_t)sizeof(float3);
		Mesh.BindOffsets[0] = 0u;
	}

	{
		std::vector<uint32_t> Indices = GetSphereIndices(Slices, Stacks);
		Mesh.IndexBuffer = tpr::CreateIndexBuffer(Indices.data(), sizeof(uint32_t) * Indices.size());
		Mesh.IndexCount = Indices.size();
		Mesh.IndexFormat = tpr::RenderFormat::R32_UINT;
	}

	Mesh.InputDesc =
	{
		{"POSITION", 0, tpr::RenderFormat::R32G32B32_FLOAT, 0, 0, tpr::InputClassification::PER_VERTEX, 0 },
	};

	return Mesh;
}

ShapeMeshPosNormal CreateSphereMeshPosNormal(float Scale, uint32_t Slices, uint32_t Stacks)
{
	ShapeMeshPosNormal Mesh = {};

	{
		const std::vector<float3> Positions = GetSpherePositions(Scale, Slices, Stacks);
		Mesh.PositionBuffer = tpr::CreateVertexBuffer(Positions.data(), sizeof(float3) * Positions.size());
		Mesh.BindBuffers[0] = Mesh.PositionBuffer;
		Mesh.BindStrides[0] = (uint32_t)sizeof(float3);
		Mesh.BindOffsets[0] = 0u;
	}

	{
		const std::vector<float3> Normals = GetSphereNormals(Slices, Stacks);
		Mesh.NormalBuffer = tpr::CreateVertexBuffer(Normals.data(), sizeof(float3) * Normals.size());
		Mesh.BindBuffers[1] = Mesh.NormalBuffer;
		Mesh.BindStrides[1] = (uint32_t)sizeof(float3);
		Mesh.BindOffsets[1] = 0u;
	}

	{
		const std::vector<uint32_t> Indices = GetSphereIndices(Slices, Stacks);
		Mesh.IndexBuffer = tpr::CreateIndexBuffer(Indices.data(), sizeof(uint32_t) * Indices.size());
		Mesh.IndexCount = Indices.size();
		Mesh.IndexFormat = tpr::RenderFormat::R32_UINT;
	}

	Mesh.InputDesc =
	{
		{"POSITION", 0, tpr::RenderFormat::R32G32B32_FLOAT, 0, 0, tpr::InputClassification::PER_VERTEX, 0 },
		{"NORMAL", 0, tpr::RenderFormat::R32G32B32_FLOAT, 1, 0, tpr::InputClassification::PER_VERTEX, 0 },
	};

	return Mesh;
}
