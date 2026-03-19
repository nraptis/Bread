#include "GamePlayDirector.hpp"

namespace peanutbutter::games {

GamePlayDirector* GamePlayDirector::TapStyle() {
  static GamePlayDirector_TapStyle gTapStyleDirector;
  return &gTapStyleDirector;
}

GamePlayDirector* GamePlayDirector::MatchThreeStyle() {
  static GamePlayDirector_MatchThreeStyle gMatchThreeStyleDirector;
  return &gMatchThreeStyleDirector;
}

bool GamePlayDirector::IsSolveAssigned(int pFlat) const {
  return (mSolveCells[pFlat].mFlags & kSolveAssignedBit) != 0U;
}

bool GamePlayDirector::IsSolveAllowedMatched(int pFlat) const {
  return (mSolveCells[pFlat].mFlags & kSolveAllowedMatchedBit) != 0U;
}

bool GamePlayDirector::IsSolvePreassigned(int pFlat) const {
  return (mSolveCells[pFlat].mFlags & kSolvePreassignedBit) != 0U;
}

bool GamePlayDirector::IsSolveInSearchRegion(int pFlat) const {
  return (mSolveCells[pFlat].mFlags & kSolveSearchRegionBit) != 0U;
}

void GamePlayDirector::SetSolveAssigned(int pFlat, bool pValue) {
  if (pValue) {
    mSolveCells[pFlat].mFlags |= kSolveAssignedBit;
  } else {
    mSolveCells[pFlat].mFlags &= static_cast<unsigned char>(~kSolveAssignedBit);
  }
}

void GamePlayDirector::SetSolveAllowedMatched(int pFlat, bool pValue) {
  if (pValue) {
    mSolveCells[pFlat].mFlags |= kSolveAllowedMatchedBit;
  } else {
    mSolveCells[pFlat].mFlags &= static_cast<unsigned char>(~kSolveAllowedMatchedBit);
  }
}

void GamePlayDirector::SetSolvePreassigned(int pFlat, bool pValue) {
  if (pValue) {
    mSolveCells[pFlat].mFlags |= kSolvePreassignedBit;
  } else {
    mSolveCells[pFlat].mFlags &= static_cast<unsigned char>(~kSolvePreassignedBit);
  }
}

void GamePlayDirector::SetSolveInSearchRegion(int pFlat, bool pValue) {
  if (pValue) {
    mSolveCells[pFlat].mFlags |= kSolveSearchRegionBit;
  } else {
    mSolveCells[pFlat].mFlags &= static_cast<unsigned char>(~kSolveSearchRegionBit);
  }
}

int GamePlayDirector::AssignedTypeAt(int pX, int pY) const {
  if (!InBounds(pX, pY)) {
    return -1;
  }
  const int aFlat = FlatIndex(pX, pY);
  if (!IsSolveAssigned(aFlat)) {
    return -1;
  }
  return mSolveCells[aFlat].mAssignedType;
}

bool GamePlayDirector::PartialLineMatch(int pX1, int pY1, int pX2, int pY2, int pX3, int pY3) const {
  const int aType1 = AssignedTypeAt(pX1, pY1);
  const int aType2 = AssignedTypeAt(pX2, pY2);
  const int aType3 = AssignedTypeAt(pX3, pY3);
  return (aType1 >= 0 && aType1 == aType2 && aType2 == aType3);
}

bool GamePlayDirector::PartialIsMatchedStreak(int pX, int pY) const {
  if (AssignedTypeAt(pX, pY) < 0) {
    return false;
  }

  if (PartialLineMatch(pX - 2, pY, pX - 1, pY, pX, pY) ||
      PartialLineMatch(pX - 1, pY, pX, pY, pX + 1, pY) ||
      PartialLineMatch(pX, pY, pX + 1, pY, pX + 2, pY) ||
      PartialLineMatch(pX, pY - 2, pX, pY - 1, pX, pY) ||
      PartialLineMatch(pX, pY - 1, pX, pY, pX, pY + 1) ||
      PartialLineMatch(pX, pY, pX, pY + 1, pX, pY + 2)) {
    return true;
  }
  return false;
}

bool GamePlayDirector::PartialIsMatchedIsland(GameBoard* pBoard, int pX, int pY) {
  if (pBoard == nullptr) {
    return false;
  }

  const int aType = AssignedTypeAt(pX, pY);
  if (aType < 0) {
    return false;
  }

  for (int aIndex = 0; aIndex < GameBoard::kGridSize; ++aIndex) {
    pBoard->mCheckVisited[aIndex] = 0U;
  }

  pBoard->mCheckStackCount = 0;
  int aComponentCount = 0;
  pBoard->mCheckStackX[pBoard->mCheckStackCount] = pX;
  pBoard->mCheckStackY[pBoard->mCheckStackCount] = pY;
  ++pBoard->mCheckStackCount;
  pBoard->mCheckVisited[FlatIndex(pX, pY)] = 1U;

  static constexpr int kDX[4] = {-1, 1, 0, 0};
  static constexpr int kDY[4] = {0, 0, -1, 1};
  while (pBoard->mCheckStackCount > 0) {
    --pBoard->mCheckStackCount;
    const int aCurrentX = pBoard->mCheckStackX[pBoard->mCheckStackCount];
    const int aCurrentY = pBoard->mCheckStackY[pBoard->mCheckStackCount];
    ++aComponentCount;
    if (aComponentCount >= 3) {
      return true;
    }

    for (int aDir = 0; aDir < 4; ++aDir) {
      const int aNextX = aCurrentX + kDX[aDir];
      const int aNextY = aCurrentY + kDY[aDir];
      if (!InBounds(aNextX, aNextY)) {
        continue;
      }

      const int aFlat = FlatIndex(aNextX, aNextY);
      if (pBoard->mCheckVisited[aFlat] != 0U || AssignedTypeAt(aNextX, aNextY) != aType) {
        continue;
      }

      pBoard->mCheckVisited[aFlat] = 1U;
      pBoard->mCheckStackX[pBoard->mCheckStackCount] = aNextX;
      pBoard->mCheckStackY[pBoard->mCheckStackCount] = aNextY;
      ++pBoard->mCheckStackCount;
    }
  }

  return false;
}

bool GamePlayDirector::PartialIsMatched(GameBoard* pBoard, int pX, int pY) {
  return (mSolveMatchBehavior == kIsland) ? PartialIsMatchedIsland(pBoard, pX, pY) : PartialIsMatchedStreak(pX, pY);
}

bool GamePlayDirector::HasDisallowedPartialMatches(GameBoard* pBoard) {
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      const int aFlat = FlatIndex(aX, aY);
      if (!IsSolveAssigned(aFlat)) {
        continue;
      }
      if (PartialIsMatched(pBoard, aX, aY) && !IsSolveAllowedMatched(aFlat)) {
        return true;
      }
    }
  }
  return false;
}

void GamePlayDirector::CaptureBoardTypes(GameBoard* pBoard, std::array<unsigned char, GameBoard::kGridSize>* pTypes) {
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      const GameTile* aTile = pBoard->TileAt(aX, aY);
      (*pTypes)[FlatIndex(aX, aY)] = (aTile == nullptr) ? 0U : aTile->mType;
    }
  }
}

void GamePlayDirector::ApplyBoardTypes(GameBoard* pBoard, const std::array<unsigned char, GameBoard::kGridSize>& pTypes) {
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      GameTile* aTile = pBoard->MutableTileAt(aX, aY);
      if (aTile != nullptr) {
        aTile->mType = pTypes[FlatIndex(aX, aY)];
      }
    }
  }
}

bool GamePlayDirector::FinalBoardHasOnlyAllowedMatches(GameBoard* pBoard) {
  if (pBoard->GetMatches() <= 0) {
    pBoard->InvalidateMatches();
    return false;
  }

  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      if (pBoard->IsTileMatched(aX, aY) && !IsSolveAllowedMatched(FlatIndex(aX, aY))) {
        pBoard->InvalidateMatches();
        return false;
      }
    }
  }

  pBoard->InvalidateMatches();
  return true;
}

bool GamePlayDirector::ValidateSolvedBoard(GameBoard* pBoard, DeterministicGoal pGoal) {
  if (pGoal == DeterministicGoal::kNoMatches) {
    return !pBoard->HasAnyMatches();
  }
  if (pGoal == DeterministicGoal::kPlantedMatch) {
    return FinalBoardHasOnlyAllowedMatches(pBoard);
  }
  return !pBoard->HasAnyMatches() && pBoard->HasAnyLegalMove();
}

bool GamePlayDirector::SolveRecursive(GameBoard* pBoard,
                                      const std::array<unsigned char, GameBoard::kGridSize>& pOriginalBoardTypes,
                                      int pSearchIndex,
                                      DeterministicGoal pGoal) {
  if (pSearchIndex >= mSolveSearchCount) {
    std::array<unsigned char, GameBoard::kGridSize> aSolvedTypes{};
    for (int aIndex = 0; aIndex < GameBoard::kGridSize; ++aIndex) {
      aSolvedTypes[aIndex] = static_cast<unsigned char>(mSolveCells[aIndex].mAssignedType);
    }
    ApplyBoardTypes(pBoard, aSolvedTypes);
    if (ValidateSolvedBoard(pBoard, pGoal)) {
      return true;
    }
    ApplyBoardTypes(pBoard, pOriginalBoardTypes);
    return false;
  }

  SolveFrame& aFrame = mSolveFrames[pSearchIndex];
  const int aFlatIndex = aFrame.mFlatIndex;
  aFrame.mCandidateCount = 0;
  if (IsSolvePreassigned(aFlatIndex)) {
    aFrame.mCandidates[aFrame.mCandidateCount] = mSolveCells[aFlatIndex].mAssignedType;
    ++aFrame.mCandidateCount;
  } else {
    const int aStart = mSolveCells[aFlatIndex].mPreferredStart % mSolveTypeCount;
    for (int aOffset = 0; aOffset < mSolveTypeCount; ++aOffset) {
      aFrame.mCandidates[aFrame.mCandidateCount] = (aStart + aOffset) % mSolveTypeCount;
      ++aFrame.mCandidateCount;
    }
  }

  for (int aCandidateIndex = 0; aCandidateIndex < aFrame.mCandidateCount; ++aCandidateIndex) {
    mSolveCells[aFlatIndex].mAssignedType = aFrame.mCandidates[aCandidateIndex];
    SetSolveAssigned(aFlatIndex, true);
    if (!HasDisallowedPartialMatches(pBoard) &&
        SolveRecursive(pBoard, pOriginalBoardTypes, pSearchIndex + 1, pGoal)) {
      return true;
    }
    SetSolveAssigned(aFlatIndex, false);
  }

  return false;
}

void GamePlayDirector::InitializeSolveState(GameBoard* pBoard, bool pRestrictToToppleSearchRegion) {
  mSolveMatchBehavior = pBoard->GetMatchBehavior();
  mSolveTypeCount = pBoard->TileTypeCount();
  mSolveSearchCount = 0;
  mSolveSearchRestricted = (pRestrictToToppleSearchRegion && pBoard->mToppleSearchCount > 0);

  for (int aIndex = 0; aIndex < GameBoard::kGridSize; ++aIndex) {
    mSolveCells[aIndex] = SolveCell{};
    mSolveFrames[aIndex] = SolveFrame{};
  }

  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      const int aFlat = FlatIndex(aX, aY);
      const GameTile* aTile = pBoard->TileAt(aX, aY);
      if (aTile == nullptr) {
        mSolveCells[aFlat].mAssignedType = 0;
        mSolveCells[aFlat].mPreferredStart = 0U;
        continue;
      }
      mSolveCells[aFlat].mAssignedType = static_cast<int>(aTile->mType);
      mSolveCells[aFlat].mPreferredStart =
          static_cast<unsigned char>((static_cast<int>(aTile->mType) + static_cast<int>(aTile->mByte) + aFlat) %
                                     mSolveTypeCount);

      const bool aIsInsideSearchWindow = (!mSolveSearchRestricted || pBoard->mToppleSearchMask[aFlat] != 0U);
      if (aIsInsideSearchWindow) {
        SetSolveInSearchRegion(aFlat, true);
      } else {
        SetSolvePreassigned(aFlat, true);
        SetSolveAssigned(aFlat, true);
      }
    }
  }
}

void GamePlayDirector::RefreshSolveSearchOrder() {
  mSolveSearchCount = 0;
  for (int aFlat = 0; aFlat < GameBoard::kGridSize; ++aFlat) {
    if (!IsSolveInSearchRegion(aFlat) || IsSolvePreassigned(aFlat)) {
      continue;
    }
    mSolveFrames[mSolveSearchCount].mFlatIndex = aFlat;
    mSolveFrames[mSolveSearchCount].mCandidateCount = 0;
    ++mSolveSearchCount;
  }
}

bool GamePlayDirector::TrySolveBoard(GameBoard* pBoard, DeterministicGoal pGoal) {
  std::array<unsigned char, GameBoard::kGridSize> aOriginalBoardTypes{};
  CaptureBoardTypes(pBoard, &aOriginalBoardTypes);
  RefreshSolveSearchOrder();
  if (SolveRecursive(pBoard, aOriginalBoardTypes, 0, pGoal)) {
    return true;
  }
  ApplyBoardTypes(pBoard, aOriginalBoardTypes);
  return false;
}

bool GamePlayDirector::TrySolveBoard_NoMatches(GameBoard* pBoard) {
  InitializeSolveState(pBoard, false);
  return TrySolveBoard(pBoard, DeterministicGoal::kNoMatches);
}

bool GamePlayDirector::TrySolveBoard_PlantedMatch(GameBoard* pBoard) {
  if (pBoard == nullptr || pBoard->TileTypeCount() <= 0) {
    return false;
  }

  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX <= (GameBoard::kGridWidth - 3); ++aX) {
      const int aFlat0 = FlatIndex(aX, aY);
      const int aFlat1 = FlatIndex(aX + 1, aY);
      const int aFlat2 = FlatIndex(aX + 2, aY);
      for (int aOffset = 0; aOffset < pBoard->TileTypeCount(); ++aOffset) {
        InitializeSolveState(pBoard, false);
        const int aType = (mSolveCells[aFlat0].mPreferredStart + aOffset) % mSolveTypeCount;
        SetSolvePreassigned(aFlat0, true);
        SetSolvePreassigned(aFlat1, true);
        SetSolvePreassigned(aFlat2, true);
        SetSolveAllowedMatched(aFlat0, true);
        SetSolveAllowedMatched(aFlat1, true);
        SetSolveAllowedMatched(aFlat2, true);
        SetSolveAssigned(aFlat0, true);
        SetSolveAssigned(aFlat1, true);
        SetSolveAssigned(aFlat2, true);
        mSolveCells[aFlat0].mAssignedType = aType;
        mSolveCells[aFlat1].mAssignedType = aType;
        mSolveCells[aFlat2].mAssignedType = aType;
        if (TrySolveBoard(pBoard, DeterministicGoal::kPlantedMatch)) {
          pBoard->RecordPlantedMatchSolve();
          return true;
        }
      }
    }
  }

  return false;
}

bool GamePlayDirector::SolverTrySeed(GameBoard* pBoard,
                                     int pFlat0,
                                     int pFlat1,
                                     int pFlat2,
                                     int pFlatTail,
                                     int pBSlot) {
  for (int aOffsetA = 0; aOffsetA < pBoard->TileTypeCount(); ++aOffsetA) {
    InitializeSolveState(pBoard, true);
    const int aTypeA = (mSolveCells[pFlat0].mPreferredStart + aOffsetA) % mSolveTypeCount;
    for (int aOffsetB = 1; aOffsetB < pBoard->TileTypeCount(); ++aOffsetB) {
      const int aTypeB = (aTypeA + aOffsetB) % mSolveTypeCount;
      const int aType0 = (pBSlot == 0) ? aTypeB : aTypeA;
      const int aType1 = (pBSlot == 1) ? aTypeB : aTypeA;
      const int aType2 = (pBSlot == 2) ? aTypeB : aTypeA;
      SetSolvePreassigned(pFlat0, true);
      SetSolvePreassigned(pFlat1, true);
      SetSolvePreassigned(pFlat2, true);
      SetSolvePreassigned(pFlatTail, true);
      SetSolveAssigned(pFlat0, true);
      SetSolveAssigned(pFlat1, true);
      SetSolveAssigned(pFlat2, true);
      SetSolveAssigned(pFlatTail, true);
      mSolveCells[pFlat0].mAssignedType = aType0;
      mSolveCells[pFlat1].mAssignedType = aType1;
      mSolveCells[pFlat2].mAssignedType = aType2;
      mSolveCells[pFlatTail].mAssignedType = aTypeA;
      if (TrySolveBoard(pBoard, DeterministicGoal::kNoMatchesWithLegalMove)) {
        return true;
      }
    }
  }
  return false;
}

bool GamePlayDirector::PatternTouchesSolveSearchRegion(int pFlat0, int pFlat1, int pFlat2, int pFlatTail) const {
  if (!mSolveSearchRestricted) {
    return true;
  }
  return IsSolveInSearchRegion(pFlat0) || IsSolveInSearchRegion(pFlat1) || IsSolveInSearchRegion(pFlat2) ||
         IsSolveInSearchRegion(pFlatTail);
}

bool GamePlayDirector::TrySolveBoard_NoMatchesWithLegalMove(GameBoard* pBoard) {
  if (pBoard == nullptr || pBoard->TileTypeCount() <= 1) {
    return false;
  }

  InitializeSolveState(pBoard, true);

  for (int aY = 0; aY <= (GameBoard::kGridHeight - 2); ++aY) {
    for (int aX = 0; aX <= (GameBoard::kGridWidth - 3); ++aX) {
      const int aTopRow[3] = {FlatIndex(aX, aY), FlatIndex(aX + 1, aY), FlatIndex(aX + 2, aY)};
      const int aBottomRow[3] = {FlatIndex(aX, aY + 1), FlatIndex(aX + 1, aY + 1), FlatIndex(aX + 2, aY + 1)};
      for (int aBSlot = 0; aBSlot < 3; ++aBSlot) {
        if (!PatternTouchesSolveSearchRegion(aTopRow[0], aTopRow[1], aTopRow[2], aBottomRow[aBSlot]) &&
            !PatternTouchesSolveSearchRegion(aBottomRow[0], aBottomRow[1], aBottomRow[2], aTopRow[aBSlot])) {
          continue;
        }
        if (SolverTrySeed(pBoard, aTopRow[0], aTopRow[1], aTopRow[2], aBottomRow[aBSlot], aBSlot) ||
            SolverTrySeed(pBoard, aBottomRow[0], aBottomRow[1], aBottomRow[2], aTopRow[aBSlot], aBSlot)) {
          return true;
        }
      }
    }
  }

  for (int aY = 0; aY <= (GameBoard::kGridHeight - 3); ++aY) {
    for (int aX = 0; aX <= (GameBoard::kGridWidth - 2); ++aX) {
      const int aLeftCol[3] = {FlatIndex(aX, aY), FlatIndex(aX, aY + 1), FlatIndex(aX, aY + 2)};
      const int aRightCol[3] = {FlatIndex(aX + 1, aY), FlatIndex(aX + 1, aY + 1), FlatIndex(aX + 1, aY + 2)};
      for (int aBSlot = 0; aBSlot < 3; ++aBSlot) {
        if (!PatternTouchesSolveSearchRegion(aLeftCol[0], aLeftCol[1], aLeftCol[2], aRightCol[aBSlot]) &&
            !PatternTouchesSolveSearchRegion(aRightCol[0], aRightCol[1], aRightCol[2], aLeftCol[aBSlot])) {
          continue;
        }
        if (SolverTrySeed(pBoard, aLeftCol[0], aLeftCol[1], aLeftCol[2], aRightCol[aBSlot], aBSlot) ||
            SolverTrySeed(pBoard, aRightCol[0], aRightCol[1], aRightCol[2], aLeftCol[aBSlot], aBSlot)) {
          return true;
        }
      }
    }
  }

  return false;
}

bool GamePlayDirector::TryConstructDeterministicBoard_NoMatches(GameBoard* pBoard) {
  if (pBoard == nullptr || pBoard->TileTypeCount() < 2) {
    return false;
  }

  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      GameTile* const aTile = pBoard->MutableTileAt(aX, aY);
      if (aTile == nullptr) {
        return false;
      }
      aTile->mType = static_cast<unsigned char>((aX + aY) % pBoard->TileTypeCount());
    }
  }

  return !pBoard->HasAnyMatches();
}

bool GamePlayDirector::TryConstructDeterministicBoard_NoMatchesThenCreateLegalMove(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }

  std::array<unsigned char, GameBoard::kGridSize> aOriginalBoardTypes{};
  CaptureBoardTypes(pBoard, &aOriginalBoardTypes);
  if (TryConstructDeterministicBoard_NoMatches(pBoard) && TryCreateLegalMoveByLocalPattern(pBoard)) {
    return true;
  }
  ApplyBoardTypes(pBoard, aOriginalBoardTypes);
  return false;
}

bool GamePlayDirector::TryConstructDeterministicBoard_NoMatchesWithLegalMove(GameBoard* pBoard) {
  if (!TryConstructDeterministicBoard_NoMatches(pBoard)) {
    return false;
  }

  GameTile* const aTopLeft = pBoard->MutableTileAt(0, 0);
  GameTile* const aTopMid = pBoard->MutableTileAt(1, 0);
  GameTile* const aTopRight = pBoard->MutableTileAt(2, 0);
  GameTile* const aTail = pBoard->MutableTileAt(1, 1);
  if (aTopLeft == nullptr || aTopMid == nullptr || aTopRight == nullptr || aTail == nullptr) {
    return false;
  }

  // Seed a single adjacent swap that creates a line match while the rest of the
  // board remains a deterministic no-match pattern.
  aTopLeft->mType = 0U;
  aTopMid->mType = 1U % pBoard->TileTypeCount();
  aTopRight->mType = 0U;
  aTail->mType = 0U;

  return !pBoard->HasAnyMatches() && pBoard->HasAnyLegalMove();
}

bool GamePlayDirector::ReportImpossible(GameBoard* pBoard) {
  if (pBoard != nullptr) {
    pBoard->RecordImpossible();
  }
  return false;
}

bool GamePlayDirector::BuildCandidateList_AllTiles(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }

  pBoard->mCandidateListCount = 0;
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      const GameTile* aTile = pBoard->mGrid[aX][aY];
      if (aTile == nullptr) {
        continue;
      }
      pBoard->mCandidateListX[pBoard->mCandidateListCount] = aX;
      pBoard->mCandidateListY[pBoard->mCandidateListCount] = aY;
      ++pBoard->mCandidateListCount;
    }
  }
  return pBoard->mCandidateListCount > 0;
}

bool GamePlayDirector::BuildExploreList_ViolatingTiles(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }
  if (pBoard->GetMatches() <= 0) {
    pBoard->mExploreListCount = 0;
    pBoard->mViolatingListCount = 0;
    return false;
  }

  pBoard->mExploreListCount = 0;
  pBoard->mViolatingListCount = 0;
  for (int aIndex = 0; aIndex < pBoard->mMatchListCount; ++aIndex) {
    const int aX = pBoard->mMatchListX[aIndex];
    const int aY = pBoard->mMatchListY[aIndex];
    pBoard->mExploreListX[pBoard->mExploreListCount] = aX;
    pBoard->mExploreListY[pBoard->mExploreListCount] = aY;
    ++pBoard->mExploreListCount;
    pBoard->mViolatingListX[pBoard->mViolatingListCount] = aX;
    pBoard->mViolatingListY[pBoard->mViolatingListCount] = aY;
    ++pBoard->mViolatingListCount;
  }
  return pBoard->mViolatingListCount > 0;
}

bool GamePlayDirector::TryCreateMatchBySingleTileEdit(GameBoard* pBoard) {
  if (pBoard == nullptr || pBoard->mTileTypeCount <= 1) {
    return false;
  }
  if (!BuildCandidateList_AllTiles(pBoard) || pBoard->mCandidateListCount <= 0) {
    return false;
  }

  for (int aIndex = 0; aIndex < pBoard->mCandidateListCount; ++aIndex) {
    const int aX = pBoard->mCandidateListX[aIndex];
    const int aY = pBoard->mCandidateListY[aIndex];
    GameTile* const aTile = pBoard->mGrid[aX][aY];
    if (aTile == nullptr) {
      continue;
    }

    const unsigned char aOriginal = aTile->mType;
    for (int aOffset = 1; aOffset < pBoard->mTileTypeCount; ++aOffset) {
      aTile->mType =
          static_cast<unsigned char>((static_cast<int>(aOriginal) + aOffset) % pBoard->mTileTypeCount);
      if (pBoard->HasAnyMatches()) {
        return true;
      }
    }
    aTile->mType = aOriginal;
  }

  return false;
}

bool GamePlayDirector::SolverTryPattern(GameBoard* pBoard,
                                        int pFlat0,
                                        int pFlat1,
                                        int pFlat2,
                                        int pFlatTail,
                                        int pBSlot) {
  const int aX0 = pFlat0 % GameBoard::kGridWidth;
  const int aY0 = pFlat0 / GameBoard::kGridWidth;
  const int aX1 = pFlat1 % GameBoard::kGridWidth;
  const int aY1 = pFlat1 / GameBoard::kGridWidth;
  const int aX2 = pFlat2 % GameBoard::kGridWidth;
  const int aY2 = pFlat2 / GameBoard::kGridWidth;
  const int aXT = pFlatTail % GameBoard::kGridWidth;
  const int aYT = pFlatTail / GameBoard::kGridWidth;

  GameTile* const aTile0 = pBoard->MutableTileAt(aX0, aY0);
  GameTile* const aTile1 = pBoard->MutableTileAt(aX1, aY1);
  GameTile* const aTile2 = pBoard->MutableTileAt(aX2, aY2);
  GameTile* const aTileTail = pBoard->MutableTileAt(aXT, aYT);
  if (aTile0 == nullptr || aTile1 == nullptr || aTile2 == nullptr || aTileTail == nullptr) {
    return false;
  }

  const unsigned char aOriginal0 = aTile0->mType;
  const unsigned char aOriginal1 = aTile1->mType;
  const unsigned char aOriginal2 = aTile2->mType;
  const unsigned char aOriginalTail = aTileTail->mType;

  for (int aTypeA = 0; aTypeA < pBoard->mTileTypeCount; ++aTypeA) {
    for (int aTypeBOffset = 1; aTypeBOffset < pBoard->mTileTypeCount; ++aTypeBOffset) {
      const int aTypeB = (aTypeA + aTypeBOffset) % pBoard->mTileTypeCount;
      aTile0->mType = static_cast<unsigned char>((pBSlot == 0) ? aTypeB : aTypeA);
      aTile1->mType = static_cast<unsigned char>((pBSlot == 1) ? aTypeB : aTypeA);
      aTile2->mType = static_cast<unsigned char>((pBSlot == 2) ? aTypeB : aTypeA);
      aTileTail->mType = static_cast<unsigned char>(aTypeA);
      if (!pBoard->HasAnyMatches() && pBoard->HasAnyLegalMove()) {
        return true;
      }
    }
  }

  aTile0->mType = aOriginal0;
  aTile1->mType = aOriginal1;
  aTile2->mType = aOriginal2;
  aTileTail->mType = aOriginalTail;
  return false;
}

bool GamePlayDirector::TryCreateLegalMoveByLocalPattern(GameBoard* pBoard) {
  if (pBoard == nullptr || pBoard->mTileTypeCount <= 1) {
    return false;
  }

  for (int aY = 0; aY <= (GameBoard::kGridHeight - 2); ++aY) {
    for (int aX = 0; aX <= (GameBoard::kGridWidth - 3); ++aX) {
      const int aTopRow[3] = {FlatIndex(aX, aY), FlatIndex(aX + 1, aY), FlatIndex(aX + 2, aY)};
      const int aBottomRow[3] = {FlatIndex(aX, aY + 1), FlatIndex(aX + 1, aY + 1), FlatIndex(aX + 2, aY + 1)};
      for (int aBSlot = 0; aBSlot < 3; ++aBSlot) {
        if (SolverTryPattern(pBoard, aTopRow[0], aTopRow[1], aTopRow[2], aBottomRow[aBSlot], aBSlot) ||
            SolverTryPattern(pBoard, aBottomRow[0], aBottomRow[1], aBottomRow[2], aTopRow[aBSlot], aBSlot)) {
          return true;
        }
      }
    }
  }

  for (int aY = 0; aY <= (GameBoard::kGridHeight - 3); ++aY) {
    for (int aX = 0; aX <= (GameBoard::kGridWidth - 2); ++aX) {
      const int aLeftCol[3] = {FlatIndex(aX, aY), FlatIndex(aX, aY + 1), FlatIndex(aX, aY + 2)};
      const int aRightCol[3] = {FlatIndex(aX + 1, aY), FlatIndex(aX + 1, aY + 1), FlatIndex(aX + 1, aY + 2)};
      for (int aBSlot = 0; aBSlot < 3; ++aBSlot) {
        if (SolverTryPattern(pBoard, aLeftCol[0], aLeftCol[1], aLeftCol[2], aRightCol[aBSlot], aBSlot) ||
            SolverTryPattern(pBoard, aRightCol[0], aRightCol[1], aRightCol[2], aLeftCol[aBSlot], aBSlot)) {
          return true;
        }
      }
    }
  }

  return false;
}

bool GamePlayDirector::TrySolveBoard_NoMatchesThenCreateLegalMove(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }

  std::array<unsigned char, GameBoard::kGridSize> aOriginalBoardTypes{};
  CaptureBoardTypes(pBoard, &aOriginalBoardTypes);
  if (TrySolveBoard_NoMatches(pBoard) && TryCreateLegalMoveByLocalPattern(pBoard)) {
    return true;
  }
  ApplyBoardTypes(pBoard, aOriginalBoardTypes);
  return false;
}

bool GamePlayDirector::TryResolveViolationsBySingleTileEdit(GameBoard* pBoard, bool pRequireLegalMove) {
  if (pBoard == nullptr || pBoard->mTileTypeCount <= 1) {
    return false;
  }
  if (!BuildExploreList_ViolatingTiles(pBoard) || pBoard->mViolatingListCount <= 0) {
    return false;
  }

  for (int aIndex = 0; aIndex < pBoard->mViolatingListCount; ++aIndex) {
    const int aX = pBoard->mViolatingListX[aIndex];
    const int aY = pBoard->mViolatingListY[aIndex];
    if (!InBounds(aX, aY)) {
      continue;
    }
    GameTile* const aTile = pBoard->mGrid[aX][aY];
    if (aTile == nullptr || aTile->mFrozen) {
      continue;
    }

    const unsigned char aOriginal = aTile->mType;
    for (int aOffset = 1; aOffset < pBoard->mTileTypeCount; ++aOffset) {
      aTile->mType =
          static_cast<unsigned char>((static_cast<int>(aOriginal) + aOffset) % pBoard->mTileTypeCount);
      if (!pBoard->HasAnyMatches() && (!pRequireLegalMove || pBoard->HasAnyLegalMove())) {
        return true;
      }
    }
    aTile->mType = aOriginal;
  }

  return false;
}

bool GamePlayDirector::TryResolveViolationsByTwoTileEdit(GameBoard* pBoard, bool pRequireLegalMove) {
  if (pBoard == nullptr || pBoard->mTileTypeCount <= 1) {
    return false;
  }
  if (!BuildExploreList_ViolatingTiles(pBoard) || pBoard->mViolatingListCount < 2) {
    return false;
  }

  int aCandidateX[GameBoard::kGridSize] = {};
  int aCandidateY[GameBoard::kGridSize] = {};
  int aCandidateCount = 0;
  unsigned char aSeen[GameBoard::kGridSize] = {};

  for (int aIndex = 0; aIndex < pBoard->mViolatingListCount; ++aIndex) {
    const int aX = pBoard->mViolatingListX[aIndex];
    const int aY = pBoard->mViolatingListY[aIndex];
    if (!InBounds(aX, aY)) {
      continue;
    }
    if (pBoard->mGrid[aX][aY] == nullptr || pBoard->mGrid[aX][aY]->mFrozen) {
      continue;
    }

    const int aFlat = FlatIndex(aX, aY);
    if (aSeen[aFlat] != 0U) {
      continue;
    }
    aSeen[aFlat] = 1U;
    aCandidateX[aCandidateCount] = aX;
    aCandidateY[aCandidateCount] = aY;
    ++aCandidateCount;
  }

  if (aCandidateCount < 2) {
    return false;
  }

  for (int aFirstIndex = 0; aFirstIndex < aCandidateCount; ++aFirstIndex) {
    GameTile* const aFirstTile = pBoard->mGrid[aCandidateX[aFirstIndex]][aCandidateY[aFirstIndex]];
    if (aFirstTile == nullptr) {
      continue;
    }
    const unsigned char aFirstOriginal = aFirstTile->mType;

    for (int aSecondIndex = aFirstIndex + 1; aSecondIndex < aCandidateCount; ++aSecondIndex) {
      GameTile* const aSecondTile = pBoard->mGrid[aCandidateX[aSecondIndex]][aCandidateY[aSecondIndex]];
      if (aSecondTile == nullptr) {
        continue;
      }
      const unsigned char aSecondOriginal = aSecondTile->mType;

      for (int aFirstOffset = 1; aFirstOffset < pBoard->mTileTypeCount; ++aFirstOffset) {
        aFirstTile->mType =
            static_cast<unsigned char>((static_cast<int>(aFirstOriginal) + aFirstOffset) % pBoard->mTileTypeCount);

        for (int aSecondOffset = 1; aSecondOffset < pBoard->mTileTypeCount; ++aSecondOffset) {
          aSecondTile->mType = static_cast<unsigned char>(
              (static_cast<int>(aSecondOriginal) + aSecondOffset) % pBoard->mTileTypeCount);

          if (!pBoard->HasAnyMatches() && (!pRequireLegalMove || pBoard->HasAnyLegalMove())) {
            pBoard->RecordBlueMoonCase();
            return true;
          }
        }

        aSecondTile->mType = aSecondOriginal;
      }

      aFirstTile->mType = aFirstOriginal;
      aSecondTile->mType = aSecondOriginal;
    }
  }

  return false;
}

bool GamePlayDirector::TryResolveViolationsByIntelligentBreakdown(GameBoard* pBoard, bool pRequireLegalMove) {
  if (pBoard == nullptr || pBoard->mTileTypeCount <= 1) {
    return false;
  }
  pBoard->ClearFrozenTiles();

  constexpr int kIntelligentBreakdownPasses = 64;
  for (int aPass = 0; aPass < kIntelligentBreakdownPasses; ++aPass) {
    if (!BuildExploreList_ViolatingTiles(pBoard) || pBoard->mViolatingListCount <= 0) {
      pBoard->ClearFrozenTiles();
      return false;
    }

    unsigned char aSeen[GameBoard::kGridSize] = {};
    pBoard->mViolatingListCount = 0;
    for (int aIndex = 0; aIndex < pBoard->mExploreListCount; ++aIndex) {
      const int aX = pBoard->mExploreListX[aIndex];
      const int aY = pBoard->mExploreListY[aIndex];
      if (!InBounds(aX, aY)) {
        continue;
      }
      GameTile* const aTile = pBoard->mGrid[aX][aY];
      if (aTile == nullptr || aTile->mFrozen) {
        continue;
      }
      const int aFlat = FlatIndex(aX, aY);
      if (aSeen[aFlat] != 0U) {
        continue;
      }
      aSeen[aFlat] = 1U;
      pBoard->mViolatingListX[pBoard->mViolatingListCount] = aX;
      pBoard->mViolatingListY[pBoard->mViolatingListCount] = aY;
      ++pBoard->mViolatingListCount;
    }

    if (pBoard->mViolatingListCount <= 0) {
      break;
    }

    pBoard->ShuffleXY(pBoard->mViolatingListX, pBoard->mViolatingListY, pBoard->mViolatingListCount);
    GameTile* const aFrozenTile =
        pBoard->mGrid[pBoard->mViolatingListX[0]][pBoard->mViolatingListY[0]];
    if (aFrozenTile == nullptr) {
      continue;
    }

    const unsigned char aOriginalType = aFrozenTile->mType;
    const int aOffset = 1 + static_cast<int>(pBoard->GetRand(pBoard->mTileTypeCount - 1));
    aFrozenTile->mType =
        static_cast<unsigned char>((static_cast<int>(aOriginalType) + aOffset) % pBoard->mTileTypeCount);
    aFrozenTile->mFrozen = true;

    if (!pBoard->HasAnyMatches() && (!pRequireLegalMove || pBoard->HasAnyLegalMove())) {
      pBoard->ClearFrozenTiles();
      pBoard->RecordHarvestMoon();
      return true;
    }

    if (TryResolveViolationsBySingleTileEdit(pBoard, pRequireLegalMove) ||
        TryResolveViolationsByTwoTileEdit(pBoard, pRequireLegalMove)) {
      pBoard->ClearFrozenTiles();
      pBoard->RecordHarvestMoon();
      return true;
    }
  }

  pBoard->ClearFrozenTiles();
  return false;
}

bool GamePlayDirector::GuaranteeMatchDoesNotExist(GameBoard* pBoard) {
  if (pBoard == nullptr || pBoard->mTileTypeCount <= 1) {
    return false;
  }
  if (!pBoard->HasAnyMatches()) {
    return true;
  }
  if (TryResolveViolationsBySingleTileEdit(pBoard, false)) {
    return true;
  }
  if (TryResolveViolationsByTwoTileEdit(pBoard, false)) {
    return true;
  }
  if (TryResolveViolationsByIntelligentBreakdown(pBoard, false)) {
    return true;
  }
  if (TrySolveBoard_NoMatches(pBoard)) {
    return true;
  }
  return ReportImpossible(pBoard);
}

bool GamePlayDirector::GuaranteeMatchDoesExist(GameBoard* pBoard) {
  if (pBoard == nullptr || pBoard->mTileTypeCount <= 0) {
    return false;
  }
  if (pBoard->HasAnyMatches()) {
    return true;
  }
  if (TryCreateMatchBySingleTileEdit(pBoard)) {
    return true;
  }
  if (TrySolveBoard_PlantedMatch(pBoard)) {
    return true;
  }
  return ReportImpossible(pBoard);
}

bool GamePlayDirector::GuaranteeMoveExistsForNewTiles_MatchDoesNotExist(GameBoard* pBoard) {
  if (pBoard == nullptr || pBoard->mTileTypeCount <= 1) {
    return false;
  }
  if (!pBoard->HasAnyMatches() && pBoard->HasAnyLegalMove()) {
    return true;
  }
  if (TryResolveViolationsBySingleTileEdit(pBoard, true)) {
    return true;
  }
  if (TryResolveViolationsByTwoTileEdit(pBoard, true)) {
    return true;
  }
  if (TryResolveViolationsByIntelligentBreakdown(pBoard, true)) {
    return true;
  }
  if (!pBoard->HasAnyMatches() && TryCreateLegalMoveByLocalPattern(pBoard)) {
    return true;
  }
  if (TrySolveBoard_NoMatchesWithLegalMove(pBoard)) {
    return true;
  }
  if (TrySolveBoard_NoMatchesThenCreateLegalMove(pBoard)) {
    return true;
  }
  if (TryConstructDeterministicBoard_NoMatchesThenCreateLegalMove(pBoard)) {
    pBoard->RecordInconsistentStateD();
    return true;
  }
  pBoard->RecordInconsistentStateE();
  if (TryConstructDeterministicBoard_NoMatchesWithLegalMove(pBoard)) {
    return true;
  }
  return ReportImpossible(pBoard);
}

bool GamePlayDirector_TapStyle::ResolveBoardState_PostSpawn(GameBoard* pBoard) {
  return GuaranteeMatchDoesExist(pBoard);
}

bool GamePlayDirector_TapStyle::ResolveBoardState_PostTopple(GameBoard* pBoard) {
  return GuaranteeMatchDoesNotExist(pBoard);
}

bool GamePlayDirector_MatchThreeStyle::ResolveBoardState_PostSpawn(GameBoard* pBoard) {
  return GuaranteeMatchDoesNotExist(pBoard);
}

bool GamePlayDirector_MatchThreeStyle::ResolveBoardState_PostTopple(GameBoard* pBoard) {
  if (pBoard == nullptr) {
    return false;
  }

  int aActiveTileCount = 0;
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      if (pBoard->TileAt(aX, aY) != nullptr) {
        ++aActiveTileCount;
      }
    }
  }

  if (aActiveTileCount < GameBoard::kGridSize) {
    return true;
  }
  if (pBoard->HasAnyMatches()) {
    return true;
  }
  return GuaranteeMoveExistsForNewTiles_MatchDoesNotExist(pBoard);
}

}  // namespace peanutbutter::games
