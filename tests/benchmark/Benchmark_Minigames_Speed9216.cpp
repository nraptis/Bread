#include <chrono>
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

void FillInput(std::vector<unsigned char>* pBytes, int pSalt) {
  const int aSeedSalt = bread::tests::config::ApplyGlobalSeed(pSalt);
  std::uint32_t aState = 0xA5A5A5A5U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x9E3779B9U);
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState = (aState * 1664525U) + 1013904223U;
    (*pBytes)[aIndex] = static_cast<unsigned char>((aState >> 24U) & 0xFFU);
  }
}

template <typename TGame>
bool RunSpeed(const char* pName,
              int pSeedLength,
              int pOutputLength,
              int pTrialsPerGame,
              std::uint64_t* pDigest,
              std::uint64_t* pOverflowCatastrophicTotal,
              std::uint64_t* pOverflowCataclysmicTotal,
              std::uint64_t* pOverflowApocalypseTotal,
              std::uint64_t* pWrapTotal,
              std::uint64_t* pDragonAttackTotal,
              std::uint64_t* pRiddlerAttackTotal) {
  auto aStart = std::chrono::steady_clock::now();

  std::uint64_t aOverflowCatastrophic = 0U;
  std::uint64_t aOverflowCataclysmic = 0U;
  std::uint64_t aOverflowApocalypse = 0U;
  std::uint64_t aWraps = 0U;
  std::uint64_t aDragonAttack = 0U;
  std::uint64_t aRiddlerAttack = 0U;

  for (int aTrial = 0; aTrial < pTrialsPerGame; ++aTrial) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pSeedLength), 0U);
    std::vector<unsigned char> aOutput(static_cast<std::size_t>(pOutputLength), 0U);
    FillInput(&aInput, aTrial + 17);
    MersenneCounter aCounter;
    TGame aGame(&aCounter);
    aGame.Seed(aInput.data(), pSeedLength);
    aGame.Get(aOutput.data(), pOutputLength);

    if (pDigest != nullptr) {
      for (unsigned char aByte : aOutput) {
        *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
      }
    }
    const bread::games::GameBoard::RuntimeStats aStats = aGame.GetRuntimeStats();
    aOverflowCatastrophic += aStats.mGameStateOverflowCatastrophic;
    aOverflowCataclysmic += aStats.mGameStateOverflowCataclysmic;
    aOverflowApocalypse += aStats.mGameStateOverflowApocalypse;
    aWraps += aStats.mPasswordExpanderWraps;
    aDragonAttack += aStats.mDragonAttack;
    aRiddlerAttack += aStats.mRiddlerAttack;
  }

  auto aEnd = std::chrono::steady_clock::now();
  const auto aMillis = std::chrono::duration_cast<std::chrono::milliseconds>(aEnd - aStart).count();
  if (pOverflowCatastrophicTotal != nullptr) {
    *pOverflowCatastrophicTotal += aOverflowCatastrophic;
  }
  if (pOverflowCataclysmicTotal != nullptr) {
    *pOverflowCataclysmicTotal += aOverflowCataclysmic;
  }
  if (pOverflowApocalypseTotal != nullptr) {
    *pOverflowApocalypseTotal += aOverflowApocalypse;
  }
  if (pWrapTotal != nullptr) {
    *pWrapTotal += aWraps;
  }
  if (pDragonAttackTotal != nullptr) {
    *pDragonAttackTotal += aDragonAttack;
  }
  if (pRiddlerAttackTotal != nullptr) {
    *pRiddlerAttackTotal += aRiddlerAttack;
  }
  std::cout << "[BENCH_EXPANDER] " << pName << " trials=" << pTrialsPerGame
            << " seed_bytes=" << pSeedLength << " out_bytes=" << pOutputLength
            << " elapsed_ms=" << aMillis
            << " overflow_catastrophic=" << aOverflowCatastrophic
            << " overflow_cataclysmic=" << aOverflowCataclysmic
            << " overflow_apocalypse=" << aOverflowApocalypse
            << " password_expander_wraps=" << aWraps
            << " dragon_attack=" << aDragonAttack
            << " riddler_attack=" << aRiddlerAttack << "\n";
  return true;
}

}  // namespace

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }

  const int aSeedLength =
      (bread::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::GAME_TEST_DATA_LENGTH;
  const int aTrialsPerGame =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aOutputLength = aSeedLength * 128;

  std::uint64_t aDigest = 2166136261ULL;
  std::uint64_t aOverflowCatastrophic = 0U;
  std::uint64_t aOverflowCataclysmic = 0U;
  std::uint64_t aOverflowApocalypse = 0U;
  std::uint64_t aWraps = 0U;
  std::uint64_t aDragonAttack = 0U;
  std::uint64_t aRiddlerAttack = 0U;

  if (!RunSpeed<bread::games::MatchThreeTapStreaks>(
          "MatchThreeTapStreaks", aSeedLength, aOutputLength, aTrialsPerGame, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunSpeed<bread::games::MatchThreeTapIslands>(
          "MatchThreeTapIslands", aSeedLength, aOutputLength, aTrialsPerGame, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunSpeed<bread::games::MatchThreeSwapStreaks>(
          "MatchThreeSwapStreaks", aSeedLength, aOutputLength, aTrialsPerGame, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunSpeed<bread::games::MatchThreeSwapIslands>(
          "MatchThreeSwapIslands", aSeedLength, aOutputLength, aTrialsPerGame, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunSpeed<bread::games::MatchThreeSlideStreaks>(
          "MatchThreeSlideStreaks", aSeedLength, aOutputLength, aTrialsPerGame, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunSpeed<bread::games::MatchThreeSlideIslands>(
          "MatchThreeSlideIslands", aSeedLength, aOutputLength, aTrialsPerGame, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack)) {
    return 1;
  }

  std::cout << "[PASS] minigames speed expander benchmark passed"
            << " loops=" << aTrialsPerGame << " seed_bytes=" << aSeedLength
            << " out_bytes=" << aOutputLength << " digest=" << aDigest
            << " overflow_catastrophic_total=" << aOverflowCatastrophic
            << " overflow_cataclysmic_total=" << aOverflowCataclysmic
            << " overflow_apocalypse_total=" << aOverflowApocalypse
            << " password_expander_wraps_total=" << aWraps
            << " dragon_attack_total=" << aDragonAttack
            << " riddler_attack_total=" << aRiddlerAttack << "\n";
  return 0;
}
