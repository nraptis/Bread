#ifndef BREAD_TESTS_COMMON_MAZEEDGECONNECTEDUTILS_HPP_
#define BREAD_TESTS_COMMON_MAZEEDGECONNECTEDUTILS_HPP_

#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "src/Tables/maze/Maze.hpp"
#include "tests/common/Tests.hpp"

namespace peanutbutter::tests::maze_quality {

inline std::uint32_t SeedState(int pSalt, std::uint32_t pMix) {
  const int aSeedSalt = peanutbutter::tests::config::ApplyGlobalSeed(pSalt);
  return pMix ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x9E3779B9U);
}

inline void FillInput(std::vector<unsigned char>* pBytes, int pTrial, std::uint32_t pMix) {
  std::uint32_t aState = SeedState(pTrial + static_cast<int>(pBytes->size()), pMix);
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

inline std::uint32_t NextWord(std::uint32_t* pState) {
  *pState ^= (*pState << 13U);
  *pState ^= (*pState >> 17U);
  *pState ^= (*pState << 5U);
  return *pState;
}

inline void CollectOpenCells(const peanutbutter::maze::Maze& pMaze, std::vector<std::pair<int, int>>* pOpenCells) {
  pOpenCells->clear();
  pOpenCells->reserve(static_cast<std::size_t>(peanutbutter::maze::Maze::kGridSize));
  for (int aY = 0; aY < peanutbutter::maze::Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < peanutbutter::maze::Maze::kGridWidth; ++aX) {
      if (!pMaze.IsWall(aX, aY)) {
        pOpenCells->emplace_back(aX, aY);
      }
    }
  }
}

inline bool VerifyQuickConnectivity(peanutbutter::maze::Maze* pMaze,
                                    const char* pLabel,
                                    int pLoop,
                                    std::uint32_t pPickMix,
                                    std::uint64_t* pDigest) {
  if (pMaze == nullptr || pLabel == nullptr || pDigest == nullptr) {
    return false;
  }

  std::vector<std::pair<int, int>> aOpenCells;
  CollectOpenCells(*pMaze, &aOpenCells);
  if (aOpenCells.size() < 2U) {
    std::cerr << "[FAIL] " << pLabel << " found too few open cells at loop " << pLoop
              << " open_cells=" << aOpenCells.size() << "\n";
    return false;
  }

  std::uint32_t aPickState = SeedState(pLoop + static_cast<int>(aOpenCells.size()), pPickMix);
  const std::size_t aStartIndex = static_cast<std::size_t>(NextWord(&aPickState)) % aOpenCells.size();
  std::size_t aEndIndex = static_cast<std::size_t>(NextWord(&aPickState)) % (aOpenCells.size() - 1U);
  if (aEndIndex >= aStartIndex) {
    ++aEndIndex;
  }

  const int aStartX = aOpenCells[aStartIndex].first;
  const int aStartY = aOpenCells[aStartIndex].second;
  const int aEndX = aOpenCells[aEndIndex].first;
  const int aEndY = aOpenCells[aEndIndex].second;

  if (!pMaze->FindPath(aStartX, aStartY, aEndX, aEndY)) {
    std::cerr << "[FAIL] " << pLabel << " path missing at loop " << pLoop
              << " from=(" << aStartX << "," << aStartY << ")"
              << " to=(" << aEndX << "," << aEndY << ")"
              << " open_cells=" << aOpenCells.size() << "\n";
    return false;
  }

  *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pMaze->PathLength());
  *pDigest = (*pDigest * 1099511628211ULL) ^
             static_cast<std::uint64_t>((aStartX & 0xFF) | ((aStartY & 0xFF) << 8) | ((aEndX & 0xFF) << 16) |
                                        ((aEndY & 0xFF) << 24));
  *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aOpenCells.size());
  return true;
}

inline bool VerifyExhaustiveConnectivity(peanutbutter::maze::Maze* pMaze,
                                         const char* pLabel,
                                         int pLoop,
                                         std::uint64_t* pPairChecks,
                                         std::uint64_t* pDigest) {
  if (pMaze == nullptr || pLabel == nullptr || pPairChecks == nullptr || pDigest == nullptr) {
    return false;
  }

  std::vector<std::pair<int, int>> aOpenCells;
  CollectOpenCells(*pMaze, &aOpenCells);
  if (aOpenCells.empty()) {
    std::cerr << "[FAIL] " << pLabel << " found no open cells at loop " << pLoop << "\n";
    return false;
  }

  for (std::size_t aFrom = 0; aFrom < aOpenCells.size(); ++aFrom) {
    for (std::size_t aTo = aFrom; aTo < aOpenCells.size(); ++aTo) {
      const int aStartX = aOpenCells[aFrom].first;
      const int aStartY = aOpenCells[aFrom].second;
      const int aEndX = aOpenCells[aTo].first;
      const int aEndY = aOpenCells[aTo].second;

      if (!pMaze->FindPath(aStartX, aStartY, aEndX, aEndY)) {
        std::cerr << "[FAIL] " << pLabel << " path missing at loop " << pLoop
                  << " from=(" << aStartX << "," << aStartY << ")"
                  << " to=(" << aEndX << "," << aEndY << ")"
                  << " open_cells=" << aOpenCells.size() << "\n";
        return false;
      }

      ++(*pPairChecks);
      *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pMaze->PathLength());
      *pDigest = (*pDigest * 1099511628211ULL) ^
                 static_cast<std::uint64_t>((aStartX & 0xFF) | ((aStartY & 0xFF) << 8) |
                                            ((aEndX & 0xFF) << 16) | ((aEndY & 0xFF) << 24));
    }
  }

  *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aOpenCells.size());
  return true;
}

}  // namespace peanutbutter::tests::maze_quality

#endif  // BREAD_TESTS_COMMON_MAZEEDGECONNECTEDUTILS_HPP_
