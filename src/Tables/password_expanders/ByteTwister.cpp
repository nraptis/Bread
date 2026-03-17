#include "ByteTwister.hpp"

#if defined(__ARM_NEON)
#include <arm_neon.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdint>
#include <cstring>

namespace peanutbutter::expansion::key_expansion {
namespace {

constexpr int kMatrixSize = 16;
constexpr int kChunkBytes = 32;

#if defined(__ARM_NEON)
inline void BlockXorInPlace(unsigned char* pDestination, const unsigned char* pSource) {
    vst1q_u8(pDestination, veorq_u8(vld1q_u8(pDestination), vld1q_u8(pSource)));
}

inline void BlockAddInPlace(unsigned char* pDestination, const unsigned char* pSource) {
    vst1q_u8(pDestination, vaddq_u8(vld1q_u8(pDestination), vld1q_u8(pSource)));
}
#elif defined(__SSE2__)
inline void BlockXorInPlace(unsigned char* pDestination, const unsigned char* pSource) {
    const __m128i aLeft = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDestination));
    const __m128i aRight = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSource));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(pDestination), _mm_xor_si128(aLeft, aRight));
}

inline void BlockAddInPlace(unsigned char* pDestination, const unsigned char* pSource) {
    const __m128i aLeft = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pDestination));
    const __m128i aRight = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSource));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(pDestination), _mm_add_epi8(aLeft, aRight));
}
#else
inline void BlockXorInPlace(unsigned char* pDestination, const unsigned char* pSource) {
    for (int i = 0; i < kMatrixSize; ++i) {
        pDestination[i] = static_cast<unsigned char>(pDestination[i] ^ pSource[i]);
    }
}

inline void BlockAddInPlace(unsigned char* pDestination, const unsigned char* pSource) {
    for (int i = 0; i < kMatrixSize; ++i) {
        pDestination[i] = static_cast<unsigned char>(pDestination[i] + pSource[i]);
    }
}
#endif

inline int WrapIndex(int pIndex) {
    int aWrapped = pIndex % PASSWORD_EXPANDED_SIZE;
    if (aWrapped < 0) {
        aWrapped += PASSWORD_EXPANDED_SIZE;
    }
    return aWrapped;
}

inline void CopyBlock16(const unsigned char* pSource, unsigned char* pDestination) {
    std::memcpy(pDestination, pSource, kMatrixSize);
}

inline void LoadWrappedBlock16(const unsigned char* pSource, int pIndex, unsigned char* pDestination) {
    const int aStart = WrapIndex(pIndex);
    if (aStart <= (PASSWORD_EXPANDED_SIZE - kMatrixSize)) {
        std::memcpy(pDestination, pSource + aStart, kMatrixSize);
        return;
    }
    for (int i = 0; i < kMatrixSize; ++i) {
        pDestination[i] = pSource[WrapIndex(aStart + i)];
    }
}

inline void StoreBlock16(unsigned char* pDestination, int pIndex, const unsigned char* pSource) {
    std::memcpy(pDestination + pIndex, pSource, kMatrixSize);
}

inline unsigned char& MatrixAt(unsigned char* pMatrix, int pRow, int pColumn) {
    return pMatrix[((pRow & 3) * 4) + (pColumn & 3)];
}

inline void XorWithBlock(unsigned char* pDestination, const unsigned char* pSource) {
    BlockXorInPlace(pDestination, pSource);
}

inline void AddWithBlock(unsigned char* pDestination, const unsigned char* pSource) {
    BlockAddInPlace(pDestination, pSource);
}

inline void InjectXorShifted(unsigned char* pDestination, const unsigned char* pSource, unsigned char pStart) {
    alignas(16) unsigned char aScratch[kMatrixSize];
    for (int i = 0; i < kMatrixSize; ++i) {
        aScratch[i] = pSource[(static_cast<int>(pStart) + i) & 15];
    }
    BlockXorInPlace(pDestination, aScratch);
}

inline void InjectAddShifted(unsigned char* pDestination, const unsigned char* pSource, unsigned char pStart) {
    alignas(16) unsigned char aScratch[kMatrixSize];
    for (int i = 0; i < kMatrixSize; ++i) {
        aScratch[i] = pSource[(static_cast<int>(pStart) + i) & 15];
    }
    BlockAddInPlace(pDestination, aScratch);
}

inline unsigned char FoldXor(const unsigned char* pMatrix) {
    unsigned char aValue = 0u;
    for (int i = 0; i < kMatrixSize; ++i) {
        aValue = static_cast<unsigned char>(aValue ^ pMatrix[i]);
    }
    return aValue;
}

inline unsigned char FoldAdd(const unsigned char* pMatrix) {
    unsigned char aValue = 0u;
    for (int i = 0; i < kMatrixSize; ++i) {
        aValue = static_cast<unsigned char>(aValue + pMatrix[i]);
    }
    return aValue;
}

inline void RotateRowLeft(unsigned char* pMatrix, unsigned char pRow, unsigned char pAmount) {
    const int aRow = pRow & 3;
    const int aShift = pAmount & 3;
    if (aShift == 0) {
        return;
    }
    unsigned char aScratch[4] = {
        MatrixAt(pMatrix, aRow, 0), MatrixAt(pMatrix, aRow, 1), MatrixAt(pMatrix, aRow, 2), MatrixAt(pMatrix, aRow, 3)};
    for (int i = 0; i < 4; ++i) {
        MatrixAt(pMatrix, aRow, i) = aScratch[(i + aShift) & 3];
    }
}

inline void RotateColumnDown(unsigned char* pMatrix, unsigned char pColumn, unsigned char pAmount) {
    const int aColumn = pColumn & 3;
    const int aShift = pAmount & 3;
    if (aShift == 0) {
        return;
    }
    unsigned char aScratch[4] = {
        MatrixAt(pMatrix, 0, aColumn), MatrixAt(pMatrix, 1, aColumn), MatrixAt(pMatrix, 2, aColumn), MatrixAt(pMatrix, 3, aColumn)};
    for (int i = 0; i < 4; ++i) {
        MatrixAt(pMatrix, (i + aShift) & 3, aColumn) = aScratch[i];
    }
}

inline void SwapRows(unsigned char* pMatrix, unsigned char pRowA, unsigned char pRowB) {
    const int aRowA = pRowA & 3;
    const int aRowB = pRowB & 3;
    if (aRowA == aRowB) {
        return;
    }
    for (int i = 0; i < 4; ++i) {
        std::swap(MatrixAt(pMatrix, aRowA, i), MatrixAt(pMatrix, aRowB, i));
    }
}

inline void XorRowIntoRow(unsigned char* pMatrix, unsigned char pDstRow, unsigned char pSrcRow) {
    const int aDst = pDstRow & 3;
    const int aSrc = pSrcRow & 3;
    if (aDst == aSrc) {
        RotateRowLeft(pMatrix, static_cast<unsigned char>(aDst), 1u);
        return;
    }
    for (int i = 0; i < 4; ++i) {
        MatrixAt(pMatrix, aDst, i) = static_cast<unsigned char>(MatrixAt(pMatrix, aDst, i) ^ MatrixAt(pMatrix, aSrc, i));
    }
}

inline void AddColumnIntoColumn(unsigned char* pMatrix, unsigned char pDstColumn, unsigned char pSrcColumn) {
    const int aDst = pDstColumn & 3;
    const int aSrc = pSrcColumn & 3;
    if (aDst == aSrc) {
        RotateColumnDown(pMatrix, static_cast<unsigned char>(aDst), 1u);
        return;
    }
    for (int i = 0; i < 4; ++i) {
        MatrixAt(pMatrix, i, aDst) = static_cast<unsigned char>(MatrixAt(pMatrix, i, aDst) + MatrixAt(pMatrix, i, aSrc));
    }
}

inline void WeaveRows(unsigned char* pMatrix, unsigned char pRowA, unsigned char pRowB) {
    const int aRowA = pRowA & 3;
    const int aRowB = pRowB & 3;
    if (aRowA == aRowB) {
        return;
    }
    std::swap(MatrixAt(pMatrix, aRowA, 1), MatrixAt(pMatrix, aRowB, 1));
    std::swap(MatrixAt(pMatrix, aRowA, 3), MatrixAt(pMatrix, aRowB, 3));
}

void TwistBytes_00(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_01(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_02(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_03(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_04(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_05(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_06(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_07(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_08(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_09(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_10(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_11(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_12(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_13(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_14(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);
void TwistBytes_15(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace);

void TwistSingleBlock(unsigned char pType,
                      unsigned char* pSource,
                      unsigned char* pWorkspace,
                      unsigned char* pDestination) {
    switch (pType & 15U) {
        case 0:
            TwistBytes_00(pSource, pDestination, pWorkspace);
            break;
        case 1:
            TwistBytes_01(pSource, pDestination, pWorkspace);
            break;
        case 2:
            TwistBytes_02(pSource, pDestination, pWorkspace);
            break;
        case 3:
            TwistBytes_03(pSource, pDestination, pWorkspace);
            break;
        case 4:
            TwistBytes_04(pSource, pDestination, pWorkspace);
            break;
        case 5:
            TwistBytes_05(pSource, pDestination, pWorkspace);
            break;
        case 6:
            TwistBytes_06(pSource, pDestination, pWorkspace);
            break;
        case 7:
            TwistBytes_07(pSource, pDestination, pWorkspace);
            break;
        case 8:
            TwistBytes_08(pSource, pDestination, pWorkspace);
            break;
        case 9:
            TwistBytes_09(pSource, pDestination, pWorkspace);
            break;
        case 10:
            TwistBytes_10(pSource, pDestination, pWorkspace);
            break;
        case 11:
            TwistBytes_11(pSource, pDestination, pWorkspace);
            break;
        case 12:
            TwistBytes_12(pSource, pDestination, pWorkspace);
            break;
        case 13:
            TwistBytes_13(pSource, pDestination, pWorkspace);
            break;
        case 14:
            TwistBytes_14(pSource, pDestination, pWorkspace);
            break;
        case 15:
        default:
            TwistBytes_15(pSource, pDestination, pWorkspace);
            break;
    }
}
// Derived from Test Candidate 2192
// Final grade: A+ (98.776450)
void TwistBytes_00(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0xA8u, 0x3Du, 0x69u, 0xA6u, 0xDBu, 0x15u, 0x98u, 0xF9u, 0x62u, 0xABu, 0x24u, 0x28u, 0xEAu, 0xD3u, 0xC4u, 0xB7u};
    static constexpr unsigned char kPhase1Seed1[16] = {0x06u, 0xB5u, 0x12u, 0x4Bu, 0x40u, 0x91u, 0xD0u, 0xC5u, 0x9Cu, 0xC3u, 0x8Au, 0x54u, 0x9Fu, 0x91u, 0xEEu, 0xF5u};
    static constexpr unsigned char kPhase2Seed0[16] = {0x98u, 0xC8u, 0x35u, 0x52u, 0x7Fu, 0x78u, 0xEDu, 0x50u, 0xB6u, 0xEEu, 0x21u, 0x50u, 0xACu, 0xA7u, 0xC6u, 0x25u};
    static constexpr unsigned char kPhase2Seed1[16] = {0xF8u, 0xE9u, 0x37u, 0x55u, 0xABu, 0xE5u, 0x7Bu, 0x40u, 0x5Bu, 0x62u, 0x83u, 0x68u, 0xF9u, 0x59u, 0xD3u, 0x78u};
    constexpr int kPhase1SourceOffset1 = 1696;
    constexpr int kPhase1ControlOffset = 5072;
    constexpr int kPhase1FeedbackOffset = 592;
    constexpr unsigned char kPhase1SaltBias = 121u;
    constexpr unsigned char kPhase1SaltStride = 11u;
    constexpr int kPhase2SourceOffset1 = 6352;
    constexpr int kPhase2ControlOffset = 3360;
    constexpr int kPhase2FeedbackOffset = 2768;
    constexpr unsigned char kPhase2SaltBias = 113u;
    constexpr unsigned char kPhase2SaltStride = 9u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        XorWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        SwapRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[4] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[5] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        XorRowIntoRow(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        RotateColumnDown(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix0, aPhase1Carry1);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry0);
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 7) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 7) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        WeaveRows(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectXorShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        RotateColumnDown(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectXorShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry1);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] + aPhase2LaneMix0 + aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] + aPhase2LaneMix1 + aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        XorRowIntoRow(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        AddColumnIntoColumn(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 4074
// Final grade: A+ (98.772737)
void TwistBytes_01(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0xB9u, 0x50u, 0x1Au, 0x1Au, 0xFAu, 0xBCu, 0x0Cu, 0xE1u, 0xABu, 0x31u, 0x9Au, 0x85u, 0xCEu, 0x93u, 0xB1u, 0xE8u};
    static constexpr unsigned char kPhase1Seed1[16] = {0x35u, 0x86u, 0xFFu, 0x35u, 0x31u, 0x82u, 0xA5u, 0x78u, 0xB1u, 0x6Du, 0xD5u, 0xCAu, 0x3Bu, 0x07u, 0x00u, 0xAFu};
    static constexpr unsigned char kPhase2Seed0[16] = {0x29u, 0x1Du, 0xC1u, 0x3Cu, 0xE4u, 0x02u, 0x77u, 0xE5u, 0x0Du, 0x7Fu, 0x50u, 0x3Eu, 0x39u, 0xD5u, 0x21u, 0x38u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x2Cu, 0x45u, 0x67u, 0x8Fu, 0x8Eu, 0xBEu, 0xBFu, 0xFDu, 0x48u, 0xC6u, 0x32u, 0x67u, 0x5Au, 0x52u, 0x40u, 0xF6u};
    constexpr int kPhase1SourceOffset1 = 3072;
    constexpr int kPhase1ControlOffset = 5232;
    constexpr int kPhase1FeedbackOffset = 912;
    constexpr unsigned char kPhase1SaltBias = 43u;
    constexpr unsigned char kPhase1SaltStride = 29u;
    constexpr int kPhase2SourceOffset1 = 6128;
    constexpr int kPhase2ControlOffset = 1760;
    constexpr int kPhase2FeedbackOffset = 5184;
    constexpr unsigned char kPhase2SaltBias = 175u;
    constexpr unsigned char kPhase2SaltStride = 5u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        SwapRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectAddShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        RotateRowLeft(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[9] ^ aPhase1Salt[10]), static_cast<unsigned char>(aPhase1ControlB[12] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        AddWithBlock(aPhase1Matrix0, aPhase1Matrix1);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 7) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 5) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        RotateRowLeft(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        SwapRows(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectAddShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectAddShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        SwapRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix0, aPhase2Carry1);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry0);
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] + aPhase2LaneMix0 + aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 5) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] + aPhase2LaneMix1 + aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        AddColumnIntoColumn(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectAddShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        XorRowIntoRow(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 3893
// Final grade: A+ (98.761656)
void TwistBytes_02(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0xDEu, 0xF7u, 0x64u, 0x1Fu, 0xC6u, 0x2Fu, 0x3Au, 0x3Bu, 0x4Eu, 0xEBu, 0x19u, 0x68u, 0xD2u, 0x21u, 0x07u, 0x76u};
    static constexpr unsigned char kPhase1Seed1[16] = {0xDBu, 0x98u, 0xD1u, 0x6Eu, 0xCCu, 0xD7u, 0xC2u, 0x13u, 0xE0u, 0x8Bu, 0x00u, 0x53u, 0x6Fu, 0xDDu, 0x36u, 0xACu};
    static constexpr unsigned char kPhase2Seed0[16] = {0x3Eu, 0x60u, 0xC9u, 0x03u, 0x5Cu, 0xB2u, 0x95u, 0xCEu, 0xD7u, 0xBBu, 0x21u, 0x56u, 0x76u, 0x80u, 0xB2u, 0x52u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x67u, 0x51u, 0x69u, 0x54u, 0x9Cu, 0x7Du, 0xFEu, 0xE4u, 0x15u, 0x70u, 0xC6u, 0x2Fu, 0x0Au, 0x48u, 0x29u, 0xDCu};
    constexpr int kPhase1SourceOffset1 = 5488;
    constexpr int kPhase1ControlOffset = 5392;
    constexpr int kPhase1FeedbackOffset = 4400;
    constexpr unsigned char kPhase1SaltBias = 63u;
    constexpr unsigned char kPhase1SaltStride = 7u;
    constexpr int kPhase2SourceOffset1 = 6480;
    constexpr int kPhase2ControlOffset = 4464;
    constexpr int kPhase2FeedbackOffset = 6192;
    constexpr unsigned char kPhase2SaltBias = 101u;
    constexpr unsigned char kPhase2SaltStride = 19u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        SwapRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[4] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[5] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        RotateColumnDown(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        WeaveRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 3) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 5) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        RotateRowLeft(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        WeaveRows(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectXorShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        RotateColumnDown(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix0, aPhase2Carry1);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry0);
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 3) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] ^ aPhase2LaneMix0 ^ aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 5) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] ^ aPhase2LaneMix1 ^ aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        RotateColumnDown(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        AddColumnIntoColumn(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectXorShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 918
// Final grade: A+ (98.757395)
void TwistBytes_03(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0xE4u, 0x21u, 0xC1u, 0x05u, 0x20u, 0xCDu, 0xB7u, 0xC0u, 0x0Du, 0x3Au, 0x8Au, 0xE7u, 0x5Bu, 0x00u, 0xE0u, 0x89u};
    static constexpr unsigned char kPhase1Seed1[16] = {0x61u, 0x6Du, 0x42u, 0x77u, 0xAFu, 0xF8u, 0x2Du, 0xCDu, 0xC4u, 0x3Au, 0x4Du, 0x2Au, 0x1Au, 0x02u, 0x4Bu, 0xE2u};
    static constexpr unsigned char kPhase2Seed0[16] = {0x6Eu, 0x66u, 0x23u, 0x67u, 0x5Fu, 0x27u, 0x2Cu, 0x7Cu, 0x79u, 0x32u, 0x8Eu, 0xC6u, 0x74u, 0x83u, 0x95u, 0x27u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x0Fu, 0xD8u, 0xD8u, 0x96u, 0xBDu, 0x96u, 0xEBu, 0x8Fu, 0x0Eu, 0xB4u, 0x25u, 0xC9u, 0xDAu, 0x09u, 0x0Bu, 0xD4u};
    constexpr int kPhase1SourceOffset1 = 6912;
    constexpr int kPhase1ControlOffset = 3600;
    constexpr int kPhase1FeedbackOffset = 3744;
    constexpr unsigned char kPhase1SaltBias = 153u;
    constexpr unsigned char kPhase1SaltStride = 17u;
    constexpr int kPhase2SourceOffset1 = 2400;
    constexpr int kPhase2ControlOffset = 3616;
    constexpr int kPhase2FeedbackOffset = 3312;
    constexpr unsigned char kPhase2SaltBias = 107u;
    constexpr unsigned char kPhase2SaltStride = 19u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        XorWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        RotateColumnDown(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        RotateColumnDown(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        SwapRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectAddShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        RotateRowLeft(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        RotateColumnDown(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        AddWithBlock(aPhase1Matrix0, aPhase1Matrix1);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 7) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 13) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        SwapRows(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectXorShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        RotateColumnDown(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectAddShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectAddShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry1);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] + aPhase2LaneMix0 + aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 13) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] + aPhase2LaneMix1 + aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        RotateRowLeft(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectAddShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        XorRowIntoRow(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectXorShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 2871
// Final grade: A+ (98.746495)
void TwistBytes_04(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x71u, 0x5Bu, 0x4Du, 0x7Cu, 0xBCu, 0xE0u, 0xBFu, 0xDBu, 0x93u, 0xD8u, 0x35u, 0xE2u, 0x87u, 0x94u, 0xCAu, 0x51u};
    static constexpr unsigned char kPhase1Seed1[16] = {0x97u, 0x74u, 0x56u, 0xDFu, 0x3Du, 0xECu, 0x8Du, 0x2Eu, 0xF6u, 0x3Cu, 0x37u, 0x9Cu, 0x5Bu, 0xCCu, 0x74u, 0x07u};
    static constexpr unsigned char kPhase2Seed0[16] = {0xD6u, 0x10u, 0x33u, 0xDAu, 0x1Du, 0x26u, 0x15u, 0xA3u, 0x15u, 0x7Cu, 0x62u, 0xD4u, 0x23u, 0x93u, 0xFCu, 0xCAu};
    static constexpr unsigned char kPhase2Seed1[16] = {0xCAu, 0x23u, 0xF6u, 0x5Au, 0xA2u, 0x7Bu, 0xA6u, 0xC6u, 0x59u, 0xDFu, 0x1Eu, 0xC1u, 0x20u, 0xEAu, 0xCAu, 0xEFu};
    constexpr int kPhase1SourceOffset1 = 1136;
    constexpr int kPhase1ControlOffset = 2720;
    constexpr int kPhase1FeedbackOffset = 7552;
    constexpr unsigned char kPhase1SaltBias = 161u;
    constexpr unsigned char kPhase1SaltStride = 13u;
    constexpr int kPhase2SourceOffset1 = 5712;
    constexpr int kPhase2ControlOffset = 3664;
    constexpr int kPhase2FeedbackOffset = 4400;
    constexpr unsigned char kPhase2SaltBias = 219u;
    constexpr unsigned char kPhase2SaltStride = 3u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        SwapRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        AddColumnIntoColumn(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        RotateColumnDown(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        RotateColumnDown(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[4] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[5] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectAddShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix0, aPhase1Carry1);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry0);
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 1) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 7) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        RotateColumnDown(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        AddColumnIntoColumn(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectAddShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectAddShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        SwapRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        WeaveRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        AddWithBlock(aPhase2Matrix0, aPhase2Matrix1);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 1) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] ^ aPhase2LaneMix0 ^ aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] ^ aPhase2LaneMix1 ^ aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        RotateRowLeft(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectAddShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        SwapRows(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectXorShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 1204
// Final grade: A+ (98.743252)
void TwistBytes_05(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x32u, 0xDFu, 0x37u, 0x92u, 0xB6u, 0xF8u, 0xB1u, 0xF5u, 0x05u, 0xB3u, 0xEDu, 0xBAu, 0x64u, 0xB5u, 0x81u, 0x72u};
    static constexpr unsigned char kPhase1Seed1[16] = {0x7Du, 0xF3u, 0xC9u, 0x54u, 0x30u, 0xC3u, 0x0Fu, 0x47u, 0x5Au, 0x11u, 0xABu, 0xE1u, 0x89u, 0xE8u, 0xBBu, 0x6Au};
    static constexpr unsigned char kPhase2Seed0[16] = {0x13u, 0xD4u, 0xC0u, 0xA6u, 0xB1u, 0xDCu, 0x09u, 0x10u, 0x02u, 0x53u, 0x80u, 0x67u, 0x52u, 0xA2u, 0x52u, 0xC2u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x5Au, 0xCBu, 0x78u, 0xA0u, 0xB6u, 0xDDu, 0x8Fu, 0xC2u, 0x74u, 0x9Fu, 0xC5u, 0xF3u, 0xDFu, 0xE9u, 0x27u, 0xA2u};
    constexpr int kPhase1SourceOffset1 = 6704;
    constexpr int kPhase1ControlOffset = 3440;
    constexpr int kPhase1FeedbackOffset = 816;
    constexpr unsigned char kPhase1SaltBias = 39u;
    constexpr unsigned char kPhase1SaltStride = 19u;
    constexpr int kPhase2SourceOffset1 = 6000;
    constexpr int kPhase2ControlOffset = 1872;
    constexpr int kPhase2FeedbackOffset = 7216;
    constexpr unsigned char kPhase2SaltBias = 29u;
    constexpr unsigned char kPhase2SaltStride = 9u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        XorWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        RotateColumnDown(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        XorRowIntoRow(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        AddWithBlock(aPhase1Matrix0, aPhase1Matrix1);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 5) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 11) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        RotateColumnDown(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectXorShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        SwapRows(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectXorShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[6] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[8] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        WeaveRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        AddWithBlock(aPhase2Matrix0, aPhase2Matrix1);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 5) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] + aPhase2LaneMix0 + aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 11) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] + aPhase2LaneMix1 + aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        AddColumnIntoColumn(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        AddColumnIntoColumn(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectXorShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 1569
// Final grade: A+ (98.760368)
void TwistBytes_06(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0xBFu, 0x85u, 0x19u, 0x70u, 0xBDu, 0xF1u, 0xCDu, 0x88u, 0xA3u, 0x18u, 0xBDu, 0x93u, 0xC1u, 0x4Au, 0x7Du, 0x6Au};
    static constexpr unsigned char kPhase1Seed1[16] = {0x00u, 0xA8u, 0x85u, 0x5Au, 0x49u, 0xB0u, 0xEEu, 0x4Fu, 0x82u, 0xE3u, 0xF9u, 0x23u, 0x5Du, 0x92u, 0x3Du, 0x0Eu};
    static constexpr unsigned char kPhase2Seed0[16] = {0x14u, 0xDAu, 0x63u, 0xA5u, 0xEEu, 0x6Du, 0xD1u, 0xF0u, 0x6Cu, 0x93u, 0x63u, 0xCCu, 0x90u, 0xECu, 0x83u, 0xDAu};
    static constexpr unsigned char kPhase2Seed1[16] = {0x2Au, 0x40u, 0xE0u, 0x2Cu, 0xF5u, 0x77u, 0x65u, 0x7Au, 0xBCu, 0xD2u, 0x60u, 0x7Eu, 0x21u, 0x76u, 0x37u, 0xF7u};
    constexpr int kPhase1SourceOffset1 = 5616;
    constexpr int kPhase1ControlOffset = 4896;
    constexpr int kPhase1FeedbackOffset = 3104;
    constexpr unsigned char kPhase1SaltBias = 99u;
    constexpr unsigned char kPhase1SaltStride = 17u;
    constexpr int kPhase2SourceOffset1 = 1056;
    constexpr int kPhase2ControlOffset = 1968;
    constexpr int kPhase2FeedbackOffset = 3888;
    constexpr unsigned char kPhase2SaltBias = 145u;
    constexpr unsigned char kPhase2SaltStride = 3u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        XorWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        XorRowIntoRow(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        RotateColumnDown(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[4] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[5] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectAddShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        XorRowIntoRow(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        WeaveRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        WeaveRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        RotateColumnDown(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[9] ^ aPhase1Salt[10]), static_cast<unsigned char>(aPhase1ControlB[12] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix0, aPhase1Carry1);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry0);
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 3) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 7) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        WeaveRows(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectXorShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        RotateColumnDown(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectAddShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[6] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[8] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        WeaveRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry1);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 3) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] ^ aPhase2LaneMix0 ^ aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] ^ aPhase2LaneMix1 ^ aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        WeaveRows(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        XorRowIntoRow(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 3399
// Final grade: A+ (98.750104)
void TwistBytes_07(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x82u, 0x15u, 0x28u, 0xDBu, 0x3Eu, 0xBAu, 0x52u, 0xD3u, 0x0Cu, 0x7Eu, 0x48u, 0x26u, 0x1Du, 0x7Eu, 0x35u, 0x0Cu};
    static constexpr unsigned char kPhase1Seed1[16] = {0xE6u, 0x15u, 0xF6u, 0xB7u, 0x61u, 0xA9u, 0x1Eu, 0x40u, 0xE3u, 0xBAu, 0xAFu, 0x70u, 0xF2u, 0x7Cu, 0x2Au, 0xF7u};
    static constexpr unsigned char kPhase2Seed0[16] = {0xD6u, 0x86u, 0x56u, 0x25u, 0xF5u, 0xDCu, 0xC6u, 0xE2u, 0xB8u, 0x04u, 0x08u, 0xFCu, 0xE4u, 0x95u, 0x13u, 0xC5u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x7Au, 0x8Bu, 0x64u, 0x7Du, 0xBEu, 0xD8u, 0x41u, 0x08u, 0x19u, 0xE8u, 0x9Fu, 0x50u, 0x68u, 0x42u, 0xAFu, 0xE0u};
    constexpr int kPhase1SourceOffset1 = 5488;
    constexpr int kPhase1ControlOffset = 4272;
    constexpr int kPhase1FeedbackOffset = 1280;
    constexpr unsigned char kPhase1SaltBias = 131u;
    constexpr unsigned char kPhase1SaltStride = 9u;
    constexpr int kPhase2SourceOffset1 = 4432;
    constexpr int kPhase2ControlOffset = 5408;
    constexpr int kPhase2FeedbackOffset = 608;
    constexpr unsigned char kPhase2SaltBias = 219u;
    constexpr unsigned char kPhase2SaltStride = 29u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        SwapRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectAddShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        RotateRowLeft(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        WeaveRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[9] ^ aPhase1Salt[10]), static_cast<unsigned char>(aPhase1ControlB[12] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        AddWithBlock(aPhase1Matrix0, aPhase1Matrix1);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 1) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 5) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        RotateRowLeft(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        AddColumnIntoColumn(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectAddShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[6] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[8] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        SwapRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix0, aPhase2Carry1);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry0);
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 1) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] ^ aPhase2LaneMix0 ^ aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 5) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] ^ aPhase2LaneMix1 ^ aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        SwapRows(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        RotateColumnDown(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectXorShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 4198
// Final grade: A+ (98.788632)
void TwistBytes_08(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x8Du, 0xA6u, 0x89u, 0x4Au, 0xA0u, 0x14u, 0x7Du, 0x96u, 0x6Au, 0x42u, 0x01u, 0xBCu, 0xC8u, 0x4Fu, 0x3Du, 0x4Cu};
    static constexpr unsigned char kPhase1Seed1[16] = {0x70u, 0x93u, 0x4Bu, 0x8Cu, 0x9Fu, 0x10u, 0x72u, 0x02u, 0xCCu, 0x90u, 0x03u, 0x0Du, 0xFEu, 0x23u, 0xE1u, 0xF8u};
    static constexpr unsigned char kPhase2Seed0[16] = {0xA4u, 0x69u, 0xBDu, 0x91u, 0x88u, 0x11u, 0x91u, 0x1Cu, 0x5Bu, 0x71u, 0xE3u, 0xACu, 0x0Du, 0x71u, 0x44u, 0x59u};
    static constexpr unsigned char kPhase2Seed1[16] = {0xB8u, 0x0Cu, 0xC4u, 0x0Au, 0x0Au, 0xC8u, 0xE5u, 0xAEu, 0x17u, 0x63u, 0x77u, 0x97u, 0x81u, 0x0Au, 0x2Bu, 0xBDu};
    constexpr int kPhase1SourceOffset1 = 144;
    constexpr int kPhase1ControlOffset = 7120;
    constexpr int kPhase1FeedbackOffset = 1776;
    constexpr unsigned char kPhase1SaltBias = 237u;
    constexpr unsigned char kPhase1SaltStride = 5u;
    constexpr int kPhase2SourceOffset1 = 6160;
    constexpr int kPhase2ControlOffset = 5424;
    constexpr int kPhase2FeedbackOffset = 3232;
    constexpr unsigned char kPhase2SaltBias = 149u;
    constexpr unsigned char kPhase2SaltStride = 11u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        XorRowIntoRow(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        AddColumnIntoColumn(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        WeaveRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        RotateColumnDown(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        WeaveRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[9] ^ aPhase1Salt[10]), static_cast<unsigned char>(aPhase1ControlB[12] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 1) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 1) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        AddColumnIntoColumn(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        WeaveRows(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectXorShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectAddShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        SwapRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        SwapRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        AddWithBlock(aPhase2Matrix0, aPhase2Matrix1);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 1) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] + aPhase2LaneMix0 + aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 1) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] + aPhase2LaneMix1 + aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        WeaveRows(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectAddShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        SwapRows(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectXorShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 2191
// Final grade: A+ (98.775218)
void TwistBytes_09(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0xA7u, 0x19u, 0xB6u, 0xC7u, 0x3Cu, 0x5Au, 0xF0u, 0x1Cu, 0x53u, 0xABu, 0x89u, 0x39u, 0x7Fu, 0xABu, 0x0Cu, 0x35u};
    static constexpr unsigned char kPhase1Seed1[16] = {0x63u, 0x32u, 0xA4u, 0x02u, 0xB8u, 0x6Cu, 0x81u, 0x9Cu, 0x7Fu, 0x7Cu, 0xE9u, 0xA1u, 0xE8u, 0xFCu, 0x91u, 0x11u};
    static constexpr unsigned char kPhase2Seed0[16] = {0xBCu, 0xA8u, 0x99u, 0xA3u, 0x88u, 0x79u, 0x55u, 0x44u, 0x27u, 0x3Du, 0x3Au, 0x30u, 0x9Bu, 0x29u, 0x9Du, 0xA8u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x10u, 0x2Eu, 0x5Au, 0x60u, 0xD4u, 0x01u, 0x19u, 0x98u, 0x86u, 0x15u, 0xE8u, 0xD6u, 0xD2u, 0x46u, 0x29u, 0x76u};
    constexpr int kPhase1SourceOffset1 = 3184;
    constexpr int kPhase1ControlOffset = 3888;
    constexpr int kPhase1FeedbackOffset = 5120;
    constexpr unsigned char kPhase1SaltBias = 245u;
    constexpr unsigned char kPhase1SaltStride = 5u;
    constexpr int kPhase2SourceOffset1 = 5312;
    constexpr int kPhase2ControlOffset = 4368;
    constexpr int kPhase2FeedbackOffset = 432;
    constexpr unsigned char kPhase2SaltBias = 29u;
    constexpr unsigned char kPhase2SaltStride = 3u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        AddColumnIntoColumn(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        RotateColumnDown(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[4] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[5] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        WeaveRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 3) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 5) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        RotateColumnDown(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        SwapRows(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectXorShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectAddShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        RotateColumnDown(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[6] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[8] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        SwapRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        SwapRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry1);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 3) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] ^ aPhase2LaneMix0 ^ aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 5) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] ^ aPhase2LaneMix1 ^ aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        RotateRowLeft(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectAddShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        SwapRows(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 3786
// Final grade: A+ (98.739052)
void TwistBytes_10(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x11u, 0xE7u, 0xD1u, 0x99u, 0x20u, 0xE0u, 0x17u, 0xE7u, 0xA0u, 0xBCu, 0x3Du, 0xB6u, 0x06u, 0xADu, 0x62u, 0x12u};
    static constexpr unsigned char kPhase1Seed1[16] = {0xDEu, 0x74u, 0x27u, 0xCAu, 0xA2u, 0x49u, 0x0Cu, 0xA9u, 0x7Fu, 0x2Cu, 0xA2u, 0x46u, 0x43u, 0xAEu, 0x9Du, 0x54u};
    static constexpr unsigned char kPhase2Seed0[16] = {0x0Bu, 0x9Eu, 0x47u, 0xEBu, 0xBEu, 0xFFu, 0x40u, 0x1Bu, 0x47u, 0x92u, 0x93u, 0x1Au, 0x62u, 0x7Cu, 0x80u, 0xD5u};
    static constexpr unsigned char kPhase2Seed1[16] = {0xCFu, 0xACu, 0x4Au, 0x6Fu, 0xFFu, 0xCFu, 0x44u, 0xC0u, 0x1Eu, 0x76u, 0xB1u, 0xB9u, 0x3Cu, 0x7Bu, 0x7Bu, 0x3Au};
    constexpr int kPhase1SourceOffset1 = 6656;
    constexpr int kPhase1ControlOffset = 0;
    constexpr int kPhase1FeedbackOffset = 2368;
    constexpr unsigned char kPhase1SaltBias = 101u;
    constexpr unsigned char kPhase1SaltStride = 17u;
    constexpr int kPhase2SourceOffset1 = 4608;
    constexpr int kPhase2ControlOffset = 1072;
    constexpr int kPhase2FeedbackOffset = 3472;
    constexpr unsigned char kPhase2SaltBias = 17u;
    constexpr unsigned char kPhase2SaltStride = 11u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        XorWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        XorRowIntoRow(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        SwapRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        RotateColumnDown(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        XorRowIntoRow(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix0, aPhase1Carry1);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry0);
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 13) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 7) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        WeaveRows(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectXorShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        XorRowIntoRow(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectXorShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix0, aPhase2Carry1);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry0);
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 13) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] + aPhase2LaneMix0 + aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] + aPhase2LaneMix1 + aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        SwapRows(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        RotateColumnDown(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectXorShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 3713
// Final grade: A+ (98.751766)
void TwistBytes_11(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x07u, 0xA4u, 0x70u, 0x84u, 0xC4u, 0x1Eu, 0xB5u, 0x98u, 0x63u, 0x4Fu, 0x27u, 0x18u, 0x82u, 0x33u, 0x86u, 0x3Au};
    static constexpr unsigned char kPhase1Seed1[16] = {0x10u, 0x66u, 0x7Eu, 0xE5u, 0x9Fu, 0x3Fu, 0xE3u, 0x7Eu, 0x92u, 0x2Au, 0x10u, 0xEDu, 0xF8u, 0xDBu, 0x32u, 0x15u};
    static constexpr unsigned char kPhase2Seed0[16] = {0x81u, 0x35u, 0x04u, 0xCAu, 0x7Eu, 0x89u, 0x53u, 0x32u, 0xF0u, 0x9Cu, 0x95u, 0x6Fu, 0x0Du, 0x51u, 0x53u, 0xB1u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x40u, 0x32u, 0x60u, 0x13u, 0xFDu, 0x50u, 0xAFu, 0xEAu, 0x12u, 0x99u, 0xBFu, 0x6Eu, 0x33u, 0xC6u, 0x6Eu, 0x46u};
    constexpr int kPhase1SourceOffset1 = 6560;
    constexpr int kPhase1ControlOffset = 624;
    constexpr int kPhase1FeedbackOffset = 3584;
    constexpr unsigned char kPhase1SaltBias = 13u;
    constexpr unsigned char kPhase1SaltStride = 11u;
    constexpr int kPhase2SourceOffset1 = 5392;
    constexpr int kPhase2ControlOffset = 7072;
    constexpr int kPhase2FeedbackOffset = 4368;
    constexpr unsigned char kPhase2SaltBias = 159u;
    constexpr unsigned char kPhase2SaltStride = 13u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        AddColumnIntoColumn(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectAddShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        WeaveRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        AddWithBlock(aPhase1Matrix0, aPhase1Matrix1);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 3) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 7) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        AddColumnIntoColumn(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        SwapRows(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectAddShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry1);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 3) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] ^ aPhase2LaneMix0 ^ aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] ^ aPhase2LaneMix1 ^ aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        WeaveRows(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        AddColumnIntoColumn(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 4271
// Final grade: A+ (98.778181)
void TwistBytes_12(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x16u, 0x6Fu, 0x73u, 0xE6u, 0xD6u, 0x1Bu, 0x21u, 0x18u, 0x3Eu, 0x1Cu, 0xDBu, 0x54u, 0x44u, 0xD1u, 0x52u, 0x91u};
    static constexpr unsigned char kPhase1Seed1[16] = {0x53u, 0xBEu, 0x9Bu, 0x10u, 0xB8u, 0x4Au, 0x5Fu, 0x9Du, 0xF5u, 0x02u, 0x3Au, 0x67u, 0xFBu, 0x7Du, 0xCEu, 0x45u};
    static constexpr unsigned char kPhase2Seed0[16] = {0x52u, 0xF4u, 0x31u, 0x9Cu, 0x2Eu, 0x70u, 0xEFu, 0xD3u, 0xA8u, 0x75u, 0x99u, 0x35u, 0xF2u, 0xE9u, 0xB5u, 0x24u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x5Du, 0xA4u, 0xD2u, 0x71u, 0x5Cu, 0x30u, 0x69u, 0x99u, 0xC7u, 0x87u, 0xD6u, 0x37u, 0x4Bu, 0xA7u, 0xD2u, 0xA8u};
    constexpr int kPhase1SourceOffset1 = 1776;
    constexpr int kPhase1ControlOffset = 3808;
    constexpr int kPhase1FeedbackOffset = 4336;
    constexpr unsigned char kPhase1SaltBias = 249u;
    constexpr unsigned char kPhase1SaltStride = 19u;
    constexpr int kPhase2SourceOffset1 = 1808;
    constexpr int kPhase2ControlOffset = 4864;
    constexpr int kPhase2FeedbackOffset = 2752;
    constexpr unsigned char kPhase2SaltBias = 91u;
    constexpr unsigned char kPhase2SaltStride = 3u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        SwapRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[4] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[5] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectAddShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        RotateRowLeft(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        XorRowIntoRow(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix0, aPhase1Carry1);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry0);
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 13) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 11) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        WeaveRows(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        AddColumnIntoColumn(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectAddShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix0, aPhase2Carry1);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry0);
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 13) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] ^ aPhase2LaneMix0 ^ aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 11) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] ^ aPhase2LaneMix1 ^ aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        WeaveRows(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        RotateColumnDown(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 874
// Final grade: A+ (98.752764)
void TwistBytes_13(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0xBDu, 0xD3u, 0xECu, 0x34u, 0xBBu, 0x64u, 0x24u, 0x33u, 0xE7u, 0x63u, 0x2Eu, 0xA3u, 0x31u, 0x14u, 0x76u, 0x7Au};
    static constexpr unsigned char kPhase1Seed1[16] = {0x2Bu, 0xF0u, 0xEDu, 0x28u, 0xE7u, 0x2Du, 0xD5u, 0x0Au, 0x03u, 0x76u, 0xCBu, 0x29u, 0xFDu, 0xE2u, 0xA7u, 0x77u};
    static constexpr unsigned char kPhase2Seed0[16] = {0x97u, 0x75u, 0xDDu, 0x59u, 0xDEu, 0x59u, 0xE1u, 0xE2u, 0xA0u, 0x7Du, 0xEBu, 0xDEu, 0x90u, 0x1Au, 0xDDu, 0x66u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x41u, 0x9Bu, 0x11u, 0x01u, 0xD0u, 0xE3u, 0x40u, 0x4Bu, 0x2Cu, 0xC9u, 0xBEu, 0x1Cu, 0x25u, 0x46u, 0x08u, 0x60u};
    constexpr int kPhase1SourceOffset1 = 6544;
    constexpr int kPhase1ControlOffset = 3952;
    constexpr int kPhase1FeedbackOffset = 2400;
    constexpr unsigned char kPhase1SaltBias = 255u;
    constexpr unsigned char kPhase1SaltStride = 13u;
    constexpr int kPhase2SourceOffset1 = 5200;
    constexpr int kPhase2ControlOffset = 4496;
    constexpr int kPhase2FeedbackOffset = 6432;
    constexpr unsigned char kPhase2SaltBias = 223u;
    constexpr unsigned char kPhase2SaltStride = 19u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        AddColumnIntoColumn(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        AddColumnIntoColumn(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        WeaveRows(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[4] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[5] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectAddShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        XorRowIntoRow(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        XorRowIntoRow(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[9] ^ aPhase1Salt[10]), static_cast<unsigned char>(aPhase1ControlB[12] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 1) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 7) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        WeaveRows(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        XorRowIntoRow(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectAddShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectAddShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        RotateColumnDown(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        SwapRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        XorWithBlock(aPhase2Matrix1, aPhase2Matrix0);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry1);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 1) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] + aPhase2LaneMix0 + aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] + aPhase2LaneMix1 + aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        SwapRows(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectAddShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        RotateRowLeft(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 1563
// Final grade: A+ (98.754374)
void TwistBytes_14(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x99u, 0x21u, 0x4Eu, 0xC2u, 0xF6u, 0x17u, 0x06u, 0x30u, 0x1Eu, 0xE6u, 0x06u, 0x01u, 0xC6u, 0x72u, 0x2Fu, 0xC8u};
    static constexpr unsigned char kPhase1Seed1[16] = {0x51u, 0x77u, 0x12u, 0xDAu, 0xEAu, 0xA1u, 0x45u, 0x8Fu, 0x9Du, 0x70u, 0xF9u, 0x5Fu, 0xE6u, 0xF1u, 0x65u, 0x96u};
    static constexpr unsigned char kPhase2Seed0[16] = {0x4Fu, 0x1Cu, 0x70u, 0x47u, 0x9Cu, 0xC9u, 0x0Bu, 0x20u, 0x0Fu, 0x64u, 0x59u, 0x14u, 0x31u, 0x47u, 0xDCu, 0xE2u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x35u, 0xACu, 0xB9u, 0x31u, 0x80u, 0x7Eu, 0x85u, 0x63u, 0xC2u, 0x8Fu, 0x17u, 0x8Fu, 0xB2u, 0xEAu, 0x06u, 0xB7u};
    constexpr int kPhase1SourceOffset1 = 128;
    constexpr int kPhase1ControlOffset = 4016;
    constexpr int kPhase1FeedbackOffset = 5680;
    constexpr unsigned char kPhase1SaltBias = 21u;
    constexpr unsigned char kPhase1SaltStride = 5u;
    constexpr int kPhase2SourceOffset1 = 3600;
    constexpr int kPhase2ControlOffset = 144;
    constexpr int kPhase2FeedbackOffset = 7360;
    constexpr unsigned char kPhase2SaltBias = 167u;
    constexpr unsigned char kPhase2SaltStride = 11u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectAddShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        XorRowIntoRow(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        RotateRowLeft(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        RotateColumnDown(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[3] ^ aPhase1Salt[6]), static_cast<unsigned char>(aPhase1ControlB[4] + FoldAdd(aPhase1Carry1)));
        AddColumnIntoColumn(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[4] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[5] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        RotateRowLeft(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        SwapRows(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix0, aPhase1Carry1);
        AddWithBlock(aPhase1Matrix1, aPhase1Carry0);
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 7) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 9) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        AddColumnIntoColumn(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectAddShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        SwapRows(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectXorShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        XorWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        XorRowIntoRow(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        RotateRowLeft(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        XorRowIntoRow(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        RotateRowLeft(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        AddWithBlock(aPhase2Matrix0, aPhase2Matrix1);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 7) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] ^ aPhase2LaneMix0 ^ aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 9) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] ^ aPhase2LaneMix1 ^ aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        XorRowIntoRow(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectXorShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        XorRowIntoRow(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}

// Derived from Test Candidate 2128
// Final grade: A+ (98.762483)
void TwistBytes_15(const unsigned char* pSource, unsigned char* pDestination, unsigned char* pWorkspace) {
    static constexpr unsigned char kPhase1Seed0[16] = {0x09u, 0xB8u, 0xD6u, 0x20u, 0x81u, 0x84u, 0xA6u, 0x84u, 0x8Cu, 0x6Du, 0xD8u, 0x12u, 0xACu, 0x85u, 0x21u, 0xB6u};
    static constexpr unsigned char kPhase1Seed1[16] = {0xFDu, 0xE6u, 0x8Cu, 0x07u, 0x15u, 0x4Fu, 0xFAu, 0xE8u, 0x6Bu, 0x00u, 0xE6u, 0xADu, 0x76u, 0xAFu, 0x68u, 0x69u};
    static constexpr unsigned char kPhase2Seed0[16] = {0xD1u, 0x4Au, 0x7Bu, 0x39u, 0x66u, 0x21u, 0xC1u, 0x4Cu, 0x88u, 0x39u, 0x57u, 0x5Eu, 0x7Fu, 0x0Eu, 0xD0u, 0xE9u};
    static constexpr unsigned char kPhase2Seed1[16] = {0x8Bu, 0x18u, 0x77u, 0x37u, 0xC4u, 0x38u, 0x59u, 0x57u, 0x4Du, 0xD2u, 0xF1u, 0x01u, 0x51u, 0x51u, 0x1Au, 0xD1u};
    constexpr int kPhase1SourceOffset1 = 7024;
    constexpr int kPhase1ControlOffset = 3104;
    constexpr int kPhase1FeedbackOffset = 2752;
    constexpr unsigned char kPhase1SaltBias = 89u;
    constexpr unsigned char kPhase1SaltStride = 7u;
    constexpr int kPhase2SourceOffset1 = 6816;
    constexpr int kPhase2ControlOffset = 2368;
    constexpr int kPhase2FeedbackOffset = 3904;
    constexpr unsigned char kPhase2SaltBias = 235u;
    constexpr unsigned char kPhase2SaltStride = 29u;
    
    alignas(16) unsigned char aPhase1Carry0[16];
    alignas(16) unsigned char aPhase1Carry1[16];
    CopyBlock16(kPhase1Seed0, aPhase1Carry0);
    CopyBlock16(kPhase1Seed1, aPhase1Carry1);
    
    int aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 1: source -> worker
        alignas(16) unsigned char aPhase1ControlA[16];
        alignas(16) unsigned char aPhase1ControlB[16];
        alignas(16) unsigned char aPhase1Salt[16];
        alignas(16) unsigned char aPhase1Matrix0[16];
        alignas(16) unsigned char aPhase1Matrix1[16];
        alignas(16) unsigned char aPhase1Store0[16];
        alignas(16) unsigned char aPhase1Store1[16];
        alignas(16) unsigned char aPhase1Emit0[16];
        alignas(16) unsigned char aPhase1Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase1ControlOffset, aPhase1ControlA);
        LoadWrappedBlock16(pSource, aIndex + kPhase1FeedbackOffset, aPhase1ControlB);
        
        const unsigned char aPhase1Fold = static_cast<unsigned char>(FoldXor(aPhase1Carry0) + FoldAdd(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase1Fold +
                                                            kPhase1SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase1SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase1SaltStride + 2u)));
        }
        
        LoadWrappedBlock16(pSource, aIndex, aPhase1Matrix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase1SourceOffset1, aPhase1Matrix1);
        
        InjectAddShifted(aPhase1Matrix0, aPhase1Salt, 3u);
        XorWithBlock(aPhase1Matrix0, aPhase1Carry0);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlA, 5u);
        InjectXorShifted(aPhase1Matrix0, aPhase1ControlB, 7u);
        XorRowIntoRow(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[1] ^ aPhase1Salt[4]), static_cast<unsigned char>(aPhase1ControlB[2] + FoldAdd(aPhase1Carry1)));
        XorRowIntoRow(aPhase1Matrix0, static_cast<unsigned char>(aPhase1ControlA[2] ^ aPhase1Salt[5]), static_cast<unsigned char>(aPhase1ControlB[3] + FoldAdd(aPhase1Carry1)));
        
        InjectAddShifted(aPhase1Matrix1, aPhase1Salt, 6u);
        XorWithBlock(aPhase1Matrix1, aPhase1Carry1);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlA, 10u);
        InjectXorShifted(aPhase1Matrix1, aPhase1ControlB, 14u);
        AddColumnIntoColumn(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[6] ^ aPhase1Salt[7]), static_cast<unsigned char>(aPhase1ControlB[9] + FoldAdd(aPhase1Carry0)));
        RotateColumnDown(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[7] ^ aPhase1Salt[8]), static_cast<unsigned char>(aPhase1ControlB[10] + FoldAdd(aPhase1Carry0)));
        RotateRowLeft(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[8] ^ aPhase1Salt[9]), static_cast<unsigned char>(aPhase1ControlB[11] + FoldAdd(aPhase1Carry0)));
        RotateRowLeft(aPhase1Matrix1, static_cast<unsigned char>(aPhase1ControlA[9] ^ aPhase1Salt[10]), static_cast<unsigned char>(aPhase1ControlB[12] + FoldAdd(aPhase1Carry0)));
        
        // Phase 1 bridge
        XorWithBlock(aPhase1Matrix1, aPhase1Matrix0);
        AddWithBlock(aPhase1Matrix0, aPhase1Carry1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Store0);
        const unsigned char aPhase1EmitFold0 = static_cast<unsigned char>(FoldXor(aPhase1Matrix0) + FoldAdd(aPhase1Carry0) + FoldXor(aPhase1Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit0[aLane] = static_cast<unsigned char>(aPhase1Store0[(aLane + 9) & 15] ^ aPhase1ControlA[(aLane + 1) & 15] ^ aPhase1Salt[(aLane + 3) & 15] ^ aPhase1EmitFold0);
        }
        StoreBlock16(pWorkspace, aIndex, aPhase1Emit0);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Store1);
        const unsigned char aPhase1EmitFold1 = static_cast<unsigned char>(FoldXor(aPhase1Matrix1) + FoldAdd(aPhase1Carry1) + FoldXor(aPhase1Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase1Emit1[aLane] = static_cast<unsigned char>(aPhase1Store1[(aLane + 13) & 15] + aPhase1ControlB[(aLane + 9) & 15] + aPhase1Salt[(aLane + 8) & 15] + aPhase1EmitFold1);
        }
        StoreBlock16(pWorkspace, aIndex + 16, aPhase1Emit1);
        
        CopyBlock16(aPhase1Matrix0, aPhase1Carry0);
        XorRowIntoRow(aPhase1Carry0, aPhase1ControlB[9], aPhase1ControlA[11]);
        InjectAddShifted(aPhase1Carry0, aPhase1Salt, 9u);
        InjectXorShifted(aPhase1Carry0, aPhase1ControlB, 11u);
        
        CopyBlock16(aPhase1Matrix1, aPhase1Carry1);
        RotateRowLeft(aPhase1Carry1, aPhase1ControlB[14], aPhase1ControlA[14]);
        InjectAddShifted(aPhase1Carry1, aPhase1Salt, 2u);
        InjectXorShifted(aPhase1Carry1, aPhase1ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
    
    alignas(16) unsigned char aPhase2Carry0[16];
    alignas(16) unsigned char aPhase2Carry1[16];
    CopyBlock16(kPhase2Seed0, aPhase2Carry0);
    CopyBlock16(kPhase2Seed1, aPhase2Carry1);
    
    aIndex = 0;
    while (aIndex < PASSWORD_EXPANDED_SIZE) {
        // Phase 2: worker -> destination
        alignas(16) unsigned char aPhase2ControlA[16];
        alignas(16) unsigned char aPhase2ControlB[16];
        alignas(16) unsigned char aPhase2Salt[16];
        alignas(16) unsigned char aPhase2Matrix0[16];
        alignas(16) unsigned char aPhase2Matrix1[16];
        alignas(16) unsigned char aPhase2Store0[16];
        alignas(16) unsigned char aPhase2Store1[16];
        alignas(16) unsigned char aPhase2SourceMix0[16];
        alignas(16) unsigned char aPhase2SourceMix1[16];
        alignas(16) unsigned char aPhase2FinalSource0[16];
        alignas(16) unsigned char aPhase2FinalSource1[16];
        alignas(16) unsigned char aPhase2Emit0[16];
        alignas(16) unsigned char aPhase2Emit1[16];
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2ControlOffset, aPhase2ControlA);
        LoadWrappedBlock16(pWorkspace, aIndex + kPhase2FeedbackOffset, aPhase2ControlB);
        
        const unsigned char aPhase2Fold = static_cast<unsigned char>(FoldAdd(aPhase2Carry0) + FoldXor(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            aPhase2Salt[aLane] = static_cast<unsigned char>(
                                                            aPhase2Fold +
                                                            kPhase2SaltBias +
                                                            static_cast<unsigned char>(aLane * kPhase2SaltStride) +
                                                            static_cast<unsigned char>(((aIndex / 16) + aLane) * (kPhase2SaltStride + 4u)));
        }
        
        LoadWrappedBlock16(pWorkspace, aIndex, aPhase2Matrix0);
        LoadWrappedBlock16(pWorkspace, aIndex + 16, aPhase2Matrix1);
        LoadWrappedBlock16(pSource, aIndex, aPhase2SourceMix0);
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2SourceMix1);
        
        InjectAddShifted(aPhase2Matrix0, aPhase2SourceMix0, 3u);
        AddWithBlock(aPhase2Matrix0, aPhase2Carry0);
        InjectAddShifted(aPhase2Matrix0, aPhase2ControlA, 5u);
        InjectAddShifted(aPhase2Matrix0, aPhase2Salt, 7u);
        InjectXorShifted(aPhase2Matrix0, aPhase2ControlB, 9u);
        SwapRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[3] ^ aPhase2Salt[6]), static_cast<unsigned char>(aPhase2ControlB[5] + FoldAdd(aPhase2Carry1)));
        AddColumnIntoColumn(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[4] ^ aPhase2Salt[7]), static_cast<unsigned char>(aPhase2ControlB[6] + FoldAdd(aPhase2Carry1)));
        WeaveRows(aPhase2Matrix0, static_cast<unsigned char>(aPhase2ControlA[5] ^ aPhase2Salt[8]), static_cast<unsigned char>(aPhase2ControlB[7] + FoldAdd(aPhase2Carry1)));
        
        InjectAddShifted(aPhase2Matrix1, aPhase2SourceMix1, 6u);
        AddWithBlock(aPhase2Matrix1, aPhase2Carry1);
        InjectAddShifted(aPhase2Matrix1, aPhase2ControlA, 10u);
        InjectAddShifted(aPhase2Matrix1, aPhase2Salt, 14u);
        InjectXorShifted(aPhase2Matrix1, aPhase2ControlB, 2u);
        AddColumnIntoColumn(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[8] ^ aPhase2Salt[9]), static_cast<unsigned char>(aPhase2ControlB[12] + FoldAdd(aPhase2Carry0)));
        WeaveRows(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[9] ^ aPhase2Salt[10]), static_cast<unsigned char>(aPhase2ControlB[13] + FoldAdd(aPhase2Carry0)));
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[10] ^ aPhase2Salt[11]), static_cast<unsigned char>(aPhase2ControlB[14] + FoldAdd(aPhase2Carry0)));
        RotateColumnDown(aPhase2Matrix1, static_cast<unsigned char>(aPhase2ControlA[11] ^ aPhase2Salt[12]), static_cast<unsigned char>(aPhase2ControlB[15] + FoldAdd(aPhase2Carry0)));
        
        // Phase 2 bridge
        AddWithBlock(aPhase2Matrix0, aPhase2Matrix1);
        XorWithBlock(aPhase2Matrix1, aPhase2Carry0);
        
        LoadWrappedBlock16(pSource, aIndex, aPhase2FinalSource0);
        CopyBlock16(aPhase2Matrix0, aPhase2Store0);
        const unsigned char aPhase2FoldX0 = static_cast<unsigned char>(FoldXor(aPhase2Matrix0) ^ FoldXor(aPhase2Carry1));
        const unsigned char aPhase2FoldA0 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix0) + FoldAdd(aPhase2Carry0));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix0 = static_cast<unsigned char>(aPhase2Store0[(aLane + 9) & 15] + aPhase2ControlA[(aLane + 7) & 15] + aPhase2ControlB[(aLane + 9) & 15] + aPhase2Salt[(aLane + 11) & 15] + aPhase2FoldA0);
            aPhase2Emit0[aLane] = static_cast<unsigned char>(aPhase2FinalSource0[aLane] + aPhase2LaneMix0 + aPhase2FoldX0);
        }
        StoreBlock16(pDestination, aIndex, aPhase2Emit0);
        
        LoadWrappedBlock16(pSource, aIndex + kPhase2SourceOffset1, aPhase2FinalSource1);
        CopyBlock16(aPhase2Matrix1, aPhase2Store1);
        const unsigned char aPhase2FoldX1 = static_cast<unsigned char>(FoldXor(aPhase2Matrix1) ^ FoldXor(aPhase2Carry0));
        const unsigned char aPhase2FoldA1 = static_cast<unsigned char>(FoldAdd(aPhase2Matrix1) + FoldAdd(aPhase2Carry1));
        for (int aLane = 0; aLane < 16; ++aLane) {
            const unsigned char aPhase2LaneMix1 = static_cast<unsigned char>(aPhase2Store1[(aLane + 13) & 15] + aPhase2ControlA[(aLane + 10) & 15] + aPhase2ControlB[(aLane + 14) & 15] + aPhase2Salt[(aLane + 2) & 15] + aPhase2FoldA1);
            aPhase2Emit1[aLane] = static_cast<unsigned char>(aPhase2FinalSource1[aLane] + aPhase2LaneMix1 + aPhase2FoldX1);
        }
        StoreBlock16(pDestination, aIndex + 16, aPhase2Emit1);
        
        CopyBlock16(aPhase2Matrix0, aPhase2Carry0);
        SwapRows(aPhase2Carry0, aPhase2ControlA[13], aPhase2ControlB[10]);
        InjectAddShifted(aPhase2Carry0, aPhase2Emit0, 13u);
        InjectAddShifted(aPhase2Carry0, aPhase2ControlB, 11u);
        
        CopyBlock16(aPhase2Matrix1, aPhase2Carry1);
        AddColumnIntoColumn(aPhase2Carry1, aPhase2ControlA[2], aPhase2ControlB[13]);
        InjectAddShifted(aPhase2Carry1, aPhase2Emit1, 10u);
        InjectAddShifted(aPhase2Carry1, aPhase2ControlB, 6u);
        
        aIndex += kChunkBytes;
    }
}
}  // namespace

void ByteTwister::Get(unsigned char* pSource,
                      unsigned char* pWorker,
                      unsigned char* pDestination,
                      unsigned int pLength) {
    TwistBytes(mType, pSource, pWorker, pDestination, pLength);
}

void ByteTwister::TwistBytes(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned int pLength) {
    TwistBytesByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pLength);
}

void ByteTwister::TwistBytesByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned int pLength) {
    if (pSource == nullptr || pDestination == nullptr) {
        return;
    }
    constexpr unsigned int kBlockLength = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE);
    if (pLength == 0U || (pLength % kBlockLength) != 0U) {
        std::abort();
    }
    
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorkspace{};
    unsigned char* aWorkspaceBuffer = (pWorker != nullptr) ? pWorker : aWorkspace.data();
    
    for (unsigned int aOffset = 0U; aOffset < pLength; aOffset += kBlockLength) {
        TwistSingleBlock(pType, pSource + aOffset, aWorkspaceBuffer, pDestination + aOffset);
    }
}

}  // namespace peanutbutter::expansion::key_expansion
