#include "GltfReader.h"
#include "FileUtils/JsonValue.h"
#include "Logging/Logging.h"
#include "StringUtils/StringUtils.h"

#include <cmath>
#include <cstring>
#include <fstream>

namespace
{

constexpr uint32_t kGlbMagic = 0x46546C67; // "glTF"
constexpr uint32_t kGlbVersion = 2;
constexpr uint32_t kGlbChunkJson = 0x4E4F534A;
constexpr uint32_t kGlbChunkBin = 0x004E4942;

constexpr uint32_t kComponentByte = 5120;
constexpr uint32_t kComponentUnsignedByte = 5121;
constexpr uint32_t kComponentShort = 5122;
constexpr uint32_t kComponentUnsignedShort = 5123;
constexpr uint32_t kComponentUnsignedInt = 5125;
constexpr uint32_t kComponentFloat = 5126;

constexpr uint32_t kModeTriangles = 4;

// Column major, matching the layout of a glTF node matrix.
struct Matrix4_s
{
	float M[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	float& At(int Row, int Col) { return M[Col * 4 + Row]; }
	float At(int Row, int Col) const { return M[Col * 4 + Row]; }
};

Matrix4_s Multiply(const Matrix4_s& A, const Matrix4_s& B)
{
	Matrix4_s Out;
	for (int Row = 0; Row < 4; Row++)
	{
		for (int Col = 0; Col < 4; Col++)
		{
			float Sum = 0.0f;
			for (int It = 0; It < 4; It++)
			{
				Sum += A.At(Row, It) * B.At(It, Col);
			}
			Out.At(Row, Col) = Sum;
		}
	}
	return Out;
}

struct GlbFile_s
{
	Json_t Json;
	std::vector<uint8_t> Bin;
};

struct AccessorView_s
{
	const uint8_t* Data = nullptr;
	uint32_t Stride = 0;
	uint32_t Count = 0;
	uint32_t ComponentType = 0;
};

uint32_t ComponentSize(uint32_t ComponentType)
{
	switch (ComponentType)
	{
	case kComponentByte:
	case kComponentUnsignedByte:
		return 1;
	case kComponentShort:
	case kComponentUnsignedShort:
		return 2;
	case kComponentUnsignedInt:
	case kComponentFloat:
		return 4;
	default:
		return 0;
	}
}

uint32_t TypeComponentCount(const std::string& Type)
{
	if (Type == "SCALAR") return 1;
	if (Type == "VEC2") return 2;
	if (Type == "VEC3") return 3;
	if (Type == "VEC4") return 4;
	return 0;
}

const Json_t* FindArrayElement(const Json_t& Root, const char* Array, uint32_t Index)
{
	auto It = Root.find(Array);
	if (It == Root.end() || !It->is_array() || Index >= It->size())
	{
		return nullptr;
	}
	return &(*It)[Index];
}

bool ReadGlb(const wchar_t* FileName, GlbFile_s& Out)
{
	std::ifstream InFile(FileName, std::ios::binary);
	if (!ENSUREMSG(!InFile.fail(), "File not found: %S", FileName))
	{
		return false;
	}

	std::vector<uint8_t> Bytes((std::istreambuf_iterator<char>(InFile)), std::istreambuf_iterator<char>());

	uint32_t Header[3] = {};
	if (Bytes.size() < sizeof(Header))
	{
		LOGWARNING("[GltfReader] File too small to be a glb: %S", FileName);
		return false;
	}
	memcpy(Header, Bytes.data(), sizeof(Header));

	if (Header[0] != kGlbMagic || Header[1] != kGlbVersion || Header[2] > Bytes.size())
	{
		LOGWARNING("[GltfReader] Not a version 2 glb: %S", FileName);
		return false;
	}

	bool FoundJson = false;
	size_t Offset = sizeof(Header);
	while (Offset + 8 <= Header[2])
	{
		uint32_t ChunkLength = 0;
		uint32_t ChunkType = 0;
		memcpy(&ChunkLength, Bytes.data() + Offset, 4);
		memcpy(&ChunkType, Bytes.data() + Offset + 4, 4);
		Offset += 8;

		if (Offset + ChunkLength > Header[2])
		{
			LOGWARNING("[GltfReader] Chunk runs past the end of the file: %S", FileName);
			return false;
		}

		const uint8_t* ChunkData = Bytes.data() + Offset;
		if (ChunkType == kGlbChunkJson && !FoundJson)
		{
			constexpr bool AllowExceptions = false;
			Out.Json = Json_t::parse(ChunkData, ChunkData + ChunkLength, nullptr, AllowExceptions);
			if (Out.Json.is_discarded() || !Out.Json.is_object())
			{
				LOGWARNING("[GltfReader] Failed to parse json chunk: %S", FileName);
				return false;
			}
			FoundJson = true;
		}
		else if (ChunkType == kGlbChunkBin && Out.Bin.empty())
		{
			Out.Bin.assign(ChunkData, ChunkData + ChunkLength);
		}

		// Chunks are padded to 4 bytes, unknown chunk types are skipped
		Offset += (ChunkLength + 3) & ~3u;
	}

	if (!FoundJson)
	{
		LOGWARNING("[GltfReader] Missing json chunk: %S", FileName);
		return false;
	}

	return true;
}

bool GetAccessor(const GlbFile_s& File, uint32_t AccessorIndex, uint32_t ExpectedComponents, AccessorView_s& Out, const wchar_t* FileName)
{
	const Json_t* Accessor = FindArrayElement(File.Json, "accessors", AccessorIndex);
	if (!Accessor)
	{
		LOGWARNING("[GltfReader] Invalid accessor %u: %S", AccessorIndex, FileName);
		return false;
	}

	if (Accessor->contains("sparse"))
	{
		LOGWARNING("[GltfReader] Sparse accessors are not supported: %S", FileName);
		return false;
	}

	std::string Type;
	uint32_t ComponentType = 0;
	uint32_t Count = 0;
	uint32_t ViewIndex = 0;
	uint32_t AccessorOffset = 0;
	if (!JsonHelpers::ParseString(*Accessor, "type", Type) || !JsonHelpers::ParseInt(*Accessor, "componentType", ComponentType) ||
		!JsonHelpers::ParseInt(*Accessor, "count", Count) || !JsonHelpers::ParseInt(*Accessor, "bufferView", ViewIndex))
	{
		LOGWARNING("[GltfReader] Accessor %u is missing type, componentType, count or bufferView: %S", AccessorIndex, FileName);
		return false;
	}
	JsonHelpers::ParseInt(*Accessor, "byteOffset", AccessorOffset);

	const uint32_t Components = TypeComponentCount(Type);
	const uint32_t ElementSize = Components * ComponentSize(ComponentType);
	if (Components != ExpectedComponents || ElementSize == 0)
	{
		LOGWARNING("[GltfReader] Accessor %u has unexpected type %s: %S", AccessorIndex, Type.c_str(), FileName);
		return false;
	}

	const Json_t* View = FindArrayElement(File.Json, "bufferViews", ViewIndex);
	if (!View)
	{
		LOGWARNING("[GltfReader] Invalid buffer view %u: %S", ViewIndex, FileName);
		return false;
	}

	uint32_t BufferIndex = 0;
	uint32_t ViewOffset = 0;
	uint32_t ViewLength = 0;
	uint32_t ViewStride = 0;
	JsonHelpers::ParseInt(*View, "buffer", BufferIndex);
	JsonHelpers::ParseInt(*View, "byteOffset", ViewOffset);
	JsonHelpers::ParseInt(*View, "byteLength", ViewLength);
	JsonHelpers::ParseInt(*View, "byteStride", ViewStride);

	// Only the glb's own binary chunk is supported, which is always buffer 0 with no uri
	const Json_t* Buffer = FindArrayElement(File.Json, "buffers", BufferIndex);
	if (BufferIndex != 0 || !Buffer || Buffer->contains("uri"))
	{
		LOGWARNING("[GltfReader] Buffer view %u references an external buffer: %S", ViewIndex, FileName);
		return false;
	}

	const uint32_t Stride = ViewStride != 0 ? ViewStride : ElementSize;
	const uint64_t Start = static_cast<uint64_t>(ViewOffset) + AccessorOffset;
	const uint64_t End = Count == 0 ? Start : Start + static_cast<uint64_t>(Stride) * (Count - 1) + ElementSize;
	if (static_cast<uint64_t>(ViewOffset) + ViewLength > File.Bin.size() || End > static_cast<uint64_t>(ViewOffset) + ViewLength)
	{
		LOGWARNING("[GltfReader] Accessor %u reads outside its buffer view: %S", AccessorIndex, FileName);
		return false;
	}

	Out.Data = File.Bin.data() + Start;
	Out.Stride = Stride;
	Out.Count = Count;
	Out.ComponentType = ComponentType;
	return true;
}

bool GetFloatAccessor(const GlbFile_s& File, uint32_t AccessorIndex, uint32_t ExpectedComponents, uint32_t ExpectedCount, AccessorView_s& Out, const wchar_t* FileName)
{
	if (!GetAccessor(File, AccessorIndex, ExpectedComponents, Out, FileName))
	{
		return false;
	}

	if (Out.ComponentType != kComponentFloat)
	{
		LOGWARNING("[GltfReader] Accessor %u is not float, quantized attributes are not supported: %S", AccessorIndex, FileName);
		return false;
	}

	if (Out.Count != ExpectedCount)
	{
		LOGWARNING("[GltfReader] Accessor %u count does not match POSITION: %S", AccessorIndex, FileName);
		return false;
	}

	return true;
}

bool ReadFloatArray(const Json_t& Node, const char* Field, float* Out, uint32_t Count)
{
	auto It = Node.find(Field);
	if (It == Node.end())
	{
		return true;
	}

	if (!It->is_array() || It->size() != Count)
	{
		return false;
	}

	for (uint32_t Index = 0; Index < Count; Index++)
	{
		if (!(*It)[Index].is_number())
		{
			return false;
		}
		Out[Index] = (*It)[Index].get<float>();
	}
	return true;
}

bool GetNodeLocalMatrix(const Json_t& Node, Matrix4_s& Out)
{
	Out = Matrix4_s{};
	if (Node.contains("matrix"))
	{
		return ReadFloatArray(Node, "matrix", Out.M, 16);
	}

	float T[3] = { 0.0f, 0.0f, 0.0f };
	float R[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float S[3] = { 1.0f, 1.0f, 1.0f };
	if (!ReadFloatArray(Node, "translation", T, 3) || !ReadFloatArray(Node, "rotation", R, 4) || !ReadFloatArray(Node, "scale", S, 3))
	{
		return false;
	}

	const float X = R[0], Y = R[1], Z = R[2], W = R[3];
	const float Rot[3][3] =
	{
		{ 1.0f - 2.0f * (Y * Y + Z * Z), 2.0f * (X * Y - Z * W), 2.0f * (X * Z + Y * W) },
		{ 2.0f * (X * Y + Z * W), 1.0f - 2.0f * (X * X + Z * Z), 2.0f * (Y * Z - X * W) },
		{ 2.0f * (X * Z - Y * W), 2.0f * (Y * Z + X * W), 1.0f - 2.0f * (X * X + Y * Y) },
	};

	// T * R * S
	for (int Row = 0; Row < 3; Row++)
	{
		for (int Col = 0; Col < 3; Col++)
		{
			Out.At(Row, Col) = Rot[Row][Col] * S[Col];
		}
		Out.At(Row, 3) = T[Row];
	}
	return true;
}

struct LoadContext_s
{
	const GlbFile_s& File;
	const wchar_t* FileName;
	GltfReader_c& Reader;
	uint32_t MaterialCount;
	bool WarnedMissingTexcoords = false;
};

bool LoadPrimitive(LoadContext_s& Context, const Json_t& Primitive, const Matrix4_s& World)
{
	const wchar_t* FileName = Context.FileName;
	GltfReader_c& Reader = Context.Reader;

	uint32_t Mode = kModeTriangles;
	JsonHelpers::ParseInt(Primitive, "mode", Mode);
	if (Mode != kModeTriangles)
	{
		LOGWARNING("[GltfReader] Primitive mode %u is not supported, only triangles: %S", Mode, FileName);
		return false;
	}

	auto AttributesIt = Primitive.find("attributes");
	if (AttributesIt == Primitive.end() || !AttributesIt->is_object())
	{
		LOGWARNING("[GltfReader] Primitive has no attributes: %S", FileName);
		return false;
	}
	const Json_t& Attributes = *AttributesIt;

	uint32_t PositionIndex = 0;
	uint32_t NormalIndex = 0;
	if (!JsonHelpers::ParseInt(Attributes, "POSITION", PositionIndex) || !JsonHelpers::ParseInt(Attributes, "NORMAL", NormalIndex))
	{
		LOGWARNING("[GltfReader] Primitive requires POSITION and NORMAL: %S", FileName);
		return false;
	}

	AccessorView_s Positions;
	if (!GetAccessor(Context.File, PositionIndex, 3, Positions, FileName) || Positions.ComponentType != kComponentFloat)
	{
		LOGWARNING("[GltfReader] POSITION must be a float VEC3: %S", FileName);
		return false;
	}
	const uint32_t VertexCount = Positions.Count;

	AccessorView_s Normals;
	if (!GetFloatAccessor(Context.File, NormalIndex, 3, VertexCount, Normals, FileName))
	{
		return false;
	}

	AccessorView_s Texcoords[GltfReader_c::MaxTexcoords];
	uint32_t TexcoordCount = 0;
	for (uint32_t Channel = 0; Channel < GltfReader_c::MaxTexcoords; Channel++)
	{
		uint32_t TexcoordIndex = 0;
		const std::string Semantic = "TEXCOORD_" + std::to_string(Channel);
		if (!JsonHelpers::ParseInt(Attributes, Semantic.c_str(), TexcoordIndex))
		{
			break;
		}

		if (!GetFloatAccessor(Context.File, TexcoordIndex, 2, VertexCount, Texcoords[Channel], FileName))
		{
			return false;
		}
		TexcoordCount++;
	}

	CLOGWARNING(Attributes.contains("TEXCOORD_" + std::to_string(GltfReader_c::MaxTexcoords)), "[GltfReader] Only the first %u texcoord channels are loaded: %S", GltfReader_c::MaxTexcoords, FileName);

	if (TexcoordCount != Reader.TexcoordCount && !Reader.Vertices.empty() && !Context.WarnedMissingTexcoords)
	{
		LOGWARNING("[GltfReader] Primitives have differing texcoord channel counts, missing channels are zero filled: %S", FileName);
		Context.WarnedMissingTexcoords = true;
	}
	Reader.TexcoordCount = TexcoordCount > Reader.TexcoordCount ? TexcoordCount : Reader.TexcoordCount;

	std::vector<uint32_t> PrimitiveIndices;
	uint32_t IndicesIndex = 0;
	if (JsonHelpers::ParseInt(Primitive, "indices", IndicesIndex))
	{
		AccessorView_s IndexView;
		if (!GetAccessor(Context.File, IndicesIndex, 1, IndexView, FileName))
		{
			return false;
		}

		PrimitiveIndices.resize(IndexView.Count);
		for (uint32_t It = 0; It < IndexView.Count; It++)
		{
			const uint8_t* Element = IndexView.Data + IndexView.Stride * It;
			switch (IndexView.ComponentType)
			{
			case kComponentUnsignedByte:
				PrimitiveIndices[It] = *Element;
				break;
			case kComponentUnsignedShort:
			{
				uint16_t Value;
				memcpy(&Value, Element, sizeof(Value));
				PrimitiveIndices[It] = Value;
				break;
			}
			case kComponentUnsignedInt:
				memcpy(&PrimitiveIndices[It], Element, sizeof(uint32_t));
				break;
			default:
				LOGWARNING("[GltfReader] Unsupported index component type %u: %S", IndexView.ComponentType, FileName);
				return false;
			}

			if (PrimitiveIndices[It] >= VertexCount)
			{
				LOGWARNING("[GltfReader] Index out of range: %S", FileName);
				return false;
			}
		}
	}
	else
	{
		PrimitiveIndices.resize(VertexCount);
		for (uint32_t It = 0; It < VertexCount; It++)
		{
			PrimitiveIndices[It] = It;
		}
	}

	if (PrimitiveIndices.size() % 3 != 0)
	{
		LOGWARNING("[GltfReader] Triangle primitive index count is not a multiple of 3: %S", FileName);
		return false;
	}

	uint32_t Slot = 0;
	uint32_t MaterialIndex = 0;
	if (JsonHelpers::ParseInt(Primitive, "material", MaterialIndex))
	{
		if (MaterialIndex >= Context.MaterialCount)
		{
			LOGWARNING("[GltfReader] Primitive references missing material %u: %S", MaterialIndex, FileName);
			return false;
		}
		Slot = MaterialIndex + 1;
	}

	// Normals take the inverse transpose, which is the cofactor matrix divided by the determinant.
	float Cofactor[3][3];
	for (int Row = 0; Row < 3; Row++)
	{
		for (int Col = 0; Col < 3; Col++)
		{
			const int R0 = (Row + 1) % 3, R1 = (Row + 2) % 3;
			const int C0 = (Col + 1) % 3, C1 = (Col + 2) % 3;
			Cofactor[Row][Col] = World.At(R0, C0) * World.At(R1, C1) - World.At(R0, C1) * World.At(R1, C0);
		}
	}
	const float Determinant = World.At(0, 0) * Cofactor[0][0] + World.At(0, 1) * Cofactor[0][1] + World.At(0, 2) * Cofactor[0][2];
	const float NormalSign = Determinant < 0.0f ? -1.0f : 1.0f;

	const uint32_t BaseVertex = static_cast<uint32_t>(Reader.Vertices.size());
	Reader.Vertices.reserve(BaseVertex + VertexCount);
	for (uint32_t It = 0; It < VertexCount; It++)
	{
		float3 Position;
		float3 Normal;
		memcpy(&Position, Positions.Data + Positions.Stride * It, sizeof(float3));
		memcpy(&Normal, Normals.Data + Normals.Stride * It, sizeof(float3));

		GltfReader_c::Vertex_s Vertex = {};
		Vertex.Position = float3(
			World.At(0, 0) * Position.x + World.At(0, 1) * Position.y + World.At(0, 2) * Position.z + World.At(0, 3),
			World.At(1, 0) * Position.x + World.At(1, 1) * Position.y + World.At(1, 2) * Position.z + World.At(1, 3),
			World.At(2, 0) * Position.x + World.At(2, 1) * Position.y + World.At(2, 2) * Position.z + World.At(2, 3));

		float3 WorldNormal = float3(
			Cofactor[0][0] * Normal.x + Cofactor[0][1] * Normal.y + Cofactor[0][2] * Normal.z,
			Cofactor[1][0] * Normal.x + Cofactor[1][1] * Normal.y + Cofactor[1][2] * Normal.z,
			Cofactor[2][0] * Normal.x + Cofactor[2][1] * Normal.y + Cofactor[2][2] * Normal.z) * NormalSign;
		const float Length = std::sqrt(WorldNormal.x * WorldNormal.x + WorldNormal.y * WorldNormal.y + WorldNormal.z * WorldNormal.z);
		Vertex.Normal = Length > 0.0f ? WorldNormal * (1.0f / Length) : Normal;

		for (uint32_t Channel = 0; Channel < TexcoordCount; Channel++)
		{
			memcpy(&Vertex.Texcoords[Channel], Texcoords[Channel].Data + Texcoords[Channel].Stride * It, sizeof(float2));
		}

		Reader.Vertices.push_back(Vertex);
	}

	// A mirroring node transform turns the triangles inside out, so reverse them to keep the same front face
	const bool Mirrored = Determinant < 0.0f;
	for (size_t It = 0; It < PrimitiveIndices.size(); It += 3)
	{
		Reader.Indices.push_back(BaseVertex + PrimitiveIndices[It + 0]);
		Reader.Indices.push_back(BaseVertex + PrimitiveIndices[It + (Mirrored ? 2 : 1)]);
		Reader.Indices.push_back(BaseVertex + PrimitiveIndices[It + (Mirrored ? 1 : 2)]);
		Reader.Attributes.push_back(Slot);
	}

	return true;
}

bool LoadNode(LoadContext_s& Context, uint32_t NodeIndex, const Matrix4_s& ParentWorld, uint32_t Depth)
{
	// glTF requires the node graph to be a forest, this only guards against malformed cycles
	constexpr uint32_t kMaxNodeDepth = 256;

	const Json_t* Node = FindArrayElement(Context.File.Json, "nodes", NodeIndex);
	if (!Node || Depth > kMaxNodeDepth)
	{
		LOGWARNING("[GltfReader] Invalid node %u: %S", NodeIndex, Context.FileName);
		return false;
	}

	Matrix4_s Local;
	if (!GetNodeLocalMatrix(*Node, Local))
	{
		LOGWARNING("[GltfReader] Node %u has a malformed transform: %S", NodeIndex, Context.FileName);
		return false;
	}
	const Matrix4_s World = Multiply(ParentWorld, Local);

	uint32_t MeshIndex = 0;
	if (JsonHelpers::ParseInt(*Node, "mesh", MeshIndex))
	{
		const Json_t* Mesh = FindArrayElement(Context.File.Json, "meshes", MeshIndex);
		auto PrimitivesIt = Mesh ? Mesh->find("primitives") : Json_t::const_iterator{};
		if (!Mesh || PrimitivesIt == Mesh->end() || !PrimitivesIt->is_array())
		{
			LOGWARNING("[GltfReader] Node %u references an invalid mesh: %S", NodeIndex, Context.FileName);
			return false;
		}

		for (const Json_t& Primitive : *PrimitivesIt)
		{
			if (!LoadPrimitive(Context, Primitive, World))
			{
				return false;
			}
		}
	}

	auto ChildrenIt = Node->find("children");
	if (ChildrenIt != Node->end() && ChildrenIt->is_array())
	{
		for (const Json_t& Child : *ChildrenIt)
		{
			if (!Child.is_number_unsigned() || !LoadNode(Context, Child.get<uint32_t>(), World, Depth + 1))
			{
				return false;
			}
		}
	}

	return true;
}

}

bool GltfReader_c::Load(const wchar_t* FileName)
{
	Clear();

	GlbFile_s File;
	if (!ReadGlb(FileName, File))
	{
		return false;
	}

	auto RequiredIt = File.Json.find("extensionsRequired");
	if (RequiredIt != File.Json.end() && RequiredIt->is_array() && !RequiredIt->empty())
	{
		LOGWARNING("[GltfReader] Required extensions are not supported (%s): %S", RequiredIt->dump().c_str(), FileName);
		return false;
	}

	wchar_t FName[_MAX_FNAME] = {};
	_wsplitpath_s(FileName, nullptr, 0, nullptr, 0, FName, _MAX_FNAME, nullptr, 0);
	Name = FName;

	MaterialNames.push_back(L"default");
	auto MaterialsIt = File.Json.find("materials");
	if (MaterialsIt != File.Json.end() && MaterialsIt->is_array())
	{
		for (const Json_t& Material : *MaterialsIt)
		{
			std::string MaterialName;
			JsonHelpers::ParseString(Material, "name", MaterialName);
			MaterialNames.push_back(NarrowToWide(MaterialName));
		}
	}

	uint32_t SceneIndex = 0;
	JsonHelpers::ParseInt(File.Json, "scene", SceneIndex);
	const Json_t* Scene = FindArrayElement(File.Json, "scenes", SceneIndex);
	if (!Scene)
	{
		LOGWARNING("[GltfReader] File has no scene: %S", FileName);
		return false;
	}

	LoadContext_s Context = { File, FileName, *this, static_cast<uint32_t>(MaterialNames.size() - 1) };

	auto NodesIt = Scene->find("nodes");
	if (NodesIt != Scene->end() && NodesIt->is_array())
	{
		for (const Json_t& NodeIndex : *NodesIt)
		{
			if (!NodeIndex.is_number_unsigned() || !LoadNode(Context, NodeIndex.get<uint32_t>(), Matrix4_s{}, 0))
			{
				Clear();
				return false;
			}
		}
	}

	return true;
}

void GltfReader_c::Clear()
{
	Vertices.clear();
	Indices.clear();
	Attributes.clear();
	MaterialNames.clear();
	TexcoordCount = 0;
	Name.clear();
}
