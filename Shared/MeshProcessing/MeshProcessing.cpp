#include "MeshProcessing.h"
#include "Logging/Logging.h"

#include <algorithm>
#include <array>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <unordered_set>

namespace MeshProcessing
{

constexpr uint32_t UNUSED32 = uint32_t(-1);

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

bool CleanMesh(index_t* Indices, size_t NumFaces, size_t NumVerts, const uint32_t* Attributes, std::vector<uint32_t>& DupedVerts, bool BreakBowTies)
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

bool AttributeSort(size_t NumFaces, uint32_t* Attributes, uint32_t* FaceRemap)
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

bool ReorderIndices(index_t* Indices, size_t NumFaces, const uint32_t* FaceRemap, index_t* OutIndices) noexcept
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
            OutIndices[j * 3] = Indices[Src * 3];
            OutIndices[j * 3 + 1] = Indices[Src * 3 + 1];
            OutIndices[j * 3 + 2] = Indices[Src * 3 + 2];
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

bool OptimizeFacesLRU(index_t* Indices, size_t NumFaces, uint32_t* FaceRemap)
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
bool OptimizeVertices(index_t* Indices, size_t NumFaces, size_t NumVerts, uint32_t* VertexRemap) noexcept
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

bool FinalizeIndices(index_t* Indices, size_t NumFaces, const uint32_t* VertexRemap, size_t NumVerts, index_t* OutIndices) noexcept
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

bool FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* VertexRemap) noexcept
{
    if (NumVerts >= UINT32_MAX)
        return RET_INVALID_ARGS;

    return SwapVertices(Vertices, Stride, NumVerts, VertexRemap);
}

bool FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* DupedVerts, size_t NumDupedVerts, const uint32_t* VertexRemap, void* OutVertices) noexcept
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

std::vector<std::pair<size_t, size_t>> ComputeSubsets(const uint32_t* Attributes, size_t NumFaces)
{
    std::vector<std::pair<size_t, size_t>> Subsets;

    if (!NumFaces)
        return Subsets;

    if (!Attributes)
    {
        Subsets.emplace_back(std::pair<size_t, size_t>(0u, NumFaces));
        return Subsets;
    }

    uint32_t LastAttr = Attributes[0];
    size_t Offset = 0;
    size_t Count = 1;

    for (size_t FaceIt = 1; FaceIt < NumFaces; ++FaceIt)
    {
        if (Attributes[FaceIt] != LastAttr)
        {
            Subsets.emplace_back(std::pair<size_t, size_t>(Offset, Count));
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
        Subsets.emplace_back(std::pair<size_t, size_t>(Offset, Count));
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

bool ComputeNormals(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, float3* Normals) noexcept
{
    if (!Indices || !Positions || !NumFaces || !NumVerts || !Normals)
        return RET_INVALID_ARGS;

    if (NumVerts >= UINT16_MAX)
        return RET_INVALID_ARGS;

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
        return RET_ARITHMETIC_OVERFLOW;

    return ComputeNormalsWeightedByAngle(Indices, NumFaces, Positions, NumVerts, Normals);
}

bool ComputeTangents(const index_t* Indices, size_t NumFaces, const float3* Positions, const float3* Normals, const float2* Texcoords, size_t NumVerts, float4* Tangents4, float3* Bitangents) noexcept
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

struct VertexHashEntry_s
{
    float3              V;
    uint32_t            Index;
    VertexHashEntry_s*  Next;
};

struct EdgeHashEntry_s
{
    uint32_t            V1;
    uint32_t            V2;
    uint32_t            VOther;
    uint32_t            Face;
    EdgeHashEntry_s*    Next;
};

bool GeneratePointReps(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, uint32_t* OutPointRep) noexcept
{
    std::unique_ptr<uint32_t[]> Temp(new (std::nothrow) uint32_t[NumVerts + NumFaces * 3]);
    if (!Temp)
        return RET_OUT_OF_MEM;

    uint32_t* VertexToCorner = Temp.get();
    uint32_t* VertexCornerList = Temp.get() + NumVerts;

    memset(VertexToCorner, 0xff, sizeof(uint32_t) * NumVerts);
    memset(VertexCornerList, 0xff, sizeof(uint32_t) * NumFaces * 3);

    // build initial lists and validate indices
    for (size_t IndexIt = 0; IndexIt < (NumFaces * 3); ++IndexIt)
    {
        index_t Index = Indices[IndexIt];
        if (Index == index_t(-1))
            continue;

        if (Index >= NumVerts)
            return RET_UNEXPECTED;

        VertexCornerList[IndexIt] = VertexToCorner[Index];
        VertexToCorner[Index] = uint32_t(IndexIt);
    }

    auto HashSize = std::max<size_t>(NumVerts / 3, 1);

    std::unique_ptr<VertexHashEntry_s* []> HashTable(new (std::nothrow) VertexHashEntry_s * [HashSize]);
    if (!HashTable)
        return RET_OUT_OF_MEM;

    memset(HashTable.get(), 0, sizeof(VertexHashEntry_s*) * HashSize);

    std::unique_ptr<VertexHashEntry_s[]> HashEntries(new (std::nothrow) VertexHashEntry_s[NumVerts]);
    if (!HashEntries)
        return RET_OUT_OF_MEM;

    uint32_t FreeEntry = 0;

    for (size_t VertIt = 0; VertIt < NumVerts; ++VertIt)
    {
        auto PX = reinterpret_cast<const uint32_t*>(&Positions[VertIt].x);
        auto PY = reinterpret_cast<const uint32_t*>(&Positions[VertIt].y);
        auto PZ = reinterpret_cast<const uint32_t*>(&Positions[VertIt].z);
        const uint32_t HashKey = (*PX + *PY + *PZ) % uint32_t(HashSize);

        uint32_t Found = UNUSED32;

        for (auto Current = HashTable[HashKey]; Current != nullptr; Current = Current->Next)
        {
            if (Current->V.x == Positions[VertIt].x
                && Current->V.y == Positions[VertIt].y
                && Current->V.z == Positions[VertIt].z)
            {
                uint32_t Head = VertexToCorner[VertIt];

                bool IsPresent = false;

                while (Head != UNUSED32)
                {
                    const uint32_t Face = Head / 3;
                    CHECK(Face < NumFaces);

                    CHECK((Indices[Face * 3] == VertIt) || (Indices[Face * 3 + 1] == VertIt) || (Indices[Face * 3 + 2] == VertIt));

                    if ((Indices[Face * 3] == Current->Index) || (Indices[Face * 3 + 1] == Current->Index) || (Indices[Face * 3 + 2] == Current->Index))
                    {
                        IsPresent = true;
                        break;
                    }

                    Head = VertexCornerList[Head];
                }

                if (!IsPresent)
                {
                    Found = Current->Index;
                    break;
                }
            }
        }

        if (Found != UNUSED32)
        {
            OutPointRep[VertIt] = Found;
        }
        else
        {
            CHECK(FreeEntry < NumVerts);

            auto NewEntry = &HashEntries[FreeEntry];
            ++FreeEntry;

            NewEntry->V = Positions[VertIt];
            NewEntry->Index = uint32_t(VertIt);
            NewEntry->Next = HashTable[HashKey];
            HashTable[HashKey] = NewEntry;

            OutPointRep[VertIt] = uint32_t(VertIt);
        }
    }

    CHECK(FreeEntry <= NumVerts);

    return true;
}

bool ConvertPointRepsToAdjacencyImpl(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, const uint32_t* PointRep, uint32_t* OutAdjacency) noexcept
{
    auto HashSize = std::max<size_t>(NumVerts / 3, 1);

    std::unique_ptr<EdgeHashEntry_s* []> HashTable(new (std::nothrow) EdgeHashEntry_s * [HashSize]);
    if (!HashTable)
        return RET_OUT_OF_MEM;

    memset(HashTable.get(), 0, sizeof(EdgeHashEntry_s*) * HashSize);

    std::unique_ptr<EdgeHashEntry_s[]> HashEntries(new (std::nothrow) EdgeHashEntry_s[3 * NumFaces]);
    if (!HashEntries)
        return RET_OUT_OF_MEM;

    uint32_t FreeEntry = 0;

    // add face edges to hash table and validate indices
    for (size_t Face = 0; Face < NumFaces; ++Face)
    {
        index_t I0 = Indices[Face * 3];
        index_t I1 = Indices[Face * 3 + 1];
        index_t I2 = Indices[Face * 3 + 2];

        if (I0 == index_t(-1)
            || I1 == index_t(-1)
            || I2 == index_t(-1))
            continue;

        if (I0 >= NumVerts
            || I1 >= NumVerts
            || I2 >= NumVerts)
            return RET_UNEXPECTED;

        const uint32_t V1 = PointRep[I0];
        const uint32_t V2 = PointRep[I1];
        const uint32_t V3 = PointRep[I2];

        // filter out degenerate triangles
        if (V1 == V2 || V1 == V3 || V2 == V3)
            continue;

        for (uint32_t PointIt = 0; PointIt < 3; ++PointIt)
        {
            const uint32_t VA = PointRep[Indices[Face * 3 + PointIt]];
            const uint32_t VB = PointRep[Indices[Face * 3 + ((PointIt + 1) % 3)]];
            const uint32_t VOther = PointRep[Indices[Face * 3 + ((PointIt + 2) % 3)]];

            const uint32_t HashKey = VA % HashSize;

            CHECK(FreeEntry < (3 * NumFaces));

            auto NewEntry = &HashEntries[FreeEntry];
            ++FreeEntry;

            NewEntry->V1 = VA;
            NewEntry->V2 = VB;
            NewEntry->VOther = VOther;
            NewEntry->Face = uint32_t(Face);
            NewEntry->Next = HashTable[HashKey];
            HashTable[HashKey] = NewEntry;
        }
    }

    CHECK(FreeEntry <= (3 * NumFaces));

    memset(OutAdjacency, 0xff, sizeof(uint32_t) * NumFaces * 3);

    for (size_t FaceIt = 0; FaceIt < NumFaces; ++FaceIt)
    {
        index_t I0 = Indices[FaceIt * 3];
        index_t I1 = Indices[FaceIt * 3 + 1];
        index_t I2 = Indices[FaceIt * 3 + 2];

        // filter out unused triangles
        if (I0 == index_t(-1)
            || I1 == index_t(-1)
            || I2 == index_t(-1))
            continue;

        CHECK(I0 < NumVerts);
        CHECK(I1 < NumVerts);
        CHECK(I2 < NumVerts);

        const uint32_t V1 = PointRep[I0];
        const uint32_t V2 = PointRep[I1];
        const uint32_t V3 = PointRep[I2];

        // filter out degenerate triangles
        if (V1 == V2 || V1 == V3 || V2 == V3)
            continue;

        for (uint32_t PointIt = 0; PointIt < 3; ++PointIt)
        {
            if (OutAdjacency[FaceIt * 3 + PointIt] != UNUSED32)
                continue;

            // see if edge already entered, if not then enter it
            const uint32_t VA = PointRep[Indices[FaceIt * 3 + ((PointIt + 1) % 3)]];
            const uint32_t VB = PointRep[Indices[FaceIt * 3 + PointIt]];
            const uint32_t VOther = PointRep[Indices[FaceIt * 3 + ((PointIt + 2) % 3)]];

            const uint32_t HashKey = VA % HashSize;

            EdgeHashEntry_s* Current = HashTable[HashKey];
            EdgeHashEntry_s* Prev = nullptr;

            uint32_t FoundFace = UNUSED32;

            while (Current != nullptr)
            {
                if ((Current->V2 == VB) && (Current->V1 == VA))
                {
                    FoundFace = Current->Face;
                    break;
                }

                Prev = Current;
                Current = Current->Next;
            }

            EdgeHashEntry_s* Found = Current;
            EdgeHashEntry_s* FoundPrev = Prev;

            float BestDiff = -2.f;

            // Scan for additional matches
            if (Current)
            {
                Prev = Current;
                Current = Current->Next;

                // find 'better' match
                while (Current != nullptr)
                {
                    if ((Current->V2 == VB) && (Current->V1 == VA))
                    {
                        const float3 PB1 = Positions[VB];
                        const float3 PB2 = Positions[VA];
                        const float3 PB3 = Positions[VOther];

                        float3 V12 = PB1 - PB2;
                        float3 V13 = PB1 - PB3;

                        const float3 BNormal = Normalize(Cross(V12, V13));

                        if (BestDiff == -2.f)
                        {
                            const float3 PA1 = Positions[Found->V1];
                            const float3 PA2 = Positions[Found->V2];
                            const float3 PA3 = Positions[Found->VOther];

                            V12 = PA1 - PA2;
                            V13 = PA1 - PA3;

                            const float3 ANormal = Normalize(Cross(V12, V13));

                            BestDiff = Dot(ANormal, BNormal);
                        }

                        const float3 PA1 = Positions[Current->V1];
                        const float3 PA2 = Positions[Current->V2];
                        const float3 PA3 = Positions[Current->VOther];

                        V12 = PA1 - PA2;
                        V13 = PA1 - PA3;

                        const float3 ANormal = Normalize(Cross(V12, V13));

                        const float Diff = Dot(ANormal, BNormal);

                        // if face normals are closer, use new match
                        if (Diff > BestDiff)
                        {
                            Found = Current;
                            FoundPrev = Prev;
                            FoundFace = Current->Face;
                            BestDiff = Diff;
                        }
                    }

                    Prev = Current;
                    Current = Current->Next;
                }
            }

            if (FoundFace != UNUSED32)
            {
                CHECK(Found != nullptr);

                // remove found face from hash table
                if (FoundPrev != nullptr)
                {
                    FoundPrev->Next = Found->Next;
                }
                else
                {
                    HashTable[HashKey] = Found->Next;
                }

                CHECK(OutAdjacency[FaceIt * 3 + PointIt] == UNUSED32);
                OutAdjacency[FaceIt * 3 + PointIt] = FoundFace;

                // Check for other edge
                const uint32_t HashKey2 = VB % HashSize;

                Current = HashTable[HashKey2];
                Prev = nullptr;

                while (Current != nullptr)
                {
                    if ((Current->Face == uint32_t(FaceIt)) && (Current->V2 == VA) && (Current->V1 == VB))
                    {
                        // trim edge from hash table
                        if (Prev != nullptr)
                        {
                            Prev->Next = Current->Next;
                        }
                        else
                        {
                            HashTable[HashKey2] = Current->Next;
                        }
                        break;
                    }

                    Prev = Current;
                    Current = Current->Next;
                }

                // mark neighbor to point back
                bool Linked = false;

                for (uint32_t Point2It = 0; Point2It < PointIt; ++Point2It)
                {
                    if (FoundFace == OutAdjacency[FaceIt * 3 + Point2It])
                    {
                        Linked = true;
                        OutAdjacency[FaceIt * 3 + PointIt] = UNUSED32;
                        break;
                    }
                }

                if (!Linked)
                {
                    uint32_t Point2 = 0;
                    for (; Point2 < 3; ++Point2)
                    {
                        index_t Index = Indices[FoundFace * 3 + Point2];
                        if (Index == index_t(-1))
                            continue;

                        CHECK(Index < NumVerts);

                        if (PointRep[Index] == VA)
                            break;
                    }

                    if (Point2 < 3)
                    {
#ifndef NDEBUG
                        uint32_t TestPoint = Indices[FoundFace * 3 + ((Point2 + 1) % 3)];
                        TestPoint = PointRep[TestPoint];
                        CHECK(TestPoint == VB);
#endif
                        CHECK(OutAdjacency[FoundFace * 3 + Point2] == UNUSED32);

                        // update neighbor to point back to this face match edge
                        OutAdjacency[FoundFace * 3 + Point2] = uint32_t(FaceIt);
                    }
                }
            }
        }
    }

    return true;
}

bool GenerateAdjacencyAndPointReps(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, float Epsilon, uint32_t* PointRep, uint32_t* Adjacency)
{
    if (!Indices || !NumFaces || !Positions || !NumVerts)
        return RET_INVALID_ARGS;

    if (!PointRep && !Adjacency)
        return RET_INVALID_ARGS;

    if (NumVerts >= UINT32_MAX)
        return RET_INVALID_ARGS;

    if ((uint64_t(NumFaces) * 3) >= UINT32_MAX)
        return RET_ARITHMETIC_OVERFLOW;

    std::unique_ptr<uint32_t[]> Temp;
    if (!PointRep)
    {
        Temp.reset(new (std::nothrow) uint32_t[NumVerts]);
        if (!Temp)
            return RET_OUT_OF_MEM;

        PointRep = Temp.get();
    }

    if (!GeneratePointReps(Indices, NumFaces, Positions, NumVerts, PointRep))
        return false;

    if (!Adjacency)
        return true;

    return ConvertPointRepsToAdjacencyImpl(Indices, NumFaces, Positions, NumVerts, PointRep, Adjacency);
}

constexpr size_t MESHLET_DEFAULT_MAX_VERTS = 128u;
constexpr size_t MESHLET_DEFAULT_MAX_PRIMS = 128u;

constexpr size_t MESHLET_MINIMUM_SIZE = 32u;
constexpr size_t MESHLET_MAXIMUM_SIZE = 256u;

template <typename T, size_t N>
class StaticVector_c
{
public:
    StaticVector_c() noexcept
        : Data{}, Size(0)
    {
    }
    ~StaticVector_c() = default;

    StaticVector_c(StaticVector_c&&) = default;
    StaticVector_c& operator= (StaticVector_c&&) = default;

    StaticVector_c(StaticVector_c const&) = default;
    StaticVector_c& operator= (StaticVector_c const&) = default;

    void push_back(const T& value) noexcept
    {
        assert(Size < N);
        Data[Size++] = value;
    }

    void push_back(T&& value) noexcept
    {
        assert(Size < N);
        Data[Size++] = std::move(value);
    }

    template <typename... Args>
    void emplace_back(Args&&... args) noexcept
    {
        assert(Size < N);
        Data[Size++] = T(std::forward<Args>(args)...);
    }

    size_t size() const noexcept { return Size; }
    bool empty() const noexcept { return Size == 0; }

    T* data() noexcept { return Data.data(); }
    const T* data() const noexcept { return Data.data(); }

    T& operator[](size_t index) noexcept { assert(index < Size); return Data[index]; }
    const T& operator[](size_t index) const noexcept { assert(index < Size); return Data[index]; }

private:
    std::array<T, N> Data;
    size_t           Size;
};

struct InlineMeshlet_s
{
    StaticVector_c<index_t, MESHLET_MAXIMUM_SIZE>           UniqueVertexIndices;
    StaticVector_c<MeshletTriangle_s, MESHLET_MAXIMUM_SIZE> PrimitiveIndices;
};

inline float3 ComputeNormal(const float3* Tri) noexcept
{
    return Normalize(Cross(Tri[0] - Tri[1], Tri[0] - Tri[2]));
}

uint8_t ComputeReuse(const InlineMeshlet_s& Meshlet, const index_t* TriIndices) noexcept
{
    uint8_t Count = 0;
    for (size_t UniqueVertexIndexIt = 0; UniqueVertexIndexIt < Meshlet.UniqueVertexIndices.size(); ++UniqueVertexIndexIt)
    {
        for (size_t TriIndexIt = 0; TriIndexIt < 3u; ++TriIndexIt)
        {
            if (Meshlet.UniqueVertexIndices[UniqueVertexIndexIt] == TriIndices[TriIndexIt])
            {
                CHECK(Count < 255);
                ++Count;
            }
        }
    }

    return Count;
}

float ComputeScore(const InlineMeshlet_s& Meshlet, BoundingSphere Sphere, float3 Normal,const index_t* TriIndices, const float3* TriVerts) noexcept
{
    // Configurable weighted sum parameters
    constexpr float kWeightReuse = 0.334f;
    constexpr float kWeightLocation = 0.333f;
    constexpr float kWeightOrientation = 1.0f - (kWeightReuse + kWeightLocation);

    // Vertex reuse -
    const uint8_t Reuse = ComputeReuse(Meshlet, TriIndices);
    const float ScrReuse = 1.0f - (float(Reuse) / 3.0f);

    // Distance from center point - log falloff to preserve normalization where it needs it
    float MaxSq = 0;
    for (size_t It = 0; It < 3u; ++It)
    {
        const float3 Pos = TriVerts[It];
        const float3 V = Sphere.Origin - Pos;

        const float DistSq = LengthSqr(V);
        MaxSq = std::max(MaxSq, DistSq);
    }

    const float R = Sphere.Radius;
    const float R2 = R * R;
    const float ScrLocation = std::max(0.0f, log2f(MaxSq / (R2 + FLT_EPSILON) + FLT_EPSILON));

    // Angle between normal and meshlet cone axis - cosine falloff
    const float3 N = ComputeNormal(TriVerts);
    const float D = Dot(N, Normal);
    const float ScrOrientation = (1.0f - D) * 0.5f;

    // Weighted sum of scores
    return kWeightReuse * ScrReuse + kWeightLocation * ScrLocation + kWeightOrientation * ScrOrientation;
}

bool TryAddToMeshlet(size_t MaxVerts, size_t MaxPrims,const index_t* Tri, InlineMeshlet_s& Meshlet)
{
    // Cull degenerate triangle and return success
    // newCount calculation will break if such triangle is passed
    if (Tri[0] == Tri[1] || Tri[1] == Tri[2] || Tri[0] == Tri[2])
        return true;

    // Are we already full of vertices?
    if (Meshlet.UniqueVertexIndices.size() >= MaxVerts)
        return false;

    // Are we full, or can we store an additional primitive?
    if (Meshlet.PrimitiveIndices.size() >= MaxPrims)
        return false;

    uint32_t Indices[3] = { uint32_t(-1), uint32_t(-1), uint32_t(-1) };
    uint8_t NewCount = 3;

    for (size_t UniqueVertexIndexIt = 0; UniqueVertexIndexIt < Meshlet.UniqueVertexIndices.size(); ++UniqueVertexIndexIt)
    {
        for (size_t IndexIt = 0; IndexIt < 3; ++IndexIt)
        {
            if (Meshlet.UniqueVertexIndices[UniqueVertexIndexIt] == Tri[IndexIt])
            {
                Indices[IndexIt] = static_cast<uint32_t>(UniqueVertexIndexIt);
                --NewCount;
            }
        }
    }

    // Will this triangle fit?
    if (Meshlet.UniqueVertexIndices.size() + NewCount > MaxVerts)
        return false;

    // Add unique vertex indices to unique vertex index list
    for (size_t IndexIt = 0; IndexIt < 3; ++IndexIt)
    {
        if (Indices[IndexIt] == uint32_t(-1))
        {
            Indices[IndexIt] = static_cast<uint32_t>(Meshlet.UniqueVertexIndices.size());
            Meshlet.UniqueVertexIndices.push_back(Tri[IndexIt]);
        }
    }

    // Add the new primitive
    MeshletTriangle_s MeshletTri = { Indices[0], Indices[1], Indices[2] };
    Meshlet.PrimitiveIndices.emplace_back(MeshletTri);

    return true;
}

inline bool IsMeshletFull(size_t MaxVerts, size_t MaxPrims, const InlineMeshlet_s& Meshlet) noexcept
{
    CHECK(Meshlet.UniqueVertexIndices.size() <= MaxVerts);
    CHECK(Meshlet.PrimitiveIndices.size() <= MaxPrims);

    return Meshlet.UniqueVertexIndices.size() >= MaxVerts || Meshlet.PrimitiveIndices.size() >= MaxPrims;
}

bool Meshletize(size_t MaxVerts, size_t MaxPrims, const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, const Subset_t& Subset, const uint32_t* Adjacency, std::vector<InlineMeshlet_s>& Meshlets)
{
    if (!Indices || !Positions || !Adjacency)
        return RET_INVALID_ARGS;

    if (Subset.first + Subset.second > NumFaces)
        return RET_UNEXPECTED;

    Meshlets.clear();

    // Bitmask of all triangles in mesh to determine whether a specific one has been added
    std::vector<bool> Checklist;
    Checklist.resize(Subset.second);

    // Cache to maintain scores for each candidate triangle
    std::vector<std::pair<uint32_t, float>> Candidates;
    std::unordered_set<uint32_t> CandidateCheck;

    // Positions and normals of the current primitive
    std::vector<float3> Vertices;
    std::vector<float3> Normals;

    // Seed the candidate list with the first triangle of the subset
    const uint32_t StartIndex = static_cast<uint32_t>(Subset.first);
    const uint32_t EndIndex = static_cast<uint32_t>(Subset.first + Subset.second);

    uint32_t TriIndex = static_cast<uint32_t>(Subset.first);

    Candidates.push_back(std::make_pair(TriIndex, 0.0f));
    CandidateCheck.insert(TriIndex);

    // Continue adding triangles until triangle list is exhausted.
    InlineMeshlet_s* Curr = nullptr;

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

        if (Tri[0] >= NumVerts ||
            Tri[1] >= NumVerts ||
            Tri[2] >= NumVerts)
        {
            return RET_UNEXPECTED;
        }

        // Create a new meshlet if necessary
        if (Curr == nullptr)
        {
            Vertices.clear();
            Normals.clear();

            Meshlets.emplace_back();
            Curr = &Meshlets.back();
        }

        // Try to add triangle to meshlet
        if (TryAddToMeshlet(MaxVerts, MaxPrims, Tri, *Curr))
        {
            // Success! Mark as added.
            Checklist[Index - StartIndex] = true;

            // Add positions & normal to list
            const float3 points[3] =
            {
                Positions[Tri[0]],
                Positions[Tri[1]],
                Positions[Tri[2]],
            };

            Vertices.push_back(points[0]);
            Vertices.push_back(points[1]);
            Vertices.push_back(points[2]);

            Normals.push_back(ComputeNormal(points));

            // Compute new bounding sphere & normal axis
            BoundingSphere PositionBounds, NormalBounds;
            PositionBounds.InitFromPoints(Vertices.size(), Vertices.data());
            NormalBounds.InitFromPoints(Normals.size(), Normals.data());

            NormalBounds.Origin = Normalize(NormalBounds.Origin);

            // Find and add all applicable adjacent triangles to candidate list
            const uint32_t AdjIndex = Index * 3;

            uint32_t Adj[3] =
            {
                Adjacency[AdjIndex],
                Adjacency[AdjIndex + 1],
                Adjacency[AdjIndex + 2],
            };

            for (size_t AdjIt = 0; AdjIt < 3u; ++AdjIt)
            {
                // Invalid triangle in adjacency slot
                if (Adj[AdjIt] == uint32_t(-1))
                    continue;

                // Primitive is outside the subset
                if (Adj[AdjIt] < Subset.first || Adj[AdjIt] > EndIndex)
                    continue;

                // Already processed triangle
                if (Checklist[Adj[AdjIt] - StartIndex])
                    continue;

                // Triangle already in the candidate list
                if (CandidateCheck.count(Adj[AdjIt]))
                    continue;

                Candidates.push_back(std::make_pair(Adj[AdjIt], FLT_MAX));
                CandidateCheck.insert(Adj[AdjIt]);
            }

            // Re-score remaining candidate triangles
            for (size_t CandidateIt = 0; CandidateIt < Candidates.size(); ++CandidateIt)
            {
                uint32_t candidate = Candidates[CandidateIt].first;

                index_t TriIndices[3] =
                {
                    Indices[candidate * 3],
                    Indices[candidate * 3 + 1],
                    Indices[candidate * 3 + 2],
                };

                if (TriIndices[0] >= NumVerts ||
                    TriIndices[1] >= NumVerts ||
                    TriIndices[2] >= NumVerts)
                {
                    return RET_UNEXPECTED;
                }

                const float3 TriVerts[3] =
                {
                    Positions[TriIndices[0]],
                    Positions[TriIndices[1]],
                    Positions[TriIndices[2]],
                };

                Candidates[CandidateIt].second = ComputeScore(*Curr, PositionBounds, NormalBounds.Origin, TriIndices, TriVerts);
            }

            // Determine whether we need to move to the next meshlet.
            if (IsMeshletFull(MaxVerts, MaxPrims, *Curr))
            {
                CandidateCheck.clear();
                Curr = nullptr;

                // Discard candidates -  one of our existing candidates as the next meshlet seed.
                if (!Candidates.empty())
                {
                    Candidates[0] = Candidates.back();
                    Candidates.resize(1);
                    CandidateCheck.insert(Candidates[0].first);
                }
            }
            else
            {
                // Sort in reverse order to use vector as a queue with pop_back
                std::stable_sort(Candidates.begin(), Candidates.end(), [](auto& a, auto& b) { return a.second > b.second; });
            }
        }
        else
        {
            // Ran out of candidates while attempting to fill the last bits of a meshlet.
            if (Candidates.empty())
            {
                CandidateCheck.clear();
                Curr = nullptr;

            }
        }

        // Ran out of candidates; add a new seed candidate to start the next meshlet.
        if (Candidates.empty())
        {
            while (TriIndex < EndIndex && Checklist[TriIndex - StartIndex])
                ++TriIndex;

            if (TriIndex == EndIndex)
                break;

            Candidates.push_back(std::make_pair(TriIndex, 0.0f));
            CandidateCheck.insert(TriIndex);
        }
    }

    return true;
}

bool ComputeMeshlets(
    const index_t* Indices, size_t NumFaces, 
    const float3* Positions, size_t NumVerts, 
    const Subset_t* Subsets, size_t NumSubsets, 
    std::vector<Meshlet_s>& Meshlets, std::vector<uint8_t>& UniqueVertexIndices, std::vector<MeshletTriangle_s>& PrimitiveIndices,
    Subset_t* OutMeshletSubsets, 
    size_t MaxVerts, size_t MaxPrims)
{
    if (!Indices || !Positions || !Subsets || !OutMeshletSubsets)
        return RET_INVALID_ARGS;

    // Validate the meshlet vertex & primitive sizes
    if (MaxVerts < MESHLET_MINIMUM_SIZE || MaxVerts > MESHLET_MAXIMUM_SIZE)
        return RET_INVALID_ARGS;

    if (MaxPrims < MESHLET_MINIMUM_SIZE || MaxPrims > MESHLET_MAXIMUM_SIZE)
        return RET_INVALID_ARGS;

    if (NumFaces == 0 || NumVerts == 0 || NumSubsets == 0)
        return RET_INVALID_ARGS;

    // Auto-generate adjacency data if not provided.
    const uint32_t* Adjacency;
    std::unique_ptr<uint32_t[]> GeneratedAdj;
    {
        GeneratedAdj.reset(new (std::nothrow) uint32_t[NumFaces * 3]);
        if (!GeneratedAdj)
            return RET_OUT_OF_MEM;

        if (!GenerateAdjacencyAndPointReps(Indices, NumFaces, Positions, NumVerts, 0.0f, nullptr, GeneratedAdj.get()))
        {
            return false;
        }

        Adjacency = GeneratedAdj.get();
    }

    // Now start generating meshlets
    for (size_t SubsetIt = 0; SubsetIt < NumSubsets; ++SubsetIt)
    {
        auto& Subset = Subsets[SubsetIt];

        if ((Subset.first + Subset.second) > NumFaces)
        {
            return RET_UNEXPECTED;
        }

        std::vector<InlineMeshlet_s> NewMeshlets;
        if (!Meshletize(MaxVerts, MaxPrims, Indices, NumFaces, Positions, NumVerts, Subset, Adjacency, NewMeshlets))
        {
            return false;
        }

        OutMeshletSubsets[SubsetIt] = std::make_pair(Meshlets.size(), NewMeshlets.size());

        // Determine final unique vertex index and primitive index counts & offsets.
        size_t StartVertCount = UniqueVertexIndices.size() / sizeof(index_t);
        size_t StartPrimCount = PrimitiveIndices.size();

        size_t UniqueVertexIndexCount = StartVertCount;
        size_t PrimitiveIndexCount = StartPrimCount;

        // Resize the meshlet output array to hold the newly formed meshlets.
        const size_t MeshletCount = Meshlets.size();
        Meshlets.resize(MeshletCount + NewMeshlets.size());

        Meshlet_s* Dest = &Meshlets[MeshletCount];
        for (auto& Meshlet : NewMeshlets)
        {
            Dest->VertOffset = static_cast<uint32_t>(UniqueVertexIndexCount);
            Dest->VertCount = static_cast<uint32_t>(Meshlet.UniqueVertexIndices.size());

            Dest->PrimOffset = static_cast<uint32_t>(PrimitiveIndexCount);
            Dest->PrimCount = static_cast<uint32_t>(Meshlet.PrimitiveIndices.size());

            UniqueVertexIndexCount += Meshlet.UniqueVertexIndices.size();
            PrimitiveIndexCount += Meshlet.PrimitiveIndices.size();

            ++Dest;
        }

        // Allocate space for the new data.
        UniqueVertexIndices.resize(UniqueVertexIndexCount * sizeof(index_t));
        PrimitiveIndices.resize(PrimitiveIndexCount);

        // Copy data from the freshly built meshlets into the output buffers.
        auto VertDest = reinterpret_cast<index_t*>(UniqueVertexIndices.data()) + StartVertCount;
        auto PrimDest = reinterpret_cast<uint32_t*>(PrimitiveIndices.data()) + StartPrimCount;

        for (auto& Meshlet : NewMeshlets)
        {
            memcpy(VertDest, Meshlet.UniqueVertexIndices.data(), Meshlet.UniqueVertexIndices.size() * sizeof(index_t));
            memcpy(PrimDest, Meshlet.PrimitiveIndices.data(), Meshlet.PrimitiveIndices.size() * sizeof(uint32_t));

            VertDest += Meshlet.UniqueVertexIndices.size();
            PrimDest += Meshlet.PrimitiveIndices.size();
        }
    }

    return true;
}

}
