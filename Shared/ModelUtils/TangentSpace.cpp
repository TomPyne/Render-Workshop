#include "TangentSpace.h"

#include "ModelUtils/MikkTSpace.h"

#include <unordered_map>

namespace
{

// Face-vertices sharing a source vertex are welded when their tangents agree to within ~1 degree.
constexpr float kTangentWeldCos = 0.9998f;

template<typename T>
const T& Fetch(const TangentSpaceStream_s& Stream, uint32_t Index)
{
	return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(Stream.Data) + Stream.Stride * Index);
}

struct MikkUserData_s
{
	const TangentSpaceInput_s* Input = nullptr;
	TangentSpaceOutput_s* Output = nullptr;

	std::unordered_map<uint32_t, std::vector<uint32_t>> SourceToEmitted;
};

MikkUserData_s& GetUserData(const SMikkTSpaceContext* Context)
{
	return *static_cast<MikkUserData_s*>(Context->m_pUserData);
}

uint32_t SourceIndex(const MikkUserData_s& Data, int Face, int Vert)
{
	return Data.Input->Indices[Face * 3 + Vert];
}

int GetNumFaces(const SMikkTSpaceContext* Context)
{
	return static_cast<int>(GetUserData(Context).Input->IndexCount / 3);
}

int GetNumVerticesOfFace(const SMikkTSpaceContext*, const int)
{
	return 3;
}

void GetPosition(const SMikkTSpaceContext* Context, float Out[], const int Face, const int Vert)
{
	const MikkUserData_s& Data = GetUserData(Context);
	const float3& Value = Fetch<float3>(Data.Input->Positions, SourceIndex(Data, Face, Vert));
	Out[0] = Value.x;
	Out[1] = Value.y;
	Out[2] = Value.z;
}

void GetNormal(const SMikkTSpaceContext* Context, float Out[], const int Face, const int Vert)
{
	const MikkUserData_s& Data = GetUserData(Context);
	const float3& Value = Fetch<float3>(Data.Input->Normals, SourceIndex(Data, Face, Vert));
	Out[0] = Value.x;
	Out[1] = Value.y;
	Out[2] = Value.z;
}

void GetTexCoord(const SMikkTSpaceContext* Context, float Out[], const int Face, const int Vert)
{
	const MikkUserData_s& Data = GetUserData(Context);
	const float2& Value = Fetch<float2>(Data.Input->Texcoords[0], SourceIndex(Data, Face, Vert));
	Out[0] = Value.x;
	Out[1] = Value.y;
}

void SetTSpaceBasic(const SMikkTSpaceContext* Context, const float Tangent[], const float Sign, const int Face, const int Vert)
{
	MikkUserData_s& Data = GetUserData(Context);
	TangentSpaceOutput_s& Out = *Data.Output;

	const uint32_t Source = SourceIndex(Data, Face, Vert);
	const float4 NewTangent = float4(Tangent[0], Tangent[1], Tangent[2], Sign);

	std::vector<uint32_t>& Emitted = Data.SourceToEmitted[Source];
	for (uint32_t Candidate : Emitted)
	{
		const float4& Existing = Out.Tangents[Candidate];
		if (Existing.w == NewTangent.w && Dot(Existing.xyz, NewTangent.xyz) > kTangentWeldCos)
		{
			Out.Indices[Face * 3 + Vert] = Candidate;
			return;
		}
	}

	const uint32_t NewIndex = static_cast<uint32_t>(Out.Positions.size());
	Out.Positions.push_back(Fetch<float3>(Data.Input->Positions, Source));
	Out.Normals.push_back(Fetch<float3>(Data.Input->Normals, Source));
	for (uint32_t Channel = 0; Channel < Data.Input->TexcoordCount; Channel++)
	{
		Out.Texcoords[Channel].push_back(Fetch<float2>(Data.Input->Texcoords[Channel], Source));
	}
	Out.Tangents.push_back(NewTangent);

	Emitted.push_back(NewIndex);
	Out.Indices[Face * 3 + Vert] = NewIndex;
}

}

bool GenerateTangents(const TangentSpaceInput_s& Input, TangentSpaceOutput_s& Output)
{
	if (!Input.Positions.Data || !Input.Normals.Data)
	{
		return false;
	}

	if (Input.TexcoordCount == 0 || Input.TexcoordCount > kTangentSpaceMaxTexcoords)
	{
		return false;
	}

	for (uint32_t Channel = 0; Channel < Input.TexcoordCount; Channel++)
	{
		if (!Input.Texcoords[Channel].Data)
		{
			return false;
		}
	}

	if (!Input.Indices || Input.IndexCount == 0 || Input.IndexCount % 3 != 0)
	{
		return false;
	}

	Output.Indices.assign(Input.IndexCount, 0u);
	Output.Positions.reserve(Input.VertexCount);
	Output.Normals.reserve(Input.VertexCount);
	for (uint32_t Channel = 0; Channel < Input.TexcoordCount; Channel++)
	{
		Output.Texcoords[Channel].reserve(Input.VertexCount);
	}
	Output.Tangents.reserve(Input.VertexCount);

	MikkUserData_s UserData = {};
	UserData.Input = &Input;
	UserData.Output = &Output;

	SMikkTSpaceInterface Interface = {};
	Interface.m_getNumFaces = GetNumFaces;
	Interface.m_getNumVerticesOfFace = GetNumVerticesOfFace;
	Interface.m_getPosition = GetPosition;
	Interface.m_getNormal = GetNormal;
	Interface.m_getTexCoord = GetTexCoord;
	Interface.m_setTSpaceBasic = SetTSpaceBasic;

	SMikkTSpaceContext MikkContext = {};
	MikkContext.m_pInterface = &Interface;
	MikkContext.m_pUserData = &UserData;

	return genTangSpaceDefault(&MikkContext) != 0;
}
