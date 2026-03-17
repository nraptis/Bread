#ifndef BREAD_TESTS_COMMON_GAME_CHUNK_RUNNER_HPP_
#define BREAD_TESTS_COMMON_GAME_CHUNK_RUNNER_HPP_

#include <cstdint>
#include <cstring>

#include "src/Tables/games/engine/GameBoard.hpp"
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
  pDestination->mGameStateOverflowCataclysmic += pSource.mGameStateOverflowCataclysmic;
  pDestination->mGameStateOverflowApocalypse += pSource.mGameStateOverflowApocalypse;
  pDestination->mPowerUpSpawned += pSource.mPowerUpSpawned;
  pDestination->mPowerUpConsumed += pSource.mPowerUpConsumed;
  pDestination->mPasswordExpanderWraps += pSource.mPasswordExpanderWraps;
  pDestination->mDragonAttack += pSource.mDragonAttack;
  pDestination->mRiddlerAttack += pSource.mRiddlerAttack;
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
