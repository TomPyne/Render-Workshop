#include "MeshProcessing.h"
#include "Logging/Logging.h"

#include <algorithm>
#include <array>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <unordered_set>

constexpr uint32_t UNUSED32 = uint32_t(-1);

using index_t = MeshProcessing::index_t;
using Subset_s = MeshProcessing::Subset_s;

bool ValidateIndices(const index_t* Indices, size_t NumFaces, size_t NumVerts)
{
    for (size_t FaceIt = 0; FaceIt < NumFaces; ++FaceIt)
    {
        // Check for values in-range
        for (size_t PointIt = 0; PointIt < 3; ++PointIt)
        {
            index_t Index = Indices[FaceIt * 3 + PointIt];
            if (Index >= NumVerts && Index != index_t(-1))
            {
                LOGERROR("An invalid index value (%u) was found on face %zu\n", Index, FaceIt);

                return false;
            }
        }

        // Check for unused faces
        index_t I0 = Indices[FaceIt * 3];
        index_t I1 = Indices[FaceIt * 3 + 1];
        index_t I2 = Indices[FaceIt * 3 + 2];
        if (I0 == index_t(-1)
            || I1 == index_t(-1)
            || I2 == index_t(-1))
        {
            // ignore unused triangles for remaining tests
            continue;
        }

        // Check for degenerate triangles
        if (I0 == I1
            || I0 == I2
            || I1 == I2)
        {
            // ignore degenerate triangles for remaining tests
            continue;
        }
    }

    return true;
}

bool Validate(const index_t* Indices, size_t NumFaces, size_t NumVerts)
{
    if (!Indices || !NumFaces || !NumVerts)
    {
        return RET_INVALID_ARGS;
    }

    if (NumVerts >= UINT32_MAX)
    {
        return RET_INVALID_ARGS;
    }

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
    {
        return RET_ARITHMETIC_OVERFLOW;
    }

    return ValidateIndices(Indices, NumFaces, NumVerts);
}

bool MeshProcessing::CleanMesh(index_t* Indices, size_t NumFaces, size_t NumVerts, const uint32_t* Attributes, std::vector<uint32_t>& DupedVerts, bool BreakBowTies)
{
    if (!Validate(Indices, NumFaces, NumVerts))
    {
        return false;
    }

    if (!Attributes)
    {
        return RET_INVALID_ARGS;
    }

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
    {
        return RET_ARITHMETIC_OVERFLOW;
    }

    DupedVerts.clear();
    size_t CurNewVert = NumVerts;

    const size_t TSize = (sizeof(bool) * NumFaces * 3) + (sizeof(uint32_t) * NumVerts) + (sizeof(index_t) * NumFaces * 3);
    std::unique_ptr<uint8_t[]> Temp(new (std::nothrow) uint8_t[TSize]);
    if (!Temp)
    {
        return RET_OUT_OF_MEM;
    }

    auto FaceSeen = reinterpret_cast<bool*>(Temp.get());
    auto IDs = reinterpret_cast<uint32_t*>(Temp.get() + sizeof(bool) * NumFaces * 3);

    // UNUSED/DEGENERATE cleanup
    for (uint32_t FaceIt = 0; FaceIt < NumFaces; ++FaceIt)
    {
        index_t I0 = Indices[FaceIt * 3];
        index_t I1 = Indices[FaceIt * 3 + 1];
        index_t I2 = Indices[FaceIt * 3 + 2];

        if (I0 == index_t(-1)
            || I1 == index_t(-1)
            || I2 == index_t(-1))
        {
            // ensure all index entries in the unused face are 'unused'
            Indices[FaceIt * 3] =
                Indices[FaceIt * 3 + 1] =
                Indices[FaceIt * 3 + 2] = index_t(-1);
        }
    }

    auto IndicesNew = reinterpret_cast<index_t*>(reinterpret_cast<uint8_t*>(IDs) + sizeof(uint32_t) * NumVerts);
    memcpy(IndicesNew, Indices, sizeof(index_t) * NumFaces * 3);

    if (Attributes)
    {
        memset(IDs, 0xFF, sizeof(uint32_t) * NumVerts);

        std::vector<uint32_t> DupAttr;
        DupAttr.reserve(DupedVerts.size());
        for (size_t j = 0; j < DupedVerts.size(); ++j)
        {
            DupAttr.push_back(UNUSED32);
        }

        std::unordered_multimap<uint32_t, size_t> Dups;

        for (size_t FaceIt = 0; FaceIt < NumFaces; ++FaceIt)
        {
            uint32_t Attr = Attributes[FaceIt];

            for (size_t PointIt = 0; PointIt < 3; ++PointIt)
            {
                uint32_t Index = IndicesNew[FaceIt * 3 + PointIt];

                const uint32_t K = (Index >= NumVerts) ? DupAttr[Index - NumVerts] : IDs[Index];

                if (K == UNUSED32)
                {
                    if (Index >= NumVerts)
                        DupAttr[Index - NumVerts] = Attr;
                    else
                        IDs[Index] = Attr;
                }
                else if (K != Attr)
                {
                    // Look for a dup with the correct attribute
                    auto Range = Dups.equal_range(Index);
                    auto It = Range.first;
                    for (; It != Range.second; ++It)
                    {
                        const uint32_t M = (It->second >= NumVerts) ? DupAttr[It->second - NumVerts] : IDs[It->second];
                        if (M == Attr)
                        {
                            IndicesNew[FaceIt * 3 + PointIt] = index_t(It->second);
                            break;
                        }
                    }

                    if (It == Range.second)
                    {
                        // Duplicate the vert
                        auto DuplicatedVert = std::pair<uint32_t, size_t>(Index, CurNewVert);
                        Dups.insert(DuplicatedVert);

                        IndicesNew[FaceIt * 3 + PointIt] = index_t(CurNewVert);
                        ++CurNewVert;

                        if (Index >= NumVerts)
                        {
                            DupedVerts.push_back(DupedVerts[Index - NumVerts]);
                        }
                        else
                        {
                            DupedVerts.push_back(Index);
                        }

                        DupAttr.push_back(Attr);

                        ASSERTMSG(DupedVerts.size() == DupAttr.size(), "Invalid duplicated vertex");
                    }
                }
            }
        }

        ASSERTMSG((NumVerts + DupedVerts.size()) == CurNewVert, "Invalid vertex");

#ifndef NDEBUG
        for (const auto It : DupedVerts)
        {
            ASSERTMSG(It < NumVerts, "Invalid index");
        }
#endif
    }

    if ((uint64_t(NumVerts) + uint64_t(DupedVerts.size())) >= index_t(-1))
    {
        return RET_ARITHMETIC_OVERFLOW;
    }

    if (!DupedVerts.empty())
    {
        memcpy(Indices, IndicesNew, sizeof(index_t) * NumFaces * 3);
    }

    return true;
}

bool MeshProcessing::AttributeSort(size_t NumFaces, uint32_t* Attributes, uint32_t* FaceRemap)
{
    if (!NumFaces || !Attributes || !FaceRemap)
        return RET_INVALID_ARGS;

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
        return RET_ARITHMETIC_OVERFLOW;

    using intpair_t = std::pair<uint32_t, uint32_t>;

    std::vector<intpair_t> List;
    List.reserve(NumFaces);
    for (size_t FaceIt = 0; FaceIt < NumFaces; ++FaceIt)
    {
        List.emplace_back(intpair_t(Attributes[FaceIt], static_cast<uint32_t>(FaceIt)));
    }

    std::stable_sort(List.begin(), List.end(), [](const intpair_t& A, const intpair_t& B) noexcept -> bool
    {
        return (A.first < B.first);
    });

    auto It = List.begin();
    for (size_t FaceIt = 0; FaceIt < NumFaces; ++FaceIt, ++It)
    {
        Attributes[FaceIt] = It->first;
        FaceRemap[FaceIt] = It->second;
    }

    return true;
}

bool MeshProcessing::ReorderIndices(index_t* Indices, size_t NumFaces, const uint32_t* FaceRemap, index_t* OutIndices) noexcept
{
    if (!Indices || !NumFaces || !FaceRemap || !OutIndices)
    {
        LOGERROR("Invalid Args");
        return false;
    }

    if (uint64_t(NumFaces) * 3 >= UINT32_MAX)
    {
        LOGERROR("Arithmetic overflow");
        return false;
    }

    if (Indices == OutIndices)
    {
        LOGERROR("Unsupported");
        return false;
    }

    for (size_t j = 0; j < NumFaces; ++j)
    {
        const uint32_t Src = FaceRemap[j];

        if (Src == UNUSED32)
            continue;

        if (Src < NumFaces)
        {
            OutIndices[j * 3] = Indices[Src * 3 + 2];
            OutIndices[j * 3 + 1] = Indices[Src * 3 + 1];
            OutIndices[j * 3 + 2] = Indices[Src * 3];
        }
        else
            return false;
    }

    return true;
}

float ComputeVertexCacheScore(uint32_t CachePosition, uint32_t VertexCacheSize) noexcept
{
    constexpr float FindVertexScore_CacheDecayPower = 1.5f;
    constexpr float FindVertexScore_LastTriScore = 0.75f;

    float Score = 0.0f;
    if (CachePosition >= VertexCacheSize)
    {
        // Vertex is not in FIFO cache - no score.
    }
    else
    {
        if (CachePosition < 3)
        {
            // This vertex was used in the last triangle,
            // so it has a fixed score, whichever of the three
            // it's in. Otherwise, you can get very different
            // answers depending on whether you add
            // the triangle 1,2,3 or 3,1,2 - which is silly.

            Score = FindVertexScore_LastTriScore;
        }
        else
        {
            // Points for being high in the cache.
            const float Scaler = 1.0f / float(VertexCacheSize - 3u);
            Score = 1.0f - float(CachePosition - 3u) * Scaler;
            Score = powf(Score, FindVertexScore_CacheDecayPower);
        }
    }

    return Score;
}

float ComputeVertexValenceScore(uint32_t NumActiveFaces) noexcept
{
    constexpr float FindVertexScore_ValenceBoostScale = 2.0f;
    constexpr float FindVertexScore_ValenceBoostPower = 0.5f;

    float Score = 0.f;

    // Bonus points for having a low number of tris still to
    // use the vert, so we get rid of lone verts quickly.
    const float ValenceBoost = powf(static_cast<float>(NumActiveFaces),
        -FindVertexScore_ValenceBoostPower);

    Score += FindVertexScore_ValenceBoostScale * ValenceBoost;
    return Score;
}

enum { kMaxVertexCacheSize = 64 };
enum { kMaxPrecomputedVertexValenceScores = 64 };

float s_VertexCacheScores[kMaxVertexCacheSize + 1][kMaxVertexCacheSize];
float s_VertexValenceScores[kMaxPrecomputedVertexValenceScores];

std::once_flag s_InitOnce;

void ComputeVertexScores() noexcept
{
    for (uint32_t CacheSize = 0; CacheSize <= kMaxVertexCacheSize; ++CacheSize)
    {
        for (uint32_t cachePos = 0; cachePos < CacheSize; ++cachePos)
        {
            s_VertexCacheScores[CacheSize][cachePos] = ComputeVertexCacheScore(cachePos, CacheSize);
        }
    }

    for (uint32_t valence = 0; valence < kMaxPrecomputedVertexValenceScores; ++valence)
    {
        s_VertexValenceScores[valence] = ComputeVertexValenceScore(valence);
    }
}

float FindVertexScore(uint32_t NumActiveFaces, uint32_t CachePosition, uint32_t VertexCacheSize) noexcept
{
    if (NumActiveFaces == 0)
    {
        // No tri needs this vertex!

        return -1.0f;
    }

    float Score = 0.f;

    if (CachePosition < VertexCacheSize)
    {
        Score += s_VertexCacheScores[VertexCacheSize][CachePosition];
    }

    if (NumActiveFaces < kMaxPrecomputedVertexValenceScores)
    {
        Score += s_VertexValenceScores[NumActiveFaces];
    }
    else
    {
        Score += ComputeVertexValenceScore(NumActiveFaces);
    }

    return Score;
}

struct OptimizeVertexData_s
{
    float       Score;
    uint32_t    ActiveFaceListStart;
    uint32_t    ActiveFaceListSize;
    index_t     CachePos0;
    index_t     CachePos1;

    OptimizeVertexData_s() noexcept : Score(0.f), ActiveFaceListStart(0), ActiveFaceListSize(0), CachePos0(0), CachePos1(0) {}
};

template <typename T>
struct IndexSortCompareIndexed_s
{
    const index_t* IndexData;

    IndexSortCompareIndexed_s(const index_t* InIndexData) noexcept : IndexData(InIndexData) {}

    bool operator()(T A, T B) const noexcept
    {
        index_t IndexA = IndexData[A];
        index_t IndexB = IndexData[B];

        if (IndexA < IndexB)
            return true;

        return false;
    }
};

template <typename T>
struct FaceValenceSort_s
{
    const OptimizeVertexData_s* VertexData;

    FaceValenceSort_s(const OptimizeVertexData_s* InVertexData) noexcept : VertexData(InVertexData) {}

    bool operator()(T A, T B) const noexcept
    {
        const OptimizeVertexData_s* VA = VertexData + size_t(A) * 3;
        const OptimizeVertexData_s* VB = VertexData + size_t(B) * 3;

        const uint32_t AValence = VA[0].ActiveFaceListSize + VA[1].ActiveFaceListSize + VA[2].ActiveFaceListSize;
        const uint32_t BValence = VB[0].ActiveFaceListSize + VB[1].ActiveFaceListSize + VB[2].ActiveFaceListSize;

        // higher scoring faces are those with lower valence totals

        // reverse sort (reverse of reverse)

        if (AValence < BValence)
            return true;

        return false;
    }
};

constexpr uint32_t kDefaultLRUCacheSize = 32;

bool MeshProcessing::OptimizeFacesLRU(index_t* Indices, size_t NumFaces, uint32_t* FaceRemap)
{
    if (!Indices || !NumFaces || !FaceRemap)
    {
        LOGERROR("Invalid args");
        return false;
    }

    if (uint64_t(NumFaces) * 3 >= UINT32_MAX)
    {
        LOGERROR("Arithmetic overflow");
        return false;
    }

    std::call_once(s_InitOnce, ComputeVertexScores);

    uint32_t IndexCount = static_cast<uint32_t>(NumFaces * 3);

    std::unique_ptr<OptimizeVertexData_s[]> VertexDataList(new (std::nothrow) OptimizeVertexData_s[IndexCount]);
    if (!VertexDataList)
    {
        LOGERROR("Out of memory!");
        return false;
    }

    std::unique_ptr<uint32_t[]> VertexRemap(new (std::nothrow) uint32_t[IndexCount]);
    std::unique_ptr<uint32_t[]> ActiveFaceList(new (std::nothrow) uint32_t[IndexCount]);
    if (!VertexRemap || !ActiveFaceList)
    {
        LOGERROR("Out of memory!");
        return false;
    }

    const uint32_t FaceCount = IndexCount / 3;

    std::unique_ptr<uint8_t[]> ProcessedFaceList(new (std::nothrow) uint8_t[FaceCount]);
    std::unique_ptr<uint32_t[]> FaceSorted(new (std::nothrow) uint32_t[FaceCount]);
    std::unique_ptr<uint32_t[]> FaceReverseLookup(new (std::nothrow) uint32_t[FaceCount]);
    if (!ProcessedFaceList || !FaceSorted || !FaceReverseLookup)
    {
        LOGERROR("Out of memory!");
        return false;
    }

    memset(ProcessedFaceList.get(), 0, sizeof(uint8_t) * FaceCount);

    // build the vertex remap table
    uint32_t UniqueVertexCount = 0;
    uint32_t Unused = 0;
    {
        using indexSorter = IndexSortCompareIndexed_s<uint32_t>;

        std::unique_ptr<uint32_t[]> IndexSorted(new (std::nothrow) uint32_t[IndexCount]);
        if (!IndexSorted)
        {
            LOGERROR("Out of memory!");
            return false;
        }

        for (uint32_t i = 0; i < IndexCount; i++)
        {
            IndexSorted[i] = i;
        }

        const indexSorter SortFunc(Indices);
        std::sort(IndexSorted.get(), IndexSorted.get() + IndexCount, SortFunc);

        bool First = false;
        for (uint32_t IndexIt = 0; IndexIt < IndexCount; IndexIt++)
        {
            const uint32_t idx = IndexSorted[IndexIt];
            if (Indices[idx] == index_t(-1))
            {
                Unused++;
                VertexRemap[idx] = UNUSED32;
                continue;
            }

            if (!IndexIt || First || SortFunc(IndexSorted[IndexIt - 1], idx))
            {
                // it's not a duplicate
                VertexRemap[idx] = UniqueVertexCount;
                UniqueVertexCount++;
                First = false;
            }
            else
            {
                VertexRemap[IndexSorted[IndexIt]] = VertexRemap[IndexSorted[IndexIt - 1]];
            }
        }
    }

    // compute face count per vertex
    for (uint32_t IndexIt = 0; IndexIt < IndexCount; ++IndexIt)
    {
        if (VertexRemap[IndexIt] == UNUSED32)
            continue;

        OptimizeVertexData_s& VertexData = VertexDataList[VertexRemap[IndexIt]];
        VertexData.ActiveFaceListSize++;
    }

    constexpr index_t kEvictedCacheIndex = std::numeric_limits<index_t>::max();
    {
        // allocate face list per vertex
        uint32_t CurActiveFaceListPos = 0;
        for (uint32_t i = 0; i < UniqueVertexCount; ++i)
        {
            OptimizeVertexData_s& VertexData = VertexDataList[i];
            VertexData.CachePos0 = kEvictedCacheIndex;
            VertexData.CachePos1 = kEvictedCacheIndex;
            VertexData.ActiveFaceListStart = CurActiveFaceListPos;
            CurActiveFaceListPos += VertexData.ActiveFaceListSize;
            VertexData.Score = FindVertexScore(VertexData.ActiveFaceListSize, VertexData.CachePos0, kDefaultLRUCacheSize);

            VertexData.ActiveFaceListSize = 0;
        }

#ifndef NDEBUG
        CHECK(CurActiveFaceListPos == (IndexCount - Unused));
#else
        std::ignore = Unused;
#endif
    }

    // sort unprocessed faces by highest score
    for (uint32_t FaceIt = 0; FaceIt < FaceCount; FaceIt++)
    {
        FaceSorted[FaceIt] = FaceIt;
    }

    const FaceValenceSort_s<uint32_t> FaceValenceSort(VertexDataList.get());
    std::sort(FaceSorted.get(), FaceSorted.get() + FaceCount, FaceValenceSort);

    for (uint32_t FaceIt = 0; FaceIt < FaceCount; FaceIt++)
    {
        FaceReverseLookup[FaceSorted[FaceIt]] = FaceIt;
    }

    // fill out face list per vertex
    for (uint32_t TriIt = 0; TriIt < IndexCount; TriIt += 3)
    {
        for (uint32_t IndexIt = 0; IndexIt < 3; ++IndexIt)
        {
            const uint32_t V = VertexRemap[size_t(TriIt) + size_t(IndexIt)];
            if (V == UNUSED32)
                continue;

            OptimizeVertexData_s& VertexData = VertexDataList[V];
            ActiveFaceList[size_t(VertexData.ActiveFaceListStart) + VertexData.ActiveFaceListSize] = TriIt;
            VertexData.ActiveFaceListSize++;
        }
    }

    uint32_t VertexCacheBuffer[(kMaxVertexCacheSize + 3) * 2] = {};
    uint32_t* Cache0 = VertexCacheBuffer;
    uint32_t* Cache1 = VertexCacheBuffer + (kMaxVertexCacheSize + 3);
    uint32_t EntriesInCache0 = 0;

    uint32_t BestFace = 0;
    for (size_t TriIt = 0; TriIt < IndexCount; TriIt += 3)
    {
        if (VertexRemap[TriIt] == UNUSED32
            || VertexRemap[TriIt + 1] == UNUSED32
            || VertexRemap[TriIt + 2] == UNUSED32)
        {
            ++BestFace;
            continue;
        }
        else
            break;
    }

    float BestScore = -1.f;

    uint32_t NextBestFace = 0;

    uint32_t CurFace = 0;
    for (size_t TriIt = 0; TriIt < IndexCount; TriIt += 3)
    {
        if (VertexRemap[TriIt] == UNUSED32
            || VertexRemap[TriIt + 1] == UNUSED32
            || VertexRemap[TriIt + 2] == UNUSED32)
        {
            continue;
        }

        if (BestScore < 0.f)
        {
            // no verts in the cache are used by any unprocessed faces so
            // search all unprocessed faces for a new starting point
            while (NextBestFace < FaceCount)
            {
                const uint32_t FaceIndex = FaceSorted[NextBestFace++];
                if (ProcessedFaceList[FaceIndex] == 0)
                {
                    const uint32_t Face = FaceIndex * 3;
                    const uint32_t I0 = VertexRemap[Face];
                    const uint32_t I1 = VertexRemap[size_t(Face) + 1];
                    const uint32_t I2 = VertexRemap[size_t(Face) + 2];
                    if (I0 != UNUSED32 && I1 != UNUSED32 && I2 != UNUSED32)
                    {
                        // we're searching a pre-sorted list, first one we find will be the best
                        BestFace = Face;
                        BestScore = VertexDataList[I0].Score
                            + VertexDataList[I1].Score
                            + VertexDataList[I2].Score;
                        break;
                    }
                }
            }
            CHECK(BestScore >= 0.f);
        }

        ProcessedFaceList[BestFace / 3] = 1;
        uint16_t EntriesInCache1 = 0;

        FaceRemap[CurFace] = (BestFace / 3);
        CurFace++;

        // add bestFace to LRU cache
        CHECK(VertexRemap[BestFace] != UNUSED32);
        CHECK(VertexRemap[size_t(BestFace) + 1] != UNUSED32);
        CHECK(VertexRemap[size_t(BestFace) + 2] != UNUSED32);

        for (size_t VertIt = 0; VertIt < 3; ++VertIt)
        {
            OptimizeVertexData_s& VertexData = VertexDataList[VertexRemap[BestFace + VertIt]];

            if (VertexData.CachePos1 >= EntriesInCache1)
            {
                VertexData.CachePos1 = EntriesInCache1;
                Cache1[EntriesInCache1++] = VertexRemap[BestFace + VertIt];

                if (VertexData.ActiveFaceListSize == 1)
                {
                    --VertexData.ActiveFaceListSize;
                    continue;
                }
            }

            CHECK(VertexData.ActiveFaceListSize > 0);
            uint32_t* Begin = ActiveFaceList.get() + VertexData.ActiveFaceListStart;
            uint32_t* End = ActiveFaceList.get() + (size_t(VertexData.ActiveFaceListStart) + VertexData.ActiveFaceListSize);
            uint32_t* It = std::find(Begin, End, BestFace);

            CHECK(It != End);

            std::swap(*It, *(End - 1));

            --VertexData.ActiveFaceListSize;
            VertexData.Score = FindVertexScore(VertexData.ActiveFaceListSize, VertexData.CachePos1, kDefaultLRUCacheSize);

            // need to re-sort the faces that use this vertex, as their score will change due to activeFaceListSize shrinking
            for (const uint32_t* FaceIt = Begin; FaceIt != End - 1; ++FaceIt)
            {
                const uint32_t FaceIndex = *FaceIt / 3;
                uint32_t N = FaceReverseLookup[FaceIndex];
                CHECK(FaceSorted[N] == FaceIndex);

                // found it, now move it up
                while (N > 0)
                {
                    if (FaceValenceSort(N, N - 1))
                    {
                        FaceReverseLookup[FaceSorted[N]] = N - 1;
                        FaceReverseLookup[FaceSorted[N - 1]] = N;
                        std::swap(FaceSorted[N], FaceSorted[N - 1]);
                        N--;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        // move the rest of the old verts in the cache down and compute their new scores
        for (uint32_t CacheIt = 0; CacheIt < EntriesInCache0; ++CacheIt)
        {
            OptimizeVertexData_s& VertexData = VertexDataList[Cache0[CacheIt]];

            if (VertexData.CachePos1 >= EntriesInCache1)
            {
                VertexData.CachePos1 = EntriesInCache1;
                Cache1[EntriesInCache1++] = Cache0[CacheIt];
                VertexData.Score = FindVertexScore(VertexData.ActiveFaceListSize, VertexData.CachePos1, kDefaultLRUCacheSize);

                // don't need to re-sort this vertex... once it gets out of the cache, it'll have its original score
            }
        }

        // find the best scoring triangle in the current cache (including up to 3 that were just evicted)
        BestScore = -1.f;

        for (uint32_t CacheIt = 0; CacheIt < EntriesInCache1; ++CacheIt)
        {
            OptimizeVertexData_s& vertexData = VertexDataList[Cache1[CacheIt]];
            vertexData.CachePos0 = vertexData.CachePos1;
            vertexData.CachePos1 = kEvictedCacheIndex;

            for (uint32_t FaceIt = 0; FaceIt < vertexData.ActiveFaceListSize; ++FaceIt)
            {
                const uint32_t Face = ActiveFaceList[size_t(vertexData.ActiveFaceListStart) + FaceIt];
                float FaceScore = 0.f;

                for (uint32_t VertIt = 0; VertIt < 3; VertIt++)
                {
                    const OptimizeVertexData_s& FaceVertexData = VertexDataList[VertexRemap[size_t(Face) + VertIt]];
                    FaceScore += FaceVertexData.Score;
                }

                if (FaceScore > BestScore)
                {
                    BestScore = FaceScore;
                    BestFace = Face;
                }
            }
        }

        std::swap(Cache0, Cache1);

        EntriesInCache0 = std::min<uint32_t>(EntriesInCache1, kDefaultLRUCacheSize);
    }

    for (; CurFace < FaceCount; ++CurFace)
    {
        FaceRemap[CurFace] = UNUSED32;
    }

    return true;
}
bool MeshProcessing::OptimizeVertices(index_t* Indices, size_t NumFaces, size_t NumVerts, uint32_t* VertexRemap) noexcept
{
    if (!Indices || !NumFaces || !NumVerts || !VertexRemap)
        return RET_INVALID_ARGS;

    if (NumVerts >= index_t(-1))
        return RET_INVALID_ARGS;

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
        return RET_ARITHMETIC_OVERFLOW;

    std::unique_ptr<uint32_t[]> TempRemap(new (std::nothrow) uint32_t[NumVerts]);
    if (!TempRemap)
        return RET_OUT_OF_MEM;

    memset(TempRemap.get(), 0xff, sizeof(uint32_t) * NumVerts);

    uint32_t CurVertex = 0;
    for (size_t IndexIt = 0; IndexIt < (NumFaces * 3); ++IndexIt)
    {
        index_t CurIndex = Indices[IndexIt];
        if (CurIndex == index_t(-1))
            continue;

        if (CurIndex >= NumVerts)
            return RET_UNEXPECTED;

        if (TempRemap[CurIndex] == UNUSED32)
        {
            TempRemap[CurIndex] = CurVertex;
            ++CurVertex;
        }
    }

    // inverse lookup
    memset(VertexRemap, 0xff, sizeof(uint32_t) * NumVerts);

    size_t Unused = 0;

    for (uint32_t VertIt = 0; VertIt < NumVerts; ++VertIt)
    {
        uint32_t Vertindex = TempRemap[VertIt];
        if (Vertindex == UNUSED32)
        {
            ++Unused;
        }
        else
        {
            if (Vertindex >= NumVerts)
                return RET_UNEXPECTED;

            VertexRemap[Vertindex] = VertIt;
        }
    }

    return true;
}

bool MeshProcessing::FinalizeIndices(index_t* Indices, size_t NumFaces, const uint32_t* VertexRemap, size_t NumVerts, index_t* OutIndices) noexcept
{
    if (!Indices || !NumFaces || !VertexRemap || !NumVerts || !OutIndices)
        return RET_INVALID_ARGS;

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
        return RET_ARITHMETIC_OVERFLOW;

    if (NumVerts >= index_t(-1))
        return RET_INVALID_ARGS;

    std::unique_ptr<uint32_t[]> VertexRemapInverse(new (std::nothrow) uint32_t[NumVerts]);
    if (!VertexRemapInverse)
        return RET_OUT_OF_MEM;

    memset(VertexRemapInverse.get(), 0xff, sizeof(uint32_t) * NumVerts);

    for (uint32_t VertIt = 0; VertIt < NumVerts; ++VertIt)
    {
        if (VertexRemap[VertIt] != UNUSED32)
        {
            if (VertexRemap[VertIt] >= NumVerts)
                return RET_UNEXPECTED;

            VertexRemapInverse[VertexRemap[VertIt]] = VertIt;
        }
    }

    for (size_t IndexIt = 0; IndexIt < (NumFaces * 3); ++IndexIt)
    {
        index_t Index = Indices[IndexIt];
        if (Index == index_t(-1))
        {
            OutIndices[IndexIt] = index_t(-1);
            continue;
        }

        if (Index >= NumVerts)
            return RET_UNEXPECTED;

        const uint32_t Dest = VertexRemapInverse[Index];
        if (Dest == UNUSED32)
        {
            OutIndices[IndexIt] = Index;
            continue;
        }

        if (Dest < NumVerts)
        {
            OutIndices[IndexIt] = index_t(Dest);
        }
        else
            return false;
    }

    return true;
}

constexpr size_t kMaxStride = 2048;
bool SwapVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* VertexRemap) noexcept
{
    if (!Vertices || !Stride || !NumVerts || !VertexRemap)
        return RET_INVALID_ARGS;

    if (Stride > kMaxStride)
        return RET_INVALID_ARGS;

    std::unique_ptr<uint8_t[]> Temp(new (std::nothrow) uint8_t[((sizeof(bool) + sizeof(uint32_t)) * NumVerts) + Stride]);
    if (!Temp)
        return RET_OUT_OF_MEM;

    auto VertexRemapInverse = reinterpret_cast<uint32_t*>(Temp.get());

    memset(VertexRemapInverse, 0xff, sizeof(uint32_t) * NumVerts);

    for (uint32_t VertIt = 0; VertIt < NumVerts; ++VertIt)
    {
        if (VertexRemap[VertIt] != UNUSED32)
        {
            if (VertexRemap[VertIt] >= NumVerts)
                return RET_UNEXPECTED;

            VertexRemapInverse[VertexRemap[VertIt]] = VertIt;
        }
    }

    auto Moved = reinterpret_cast<bool*>(Temp.get() + sizeof(uint32_t) * NumVerts);
    memset(Moved, 0, sizeof(bool) * NumVerts);

    auto VBTemp = Temp.get() + ((sizeof(bool) + sizeof(uint32_t)) * NumVerts);

    auto Ptr = static_cast<uint8_t*>(Vertices);

    for (size_t VertIt = 0; VertIt < NumVerts; ++VertIt)
    {
        if (Moved[VertIt])
            continue;

        uint32_t Dest = VertexRemapInverse[VertIt];

        if (Dest == UNUSED32)
            continue;

        if (Dest >= NumVerts)
            return RET_UNEXPECTED;

        bool Next = false;

        while (Dest != VertIt)
        {
            // Swap vertex
            memcpy(VBTemp, Ptr + Dest * Stride, Stride);
            memcpy(Ptr + Dest * Stride, Ptr + VertIt * Stride, Stride);
            memcpy(Ptr + VertIt * Stride, VBTemp, Stride);

            Moved[Dest] = true;

            Dest = VertexRemapInverse[Dest];

            if (Dest == UNUSED32 || Moved[Dest])
            {
                Next = true;
                break;
            }

            if (Dest >= NumVerts)
                return false;
        }

        if (Next)
            continue;
    }

    return true;
}

bool MeshProcessing::FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* VertexRemap) noexcept
{
    if (NumVerts >= UINT32_MAX)
        return RET_INVALID_ARGS;

    return SwapVertices(Vertices, Stride, NumVerts, VertexRemap);
}

bool MeshProcessing::FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* DupedVerts, size_t NumDupedVerts, const uint32_t* VertexRemap, void* OutVertices) noexcept
{
    if (!Vertices || !Stride || !NumVerts || !OutVertices)
        return RET_INVALID_ARGS;

    if (!DupedVerts && !VertexRemap)
        return RET_INVALID_ARGS;

    if (DupedVerts && !NumDupedVerts)
        return RET_INVALID_ARGS;

    if (!DupedVerts && NumDupedVerts > 0)
        return RET_INVALID_ARGS;

    if (NumVerts >= UINT32_MAX)
        return RET_INVALID_ARGS;

    if (Stride > kMaxStride)
        return RET_INVALID_ARGS;

    if ((uint64_t(NumVerts) + uint64_t(NumDupedVerts)) >= UINT32_MAX)
        return RET_ARITHMETIC_OVERFLOW;

    if (Vertices == OutVertices)
        return RET_UNSUPPORTED;

    const size_t NewVerts = NumVerts + NumDupedVerts;
    if (!NewVerts)
        return RET_INVALID_ARGS;

    auto SrcPtr = static_cast<const uint8_t*>(Vertices);
    auto DstPtr = static_cast<uint8_t*>(OutVertices);

#ifdef _DEBUG
    memset(OutVertices, 0, NewVerts * Stride);
#endif

    for (size_t VertIt = 0; VertIt < NewVerts; ++VertIt)
    {
        const uint32_t Src = (VertexRemap) ? VertexRemap[VertIt] : uint32_t(VertIt);

        if (Src == UNUSED32)
        {
            // remap entry is unused
        }
        else if (Src >= NewVerts)
        {
            return false;
        }
        else if (Src < NumVerts)
        {
            memcpy(DstPtr, SrcPtr + Src * Stride, Stride);
        }
        else if (DupedVerts)
        {
            const uint32_t Dupe = DupedVerts[Src - NumVerts];
            memcpy(DstPtr, SrcPtr + Dupe * Stride, Stride);
        }
        else
            return false;

        DstPtr += Stride;
    }

    return true;
}

std::vector<Subset_s> MeshProcessing::ComputeSubsets(const uint32_t* Attributes, size_t NumFaces)
{
    std::vector<Subset_s> Subsets;

    if (!NumFaces)
        return Subsets;

    if (!Attributes)
    {
        Subsets.emplace_back(0u, static_cast<uint32_t>(NumFaces));
        return Subsets;
    }

    uint32_t LastAttr = Attributes[0];
    size_t Offset = 0;
    size_t Count = 1;

    for (size_t FaceIt = 1; FaceIt < NumFaces; ++FaceIt)
    {
        if (Attributes[FaceIt] != LastAttr)
        {
            Subsets.emplace_back(static_cast<uint32_t>(Offset), static_cast<uint32_t>(Count));
            LastAttr = Attributes[FaceIt];
            Offset = FaceIt;
            Count = 1;
        }
        else
        {
            Count += 1;
        }
    }

    if (Count > 0)
    {
        Subsets.emplace_back(static_cast<uint32_t>(Offset), static_cast<uint32_t>(Count));
    }

    return Subsets;
}

struct AlignedDeleter_s { void operator()(void* p) noexcept { _aligned_free(p); } };
using AlignedArray_float4 = std::unique_ptr<float4[], AlignedDeleter_s>;
AlignedArray_float4 MakeAlignedArray_float4(uint64_t Count)
{
    const uint64_t Size = sizeof(float4) * Count;
    if (Size > static_cast<uint64_t>(UINT32_MAX))
        return nullptr;

    auto Ptr = _aligned_malloc(static_cast<size_t>(Size), 16);

    return AlignedArray_float4(static_cast<float4*>(Ptr));
}

bool ComputeNormalsWeightedByAngle(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts,float3* Normals) noexcept
{
    auto Temp = MakeAlignedArray_float4(NumVerts);
    if (!Temp)
        return RET_OUT_OF_MEM;

    float4* VertNormals = Temp.get();
    memset(VertNormals, 0, sizeof(float4) * NumVerts);

    for (size_t FaceIt = 0; FaceIt < NumFaces; ++FaceIt)
    {
        index_t I0 = Indices[FaceIt * 3];
        index_t I1 = Indices[FaceIt * 3 + 1];
        index_t I2 = Indices[FaceIt * 3 + 2];

        if (I0 == index_t(-1)
            || I1 == index_t(-1)
            || I2 == index_t(-1))
            continue;

        if (I0 >= NumVerts
            || I1 >= NumVerts
            || I2 >= NumVerts)
            return RET_UNEXPECTED;

        const float3 P0 = Positions[I0];
        const float3 P1 = Positions[I1];
        const float3 P2 = Positions[I2];

        const float3 U = P1 - P0;
        const float3 V = P2 - P0;

        const float3 FaceNormal = Normalize(Cross(U, V));

        // Corner 0 -> 1 - 0, 2 - 0
        const float3 A = Normalize(U);
        const float3 B = Normalize(V);
        float W0 = Dot(A, B);
        W0 = Clamp(W0, -1.0f, 1.0f);
        W0 = ACos(W0);

        // Corner 1 -> 2 - 1, 0 - 1
        const float3 C = Normalize(P2 - P1);
        const float3 D = Normalize(P0 - P1);
        float W1 = Dot(C, D);
        W1 = Clamp(W1, -1.0f, 1.0f);
        W1 = ACos(W1);

        // Corner 2 -> 0 - 2, 1 - 2
        const float3 E = Normalize(P0 - P2);
        const float3 F = Normalize(P1 - P2);
        float W2 = Dot(E, F);
        W2 = Clamp(W2, -1.0f, 1.0f);
        W2 = ACos(W2);

        VertNormals[I0] = MultiplyAdd(FaceNormal, W0, VertNormals[I0].xyz);
        VertNormals[I1] = MultiplyAdd(FaceNormal, W1, VertNormals[I1].xyz);
        VertNormals[I2] = MultiplyAdd(FaceNormal, W2, VertNormals[I2].xyz);
    }

    // Store results
    for (size_t VertIt = 0; VertIt < NumVerts; ++VertIt)
    {
        Normals[VertIt] = Normalize(VertNormals[VertIt].xyz);
    }

    return true;
}

bool MeshProcessing::ComputeNormals(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, float3* Normals) noexcept
{
    if (!Indices || !Positions || !NumFaces || !NumVerts || !Normals)
        return RET_INVALID_ARGS;

    if (NumVerts >= UINT16_MAX)
        return RET_INVALID_ARGS;

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
        return RET_ARITHMETIC_OVERFLOW;

    return ComputeNormalsWeightedByAngle(Indices, NumFaces, Positions, NumVerts, Normals);
}

bool MeshProcessing::ComputeTangents(const index_t* Indices, size_t NumFaces, const float3* Positions, const float3* Normals, const float2* Texcoords, size_t NumVerts, float4* Tangents4, float3* Bitangents) noexcept
{
    if (!Tangents4 && !Bitangents)
        return RET_INVALID_ARGS;

    if (!Indices || !NumFaces || !Positions || !Normals || !Texcoords || !NumVerts)
        return RET_INVALID_ARGS;

    if (NumVerts >= index_t(-1))
        return RET_INVALID_ARGS;

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
        return RET_ARITHMETIC_OVERFLOW;

    static constexpr float EPSILON = 0.0001f;
    static constexpr float4 kFlips = { 1.f, -1.f, -1.f, 1.f };

    auto Temp = MakeAlignedArray_float4(uint64_t(NumVerts) * 2);
    if (!Temp)
        return RET_OUT_OF_MEM;

    memset(Temp.get(), 0, sizeof(float4) * NumVerts * 2);

    float4* Tangent1 = Temp.get();
    float4* Tangent2 = Temp.get() + NumVerts;

    for (size_t FaceIt = 0; FaceIt < NumFaces; ++FaceIt)
    {
        index_t I0 = Indices[FaceIt * 3];
        index_t I1 = Indices[FaceIt * 3 + 1];
        index_t I2 = Indices[FaceIt * 3 + 2];

        if (I0 == index_t(-1)
            || I1 == index_t(-1)
            || I2 == index_t(-1))
            continue;

        if (I0 >= NumVerts
            || I1 >= NumVerts
            || I2 >= NumVerts)
            return RET_UNEXPECTED;

        const float2 T0 = Texcoords[I0];
        const float2 T1 = Texcoords[I1];
        const float2 T2 = Texcoords[I2];

        float4 S = MergeXY(T1 - T0, T2 - T0);

        float4 Tmp = S;

        float D = Tmp.x * Tmp.w - Tmp.z * Tmp.y;
        D = (fabsf(D) <= EPSILON) ? 1.f : (1.f / D);
        S = S * D;
        S = S * kFlips;

        matrix M0;
        M0.r[0] = float4{ S.w, S.z, 0.0f, 0.0f };
        M0.r[1] = float4{ S.y, S.x, 0.0f, 0.0f };

        M0.r[2] = M0.r[3] = float4{ 0.0f };

        const float3 P0 = Positions[I0];
        const float3 P1 = Positions[I1];
        float3 P2 = Positions[I2];

        matrix M1;
        M1.r[0] = P1 - P0;
        M1.r[1] = P2 - P0;
        M1.r[2] = M1.r[3] = float4{ 0.0f };

        const matrix UV = M0 * M1;

        Tangent1[I0] = Tangent1[I0] + UV.r[0];
        Tangent1[I1] = Tangent1[I1] + UV.r[0];
        Tangent1[I2] = Tangent1[I2] + UV.r[0];
                                    
        Tangent2[I0] = Tangent2[I0] + UV.r[1];
        Tangent2[I1] = Tangent2[I1] + UV.r[1];
        Tangent2[I2] = Tangent2[I2] + UV.r[1];
    }

    for (size_t VertIt = 0; VertIt < NumVerts; ++VertIt)
    {
        // Gram-Schmidt orthonormalization
        float4 B0 = Normals[VertIt];
        B0 = Normalize(B0.xyz);

        const float4 Tan1 = Tangent1[VertIt];
        float4 B1 = Tan1 - (Dot3(B0, Tan1) * B0);
        B1 = Normalize(B1.xyz);

        const float4 Tan2 = Tangent2[VertIt];

        float4 B2 = ((Tan2 - (Dot3(B0, Tan2) * B0)) - (Dot3(B1, Tan2) * B1));

        B2 = Normalize(B2.xyz);

        // handle degenerate vectors
        const float Len1 = Length(B1);
        const float Len2 = Length(B2);

        if ((Len1 <= EPSILON) || (Len2 <= EPSILON))
        {
            if (Len1 > 0.5f)
            {
                // Reset bi-tangent from tangent and normal
                B2 = Cross3(B0, B1);
            }
            else if (Len2 > 0.5f)
            {
                // Reset tangent from bi-tangent and normal
                B1 = Cross3(B2, B0);
            }
            else
            {
                // Reset both tangent and bi-tangent from normal
                float4 Axis;

                const float D0 = fabsf(Dot3(K_IdentityR0, B0));
                const float D1 = fabsf(Dot3(K_IdentityR1, B0));
                const float D2 = fabsf(Dot3(K_IdentityR2, B0));
                if (D0 < D1)
                {
                    Axis = (D0 < D2) ? K_IdentityR0 : K_IdentityR2;
                }
                else if (D1 < D2)
                {
                    Axis = K_IdentityR1;
                }
                else
                {
                    Axis = K_IdentityR2;
                }

                B1 = Cross3(B0, Axis);
                B2 = Cross3(B0, B1);
            }
        }

        if (Tangents4)
        {
            float4 Bi = Cross3(B0, Tan1);
            const float W = AllLess(Dot3(Bi, Tan2), k_Vec3Zero) ? -1.f : 1.f;

            Tangents4[VertIt] = float4{ Bi.xyz, W };
        }

        if (Bitangents)
        {
            Bitangents[VertIt] = B2.xyz;
        }
    }

    return true;
}

namespace
{
    struct EdgeEntry_s
    {
        uint32_t   I0;
        uint32_t   I1;
        uint32_t   I2;

        uint32_t   Face;
        EdgeEntry_s* Next;
    };

    size_t CRCHash(const uint32_t* DWords, uint32_t DWordCount)
    {
        size_t H = 0;

        for (uint32_t DWordIt = 0; DWordIt < DWordCount; ++DWordIt)
        {
            uint32_t HighOrd = H & 0xf8000000;
            H = H << 5;
            H = H ^ (HighOrd >> 27);
            H = H ^ size_t(DWords[DWordIt]);
        }

        return H;
    }

    template <typename T>
    inline size_t Hash(const T& Value)
    {
        return std::hash<T>()(Value);
    }
}

namespace std
{
    template <> struct hash<float3> { size_t operator()(const float3& Value) const { return CRCHash(reinterpret_cast<const uint32_t*>(&Value), sizeof(Value) / 4); } };
}

void BuildAdjacencyList(
    const index_t* Indices, uint32_t IndexCount,
    const float3* Positions, uint32_t VertexCount,
    uint32_t* Adjacency
)
{
    const uint32_t TriCount = IndexCount / 3;
    // Find point reps (unique positions) in the position stream
    // Create a mapping of non-unique vertex indices to point reps
    std::vector<index_t> PointRep;
    PointRep.resize(VertexCount);

    std::unordered_map<size_t, index_t> UniquePositionMap;
    UniquePositionMap.reserve(VertexCount);

    for (uint32_t VertexIt = 0; VertexIt < VertexCount; ++VertexIt)
    {
        float3 Position = *(Positions + VertexIt);
        size_t H = Hash(Position);

        auto It = UniquePositionMap.find(H);
        if (It != UniquePositionMap.end())
        {
            // Position already encountered - reference previous index
            PointRep[VertexIt] = It->second;
        }
        else
        {
            // New position found - add to hash table and LUT
            UniquePositionMap.insert(std::make_pair(H, static_cast<index_t>(VertexIt)));
            PointRep[VertexIt] = static_cast<index_t>(VertexIt);
        }
    }

    // Create a linked list of edges for each vertex to determine adjacency
    const uint32_t HashSize = VertexCount / 3;

    std::unique_ptr<EdgeEntry_s* []> HashTable(new EdgeEntry_s * [HashSize]);
    std::unique_ptr<EdgeEntry_s[]> Entries(new EdgeEntry_s[TriCount * 3]);

    std::memset(HashTable.get(), 0, sizeof(EdgeEntry_s*) * HashSize);
    uint32_t EntryIndex = 0;

    for (uint32_t FaceIt = 0; FaceIt < TriCount; ++FaceIt)
    {
        uint32_t Index = FaceIt * 3;

        // Create a hash entry in the hash table for each each.
        for (uint32_t EdgeIt = 0; EdgeIt < 3; ++EdgeIt)
        {
            index_t I0 = PointRep[Indices[Index + (EdgeIt % 3)]];
            index_t I1 = PointRep[Indices[Index + ((EdgeIt + 1) % 3)]];
            index_t I2 = PointRep[Indices[Index + ((EdgeIt + 2) % 3)]];

            auto& Entry = Entries[EntryIndex++];
            Entry.I0 = I0;
            Entry.I1 = I1;
            Entry.I2 = I2;

            uint32_t Key = Entry.I0 % HashSize;

            Entry.Next = HashTable[Key];
            Entry.Face = FaceIt;

            HashTable[Key] = &Entry;
        }
    }

    // Initialize the adjacency list
    std::memset(Adjacency, uint32_t(-1), IndexCount * sizeof(uint32_t));

    for (uint32_t FaceIt = 0; FaceIt < TriCount; ++FaceIt)
    {
        uint32_t Index = FaceIt * 3;

        for (uint32_t PointIt = 0; PointIt < 3; ++PointIt)
        {
            if (Adjacency[FaceIt * 3 + PointIt] != uint32_t(-1))
                continue;

            // Look for edges directed in the opposite direction.
            index_t I0 = PointRep[Indices[Index + ((PointIt + 1) % 3)]];
            index_t I1 = PointRep[Indices[Index + (PointIt % 3)]];
            index_t I2 = PointRep[Indices[Index + ((PointIt + 2) % 3)]];

            // Find a face sharing this edge
            uint32_t Key = I0 % HashSize;

            EdgeEntry_s* Found = nullptr;
            EdgeEntry_s* FoundPrev = nullptr;

            for (EdgeEntry_s* Current = HashTable[Key], *Prev = nullptr; Current != nullptr; Prev = Current, Current = Current->Next)
            {
                if (Current->I1 == I1 && Current->I0 == I0)
                {
                    Found = Current;
                    FoundPrev = Prev;
                    break;
                }
            }

            // Cache this face's normal
            float3 N0;
            {
                float3 P0 = Positions[I1];
                float3 P1 = Positions[I0];
                float3 P2 = Positions[I2];

                float3 E0 = P0 - P1;
                float3 E1 = P1 - P2;

                N0 = Normalize(Cross(E0, E1));
            }

            // Use face normal dot product to determine best edge-sharing candidate.
            float BestDot = -2.0f;
            for (EdgeEntry_s* Current = Found, *Prev = FoundPrev; Current != nullptr; Prev = Current, Current = Current->Next)
            {
                if (BestDot == -2.0f || (Current->I1 == I1 && Current->I0 == I0))
                {
                    float3 P0 = Positions[Current->I0];
                    float3 P1 = Positions[Current->I1];
                    float3 P2 = Positions[Current->I2];

                    float3 E0 = P0 - P1;
                    float3 E1 = P1 - P2;

                    float3 N1 = Normalize(Cross(E0, E1));

                    float D = Dot(N0, N1);

                    if (D > BestDot)
                    {
                        Found = Current;
                        FoundPrev = Prev;
                        BestDot = D;
                    }
                }
            }

            // Update hash table and adjacency list
            if (Found && Found->Face != uint32_t(-1))
            {
                // Erase the found from the hash table linked list.
                if (FoundPrev != nullptr)
                {
                    FoundPrev->Next = Found->Next;
                }
                else
                {
                    HashTable[Key] = Found->Next;
                }

                // Update adjacency information
                Adjacency[FaceIt * 3 + PointIt] = Found->Face;

                // Search & remove this face from the table linked list
                uint32_t Key2 = I1 % HashSize;

                for (EdgeEntry_s* Current = HashTable[Key2], *Prev = nullptr; Current != nullptr; Prev = Current, Current = Current->Next)
                {
                    if (Current->Face == FaceIt && Current->I1 == I0 && Current->I0 == I1)
                    {
                        if (Prev != nullptr)
                        {
                            Prev->Next = Current->Next;
                        }
                        else
                        {
                            HashTable[Key2] = Current->Next;
                        }

                        break;
                    }
                }

                bool Linked = false;
                for (uint32_t Point2It = 0; Point2It < PointIt; ++Point2It)
                {
                    if (Found->Face == Adjacency[FaceIt * 3 + Point2It])
                    {
                        Linked = true;
                        Adjacency[FaceIt * 3 + PointIt] = uint32_t(-1);
                        break;
                    }
                }

                if (!Linked)
                {
                    uint32_t Edge2 = 0;
                    for (; Edge2 < 3; ++Edge2)
                    {
                        index_t K = Indices[Found->Face * 3 + Edge2];
                        if (K == uint32_t(-1))
                            continue;

                        if (PointRep[K] == I0)
                            break;
                    }

                    if (Edge2 < 3)
                    {
                        Adjacency[Found->Face * 3 + Edge2] = FaceIt;
                    }
                }
            }
        }
    }
}

float3 ComputeNormal(float3* Tri)
{
    return Normalize(Cross(Tri[0] - Tri[1], Tri[0] - Tri[2]));
}

struct InlineMeshlet
{
    std::vector<index_t>                                UniqueVertexIndices;
    std::vector<MeshProcessing::PackedTriangle_s>       PrimitiveIndices;
};      

uint32_t ComputeReuse(const InlineMeshlet& Meshlet, index_t(&triIndices)[3])
{
    uint32_t Count = 0;

    for (uint32_t UniqueVertexIndexIt = 0; UniqueVertexIndexIt < static_cast<uint32_t>(Meshlet.UniqueVertexIndices.size()); ++UniqueVertexIndexIt)
    {
        for (uint32_t TriIndexIt = 0; TriIndexIt < 3u; ++TriIndexIt)
        {
            if (Meshlet.UniqueVertexIndices[UniqueVertexIndexIt] == triIndices[TriIndexIt])
            {
                ++Count;
            }
        }
    }

    return Count;
}

float ComputeScore(const InlineMeshlet& Meshlet, float4 Sphere, float4 Normal, index_t(&TriIndices)[3], float3* TriVerts)
{
    const float ReuseWeight = 0.334f;
    const float LocWeight = 0.333f;
    const float OriWeight = 0.333f;

    // Vertex reuse
    uint32_t Reuse = ComputeReuse(Meshlet, TriIndices);
    float4 ReuseScore = float4(1.0f) - (float4(float(Reuse)) / 3.0f);

    // Distance from center point
    float4 MaxSq = float4{0.0f};
    for (uint32_t VertIt = 0; VertIt < 3u; ++VertIt)
    {
        float4 V = Sphere - float4(TriVerts[VertIt]);
        MaxSq = MaxVector(MaxSq, float4(Dot(V.xyz, V.xyz)));
    }
    float4 Radius = float4(Sphere.w);
    float4 Radius2 = Radius * Radius;
    float4 LocScore = Log2(MaxSq / Radius2 + float4(1.0f));

    // Angle between normal and meshlet cone axis
    float4 N = ComputeNormal(TriVerts);
    float4 D = Dot(N.xyz, Normal.xyz);
    float4 OriScore = (-D + float4(1.0f)) / 2.0f;

    float4 B = ReuseWeight * ReuseScore + LocWeight * LocScore + OriWeight * OriScore;

    return B.x;
}

bool AddToMeshlet(uint32_t MaxVerts, uint32_t MaxPrims, InlineMeshlet& Meshlet, index_t(&Tri)[3])
{
    // Are we already full of vertices?
    if (Meshlet.UniqueVertexIndices.size() == MaxVerts)
        return false;

    // Are we full, or can we store an additional primitive?
    if (Meshlet.PrimitiveIndices.size() == MaxPrims)
        return false;

    static const uint32_t Undef = uint32_t(-1);
    uint32_t Indices[3] = { Undef, Undef, Undef };
    uint32_t NewCount = 3;

    for (uint32_t UniqueVertexIndexIt = 0; UniqueVertexIndexIt < Meshlet.UniqueVertexIndices.size(); ++UniqueVertexIndexIt)
    {
        for (uint32_t IndexIt = 0; IndexIt < 3; ++IndexIt)
        {
            if (Meshlet.UniqueVertexIndices[UniqueVertexIndexIt] == Tri[IndexIt])
            {
                Indices[IndexIt] = UniqueVertexIndexIt;
                --NewCount;
            }
        }
    }

    // Will this triangle fit?
    if (Meshlet.UniqueVertexIndices.size() + NewCount > MaxVerts)
        return false;

    // Add unique vertex indices to unique vertex index list
    for (uint32_t IndexIt = 0; IndexIt < 3; ++IndexIt)
    {
        if (Indices[IndexIt] == Undef)
        {
            Indices[IndexIt] = static_cast<uint32_t>(Meshlet.UniqueVertexIndices.size());
            Meshlet.UniqueVertexIndices.push_back(Tri[IndexIt]);
        }
    }

    // Add the new primitive 
    typename MeshProcessing::PackedTriangle_s Prim = {};
    Prim.Indices.I0 = Indices[0];
    Prim.Indices.I1 = Indices[1];
    Prim.Indices.I2 = Indices[2];

    Meshlet.PrimitiveIndices.push_back(Prim);

    return true;
}

bool IsMeshletFull(uint32_t MaxVerts, uint32_t MaxPrims, const InlineMeshlet& Meshlet)
{
    CHECK(Meshlet.UniqueVertexIndices.size() <= MaxVerts);
    CHECK(Meshlet.PrimitiveIndices.size() <= MaxPrims);

    return Meshlet.UniqueVertexIndices.size() == MaxVerts
        || Meshlet.PrimitiveIndices.size() == MaxPrims;
}

float4 MinimumBoundingSphere(float3* points, size_t count)
{
    CHECK(points != nullptr && count != 0);

    // Find the min & max points indices along each axis.
    uint32_t minAxis[3] = { 0, 0, 0 };
    uint32_t maxAxis[3] = { 0, 0, 0 };

    for (uint32_t i = 1; i < count; ++i)
    {
        float* point = (float*)(points + i);

        for (uint32_t j = 0; j < 3; ++j)
        {
            float* min = (float*)(&points[minAxis[j]]);
            float* max = (float*)(&points[maxAxis[j]]);

            minAxis[j] = point[j] < min[j] ? i : minAxis[j];
            maxAxis[j] = point[j] > max[j] ? i : maxAxis[j];
        }
    }

    // Find axis with maximum span.
    float distSqMax = 0;
    uint32_t axis = 0;

    for (uint32_t i = 0; i < 3u; ++i)
    {
        float3 min = points[minAxis[i]];
        float3 max = points[maxAxis[i]];

        float distSq = LengthSqr(max - min);
        if (distSq > distSqMax)
        {
            distSqMax = distSq;
            axis = i;
        }
    }

    // Calculate an initial starting center point & radius.
    float3 p1 = points[minAxis[axis]];
    float3 p2 = points[maxAxis[axis]];

    float3 center = (p1 + p2) * 0.5f;
    float radius = Length(p2 - p1) * 0.5f;
    float radiusSq = radius * radius;

    // Add all our points to bounding sphere expanding radius & recalculating center point as necessary.
    for (uint32_t i = 0; i < count; ++i)
    {
        float3 point = points[i];
        float distSq = LengthSqr(point - center);

        if (distSq > radiusSq)
        {
            float dist = sqrtf(distSq);
            float k = (radius / dist) * 0.5f + 0.5f;

            center = center * k + point * (1.0f - k);
            radius = (radius + dist) * 0.5f;
        }
    }
    return float4(center, radius);
}

void Meshletize(
    uint32_t MaxVerts, uint32_t MaxPrims,
    const index_t* Indices, uint32_t IndexCount,
    const float3* Positions, uint32_t VertexCount,
    std::vector<InlineMeshlet>& Output
)
{
    const uint32_t TriCount = IndexCount / 3;

    // Build a primitive adjacency list
    std::vector<uint32_t> Adjacency;
    Adjacency.resize(IndexCount);

    BuildAdjacencyList(Indices, IndexCount, Positions, VertexCount, Adjacency.data());

    // Rest our outputs
    Output.clear();
    Output.emplace_back();
    auto* Curr = &Output.back();

    // Bitmask of all triangles in mesh to determine whether a specific one has been added.
    std::vector<bool> Checklist;
    Checklist.resize(TriCount);

    std::vector<float3> TrackedPositions;
    std::vector<float3> TrackedNormals;
    std::vector<std::pair<uint32_t, float>> Candidates;
    std::unordered_set<uint32_t> CandidateCheck;

    float4 Sphere, Normal;

    // Arbitrarily start at triangle zero.
    uint32_t TriIndex = 0;
    Candidates.push_back(std::make_pair(TriIndex, 0.0f));
    CandidateCheck.insert(TriIndex);

    // Continue adding triangles until 
    while (!Candidates.empty())
    {
        uint32_t Index = Candidates.back().first;
        Candidates.pop_back();

        index_t Tri[3] =
        {
            Indices[Index * 3],
            Indices[Index * 3 + 1],
            Indices[Index * 3 + 2],
        };

        CHECK(Tri[0] < VertexCount);
        CHECK(Tri[1] < VertexCount);
        CHECK(Tri[2] < VertexCount);

        // Try to add triangle to meshlet
        if (AddToMeshlet(MaxVerts, MaxPrims, *Curr, Tri))
        {
            // Success! Mark as added.
            Checklist[Index] = true;

            // Add m_positions & normal to list
            float3 Points[3] =
            {
                Positions[Tri[0]],
                Positions[Tri[1]],
                Positions[Tri[2]],
            };

            TrackedPositions.push_back(Points[0]);
            TrackedPositions.push_back(Points[1]);
            TrackedPositions.push_back(Points[2]);

            float3 Normal = ComputeNormal(Points);
            TrackedNormals.push_back(Normal);

            // Compute new bounding sphere & normal axis

            BoundingSphere BSpherePos;
            BSpherePos.InitFromPoints(TrackedPositions.size(), TrackedPositions.data());

            BoundingSphere BSphereNorm;
            BSphereNorm.InitFromPoints(TrackedNormals.size(), TrackedNormals.data());
            Sphere = MinimumBoundingSphere(TrackedPositions.data(), TrackedPositions.size());// float4(BSpherePos.Origin, BSpherePos.Radius);
            Normal = Normalize(MinimumBoundingSphere(TrackedNormals.data(), TrackedNormals.size()).xyz);// Normalize(BSphereNorm.Origin);

            // Find and add all applicable adjacent triangles to candidate list
            const uint32_t AdjIndex = Index * 3;

            uint32_t Adj[3] =
            {
                Adjacency[AdjIndex],
                Adjacency[AdjIndex + 1],
                Adjacency[AdjIndex + 2],
            };

            for (uint32_t It = 0; It < 3u; ++It)
            {
                // Invalid triangle in adjacency slot
                if (Adj[It] == -1)
                    continue;

                // Already processed triangle
                if (Checklist[Adj[It]])
                    continue;

                // Triangle already in the candidate list
                if (CandidateCheck.count(Adj[It]))
                    continue;

                Candidates.push_back(std::make_pair(Adj[It], FLT_MAX));
                CandidateCheck.insert(Adj[It]);
            }

            // Re-score remaining candidate triangles
            for (uint32_t CandidateIt = 0; CandidateIt < static_cast<uint32_t>(Candidates.size()); ++CandidateIt)
            {
                uint32_t Candidate = Candidates[CandidateIt].first;

                index_t TriIndices[3] =
                {
                    Indices[Candidate * 3],
                    Indices[Candidate * 3 + 1],
                    Indices[Candidate * 3 + 2],
                };

                CHECK(TriIndices[0] < VertexCount);
                CHECK(TriIndices[1] < VertexCount);
                CHECK(TriIndices[2] < VertexCount);

                float3 TriVerts[3] =
                {
                    Positions[TriIndices[0]],
                    Positions[TriIndices[1]],
                    Positions[TriIndices[2]],
                };

                Candidates[CandidateIt].second = ComputeScore(*Curr, Sphere, Normal, TriIndices, TriVerts);
            }

            // Determine whether we need to move to the next meshlet.
            if (IsMeshletFull(MaxVerts, MaxPrims, *Curr))
            {
                TrackedPositions.clear();
                TrackedNormals.clear();
                CandidateCheck.clear();

                // Use one of our existing candidates as the next meshlet seed.
                if (!Candidates.empty())
                {
                    Candidates[0] = Candidates.back();
                    Candidates.resize(1);
                    CandidateCheck.insert(Candidates[0].first);
                }

                Output.emplace_back();
                Curr = &Output.back();
            }
            else
            {
                std::sort(Candidates.begin(), Candidates.end(), [](const std::pair<uint32_t, float>& a, const std::pair<uint32_t, float>& b)
                {
                    return a.second > b.second;
                });
            }
        }
        else
        {
            if (Candidates.empty())
            {
                TrackedPositions.clear();
                TrackedNormals.clear();
                CandidateCheck.clear();

                Output.emplace_back();
                Curr = &Output.back();
            }
        }

        // Ran out of candidates; add a new seed candidate to start the next meshlet.
        if (Candidates.empty())
        {
            while (TriIndex < TriCount && Checklist[TriIndex])
                ++TriIndex;

            if (TriIndex == TriCount)
                break;

            Candidates.push_back(std::make_pair(TriIndex, 0.0f));
            CandidateCheck.insert(TriIndex);
        }
    }

    // The last meshlet may have never had any primitives added to it - in which case we want to remove it.
    if (Output.back().PrimitiveIndices.empty())
    {
        Output.pop_back();
    }
}

bool MeshProcessing::ComputeMeshlets(
    uint32_t MaxVerts, uint32_t MaxPrims, 
    const index_t* Indices, uint32_t indexCount, 
    const Subset_s* IndexSubsets, uint32_t SubsetCount, 
    const float3* Positions, uint32_t VertexCount, 
    std::vector<Subset_s>& MeshletSubsets, 
    std::vector<Meshlet_s>& Meshlets, 
    std::vector<uint8_t>& UniqueVertexIndices, 
    std::vector<PackedTriangle_s>& PrimitiveIndices)
{
    for (uint32_t SubsetIt = 0; SubsetIt < SubsetCount; ++SubsetIt)
    {
        Subset_s Subset = IndexSubsets[SubsetIt];

        assert(Subset.Offset + Subset.Count <= indexCount);

        std::vector<InlineMeshlet> BuiltMeshlets;
        Meshletize(MaxVerts, MaxPrims, Indices + Subset.Offset, Subset.Count, Positions, VertexCount, BuiltMeshlets);

        Subset_s MeshletSubset;
        MeshletSubset.Offset = static_cast<uint32_t>(Meshlets.size());
        MeshletSubset.Count = static_cast<uint32_t>(BuiltMeshlets.size());
        MeshletSubsets.push_back(MeshletSubset);

        // Determine final unique vertex index and primitive index counts & offsets.
        uint32_t StartVertCount = static_cast<uint32_t>(UniqueVertexIndices.size()) / sizeof(index_t);
        uint32_t StartPrimCount = static_cast<uint32_t>(PrimitiveIndices.size());

        uint32_t UniqueVertexIndexCount = StartVertCount;
        uint32_t PrimitiveIndexCount = StartPrimCount;

        // Resize the meshlet output array to hold the newly formed meshlets.
        uint32_t MeshletCount = static_cast<uint32_t>(Meshlets.size());
        Meshlets.resize(MeshletCount + BuiltMeshlets.size());

        for (uint32_t BuiltMeshletIt = 0, Dest = MeshletCount; BuiltMeshletIt < static_cast<uint32_t>(BuiltMeshlets.size()); ++BuiltMeshletIt, ++Dest)
        {
            Meshlets[Dest].VertOffset = UniqueVertexIndexCount;
            Meshlets[Dest].VertCount = static_cast<uint32_t>(BuiltMeshlets[BuiltMeshletIt].UniqueVertexIndices.size());
            UniqueVertexIndexCount += static_cast<uint32_t>(BuiltMeshlets[BuiltMeshletIt].UniqueVertexIndices.size());

            Meshlets[Dest].PrimOffset = PrimitiveIndexCount;
            Meshlets[Dest].PrimCount = static_cast<uint32_t>(BuiltMeshlets[BuiltMeshletIt].PrimitiveIndices.size());
            PrimitiveIndexCount += static_cast<uint32_t>(BuiltMeshlets[BuiltMeshletIt].PrimitiveIndices.size());
        }

        // Allocate space for the new data.
        UniqueVertexIndices.resize(UniqueVertexIndexCount * sizeof(index_t));
        PrimitiveIndices.resize(PrimitiveIndexCount);

        // Copy data from the freshly built meshlets into the output buffers.
        auto VertDest = reinterpret_cast<index_t*>(UniqueVertexIndices.data()) + StartVertCount;
        auto PrimDest = reinterpret_cast<uint32_t*>(PrimitiveIndices.data()) + StartPrimCount;

        for (uint32_t BuiltMeshletIt = 0; BuiltMeshletIt < static_cast<uint32_t>(BuiltMeshlets.size()); ++BuiltMeshletIt)
        {
            std::memcpy(VertDest, BuiltMeshlets[BuiltMeshletIt].UniqueVertexIndices.data(), BuiltMeshlets[BuiltMeshletIt].UniqueVertexIndices.size() * sizeof(index_t));
            std::memcpy(PrimDest, BuiltMeshlets[BuiltMeshletIt].PrimitiveIndices.data(), BuiltMeshlets[BuiltMeshletIt].PrimitiveIndices.size() * sizeof(uint32_t));

            VertDest += BuiltMeshlets[BuiltMeshletIt].UniqueVertexIndices.size();
            PrimDest += BuiltMeshlets[BuiltMeshletIt].PrimitiveIndices.size();
        }
    }

    return true;
}
