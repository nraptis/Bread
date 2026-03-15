#include "src/games/engine/GamePlayDirector.hpp"

#include "src/games/engine/GameBoard.hpp"

namespace bread::games {

namespace {
constexpr int kGuaranteePhaseIterations = 256;
constexpr int kGuaranteeRestartIterations = 32;

GamePlayDirector_CollapseStyle gCollapseStyleDirector;
GamePlayDirector_BejeweledStyle gBejeweledStyleDirector;
}  // namespace

GamePlayDirector* GamePlayDirector::CollapseStyle() {
  return &gCollapseStyleDirector;
}

GamePlayDirector* GamePlayDirector::BejeweledStyle() {
  return &gBejeweledStyleDirector;
}

bool GamePlayDirector::BuildExploreList_AllTiles(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }

  pBoard->mExploreListCount = 0;
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      const GameTile* aTile = pBoard->mGrid[aX][aY];
      if (aTile == nullptr) {
        continue;
      }
      pBoard->mExploreListX[pBoard->mExploreListCount] = aX;
      pBoard->mExploreListY[pBoard->mExploreListCount] = aY;
      ++pBoard->mExploreListCount;
    }
  }
  pBoard->ShuffleXY(pBoard->mExploreListX, pBoard->mExploreListY, pBoard->mExploreListCount);
  return pBoard->mExploreListCount > 0;
}

bool GamePlayDirector::BuildExploreList_NewTiles(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }

  pBoard->mExploreListCount = 0;
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      const GameTile* aTile = pBoard->mGrid[aX][aY];
      if (aTile == nullptr || !aTile->mIsNew) {
        continue;
      }
      pBoard->mExploreListX[pBoard->mExploreListCount] = aX;
      pBoard->mExploreListY[pBoard->mExploreListCount] = aY;
      ++pBoard->mExploreListCount;
    }
  }
  pBoard->ShuffleXY(pBoard->mExploreListX, pBoard->mExploreListY, pBoard->mExploreListCount);
  return pBoard->mExploreListCount > 0;
}

bool GamePlayDirector::BuildExploreList_MatchedTiles(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }
  if (pBoard->GetMatches() <= 0) {
    pBoard->mExploreListCount = 0;
    return false;
  }

  pBoard->mExploreListCount = 0;
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      const GameTile* aTile = pBoard->mGrid[aX][aY];
      if (aTile == nullptr || !aTile->mIsMatched) {
        continue;
      }
      pBoard->mExploreListX[pBoard->mExploreListCount] = aX;
      pBoard->mExploreListY[pBoard->mExploreListCount] = aY;
      ++pBoard->mExploreListCount;
    }
  }
  pBoard->ShuffleXY(pBoard->mExploreListX, pBoard->mExploreListY, pBoard->mExploreListCount);
  return pBoard->mExploreListCount > 0;
}

void GamePlayDirector::RandomFlipOneTileType(GameBoard* pBoard, int pX, int pY) {
  if (pBoard == nullptr || pX < 0 || pX >= GameBoard::kGridWidth || pY < 0 || pY >= GameBoard::kGridHeight) {
    return;
  }
  GameTile* aTile = pBoard->mGrid[pX][pY];
  if (aTile == nullptr || GameBoard::kTypeCount <= 1) {
    return;
  }

  const unsigned char aOriginal = aTile->mType;
  const int aOffset = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(GameBoard::kTypeCount - 1)) + 1;
  aTile->mType = static_cast<unsigned char>((static_cast<int>(aOriginal) + aOffset) % GameBoard::kTypeCount);
}

void GamePlayDirector::RandomizeAllTileTypes(GameBoard* pBoard) {
  if (pBoard == nullptr || GameBoard::kTypeCount <= 0) {
    return;
  }
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      GameTile* aTile = pBoard->mGrid[aX][aY];
      if (aTile == nullptr) {
        continue;
      }
      aTile->mType = static_cast<unsigned char>(pBoard->NextTypeByte() % static_cast<unsigned char>(GameBoard::kTypeCount));
    }
  }
}

bool GamePlayDirector::GuaranteeMatchDoesNotExist(GameBoard* pBoard) {
  if (pBoard == nullptr || GameBoard::kTypeCount <= 1) {
    return false;
  }

  for (int aRestart = 0; aRestart < kGuaranteeRestartIterations; ++aRestart) {
    if (!pBoard->HasAnyMatches()) {
      return true;
    }

    bool aPhase1Done = false;
    for (int aIter = 0; aIter < kGuaranteePhaseIterations; ++aIter) {
      if (!BuildExploreList_MatchedTiles(pBoard) || pBoard->mExploreListCount <= 0) {
        aPhase1Done = true;
        break;
      }
      const int aPick = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(pBoard->mExploreListCount));
      RandomFlipOneTileType(pBoard, pBoard->mExploreListX[aPick], pBoard->mExploreListY[aPick]);
      if (!pBoard->HasAnyMatches()) {
        aPhase1Done = true;
        break;
      }
    }
    if (aPhase1Done && !pBoard->HasAnyMatches()) {
      return true;
    }
    pBoard->RecordGameStateOverflowCatastrophic();

    for (int aIter = 0; aIter < kGuaranteePhaseIterations; ++aIter) {
      if (!BuildExploreList_AllTiles(pBoard) || pBoard->mExploreListCount <= 0) {
        break;
      }
      const int aPick = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(pBoard->mExploreListCount));
      RandomFlipOneTileType(pBoard, pBoard->mExploreListX[aPick], pBoard->mExploreListY[aPick]);
      if (!pBoard->HasAnyMatches()) {
        return true;
      }
    }
    pBoard->RecordGameStateOverflowCatastrophic();
    RandomizeAllTileTypes(pBoard);
  }

  return !pBoard->HasAnyMatches();
}

bool GamePlayDirector::GuaranteeMatchDoesExist(GameBoard* pBoard) {
  if (pBoard == nullptr || GameBoard::kTypeCount <= 0) {
    return false;
  }

  for (int aRestart = 0; aRestart < kGuaranteeRestartIterations; ++aRestart) {
    if (pBoard->HasAnyMatches()) {
      return true;
    }

    for (int aIter = 0; aIter < kGuaranteePhaseIterations; ++aIter) {
      if (!BuildExploreList_AllTiles(pBoard) || pBoard->mExploreListCount <= 0) {
        break;
      }
      const int aPick = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(pBoard->mExploreListCount));
      RandomFlipOneTileType(pBoard, pBoard->mExploreListX[aPick], pBoard->mExploreListY[aPick]);
      if (pBoard->HasAnyMatches()) {
        return true;
      }
    }
    pBoard->RecordGameStateOverflowCatastrophic();

    for (int aIter = 0; aIter < kGuaranteePhaseIterations; ++aIter) {
      if (!BuildExploreList_AllTiles(pBoard) || pBoard->mExploreListCount <= 0) {
        break;
      }
      const int aPick = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(pBoard->mExploreListCount));
      RandomFlipOneTileType(pBoard, pBoard->mExploreListX[aPick], pBoard->mExploreListY[aPick]);
      if (pBoard->HasAnyMatches()) {
        return true;
      }
    }
    pBoard->RecordGameStateOverflowCatastrophic();
    RandomizeAllTileTypes(pBoard);
  }

  return pBoard->HasAnyMatches();
}

bool GamePlayDirector::GuaranteeMoveExistsForNewTiles_MatchDoesExist(GameBoard* pBoard) {
  if (pBoard == nullptr || GameBoard::kTypeCount <= 0) {
    return false;
  }
  if (pBoard->ActiveTileCount() < GameBoard::kGridSize) {
    return pBoard->HasAnyMatches();
  }

  for (int aRestart = 0; aRestart < kGuaranteeRestartIterations; ++aRestart) {
    if (pBoard->HasAnyMatches()) {
      return true;
    }

    for (int aIter = 0; aIter < kGuaranteePhaseIterations; ++aIter) {
      if (!BuildExploreList_NewTiles(pBoard) || pBoard->mExploreListCount <= 0) {
        break;
      }
      const int aPick = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(pBoard->mExploreListCount));
      RandomFlipOneTileType(pBoard, pBoard->mExploreListX[aPick], pBoard->mExploreListY[aPick]);
      if (pBoard->HasAnyMatches()) {
        return true;
      }
    }
    pBoard->RecordGameStateOverflowCatastrophic();

    for (int aIter = 0; aIter < kGuaranteePhaseIterations; ++aIter) {
      if (!BuildExploreList_AllTiles(pBoard) || pBoard->mExploreListCount <= 0) {
        break;
      }
      const int aPick = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(pBoard->mExploreListCount));
      RandomFlipOneTileType(pBoard, pBoard->mExploreListX[aPick], pBoard->mExploreListY[aPick]);
      if (pBoard->HasAnyMatches()) {
        return true;
      }
    }
    pBoard->RecordGameStateOverflowCatastrophic();
    RandomizeAllTileTypes(pBoard);
  }

  return pBoard->HasAnyMatches();
}

bool GamePlayDirector::GuaranteeMoveExistsForNewTiles_MatchDoesNotExist(GameBoard* pBoard) {
  if (pBoard == nullptr || GameBoard::kTypeCount <= 1) {
    return false;
  }

  for (int aRestart = 0; aRestart < kGuaranteeRestartIterations; ++aRestart) {
    if (!pBoard->HasAnyMatches() && pBoard->HasAnyLegalMove()) {
      return true;
    }

    for (int aIter = 0; aIter < kGuaranteePhaseIterations; ++aIter) {
      if (!BuildExploreList_NewTiles(pBoard) || pBoard->mExploreListCount <= 0) {
        break;
      }
      const int aPick = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(pBoard->mExploreListCount));
      RandomFlipOneTileType(pBoard, pBoard->mExploreListX[aPick], pBoard->mExploreListY[aPick]);
      if (!pBoard->HasAnyMatches() && pBoard->HasAnyLegalMove()) {
        return true;
      }
    }
    pBoard->RecordGameStateOverflowCatastrophic();

    for (int aIter = 0; aIter < kGuaranteePhaseIterations; ++aIter) {
      if (!BuildExploreList_AllTiles(pBoard) || pBoard->mExploreListCount <= 0) {
        break;
      }
      const int aPick = static_cast<int>(pBoard->NextTypeByte() % static_cast<unsigned char>(pBoard->mExploreListCount));
      RandomFlipOneTileType(pBoard, pBoard->mExploreListX[aPick], pBoard->mExploreListY[aPick]);
      if (!pBoard->HasAnyMatches() && pBoard->HasAnyLegalMove()) {
        return true;
      }
    }
    pBoard->RecordGameStateOverflowCatastrophic();
    RandomizeAllTileTypes(pBoard);
  }

  return (!pBoard->HasAnyMatches() && pBoard->HasAnyLegalMove());
}

bool GamePlayDirector_CollapseStyle::BuildExploreList(GameBoard* pBoard) {
  return BuildExploreList_AllTiles(pBoard);
}

bool GamePlayDirector_CollapseStyle::ResolveBoardState_PostSpawn(GameBoard* pBoard) {
  return GuaranteeMatchDoesExist(pBoard);
}

bool GamePlayDirector_CollapseStyle::ResolveBoardState_PostTopple(GameBoard* pBoard) {
  return GuaranteeMoveExistsForNewTiles_MatchDoesExist(pBoard);
}

bool GamePlayDirector_BejeweledStyle::BuildExploreList(GameBoard* pBoard) {
  return BuildExploreList_AllTiles(pBoard);
}

bool GamePlayDirector_BejeweledStyle::ResolveBoardState_PostSpawn(GameBoard* pBoard) {
  return GuaranteeMatchDoesNotExist(pBoard);
}

bool GamePlayDirector_BejeweledStyle::ResolveBoardState_PostTopple(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }
  if (pBoard->HasAnyMatches()) {
    return true;
  }
  return GuaranteeMoveExistsForNewTiles_MatchDoesNotExist(pBoard);
}

}  // namespace bread::games
