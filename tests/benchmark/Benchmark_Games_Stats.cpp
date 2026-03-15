#include <cstdint>
#include <iostream>
#include <vector>

#include "src/counters/MersenneCounter.hpp"
#include "src/games/engine/GameBoard.hpp"
#include "src/games/minigames/MatchThreeSlideIslands.hpp"
#include "src/games/minigames/MatchThreeSlideStreaks.hpp"
#include "src/games/minigames/MatchThreeSwapIslands.hpp"
#include "src/games/minigames/MatchThreeSwapStreaks.hpp"
#include "src/games/minigames/MatchThreeTapIslands.hpp"
#include "src/games/minigames/MatchThreeTapStreaks.hpp"
#include "tests/common/Tests.hpp"

namespace {

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = bread::tests::config::ApplyGlobalSeed(pSalt);
  return 0x9E3779B9U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x85EBCA6BU);
}

void FillInput(std::vector<unsigned char>* pBytes, int pSalt) {
  std::uint32_t aState = SeedState(pSalt + static_cast<int>(pBytes->size()));
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

template <typename TGame>
bool RunOne(const char* pName, int pInputLength, int pSalt, std::uint64_t* pDigest) {
  std::vector<unsigned char> aInput(static_cast<std::size_t>(pInputLength), 0U);
  std::vector<unsigned char> aOutput(static_cast<std::size_t>(pInputLength), 0U);
  FillInput(&aInput, pSalt);
  MersenneCounter aCounter;
  TGame aGame(&aCounter);
  aGame.Seed(aInput.data(), pInputLength);
  aGame.Get(aOutput.data(), pInputLength);

  if (pDigest != nullptr) {
    for (unsigned char aByte : aOutput) {
      *pDigest = (*pDigest * 1315423911ULL) ^ static_cast<std::uint64_t>(aByte);
    }
  }

  const bread::games::GameBoard::RuntimeStats aStats = aGame.GetRuntimeStats();
  std::cout << "[STAT] " << pName << " runs=1 bytes=" << pInputLength
            << " stuck=" << aStats.mStuck
            << " topple=" << aStats.mTopple
            << " user_match=" << aStats.mUserMatch
            << " cascade_match=" << aStats.mCascadeMatch
            << " game_state_overflow_catastrophic=" << aStats.mGameStateOverflowCatastrophic
            << " game_state_overflow_cataclysmic=" << aStats.mGameStateOverflowCataclysmic
            << " game_state_overflow_apocalypse=" << aStats.mGameStateOverflowApocalypse
            << " apocalypse_event=" << ((aStats.mGameStateOverflowApocalypse > 0U) ? 1 : 0)
            << " powerup_spawned=" << aStats.mPowerUpSpawned
            << " powerup_consumed=" << aStats.mPowerUpConsumed
            << " password_expander_wraps=" << aStats.mPasswordExpanderWraps
            << " dragon_attack=" << aStats.mDragonAttack
            << " riddler_attack=" << aStats.mRiddlerAttack << "\n";
  return true;
}

}  // namespace

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }

  const int aInputLength =
      (bread::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::GAME_TEST_DATA_LENGTH;
  std::uint64_t aDigest = 1469598103934665603ULL;

  if (!RunOne<bread::games::MatchThreeTapStreaks>("MatchThreeTapStreaks", aInputLength, 11, &aDigest) ||
      !RunOne<bread::games::MatchThreeTapIslands>("MatchThreeTapIslands", aInputLength, 23, &aDigest) ||
      !RunOne<bread::games::MatchThreeSwapStreaks>("MatchThreeSwapStreaks", aInputLength, 37, &aDigest) ||
      !RunOne<bread::games::MatchThreeSwapIslands>("MatchThreeSwapIslands", aInputLength, 41, &aDigest) ||
      !RunOne<bread::games::MatchThreeSlideStreaks>("MatchThreeSlideStreaks", aInputLength, 53, &aDigest) ||
      !RunOne<bread::games::MatchThreeSlideIslands>("MatchThreeSlideIslands", aInputLength, 67, &aDigest)) {
    return 1;
  }

  std::cout << "[PASS] games stats benchmark passed"
            << " runs_per_game=1"
            << " bytes=" << aInputLength
            << " digest=" << aDigest << "\n";
  return 0;
}
