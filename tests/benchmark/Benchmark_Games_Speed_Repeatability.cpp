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

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = bread::tests::config::ApplyGlobalSeed(pSalt);
  return 0x9E3779B9U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x85EBCA6BU);
}

void FillInput(std::vector<unsigned char>* pBytes, int pTrial) {
  std::uint32_t aState = SeedState(pTrial + static_cast<int>(pBytes->size()));
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

template <typename TGame>
bool RunOne(const char* pName,
            int pInputLength,
            int pTrialsPerGame,
            int* pMoveTotal,
            int* pBrokenTotal,
            std::uint64_t* pDigest,
            std::uint64_t* pOverflowCatastrophicTotal,
            std::uint64_t* pOverflowCataclysmicTotal,
            std::uint64_t* pOverflowApocalypseTotal,
            std::uint64_t* pWrapTotal,
            std::uint64_t* pDragonAttackTotal,
            std::uint64_t* pRiddlerAttackTotal) {
  std::cout << "[RUN] " << pName
            << " trials=" << pTrialsPerGame
            << " bytes=" << pInputLength << std::endl;

  auto aStart = std::chrono::steady_clock::now();

  std::uint64_t aOverflowCatastrophic = 0U;
  std::uint64_t aOverflowCataclysmic = 0U;
  std::uint64_t aOverflowApocalypse = 0U;
  std::uint64_t aWraps = 0U;
  std::uint64_t aDragonAttack = 0U;
  std::uint64_t aRiddlerAttack = 0U;

  for (int aTrial = 0; aTrial < pTrialsPerGame; ++aTrial) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pInputLength), 0U);
    std::vector<unsigned char> aOutputA(static_cast<std::size_t>(pInputLength), 0U);
    std::vector<unsigned char> aOutputB(static_cast<std::size_t>(pInputLength), 0U);
    FillInput(&aInput, aTrial);
    MersenneCounter aCounterA;
    TGame aGameA(&aCounterA);
    aGameA.Seed(aInput.data(), pInputLength);
    aGameA.Get(aOutputA.data(), pInputLength);
    MersenneCounter aCounterB;
    TGame aGameB(&aCounterB);
    aGameB.Seed(aInput.data(), pInputLength);
    aGameB.Get(aOutputB.data(), pInputLength);

    if (aOutputA != aOutputB) {
      std::cerr << "[FAIL] repeatability mismatch in " << pName << ", trial=" << aTrial << "\n";
      return false;
    }

    if (pMoveTotal != nullptr) {
      *pMoveTotal += aGameA.SuccessfulMoveCount();
    }
    if (pBrokenTotal != nullptr) {
      *pBrokenTotal += aGameA.BrokenCount();
    }
    if (pDigest != nullptr) {
      for (unsigned char aByte : aOutputA) {
        *pDigest = (*pDigest * 1315423911ULL) ^ static_cast<std::uint64_t>(aByte);
      }
    }
    const bread::games::GameBoard::RuntimeStats aStats = aGameA.GetRuntimeStats();
    aOverflowCatastrophic += aStats.mGameStateOverflowCatastrophic;
    aOverflowCataclysmic += aStats.mGameStateOverflowCataclysmic;
    aOverflowApocalypse += aStats.mGameStateOverflowApocalypse;
    aWraps += aStats.mPasswordExpanderWraps;
    aDragonAttack += aStats.mDragonAttack;
    aRiddlerAttack += aStats.mRiddlerAttack;
  }

  auto aEnd = std::chrono::steady_clock::now();
  const auto aMicros = std::chrono::duration_cast<std::chrono::microseconds>(aEnd - aStart).count();
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
  std::cout << "[BENCH] " << pName << " trials=" << pTrialsPerGame
            << " bytes=" << pInputLength << " elapsed_us=" << aMicros
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

  const int aInputLength =
      (bread::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::GAME_TEST_DATA_LENGTH;
  const int aTrialsPerGame =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;

  std::cout << "[INFO] benchmark_games_speed_repeatability"
            << " loops=" << aTrialsPerGame
            << " data_length=" << aInputLength
            << " benchmark_enabled=" << (bread::tests::config::BENCHMARK_ENABLED ? 1 : 0)
            << " repeatability_enabled=" << (bread::tests::config::REPEATABILITY_ENABLED ? 1 : 0)
            << std::endl;

  int aMoves = 0;
  int aBroken = 0;
  std::uint64_t aDigest = 1469598103934665603ULL;
  std::uint64_t aOverflowCatastrophic = 0U;
  std::uint64_t aOverflowCataclysmic = 0U;
  std::uint64_t aOverflowApocalypse = 0U;
  std::uint64_t aWraps = 0U;
  std::uint64_t aDragonAttack = 0U;
  std::uint64_t aRiddlerAttack = 0U;

  if (!RunOne<bread::games::MatchThreeTapStreaks>(
          "MatchThreeTapStreaks", aInputLength, aTrialsPerGame, &aMoves, &aBroken, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunOne<bread::games::MatchThreeTapIslands>(
          "MatchThreeTapIslands", aInputLength, aTrialsPerGame, &aMoves, &aBroken, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunOne<bread::games::MatchThreeSwapStreaks>(
          "MatchThreeSwapStreaks", aInputLength, aTrialsPerGame, &aMoves, &aBroken, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunOne<bread::games::MatchThreeSwapIslands>(
          "MatchThreeSwapIslands", aInputLength, aTrialsPerGame, &aMoves, &aBroken, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunOne<bread::games::MatchThreeSlideStreaks>(
          "MatchThreeSlideStreaks", aInputLength, aTrialsPerGame, &aMoves, &aBroken, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack) ||
      !RunOne<bread::games::MatchThreeSlideIslands>(
          "MatchThreeSlideIslands", aInputLength, aTrialsPerGame, &aMoves, &aBroken, &aDigest, &aOverflowCatastrophic,
          &aOverflowCataclysmic, &aOverflowApocalypse, &aWraps, &aDragonAttack, &aRiddlerAttack)) {
    return 1;
  }

  std::cout << "[PASS] games speed/repeatability benchmark passed"
            << " loops=" << aTrialsPerGame << " bytes=" << aInputLength << " moves=" << aMoves
            << " broken=" << aBroken << " digest=" << aDigest
            << " overflow_catastrophic_total=" << aOverflowCatastrophic
            << " overflow_cataclysmic_total=" << aOverflowCataclysmic
            << " overflow_apocalypse_total=" << aOverflowApocalypse
            << " password_expander_wraps_total=" << aWraps
            << " dragon_attack_total=" << aDragonAttack
            << " riddler_attack_total=" << aRiddlerAttack << "\n";
  return 0;
}
