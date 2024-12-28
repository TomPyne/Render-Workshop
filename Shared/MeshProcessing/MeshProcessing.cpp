#include "MeshProcessing.h"
#include "Logging/Logging.h"

#include <algorithm>
#include <unordered_map>
#include <memory>
#include <mutex>

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
                        const uint32_t M = (It->second >= NumVerts) ? DupAttr[It->second - nVerts] : ids[It->second];
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

bool ReorderIndices(index_t* Indices, size_t NumFaces, const uint32_t* FaceRemap, index_t* OutIndices)
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
        assert(curActiveFaceListPos == (indexCount - unused));
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
bool OptimizeVertices(index_t* Indices, size_t NumFaces, size_t NumVerts, uint32_t* VertexRemap)
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
}
