#ifndef BREAD_TESTS_COMMON_GAME_CHUNK_RUNNER_HPP_
#define BREAD_TESTS_COMMON_GAME_CHUNK_RUNNER_HPP_

#include <cstdint>
#include <cstring>

#include "src/Tables/games/GameBoard.hpp"
#include "tests/common/CounterSeedBuffer.hpp"

namespace peanutbutter::tests::games {

struct GameRunSummary {
  int mSuccessfulMoveCount = 0;
  int mBrokenCount = 0;
  peanutbutter::games::GameBoard::RuntimeStats mRuntimeStats{};
};

inline bool IsValidGameByteLength(int pByteLength) {
  return pByteLength > 0 && (pByteLength % peanutbutter::games::GameBoard::kSeedBufferCapacity) == 0;
}

inline void AccumulateRuntimeStats(const peanutbutter::games::GameBoard::RuntimeStats& pSource,
                                   peanutbutter::games::GameBoard::RuntimeStats* pDestination) {
  if (pDestination == nullptr) {
    return;
  }

  pDestination->mStuck += pSource.mStuck;
  pDestination->mTopple += pSource.mTopple;
  pDestination->mUserMatch += pSource.mUserMatch;
  pDestination->mCascadeMatch += pSource.mCascadeMatch;
  pDestination->mGameStateOverflowCatastrophic += pSource.mGameStateOverflowCatastrophic;
  pDestination->mCatastropheCaseA += pSource.mCatastropheCaseA;
  pDestination->mCatastropheCaseB += pSource.mCatastropheCaseB;
  pDestination->mCatastropheCaseC += pSource.mCatastropheCaseC;
  pDestination->mGameStateOverflowCataclysmic += pSource.mGameStateOverflowCataclysmic;
  pDestination->mGameStateOverflowApocalypse += pSource.mGameStateOverflowApocalypse;
  pDestination->mPowerUpSpawned += pSource.mPowerUpSpawned;
  pDestination->mPowerUpConsumed += pSource.mPowerUpConsumed;
  pDestination->mPasswordExpanderWraps += pSource.mPasswordExpanderWraps;
  pDestination->mDragonAttack += pSource.mDragonAttack;
  pDestination->mPhoenixAttack += pSource.mPhoenixAttack;
  pDestination->mWyvernAttack += pSource.mWyvernAttack;
  pDestination->mBlueMoonCase += pSource.mBlueMoonCase;
  pDestination->mHarvestMoon += pSource.mHarvestMoon;
  pDestination->mImpossible += pSource.mImpossible;
  pDestination->mPlantedMatchSolve += pSource.mPlantedMatchSolve;
  pDestination->mInconsistentStateA += pSource.mInconsistentStateA;
  pDestination->mInconsistentStateB += pSource.mInconsistentStateB;
  pDestination->mInconsistentStateC += pSource.mInconsistentStateC;
  pDestination->mInconsistentStateD += pSource.mInconsistentStateD;
  pDestination->mInconsistentStateE += pSource.mInconsistentStateE;
  pDestination->mInconsistentStateF += pSource.mInconsistentStateF;
  pDestination->mInconsistentStateG += pSource.mInconsistentStateG;
  pDestination->mInconsistentStateH += pSource.mInconsistentStateH;
  pDestination->mInconsistentStateI += pSource.mInconsistentStateI;
  pDestination->mInconsistentStateJ += pSource.mInconsistentStateJ;
  pDestination->mInconsistentStateK += pSource.mInconsistentStateK;
  pDestination->mInconsistentStateL += pSource.mInconsistentStateL;
  pDestination->mInconsistentStateM += pSource.mInconsistentStateM;
  pDestination->mInconsistentStateN += pSource.mInconsistentStateN;
  pDestination->mInconsistentStateO += pSource.mInconsistentStateO;
}

inline std::uint32_t RuntimeFlags(const peanutbutter::games::GameBoard::RuntimeStats& pStats) {
  std::uint32_t aFlags = 0U;
  if (pStats.mStuck > 0U) aFlags |= (1U << 0U);
  if (pStats.mTopple > 0U) aFlags |= (1U << 1U);
  if (pStats.mUserMatch > 0U) aFlags |= (1U << 2U);
  if (pStats.mCascadeMatch > 0U) aFlags |= (1U << 3U);
  if (pStats.mGameStateOverflowCatastrophic > 0U) aFlags |= (1U << 4U);
  if (pStats.mCatastropheCaseA > 0U) aFlags |= (1U << 5U);
  if (pStats.mCatastropheCaseB > 0U) aFlags |= (1U << 6U);
  if (pStats.mCatastropheCaseC > 0U) aFlags |= (1U << 7U);
  if (pStats.mGameStateOverflowCataclysmic > 0U) aFlags |= (1U << 8U);
  if (pStats.mGameStateOverflowApocalypse > 0U) aFlags |= (1U << 9U);
  if (pStats.mPowerUpSpawned > 0U) aFlags |= (1U << 10U);
  if (pStats.mPowerUpConsumed > 0U) aFlags |= (1U << 11U);
  if (pStats.mPasswordExpanderWraps > 0U) aFlags |= (1U << 12U);
  if (pStats.mDragonAttack > 0U) aFlags |= (1U << 13U);
  if (pStats.mPhoenixAttack > 0U) aFlags |= (1U << 14U);
  if (pStats.mWyvernAttack > 0U) aFlags |= (1U << 15U);
  if (pStats.mBlueMoonCase > 0U) aFlags |= (1U << 16U);
  if (pStats.mHarvestMoon > 0U) aFlags |= (1U << 17U);
  if (pStats.mImpossible > 0U) aFlags |= (1U << 18U);
  if (pStats.mPlantedMatchSolve > 0U) aFlags |= (1U << 19U);
  return aFlags;
}

inline std::uint32_t InconsistentFlags(const peanutbutter::games::GameBoard::RuntimeStats& pStats) {
  std::uint32_t aFlags = 0U;
  if (pStats.mInconsistentStateA > 0U) aFlags |= (1U << 0U);
  if (pStats.mInconsistentStateB > 0U) aFlags |= (1U << 1U);
  if (pStats.mInconsistentStateC > 0U) aFlags |= (1U << 2U);
  if (pStats.mInconsistentStateD > 0U) aFlags |= (1U << 3U);
  if (pStats.mInconsistentStateE > 0U) aFlags |= (1U << 4U);
  if (pStats.mInconsistentStateF > 0U) aFlags |= (1U << 5U);
  if (pStats.mInconsistentStateG > 0U) aFlags |= (1U << 6U);
  if (pStats.mInconsistentStateH > 0U) aFlags |= (1U << 7U);
  if (pStats.mInconsistentStateI > 0U) aFlags |= (1U << 8U);
  if (pStats.mInconsistentStateJ > 0U) aFlags |= (1U << 9U);
  if (pStats.mInconsistentStateK > 0U) aFlags |= (1U << 10U);
  if (pStats.mInconsistentStateL > 0U) aFlags |= (1U << 11U);
  if (pStats.mInconsistentStateM > 0U) aFlags |= (1U << 12U);
  if (pStats.mInconsistentStateN > 0U) aFlags |= (1U << 13U);
  if (pStats.mInconsistentStateO > 0U) aFlags |= (1U << 14U);
  return aFlags;
}

inline const char* GetGameName(peanutbutter::games::GameBoard::GameIndex pGameIndex) {
  peanutbutter::games::GameBoard aGame;
  aGame.SetGame(pGameIndex);
  return aGame.GetName();
}

inline bool RunGameChunks(peanutbutter::games::GameBoard::GameIndex pGameIndex,
                          unsigned char* pSeedBytes,
                          int pTotalByteLength,
                          unsigned char* pOutputBytes,
                          GameRunSummary* pSummary) {
  if (!IsValidGameByteLength(pTotalByteLength) || pSeedBytes == nullptr || pOutputBytes == nullptr) {
    return false;
  }

  if (pSummary != nullptr) {
    *pSummary = GameRunSummary{};
  }

  for (int aOffset = 0; aOffset < pTotalByteLength; aOffset += peanutbutter::games::GameBoard::kSeedBufferCapacity) {
    peanutbutter::games::GameBoard aGame;
    aGame.SetGame(pGameIndex);
    aGame.Seed(pSeedBytes + aOffset, peanutbutter::games::GameBoard::kSeedBufferCapacity);
    aGame.Get(pOutputBytes + aOffset, peanutbutter::games::GameBoard::kSeedBufferCapacity);

    if (pSummary != nullptr) {
      pSummary->mSuccessfulMoveCount += aGame.SuccessfulMoveCount();
      pSummary->mBrokenCount += aGame.BrokenCount();
      AccumulateRuntimeStats(aGame.GetRuntimeStats(), &pSummary->mRuntimeStats);
    }
  }

  return true;
}

template <typename TCounter>
inline bool RunGameChunksFromInput(peanutbutter::games::GameBoard::GameIndex pGameIndex,
                                   const unsigned char* pInputBytes,
                                   int pTotalByteLength,
                                   unsigned char* pOutputBytes,
                                   GameRunSummary* pSummary,
                                   unsigned char* pExpandedSeedBytes = nullptr) {
  if (!IsValidGameByteLength(pTotalByteLength) || pInputBytes == nullptr || pOutputBytes == nullptr) {
    return false;
  }

  if (pSummary != nullptr) {
    *pSummary = GameRunSummary{};
  }

  for (int aOffset = 0; aOffset < pTotalByteLength; aOffset += peanutbutter::games::GameBoard::kSeedBufferCapacity) {
    std::vector<unsigned char> aSeedChunk = peanutbutter::tests::BuildCounterSeedBuffer<TCounter>(
        pInputBytes + aOffset,
        peanutbutter::games::GameBoard::kSeedBufferCapacity,
        peanutbutter::games::GameBoard::kSeedBufferCapacity);
    if (static_cast<int>(aSeedChunk.size()) != peanutbutter::games::GameBoard::kSeedBufferCapacity) {
      return false;
    }

    if (pExpandedSeedBytes != nullptr) {
      std::memcpy(pExpandedSeedBytes + aOffset,
                  aSeedChunk.data(),
                  static_cast<std::size_t>(peanutbutter::games::GameBoard::kSeedBufferCapacity));
    }

    peanutbutter::games::GameBoard aGame;
    aGame.SetGame(pGameIndex);
    aGame.Seed(aSeedChunk.data(), peanutbutter::games::GameBoard::kSeedBufferCapacity);
    aGame.Get(pOutputBytes + aOffset, peanutbutter::games::GameBoard::kSeedBufferCapacity);

    if (pSummary != nullptr) {
      pSummary->mSuccessfulMoveCount += aGame.SuccessfulMoveCount();
      pSummary->mBrokenCount += aGame.BrokenCount();
      AccumulateRuntimeStats(aGame.GetRuntimeStats(), &pSummary->mRuntimeStats);
    }
  }

  return true;
}

}  // namespace peanutbutter::tests::games

#endif  // BREAD_TESTS_COMMON_GAME_CHUNK_RUNNER_HPP_
