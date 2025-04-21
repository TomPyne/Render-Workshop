#include "WaveFrontReader.h"
#include "Logging/Logging.h"

#include <cstring>
#include <filesystem>

// Get a path relative to the base objects path
std::wstring GetFileRelativePath(const std::wstring& BasePath, const std::wstring& InPath)
{
    std::filesystem::path Path = std::filesystem::path(BasePath).parent_path() / std::filesystem::path(InPath);
    return std::filesystem::relative(Path).wstring();
}

bool WaveFrontReader_c::Load(const wchar_t* FileName)
{
    Clear();

    static const size_t MAX_POLY = 64;

    std::wifstream InFile(FileName);
    if (!ENSUREMSG(!InFile.fail(), "File not found: %S", FileName))
    {
        return false;
    }

    wchar_t FName[_MAX_FNAME] = {};
    _wsplitpath_s(FileName, nullptr, 0, nullptr, 0, FName, _MAX_FNAME, nullptr, 0);

    Name = FName;

    std::vector<float3>   Positions;
    std::vector<float3>   Normals;
    std::vector<float2>   TexCoords;

    VertexCache_t  VertexCache;

    Material_s DefMat;

    DefMat.Name = L"default";
    Materials.emplace_back(DefMat);

    uint32_t CurSubset = 0;

    MtlFile = {};
    for (;; )
    {
        std::wstring Command;
        InFile >> Command;
        if (!InFile)
            break;

        if (*Command.c_str() == L'#')
        {
            // Comment
        }
        else if (0 == wcscmp(Command.c_str(), L"o"))
        {
            // Object name ignored
        }
        else if (0 == wcscmp(Command.c_str(), L"g"))
        {
            // Group name ignored
        }
        else if (0 == wcscmp(Command.c_str(), L"s"))
        {
            // Smoothing group ignored
        }
        else if (0 == wcscmp(Command.c_str(), L"v"))
        {
            // Vertex Position
            float X, Y, Z;
            InFile >> X >> Y >> Z;
            Positions.emplace_back(float3(X, Y, Z));
        }
        else if (0 == wcscmp(Command.c_str(), L"vt"))
        {
            // Vertex TexCoord
            float U, V;
            InFile >> U >> V;
            TexCoords.emplace_back(float2(U, V));

            HasTexcoords = true;
        }
        else if (0 == wcscmp(Command.c_str(), L"vn"))
        {
            // Vertex Normal
            float X, Y, Z;
            InFile >> X >> Y >> Z;
            Normals.emplace_back(float3(X, Y, Z));

            HasNormals = true;
        }
        else if (0 == wcscmp(Command.c_str(), L"f"))
        {
            // Face
            int32_t Position, TexCoord, Normal;
            Vertex_s Vertex;

            uint32_t FaceIndex[MAX_POLY];
            size_t Face = 0;
            for (;;)
            {
                if (Face >= MAX_POLY)
                {
                    LOGERROR("Too many polygon verts for the reader");
                    return false;
                }

                memset(&Vertex, 0, sizeof(Vertex));

                InFile >> Position;

                uint32_t VertexIndex = 0;
                if (!Position)
                {
                    LOGERROR("0 is not allowed for index");
                    return false;
                }
                else if (Position < 0)
                {
                    // Negative values are relative indices
                    VertexIndex = uint32_t(ptrdiff_t(Positions.size()) + Position);
                }
                else
                {
                    // OBJ format uses 1-based arrays
                    VertexIndex = uint32_t(Position - 1);
                }

                if (VertexIndex >= Positions.size())
                    return false;

                Vertex.Position = Positions[VertexIndex];

                if ('/' == InFile.peek())
                {
                    InFile.ignore();

                    if ('/' != InFile.peek())
                    {
                        // Optional texture coordinate
                        InFile >> TexCoord;

                        uint32_t CoordIndex = 0;
                        if (!TexCoord)
                        {
                            LOGERROR("0 is not allowed for index");
                            return false;
                        }
                        else if (TexCoord < 0)
                        {
                            // Negative values are relative indices
                            CoordIndex = uint32_t(ptrdiff_t(TexCoords.size()) + TexCoord);
                        }
                        else
                        {
                            // OBJ format uses 1-based arrays
                            CoordIndex = uint32_t(TexCoord - 1);
                        }

                        if (CoordIndex >= TexCoords.size())
                            return false;

                        Vertex.Texcoord = TexCoords[CoordIndex];
                    }

                    if ('/' == InFile.peek())
                    {
                        InFile.ignore();

                        // Optional vertex normal
                        InFile >> Normal;

                        uint32_t NormIndex = 0;
                        if (!Normal)
                        {
                            LOGERROR("0 is not allowed for index");
                            return false;
                        }
                        else if (Normal < 0)
                        {
                            // Negative values are relative indices
                            NormIndex = uint32_t(ptrdiff_t(Normals.size()) + Normal);
                        }
                        else
                        {
                            // OBJ format uses 1-based arrays
                            NormIndex = uint32_t(Normal - 1);
                        }

                        if (NormIndex >= Normals.size())
                            return false;

                        Vertex.Normal = Normals[NormIndex];
                    }
                }

                // If a duplicate vertex doesn't exist, add this vertex to the Vertices
                // list. Store the index in the Indices array. The Vertices and Indices
                // lists will eventually become the Vertex Buffer and Index Buffer for
                // the mesh.
                uint32_t Index = AddVertex(VertexIndex, &Vertex, VertexCache);
                if (Index == uint32_t(-1))
                {
                    ASSERT0MSG("Out of memory");
                    return false;
                }

                if (Index >= 0xFFFFFFFF)
                {
                    LOGERROR("Too many indices for 32-bit IB!");
                    return false;
                }

                FaceIndex[Face] = Index;
                ++Face;

                // Check for more face data or end of the face statement
                bool FaceEnd = false;
                for (;;)
                {
                    wchar_t P = InFile.peek();

                    if ('\n' == P || !InFile)
                    {
                        FaceEnd = true;
                        break;
                    }
                    else if (isdigit(P) || P == '-' || P == '+')
                        break;

                    InFile.ignore();
                }

                if (FaceEnd)
                    break;
            }

            if (Face < 3)
            {
                LOGERROR("Need at least 3 points to form a triangle");
                return false;
            }

            // Convert polygons to triangles
            uint32_t I0 = FaceIndex[0];
            uint32_t I1 = FaceIndex[1];

            for (size_t j = 2; j < Face; ++j)
            {
                uint32_t Index = FaceIndex[j];
                Indices.emplace_back(static_cast<index_t>(I0));                
                Indices.emplace_back(static_cast<index_t>(I1));
                Indices.emplace_back(static_cast<index_t>(Index));

                Attributes.emplace_back(CurSubset);

                I1 = Index;
            }

            CHECK(Attributes.size() * 3 == Indices.size());
        }
        else if (0 == wcscmp(Command.c_str(), L"mtllib"))
        {
            // Material library
            InFile >> MtlFile;
        }
        else if (0 == wcscmp(Command.c_str(), L"usemtl"))
        {
            // Material
            std::wstring Name = {};
            InFile >> Name;

            bool Found = false;
            uint32_t count = 0;
            for (auto it = Materials.cbegin(); it != Materials.cend(); ++it, ++count)
            {
                if (0 == wcscmp(it->Name.c_str(), Name.c_str()))
                {
                    Found = true;
                    CurSubset = count;
                    break;
                }
            }

            if (!Found)
            {
                Material_s Mat;
                CurSubset = static_cast<uint32_t>(Materials.size());
                Mat.Name = Name;
                Materials.emplace_back(Mat);
            }
        }
        else
        {
#ifdef _DEBUG
            // Unimplemented or unrecognized command
            LOGWARNING("Unimplemented or unrecognized command:");
#endif
        }

        InFile.ignore(1000, L'\n');
    }

    if (Positions.empty())
        return false;

    // Cleanup
    InFile.close();

    Bounds.InitFromPoints(Positions.data(), Positions.size());

    // If an associated material file was found, read that in as well.
    if (!MtlFile.empty())
    {
        MtlFile = GetFileRelativePath(FileName, MtlFile);
        return LoadMTL(MtlFile.c_str());
    }

    return true;
}

bool WaveFrontReader_c::LoadMTL(const wchar_t* FileName)
{
    // Assumes MTL is in CWD along with OBJ
    std::wifstream InFile(FileName);
    if (!InFile)
    {
        LOGERROR("File not found");
        return false;
    }

    std::wstring BasePath(FileName);

    auto CurMaterial = Materials.end();

    for (;; )
    {
        std::wstring Command;
        InFile >> Command;
        if (!InFile)
            break;

        if (0 == wcscmp(Command.c_str(), L"newmtl"))
        {
            // Switching active materials
            wchar_t Name[260] = {};
            InFile >> Name;

            CurMaterial = Materials.end();
            for (auto Iter = Materials.begin(); Iter != Materials.end(); ++Iter)
            {
                if (0 == wcscmp(Iter->Name.c_str(), Name))
                {
                    CurMaterial = Iter;
                    break;
                }
            }
        }

        // The rest of the commands rely on an active material
        if (CurMaterial == Materials.end())
            continue;

        if (0 == wcscmp(Command.c_str(), L"#"))
        {
            // Comment
        }
        else if (0 == wcscmp(Command.c_str(), L"Ka"))
        {
            // Ambient color
            float R, G, B;
            InFile >> R >> G >> B;
            CurMaterial->Ambient = float3(R, G, B);
        }
        else if (0 == wcscmp(Command.c_str(), L"Kd"))
        {
            // Diffuse color
            float R, G, B;
            InFile >> R >> G >> B;
            CurMaterial->Diffuse = float3(R, G, B);
        }
        else if (0 == wcscmp(Command.c_str(), L"Ks"))
        {
            // Specular color
            float R, G, B;
            InFile >> R >> G >> B;
            CurMaterial->Specular = float3(R, G, B);
        }
        else if (0 == wcscmp(Command.c_str(), L"Ke"))
        {
            // Emissive color
            float R, G, B;
            InFile >> R >> G >> B;
            CurMaterial->Emissive = float3(R, G, B);
            if (R > 0.f || G > 0.f || B > 0.f)
            {
                CurMaterial->Emissive = true;
            }
        }
        else if (0 == wcscmp(Command.c_str(), L"d"))
        {
            // Alpha
            float Alpha;
            InFile >> Alpha;
            CurMaterial->Alpha = std::min(1.f, std::max(0.f, Alpha));
        }
        else if (0 == wcscmp(Command.c_str(), L"Tr"))
        {
            // Transparency (inverse of alpha)
            float InvAlpha;
            InFile >> InvAlpha;
            CurMaterial->Alpha = std::min(1.f, std::max(0.f, 1.f - InvAlpha));
        }
        else if (0 == wcscmp(Command.c_str(), L"Ns"))
        {
            // Shininess
            int Shininess;
            InFile >> Shininess;
            CurMaterial->Shininess = uint32_t(Shininess);
        }
        else if (0 == wcscmp(Command.c_str(), L"illum"))
        {
            // Specular on/off
            int Illumination;
            InFile >> Illumination;
            CurMaterial->Specular = (Illumination == 2);
        }
        else if (0 == wcscmp(Command.c_str(), L"Pr"))
        {
            float Roughness;
            InFile >> Roughness;
            CurMaterial->Roughness = Roughness;

        }
        else if (0 == wcscmp(Command.c_str(), L"Pm"))
        {
            float Metallic;
            InFile >> Metallic;
            CurMaterial->Metallic = Metallic;

        }
        else if (0 == wcscmp(Command.c_str(), L"map_Kd"))
        {
            // Diffuse texture
            LoadTexturePath(InFile, BasePath, CurMaterial->DiffuseTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Ks"))
        {
            // Specular texture
            LoadTexturePath(InFile, BasePath, CurMaterial->SpecularTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Kn")
            || 0 == wcscmp(Command.c_str(), L"norm"))
        {
            // Normal texture
            LoadTexturePath(InFile, BasePath, CurMaterial->NormalTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Ke")
            || 0 == wcscmp(Command.c_str(), L"map_emissive"))
        {
            // Emissive texture
            LoadTexturePath(InFile, BasePath, CurMaterial->EmissiveTexture);
            CurMaterial->Emissive = true;
        }
        else if (0 == wcscmp(Command.c_str(), L"map_RMA")
            || 0 == wcscmp(Command.c_str(), L"map_ORM"))
        {
            // RMA texture
            LoadTexturePath(InFile, BasePath, CurMaterial->RMATexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Pr"))
        {
            // Roughness texture
            LoadTexturePath(InFile, BasePath, CurMaterial->RoughnessTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Pm"))
        {
            // Metallic texture
            LoadTexturePath(InFile, BasePath, CurMaterial->MetallicTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Bump"))
        {
            LoadBumpTexturePath(InFile, BasePath, CurMaterial->BumpScale, CurMaterial->BumpTexture);
        }
        else
        {
            // Unimplemented or unrecognized command
        }

        InFile.ignore(1000, L'\n');
    }

    InFile.close();

    return true;
}

void WaveFrontReader_c::Clear()
{
    Vertices.clear();
    Indices.clear();
    Attributes.clear();
    Materials.clear();
    Name.clear();
    HasNormals = false;
    HasTexcoords = false;

    Bounds.centre.x = Bounds.centre.y = Bounds.centre.z = 0.f;
    Bounds.extents.x = Bounds.extents.y = Bounds.extents.z = 0.f;
}

uint32_t WaveFrontReader_c::AddVertex(uint32_t Hash, const Vertex_s* Vertex, VertexCache_t& Cache)
{
    auto F = Cache.equal_range(Hash);

    for (auto Iter = F.first; Iter != F.second; ++Iter)
    {
        auto& TV = Vertices[Iter->second];

        if (0 == memcmp(Vertex, &TV, sizeof(Vertex_s)))
        {
            return Iter->second;
        }
    }

    auto Index = static_cast<uint32_t>(Vertices.size());
    Vertices.emplace_back(*Vertex);

    VertexCache_t::value_type Entry(Hash, Index);
    Cache.insert(Entry);
    return Index;
}

void WaveFrontReader_c::LoadTexturePath(std::wifstream& InFile, const std::wstring& BasePath, std::wstring& Texture)
{
    wchar_t Buff[1024] = {};
    InFile.getline(Buff, 1024, L'\n');
    InFile.putback(L'\n');

    std::wstring Path = Buff;

    // Ignore any end-of-line comment
    size_t Pos = Path.find_first_of(L'#');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(0, Pos);
    }

    // Trim any trailing whitespace
    Pos = Path.find_last_not_of(L" \t");
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(0, Pos + 1);
    }

    // Texture path should be last element in line
    Pos = Path.find_last_of(' ');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(Pos + 1);
    }

    if (!Path.empty())
    {
        Texture = GetFileRelativePath(BasePath, Path);
    }
}

void WaveFrontReader_c::LoadBumpTexturePath(std::wifstream& InFile, const std::wstring& BasePath, float& Scale, std::wstring& Texture)
{
    wchar_t Buff[1024] = {};
    InFile.getline(Buff, 1024, L'\n');
    InFile.putback(L'\n');

    std::wstring Path = Buff;

    // Ignore any end-of-line comment
    size_t Pos = Path.find_first_of(L'#');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(0, Pos);
    }

    // Trim any trailing whitespace
    Pos = Path.find_last_not_of(L" \t");
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(0, Pos + 1);
    }

    Pos = Path.find_first_of('-');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(Pos + 1);

        if (Path.starts_with(L"bm"))
        {
            Pos = Path.find_first_of(' ');
            Path = Path.substr(Pos + 1);

            Pos = Path.find_first_of(' ');
            std::wstring BumpScaleStr = Path.substr(0, Path.find_first_of(' '));

            Scale = _wtof(BumpScaleStr.c_str());

            Path = Path.substr(Pos + 1);
        }
    }

    // Texture path should be last element in line
    Pos = Path.find_last_of(' ');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(Pos + 1);
    }
    
    if (!Path.empty())
    {
        Texture = GetFileRelativePath(BasePath, Path);
    }
}

bool WaveFrontMtlReader_c::Load(const wchar_t* FileName)
{
    // Assumes MTL is in CWD along with OBJ
    std::wifstream InFile(FileName);
    if (!InFile)
    {
        LOGERROR("File not found");
        return false;
    }

    std::wstring BasePath(FileName);

    Material_s* CurMaterial = nullptr;

    for (;; )
    {
        std::wstring Command;
        InFile >> Command;
        if (!InFile)
            break;

        if (0 == wcscmp(Command.c_str(), L"newmtl"))
        {
            // Switching active materials
            wchar_t Name[260] = {};
            InFile >> Name;

            CurMaterial = nullptr;
            for (auto Iter = Materials.begin(); Iter != Materials.end(); ++Iter)
            {
                if (0 == wcscmp(Iter->Name.c_str(), Name))
                {
                    CurMaterial = Iter._Ptr;
                    break;
                }
            }

            if (CurMaterial == nullptr)
            {
                Materials.push_back({});
                Materials.back().Name = Name;

                CurMaterial = &Materials.back();
            }
        }

        // The rest of the commands rely on an active material
        if (CurMaterial == nullptr)
            continue;

        if (0 == wcscmp(Command.c_str(), L"#"))
        {
            // Comment
        }
        else if (0 == wcscmp(Command.c_str(), L"Ka"))
        {
            // Ambient color
            float R, G, B;
            InFile >> R >> G >> B;
            CurMaterial->Ambient = float3(R, G, B);
        }
        else if (0 == wcscmp(Command.c_str(), L"Kd"))
        {
            // Diffuse color
            float R, G, B;
            InFile >> R >> G >> B;
            CurMaterial->Diffuse = float3(R, G, B);
        }
        else if (0 == wcscmp(Command.c_str(), L"Ks"))
        {
            // Specular color
            float R, G, B;
            InFile >> R >> G >> B;
            CurMaterial->Specular = float3(R, G, B);
        }
        else if (0 == wcscmp(Command.c_str(), L"Ke"))
        {
            // Emissive color
            float R, G, B;
            InFile >> R >> G >> B;
            CurMaterial->Emissive = float3(R, G, B);
            if (R > 0.f || G > 0.f || B > 0.f)
            {
                CurMaterial->Emissive = true;
            }
        }
        else if (0 == wcscmp(Command.c_str(), L"d"))
        {
            // Alpha
            float Alpha;
            InFile >> Alpha;
            CurMaterial->Alpha = std::min(1.f, std::max(0.f, Alpha));
        }
        else if (0 == wcscmp(Command.c_str(), L"Tr"))
        {
            // Transparency (inverse of alpha)
            float InvAlpha;
            InFile >> InvAlpha;
            CurMaterial->Alpha = std::min(1.f, std::max(0.f, 1.f - InvAlpha));
        }
        else if (0 == wcscmp(Command.c_str(), L"Ns"))
        {
            // Shininess
            int Shininess;
            InFile >> Shininess;
            CurMaterial->Shininess = uint32_t(Shininess);
        }
        else if (0 == wcscmp(Command.c_str(), L"illum"))
        {
            // Specular on/off
            int Illumination;
            InFile >> Illumination;
            CurMaterial->Specular = (Illumination == 2);
        }
        else if (0 == wcscmp(Command.c_str(), L"Pr"))
        {
            float Roughness;
            InFile >> Roughness;
            CurMaterial->Roughness = Roughness;

        }
        else if (0 == wcscmp(Command.c_str(), L"Pm"))
        {
            float Metallic;
            InFile >> Metallic;
            CurMaterial->Metallic = Metallic;

        }
        else if (0 == wcscmp(Command.c_str(), L"map_Kd"))
        {
            // Diffuse texture
            LoadTexturePath(InFile, BasePath, CurMaterial->DiffuseTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Ks"))
        {
            // Specular texture
            LoadTexturePath(InFile, BasePath, CurMaterial->SpecularTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Kn")
            || 0 == wcscmp(Command.c_str(), L"norm"))
        {
            // Normal texture
            LoadTexturePath(InFile, BasePath, CurMaterial->NormalTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Ke")
            || 0 == wcscmp(Command.c_str(), L"map_emissive"))
        {
            // Emissive texture
            LoadTexturePath(InFile, BasePath, CurMaterial->EmissiveTexture);
            CurMaterial->Emissive = true;
        }
        else if (0 == wcscmp(Command.c_str(), L"map_RMA")
            || 0 == wcscmp(Command.c_str(), L"map_ORM"))
        {
            // RMA texture
            LoadTexturePath(InFile, BasePath, CurMaterial->RMATexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Pr"))
        {
            // Roughness texture
            LoadTexturePath(InFile, BasePath, CurMaterial->RoughnessTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Pm"))
        {
            // Metallic texture
            LoadTexturePath(InFile, BasePath, CurMaterial->MetallicTexture);
        }
        else if (0 == wcscmp(Command.c_str(), L"map_Bump"))
        {
            LoadBumpTexturePath(InFile, BasePath, CurMaterial->BumpScale, CurMaterial->BumpTexture);
        }
        else
        {
            // Unimplemented or unrecognized command
        }

        InFile.ignore(1000, L'\n');
    }

    InFile.close();

    return true;
}

void WaveFrontMtlReader_c::LoadTexturePath(std::wifstream& InFile, const std::wstring& BasePath, std::wstring& Texture)
{
    wchar_t Buff[1024] = {};
    InFile.getline(Buff, 1024, L'\n');
    InFile.putback(L'\n');

    std::wstring Path = Buff;

    // Ignore any end-of-line comment
    size_t Pos = Path.find_first_of(L'#');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(0, Pos);
    }

    // Trim any trailing whitespace
    Pos = Path.find_last_not_of(L" \t");
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(0, Pos + 1);
    }

    // Texture path should be last element in line
    Pos = Path.find_last_of(' ');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(Pos + 1);
    }

    if (!Path.empty())
    {
        Texture = GetFileRelativePath(BasePath, Path);
    }
}

void WaveFrontMtlReader_c::LoadBumpTexturePath(std::wifstream& InFile, const std::wstring& BasePath, float& Scale, std::wstring& Texture)
{
    wchar_t Buff[1024] = {};
    InFile.getline(Buff, 1024, L'\n');
    InFile.putback(L'\n');

    std::wstring Path = Buff;

    // Ignore any end-of-line comment
    size_t Pos = Path.find_first_of(L'#');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(0, Pos);
    }

    // Trim any trailing whitespace
    Pos = Path.find_last_not_of(L" \t");
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(0, Pos + 1);
    }

    Pos = Path.find_first_of('-');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(Pos + 1);

        if (Path.starts_with(L"bm"))
        {
            Pos = Path.find_first_of(' ');
            Path = Path.substr(Pos + 1);

            Pos = Path.find_first_of(' ');
            std::wstring BumpScaleStr = Path.substr(0, Path.find_first_of(' '));

            Scale = _wtof(BumpScaleStr.c_str());

            Path = Path.substr(Pos + 1);
        }
    }

    // Texture path should be last element in line
    Pos = Path.find_last_of(' ');
    if (Pos != std::wstring::npos)
    {
        Path = Path.substr(Pos + 1);
    }

    if (!Path.empty())
    {
        Texture = GetFileRelativePath(BasePath, Path);
    }
}
