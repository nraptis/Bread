#include "ByteTwister.hpp"

#undef PASSWORD_EXPANDED_SIZE
#define PASSWORD_EXPANDED_SIZE kPasswordExpandedSize

#include <array>
#include <cstdlib>

namespace twist {

// Exported from shard sources for ByteTwister compatibility
// selected_candidate_ids=224,340,420,631,682,737,748,750,786,802,872,1093,1170,1200,1266,1301

// Candidate 224: TwistCandidate_0224
// family=mechanical_loops op_budget=3 loop_shapes=1x0/3x2/3x0
// mechanical_loops[l1=1x0; l2=3x2; l3=3x0; key_rot=27; twiddle=0xdd5a874a]
static void TwistCandidate_0224_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-3739)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-1620)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 242u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0224_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 8u + (2281)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 11u + (1270)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 8u + static_cast<unsigned int>(242u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 184u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0224_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xDD5A874Au ^ (pRound * 60u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-3739), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x48F02208u ^ static_cast<std::uint32_t>(3097u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4268), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ (a) ^ (salt_byte ^ 12u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + 7u) ^ ((a >> 2u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 25u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 64u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 14u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x86EB5CD0u ^ static_cast<std::uint32_t>(3375u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1184), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 17u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2526), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (2033), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b + 15u + (salt_byte ^ 11u)) ^ ((c >> 4u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 23u ^ (key_byte)) ^ (b) + ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 3u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 6u), 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 170u, 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD6ED1620u ^ static_cast<std::uint32_t>(698u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1451), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2391), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4540), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 25u + (salt_byte ^ 23u)) ^ ((b << 3u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (key_byte) ^ ((feedback_byte + 16u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b + 12u + (twiddle_byte)) ^ (a) ^ (salt_byte ^ 23u) ^ (((feedback_byte + 16u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 16u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 214u, 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 135u), 13u);
    }
  }

}

static void TwistCandidate_0224_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-5695)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (1387)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 7u + (6770)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 4u + static_cast<unsigned int>(27u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(17u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 6u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 184u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0224(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0224_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0224_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0224_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0224_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 340: TwistCandidate_0340
// family=mechanical_loops op_budget=3 loop_shapes=3x0/2x1/3x1
// mechanical_loops[l1=3x0; l2=2x1; l3=3x1; key_rot=29; twiddle=0x269edadd]
static void TwistCandidate_0340_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (3631)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-1900)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 230u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0340_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (-3040)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-4797)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 22u + static_cast<unsigned int>(230u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 17u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0340_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x269EDADDu ^ (pRound * 45u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (3631), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x71BD7E78u ^ static_cast<std::uint32_t>(7188u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (607), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 16u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6852), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-943), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 20u + (salt_byte ^ 6u)) ^ (b) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 1u) & 0xFFu) ^ ((feedback_byte + 8u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b + 31u + (twiddle_byte)) ^ (a) ^ (salt_byte ^ 6u) ^ (((feedback_byte + 8u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 8u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 1u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 61u), 11u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 152u, 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xFF4E58E1u ^ static_cast<std::uint32_t>(3424u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2749), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 23u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1032), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 18u + ((key_byte + 24u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 19u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 8u), 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 49u, 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x505C7BAAu ^ static_cast<std::uint32_t>(8971u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4617), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 5u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 15u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7593), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5995), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) + (c) + (salt_byte ^ 20u)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c) ^ ((a >> 1u) & 0xFFu) ^ ((key_byte + 7u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 20u) ^ (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 19u), 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 170u), 1u);
    }
  }

}

static void TwistCandidate_0340_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (1738)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (7361)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-7212)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 25u + static_cast<unsigned int>(29u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 4u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 17u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0340(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0340_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0340_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0340_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0340_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 420: TwistCandidate_0420
// family=mechanical_loops op_budget=3 loop_shapes=3x0/2x2/3x0
// mechanical_loops[l1=3x0; l2=2x2; l3=3x0; key_rot=24; twiddle=0x657883f2]
static void TwistCandidate_0420_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (7019)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-236)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 120u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0420_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-4145)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (6629)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 2u + static_cast<unsigned int>(120u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 166u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0420_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x657883F2u ^ (pRound * 19u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (7019), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1C0F863Bu ^ static_cast<std::uint32_t>(355u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3049), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1962), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-1442), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 17u + (salt_byte ^ 11u)) ^ ((b << 3u) & 0xFFu) ^ (c) ^ ((key_byte + 7u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b + 25u + (twiddle_byte)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 11u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (twiddle_byte) + (feedback_byte)) & 0xFFu) ^ ((key_byte + 7u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 56u, 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 66u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x77C8649Bu ^ static_cast<std::uint32_t>(2653u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2984), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5620) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 5u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 6u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 30u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a >> 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 6u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 115u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 117u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x78C976F2u ^ static_cast<std::uint32_t>(3619u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-346), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 4u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-641), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2632), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a) ^ ((b << 2u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (key_byte) ^ ((feedback_byte + 9u) & 0xFFu));
      const std::uint32_t e = (((c >> 3u) & 0xFFu) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte ^ 10u) ^ (((feedback_byte + 9u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 9u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 253u, 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 191u), 6u);
    }
  }

}

static void TwistCandidate_0420_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 3u + (2642)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-4966)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 19u + (-4264)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 3u + static_cast<unsigned int>(24u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 15u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 166u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0420(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0420_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0420_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0420_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0420_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 631: TwistCandidate_0631
// family=mechanical_loops op_budget=3 loop_shapes=2x1/2x3/3x2
// mechanical_loops[l1=2x1; l2=2x3; l3=3x2; key_rot=29; twiddle=0x9b5f0c2c]
static void TwistCandidate_0631_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-4145)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (2332)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 45u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0631_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 8u + (5604)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (7300)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 8u + static_cast<unsigned int>(45u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 237u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0631_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x9B5F0C2Cu ^ (pRound * 12u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-4145), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x61B26EEDu ^ static_cast<std::uint32_t>(5673u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3189), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3786), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 30u + ((key_byte + 2u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 16u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 5u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 2u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 29u), 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 151u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xCB0588AEu ^ static_cast<std::uint32_t>(15343u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2214), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5745) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte ^ 28u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 4u + (twiddle_byte)) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 11u, 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 116u, 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3D175841u ^ static_cast<std::uint32_t>(4887u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6571), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3717), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5401), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ ((c >> 4u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 20u ^ (key_byte)) ^ ((b << 3u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 219u, 10u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 34u), 4u);
    }
  }

}

static void TwistCandidate_0631_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-1874)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-7518)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 27u + (1182)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 25u + static_cast<unsigned int>(29u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(23u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 10u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 237u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0631(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0631_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0631_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0631_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0631_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 682: TwistCandidate_0682
// family=mechanical_loops op_budget=3 loop_shapes=1x2/2x2/3x0
// mechanical_loops[l1=1x2; l2=2x2; l3=3x0; key_rot=1; twiddle=0xf501def0]
static void TwistCandidate_0682_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-5580)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-1189)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 135u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0682_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-5217)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-6955)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 8u + static_cast<unsigned int>(135u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 63u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0682_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xF501DEF0u ^ (pRound * 62u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5580), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x75243503u ^ static_cast<std::uint32_t>(4521u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3994), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 25u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ ((a << 5u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 18u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 3u) ^ (a)) & 0xFFu) ^ ((key_byte + 31u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 76u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 159u), 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3770BA34u ^ static_cast<std::uint32_t>(10357u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1232), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 9u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6343) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte ^ 25u) ^ ((feedback_byte + 24u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 24u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 149u, 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 63u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x33A1E464u ^ static_cast<std::uint32_t>(721u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4073), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 30u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-574), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2778), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 17u + (salt_byte ^ 11u)) ^ (b) ^ (c) ^ (key_byte) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b) ^ (a) ^ (salt_byte ^ 11u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 52u), 1u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 165u, 3u);
    }
  }

}

static void TwistCandidate_0682_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (3385)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-7158)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-4357)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 29u + static_cast<unsigned int>(1u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 63u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0682(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0682_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0682_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0682_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0682_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 737: TwistCandidate_0737
// family=mechanical_loops op_budget=3 loop_shapes=2x2/3x3/3x2
// mechanical_loops[l1=2x2; l2=3x3; l3=3x2; key_rot=10; twiddle=0x3d425df6]
static void TwistCandidate_0737_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-2013)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (1739)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 21u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0737_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (6767)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-1723)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 26u + static_cast<unsigned int>(21u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 50u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0737_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x3D425DF6u ^ (pRound * 17u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-2013), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x32452BDFu ^ static_cast<std::uint32_t>(14666u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2430), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 12u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4812), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 5u) & 0xFFu) ^ (salt_byte ^ 12u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 16u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 186u), 3u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 30u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xAA3FD809u ^ static_cast<std::uint32_t>(18497u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5249), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 15u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-7363), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-5885), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 17u + ((key_byte + 22u) & 0xFFu)) ^ (c + 15u + (salt_byte ^ 31u)) ^ (b) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 5u) & 0xFFu) ^ (a) ^ ((c * 5u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 22u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 36u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 234u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1135E5B3u ^ static_cast<std::uint32_t>(4877u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-761), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2298), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-1818), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ (c) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 24u ^ (key_byte)) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 167u), 3u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 127u), 9u);
    }
  }

}

static void TwistCandidate_0737_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-6661)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 25u + (6326)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 2u + (4196)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 5u + static_cast<unsigned int>(10u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(11u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 0u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 50u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0737(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0737_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0737_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0737_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0737_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 748: TwistCandidate_0748
// family=mechanical_loops op_budget=3 loop_shapes=2x1/3x0/3x2
// mechanical_loops[l1=2x1; l2=3x0; l3=3x2; key_rot=3; twiddle=0xc2dfbc35]
static void TwistCandidate_0748_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (2994)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (2607)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 51u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0748_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-6954)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (889)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 8u + static_cast<unsigned int>(51u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 219u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0748_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xC2DFBC35u ^ (pRound * 54u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2994), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9394680Eu ^ static_cast<std::uint32_t>(2285u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1389), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 31u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3348), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 15u + (key_byte)) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 21u + (twiddle_byte)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 2u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 62u), 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 95u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x95A78531u ^ static_cast<std::uint32_t>(9034u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2320), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 30u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3893), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-2821), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 5u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 1u) & 0xFFu) ^ (b) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 25u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 140u), 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 138u, 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xAB1CC1D6u ^ static_cast<std::uint32_t>(10704u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6394), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-525), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4835), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b + 3u + (salt_byte ^ 13u)) ^ ((c >> 2u) & 0xFFu) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 20u ^ (key_byte)) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 215u, 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 167u), 3u);
    }
  }

}

static void TwistCandidate_0748_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-6965)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-1846)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 14u + (226)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 20u + static_cast<unsigned int>(3u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(27u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 5u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 219u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0748(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0748_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0748_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0748_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0748_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 750: TwistCandidate_0750
// family=mechanical_loops op_budget=3 loop_shapes=2x3/2x1/3x2
// mechanical_loops[l1=2x3; l2=2x1; l3=3x2; key_rot=10; twiddle=0x4868d40e]
static void TwistCandidate_0750_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (-6241)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (1500)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 42u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0750_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-5546)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-6451)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 26u + static_cast<unsigned int>(42u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 69u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0750_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x4868D40Eu ^ (pRound * 25u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-6241), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xE0192B1Fu ^ static_cast<std::uint32_t>(1612u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3817), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5152), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 28u + (key_byte)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 13u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 15u + (twiddle_byte)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 103u, 10u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 247u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x0BEB7C64u ^ static_cast<std::uint32_t>(3509u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6543), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 28u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4936) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 25u + (key_byte)) ^ ((b << 2u) & 0xFFu) ^ (salt_byte ^ 21u) ^ ((feedback_byte + 19u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 3u + (twiddle_byte)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 19u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 173u), 1u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 216u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xB3DBC34Eu ^ static_cast<std::uint32_t>(6262u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5592), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3433), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4103), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ ((c >> 1u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 12u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 10u ^ ((key_byte + 17u) & 0xFFu)) ^ (b) + ((feedback_byte + 12u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 17u) & 0xFFu) + ((feedback_byte + 12u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 36u, 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 162u), 11u);
    }
  }

}

static void TwistCandidate_0750_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-324)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (5141)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 28u + (4500)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 20u + static_cast<unsigned int>(10u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(29u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 4u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 69u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0750(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0750_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0750_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0750_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0750_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 786: TwistCandidate_0786
// family=mechanical_loops op_budget=3 loop_shapes=1x0/2x2/3x3
// mechanical_loops[l1=1x0; l2=2x2; l3=3x3; key_rot=6; twiddle=0x367b02ea]
static void TwistCandidate_0786_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (7587)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (6208)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 13u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0786_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-3192)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-2018)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 21u + static_cast<unsigned int>(13u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 167u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0786_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x367B02EAu ^ (pRound * 31u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (7587), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF1F53D91u ^ static_cast<std::uint32_t>(5916u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2031), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ (a) ^ (salt_byte) ^ ((feedback_byte + 10u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 8u) ^ ((a >> 5u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 2u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 125u, 2u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 70u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xDDEC8833u ^ static_cast<std::uint32_t>(1424u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4777), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4132), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 7u + ((key_byte + 25u) & 0xFFu)) ^ (b) ^ (salt_byte ^ 7u) ^ ((feedback_byte + 15u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 26u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 170u, 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 149u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x08F9C9E0u ^ static_cast<std::uint32_t>(7679u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6421), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 18u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (753), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2011), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a + 30u + (key_byte)) ^ (c + 23u + (salt_byte ^ 7u)) ^ (b) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 2u) & 0xFFu) ^ ((a << 4u) & 0xFFu) ^ (c) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 241u, 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 48u), 12u);
    }
  }

}

static void TwistCandidate_0786_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-6543)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-1014)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 22u + (-4980)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 21u + static_cast<unsigned int>(6u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(11u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 167u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0786(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0786_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0786_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0786_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0786_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 802: TwistCandidate_0802
// family=mechanical_loops op_budget=3 loop_shapes=3x1/3x0/3x2
// mechanical_loops[l1=3x1; l2=3x0; l3=3x2; key_rot=1; twiddle=0x30cea67a]
static void TwistCandidate_0802_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 20u + (5919)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (854)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 216u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0802_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (3630)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-4193)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 16u + static_cast<unsigned int>(216u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 102u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0802_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x30CEA67Au ^ (pRound * 53u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5919), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x2EE4412Du ^ static_cast<std::uint32_t>(9528u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5768), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3844), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (7604), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 1u + (key_byte) + (feedback_byte))) + (c) + (salt_byte ^ 15u)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c) ^ ((a >> 5u) & 0xFFu) ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + e) ^ b ^ c ^ (salt_byte ^ 15u) ^ (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 72u), 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 76u, 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x02CEB679u ^ static_cast<std::uint32_t>(8161u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7040), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 5u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (811), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-1932), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a + 31u + (salt_byte)) ^ ((b << 1u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 10u) & 0xFFu) ^ ((feedback_byte + 10u) & 0xFFu));
      const std::uint32_t e = (((c >> 3u) & 0xFFu) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 10u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 10u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 127u), 7u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 152u, 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x0AD751DAu ^ static_cast<std::uint32_t>(3638u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6006), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 21u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-485), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (2853), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ (c) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 11u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a) ^ ((b << 5u) & 0xFFu) + ((feedback_byte + 11u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 11u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 203u, 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 130u), 9u);
    }
  }

}

static void TwistCandidate_0802_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (-3698)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (3092)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 28u + (-1194)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 9u + static_cast<unsigned int>(1u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(19u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 102u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0802(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0802_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0802_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0802_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0802_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 872: TwistCandidate_0872
// family=mechanical_loops op_budget=3 loop_shapes=3x2/3x1/3x1
// mechanical_loops[l1=3x2; l2=3x1; l3=3x1; key_rot=30; twiddle=0xe25b3345]
static void TwistCandidate_0872_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-657)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (3578)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 230u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0872_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-5601)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (1435)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 25u + static_cast<unsigned int>(230u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 162u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0872_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xE25B3345u ^ (pRound * 63u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-657), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x93C4C345u ^ static_cast<std::uint32_t>(1352u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4033), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5612), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (2931), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b) ^ ((c >> 3u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a ^ 4u ^ ((key_byte + 12u) & 0xFFu)) ^ ((b << 4u) & 0xFFu) + ((feedback_byte + 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((key_byte + 12u) & 0xFFu) + ((feedback_byte + 4u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 12u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 158u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 73u), 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x4B6999CDu ^ static_cast<std::uint32_t>(4929u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (136), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3413) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-1652) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 25u + ((key_byte + 19u) & 0xFFu) + (feedback_byte))) + ((c << 5u) & 0xFFu) + (salt_byte ^ 3u)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c + 25u + (twiddle_byte)) ^ ((a >> 3u) & 0xFFu) ^ ((key_byte + 19u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 3u) ^ (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 3u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 38u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9EF2B03Du ^ static_cast<std::uint32_t>(9608u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6492), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 22u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4562), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (7678), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) + (c) + (salt_byte ^ 23u)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c + 23u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 3u) & 0xFFu) ^ ((key_byte + 19u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 23u) ^ (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 173u), 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 60u), 4u);
    }
  }

}

static void TwistCandidate_0872_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-7427)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-2212)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 18u + (2548)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 24u + static_cast<unsigned int>(30u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 162u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0872(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0872_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0872_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0872_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0872_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 1093: TwistCandidate_1093
// family=mechanical_loops op_budget=3 loop_shapes=3x2/3x0/3x1
// mechanical_loops[l1=3x2; l2=3x0; l3=3x1; key_rot=19; twiddle=0x02acde28]
static void TwistCandidate_1093_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 19u + (6360)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (5130)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 215u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1093_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-2938)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (1751)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 21u + static_cast<unsigned int>(215u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 227u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1093_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x02ACDE28u ^ (pRound * 61u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (6360), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x0A1DEE55u ^ static_cast<std::uint32_t>(12972u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2535), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 19u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6291), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (4146), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b + 12u + (salt_byte ^ 4u)) ^ ((c >> 5u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 23u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 29u ^ (key_byte)) ^ ((b << 2u) & 0xFFu) + ((feedback_byte + 23u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (key_byte) + ((feedback_byte + 23u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 171u, 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 147u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x587E8C2Du ^ static_cast<std::uint32_t>(2792u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-119), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 4u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5874), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-2963), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a + 18u + (salt_byte)) ^ ((b << 5u) & 0xFFu) ^ (c) ^ ((key_byte + 12u) & 0xFFu) ^ ((feedback_byte + 28u) & 0xFFu));
      const std::uint32_t e = (((c >> 4u) & 0xFFu) ^ (b) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 28u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 28u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 25u, 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 208u, 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xCCFBF5F9u ^ static_cast<std::uint32_t>(4726u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6579), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5434), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3581), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 8u + ((key_byte + 28u) & 0xFFu) + ((feedback_byte + 26u) & 0xFFu))) + (c) + (salt_byte ^ 21u)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c) ^ ((a >> 3u) & 0xFFu) ^ ((key_byte + 28u) & 0xFFu) ^ ((feedback_byte + 26u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 21u) ^ ((feedback_byte + 26u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 141u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 125u, 5u);
    }
  }

}

static void TwistCandidate_1093_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (3081)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (6986)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-5532)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 22u + static_cast<unsigned int>(19u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(23u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 227u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1093(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1093_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1093_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1093_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1093_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 1170: TwistCandidate_1170
// family=mechanical_loops op_budget=3 loop_shapes=1x2/3x3/3x2
// mechanical_loops[l1=1x2; l2=3x3; l3=3x2; key_rot=11; twiddle=0xe3c02aa4]
static void TwistCandidate_1170_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-151)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-2642)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 61u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1170_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (5781)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (888)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 12u + static_cast<unsigned int>(61u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 223u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1170_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xE3C02AA4u ^ (pRound * 62u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-151), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xEC3ADB6Au ^ static_cast<std::uint32_t>(9131u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4625), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 13u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 3u + ((key_byte + 3u) & 0xFFu)) ^ ((a << 2u) & 0xFFu) ^ (salt_byte ^ 6u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + 14u) ^ ((a >> 2u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 3u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 32u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 216u, 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC203C31Eu ^ static_cast<std::uint32_t>(15542u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2920), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 13u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7152), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (5470), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 18u + (key_byte)) ^ (c + 1u + (salt_byte)) ^ ((b * 5u) & 0xFFu) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 3u) & 0xFFu) ^ (a) ^ ((c * 5u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 217u, 11u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 109u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xE3692BF6u ^ static_cast<std::uint32_t>(7363u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6126), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4141), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-5378), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 28u + (salt_byte ^ 6u)) ^ ((c >> 1u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 12u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a) ^ (b) + ((feedback_byte + 12u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 12u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 164u), 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 83u), 1u);
    }
  }

}

static void TwistCandidate_1170_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (2878)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 7u + (5151)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-2160)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 7u + static_cast<unsigned int>(11u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 223u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1170(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1170_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1170_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1170_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1170_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 1200: TwistCandidate_1200
// family=mechanical_loops op_budget=3 loop_shapes=2x2/2x3/3x1
// mechanical_loops[l1=2x2; l2=2x3; l3=3x1; key_rot=29; twiddle=0xf28a26b7]
static void TwistCandidate_1200_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (1026)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (6672)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 192u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1200_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (1146)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-5822)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 29u + static_cast<unsigned int>(192u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 216u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1200_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xF28A26B7u ^ (pRound * 36u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (1026), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x41167C87u ^ static_cast<std::uint32_t>(3581u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3214), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5559), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte ^ 30u) ^ ((feedback_byte + 26u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 26u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 238u), 5u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 16u, 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC4B2AD13u ^ static_cast<std::uint32_t>(8760u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7427), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (728), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte ^ 13u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 5u + (twiddle_byte)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 248u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 148u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xB645E4A3u ^ static_cast<std::uint32_t>(10195u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2105), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6631), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5669), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) + ((c << 2u) & 0xFFu) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c + 22u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a >> 1u) & 0xFFu) ^ ((key_byte + 7u) & 0xFFu) ^ ((feedback_byte + 29u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ ((feedback_byte + 29u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 95u, 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 45u), 5u);
    }
  }

}

static void TwistCandidate_1200_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-3986)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (72)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-3089)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 31u + static_cast<unsigned int>(29u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 4u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 216u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1200(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1200_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1200_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1200_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1200_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 1266: TwistCandidate_1266
// family=mechanical_loops op_budget=3 loop_shapes=2x0/2x0/3x0
// mechanical_loops[l1=2x0; l2=2x0; l3=3x0; key_rot=28; twiddle=0x137ae7a7]
static void TwistCandidate_1266_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-4795)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-6045)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 18u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1266_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-5310)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (3530)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 4u + static_cast<unsigned int>(18u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 84u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1266_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x137AE7A7u ^ (pRound * 41u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-4795), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x575B1950u ^ static_cast<std::uint32_t>(2283u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4538), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3237), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 6u + ((key_byte + 14u) & 0xFFu)) ^ (b) ^ (salt_byte ^ 27u) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 29u + (twiddle_byte)) ^ ((a >> 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 5u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 14u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 17u, 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 29u, 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6D6EC951u ^ static_cast<std::uint32_t>(10608u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1635), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5884) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 28u + (key_byte)) ^ (b) ^ (salt_byte ^ 11u) ^ ((feedback_byte + 19u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 26u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 19u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 174u, 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 59u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x428FF329u ^ static_cast<std::uint32_t>(10790u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1569), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5197), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4024), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a + 17u + (salt_byte ^ 20u)) ^ (b) ^ ((c * 5u) & 0xFFu) ^ (key_byte) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b + 20u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a) ^ (salt_byte ^ 20u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 80u), 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 243u), 7u);
    }
  }

}

static void TwistCandidate_1266_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-2246)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-5232)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 8u + (2143)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 27u + static_cast<unsigned int>(28u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 4u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 84u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1266(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1266_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1266_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1266_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1266_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// Candidate 1301: TwistCandidate_1301
// family=mechanical_loops op_budget=3 loop_shapes=2x2/2x3/3x2
// mechanical_loops[l1=2x2; l2=2x3; l3=3x2; key_rot=31; twiddle=0x7e08eb3d]
static void TwistCandidate_1301_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-1498)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-3128)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 215u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1301_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (7280)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-1021)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 22u + static_cast<unsigned int>(215u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 196u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1301_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x7E08EB3Du ^ (pRound * 36u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-1498), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xCD4B3BEEu ^ static_cast<std::uint32_t>(8821u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1685), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3332), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 4u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 16u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 3u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 100u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 53u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x46BB7905u ^ static_cast<std::uint32_t>(8856u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3848), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 13u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2949), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 26u + (key_byte)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 4u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 25u + (twiddle_byte)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 29u), 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 70u), 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xDE06FD79u ^ static_cast<std::uint32_t>(11277u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5779), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 9u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1513), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3985), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ (c) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 28u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 25u ^ ((key_byte + 25u) & 0xFFu)) ^ (b) + ((feedback_byte + 28u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 25u) & 0xFFu) + ((feedback_byte + 28u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 218u), 3u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 61u, 4u);
    }
  }

}

static void TwistCandidate_1301_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (2685)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 28u + (-32)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 28u + (3881)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 15u + static_cast<unsigned int>(31u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(23u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 15u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 196u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1301(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1301_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1301_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1301_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1301_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

}  // namespace twist

namespace peanutbutter::expansion::key_expansion {

void ByteTwister::Get(unsigned char* pSource,
                      unsigned char* pWorker,
                      unsigned char* pDestination,
                      unsigned int pLength) {
  TwistBytes(mType, pSource, pWorker, pDestination, pLength);
}

void ByteTwister::SeedKey(Type pType,
                          unsigned char* pSource,
                          unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                          unsigned int pLength) {
  SeedKeyByIndex(static_cast<unsigned char>(pType), pSource, pKeyStack, pLength);
}

void ByteTwister::SeedKeyByIndex(unsigned char pType,
                                 unsigned char* pSource,
                                 unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                 unsigned int pLength) {
  if (pSource == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {
    return;
  }
  switch (pType % static_cast<unsigned char>(kTypeCount)) {
    case 0u: twist::TwistCandidate_0224_KeySeed(pSource, pKeyStack, pLength); break;
    case 1u: twist::TwistCandidate_0340_KeySeed(pSource, pKeyStack, pLength); break;
    case 2u: twist::TwistCandidate_0420_KeySeed(pSource, pKeyStack, pLength); break;
    case 3u: twist::TwistCandidate_0631_KeySeed(pSource, pKeyStack, pLength); break;
    case 4u: twist::TwistCandidate_0682_KeySeed(pSource, pKeyStack, pLength); break;
    case 5u: twist::TwistCandidate_0737_KeySeed(pSource, pKeyStack, pLength); break;
    case 6u: twist::TwistCandidate_0748_KeySeed(pSource, pKeyStack, pLength); break;
    case 7u: twist::TwistCandidate_0750_KeySeed(pSource, pKeyStack, pLength); break;
    case 8u: twist::TwistCandidate_0786_KeySeed(pSource, pKeyStack, pLength); break;
    case 9u: twist::TwistCandidate_0802_KeySeed(pSource, pKeyStack, pLength); break;
    case 10u: twist::TwistCandidate_0872_KeySeed(pSource, pKeyStack, pLength); break;
    case 11u: twist::TwistCandidate_1093_KeySeed(pSource, pKeyStack, pLength); break;
    case 12u: twist::TwistCandidate_1170_KeySeed(pSource, pKeyStack, pLength); break;
    case 13u: twist::TwistCandidate_1200_KeySeed(pSource, pKeyStack, pLength); break;
    case 14u: twist::TwistCandidate_1266_KeySeed(pSource, pKeyStack, pLength); break;
    case 15u: twist::TwistCandidate_1301_KeySeed(pSource, pKeyStack, pLength); break;
    default: std::abort();
  }
}

void ByteTwister::SeedSalt(Type pType,
                           unsigned char* pSource,
                           unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                           unsigned int pLength) {
  SeedSaltByIndex(static_cast<unsigned char>(pType), pSource, pSaltBuffer, pLength);
}

void ByteTwister::SeedSaltByIndex(unsigned char pType,
                                  unsigned char* pSource,
                                  unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                  unsigned int pLength) {
  if (pSource == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {
    return;
  }
  switch (pType % static_cast<unsigned char>(kTypeCount)) {
    case 0u: twist::TwistCandidate_0224_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 1u: twist::TwistCandidate_0340_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 2u: twist::TwistCandidate_0420_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 3u: twist::TwistCandidate_0631_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 4u: twist::TwistCandidate_0682_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 5u: twist::TwistCandidate_0737_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 6u: twist::TwistCandidate_0748_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 7u: twist::TwistCandidate_0750_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 8u: twist::TwistCandidate_0786_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 9u: twist::TwistCandidate_0802_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 10u: twist::TwistCandidate_0872_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 11u: twist::TwistCandidate_1093_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 12u: twist::TwistCandidate_1170_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 13u: twist::TwistCandidate_1200_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 14u: twist::TwistCandidate_1266_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 15u: twist::TwistCandidate_1301_SaltSeed(pSource, pSaltBuffer, pLength); break;
    default: std::abort();
  }
}

void ByteTwister::TwistBlock(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned int pRound,
                             const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                             unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                             unsigned int pLength) {
  TwistBlockByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength);
}

void ByteTwister::TwistBlockByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned int pRound,
                                    const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                    unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                    unsigned int pLength) {
  if (pSource == nullptr || pDestination == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {
    return;
  }
  switch (pType % static_cast<unsigned char>(kTypeCount)) {
    case 0u: twist::TwistCandidate_0224_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 1u: twist::TwistCandidate_0340_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 2u: twist::TwistCandidate_0420_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 3u: twist::TwistCandidate_0631_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 4u: twist::TwistCandidate_0682_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 5u: twist::TwistCandidate_0737_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 6u: twist::TwistCandidate_0748_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 7u: twist::TwistCandidate_0750_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 8u: twist::TwistCandidate_0786_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 9u: twist::TwistCandidate_0802_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 10u: twist::TwistCandidate_0872_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 11u: twist::TwistCandidate_1093_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 12u: twist::TwistCandidate_1170_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 13u: twist::TwistCandidate_1200_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 14u: twist::TwistCandidate_1266_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 15u: twist::TwistCandidate_1301_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    default: std::abort();
  }
}

void ByteTwister::PushKeyRound(Type pType,
                               unsigned char* pDestination,
                               const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                               unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                               unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                               unsigned int pLength) {
  PushKeyRoundByIndex(static_cast<unsigned char>(pType), pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength);
}

void ByteTwister::PushKeyRoundByIndex(unsigned char pType,
                                      unsigned char* pDestination,
                                      const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                      unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                      unsigned int pLength) {
  if (pDestination == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {
    return;
  }
  switch (pType % static_cast<unsigned char>(kTypeCount)) {
    case 0u: twist::TwistCandidate_0224_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 1u: twist::TwistCandidate_0340_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 2u: twist::TwistCandidate_0420_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 3u: twist::TwistCandidate_0631_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 4u: twist::TwistCandidate_0682_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 5u: twist::TwistCandidate_0737_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 6u: twist::TwistCandidate_0748_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 7u: twist::TwistCandidate_0750_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 8u: twist::TwistCandidate_0786_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 9u: twist::TwistCandidate_0802_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 10u: twist::TwistCandidate_0872_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 11u: twist::TwistCandidate_1093_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 12u: twist::TwistCandidate_1170_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 13u: twist::TwistCandidate_1200_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 14u: twist::TwistCandidate_1266_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 15u: twist::TwistCandidate_1301_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    default: std::abort();
  }
}

void ByteTwister::TwistBytes(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned int pLength) {
  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes]{};
  TwistBytes(pType, pSource, pWorker, pDestination, aKeyBuffer, aNextRoundKeyBuffer, pLength);
}

void ByteTwister::TwistBytes(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                             unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                             unsigned int pLength) {
  TwistBytesByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pKeyStack, pNextRoundKeyBuffer, pLength);
}

void ByteTwister::TwistBytesByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned int pLength) {
  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes]{};
  TwistBytesByIndex(pType, pSource, pWorker, pDestination, aKeyBuffer, aNextRoundKeyBuffer, pLength);
}

void ByteTwister::TwistBytesByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                    unsigned int pLength) {
  if (pSource == nullptr || pDestination == nullptr) {
    return;
  }
  constexpr unsigned int kBlockLength = static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE);
  if (pLength == 0U || (pLength % kBlockLength) != 0U) {
    std::abort();
  }

  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> aWorkspace{};
  unsigned char* aWorkspaceBuffer = (pWorker != nullptr) ? pWorker : aWorkspace.data();
  unsigned char aSaltBuffer[twist::kSaltBytes]{};

  SeedKeyByIndex(pType, pSource, pKeyStack, kBlockLength);
  SeedSaltByIndex(pType, pSource, aSaltBuffer, kBlockLength);
  for (unsigned int offset = 0U, round = 0U; offset < pLength; offset += kBlockLength, ++round) {
    unsigned char* aRoundSource = (round == 0U) ? pSource : (pDestination + offset - kBlockLength);
    unsigned char* aRoundDestination = pDestination + offset;
    TwistBlockByIndex(pType, aRoundSource, aWorkspaceBuffer, aRoundDestination, round, aSaltBuffer, pKeyStack, kBlockLength);
    PushKeyRoundByIndex(pType, aRoundDestination, aSaltBuffer, pKeyStack, pNextRoundKeyBuffer, kBlockLength);
  }
}

}  // namespace peanutbutter::expansion::key_expansion

#undef PASSWORD_EXPANDED_SIZE
