#ifndef BREAD_SRC_GAMES_GAMEPLAYDIRECTOR_HPP_
#define BREAD_SRC_GAMES_GAMEPLAYDIRECTOR_HPP_

#include <array>

#include "GameBoard.hpp"

namespace peanutbutter::games {

class GamePlayDirector {
 public:
  virtual ~GamePlayDirector() = default;
  virtual bool ResolveBoardState_PostSpawn(GameBoard* pBoard) = 0;
  virtual bool ResolveBoardState_PostTopple(GameBoard* pBoard) = 0;

  static GamePlayDirector* TapStyle();
  static GamePlayDirector* MatchThreeStyle();

 protected:
  bool BuildCandidateList_AllTiles(GameBoard* pBoard);
  bool BuildExploreList_ViolatingTiles(GameBoard* pBoard);
  bool TryCreateMatchBySingleTileEdit(GameBoard* pBoard);
  bool TryResolveViolationsBySingleTileEdit(GameBoard* pBoard, bool pRequireLegalMove);
  bool TryResolveViolationsByTwoTileEdit(GameBoard* pBoard, bool pRequireLegalMove);
  bool TryResolveViolationsByIntelligentBreakdown(GameBoard* pBoard, bool pRequireLegalMove);
  bool TryCreateLegalMoveByLocalPattern(GameBoard* pBoard);
  bool TrySolveBoard_NoMatchesThenCreateLegalMove(GameBoard* pBoard);
  bool GuaranteeMatchDoesNotExist(GameBoard* pBoard);
  bool GuaranteeMatchDoesExist(GameBoard* pBoard);
  bool GuaranteeMoveExistsForNewTiles_MatchDoesNotExist(GameBoard* pBoard);

 private:
  struct SolveCell {
    int mAssignedType = 0;
    unsigned char mPreferredStart = 0U;
    unsigned char mFlags = 0U;
  };

  struct SolveFrame {
    int mFlatIndex = -1;
    int mCandidateCount = 0;
    int mCandidates[GameBoard::kTypeCount + 2] = {};
  };

  enum class DeterministicGoal {
    kNoMatches,
    kPlantedMatch,
    kNoMatchesWithLegalMove,
  };

  static constexpr unsigned char kSolveAssignedBit = 1U << 0U;
  static constexpr unsigned char kSolveAllowedMatchedBit = 1U << 1U;
  static constexpr unsigned char kSolvePreassignedBit = 1U << 2U;
  static constexpr unsigned char kSolveSearchRegionBit = 1U << 3U;

  static constexpr int FlatIndex(int pX, int pY) {
    return (pY * GameBoard::kGridWidth) + pX;
  }

  static constexpr bool InBounds(int pX, int pY) {
    return pX >= 0 && pX < GameBoard::kGridWidth && pY >= 0 && pY < GameBoard::kGridHeight;
  }

  bool IsSolveAssigned(int pFlat) const;
  bool IsSolveAllowedMatched(int pFlat) const;
  bool IsSolvePreassigned(int pFlat) const;
  bool IsSolveInSearchRegion(int pFlat) const;
  void SetSolveAssigned(int pFlat, bool pValue);
  void SetSolveAllowedMatched(int pFlat, bool pValue);
  void SetSolvePreassigned(int pFlat, bool pValue);
  void SetSolveInSearchRegion(int pFlat, bool pValue);

  int AssignedTypeAt(int pX, int pY) const;
  bool PartialLineMatch(int pX1, int pY1, int pX2, int pY2, int pX3, int pY3) const;
  bool PartialIsMatchedStreak(int pX, int pY) const;
  bool PartialIsMatchedIsland(GameBoard* pBoard, int pX, int pY);
  bool PartialIsMatched(GameBoard* pBoard, int pX, int pY);
  bool HasDisallowedPartialMatches(GameBoard* pBoard);

  static void CaptureBoardTypes(GameBoard* pBoard, std::array<unsigned char, GameBoard::kGridSize>* pTypes);
  static void ApplyBoardTypes(GameBoard* pBoard, const std::array<unsigned char, GameBoard::kGridSize>& pTypes);

  bool FinalBoardHasOnlyAllowedMatches(GameBoard* pBoard);
  bool ValidateSolvedBoard(GameBoard* pBoard, DeterministicGoal pGoal);
  bool SolveRecursive(GameBoard* pBoard,
                      const std::array<unsigned char, GameBoard::kGridSize>& pOriginalBoardTypes,
                      int pSearchIndex,
                      DeterministicGoal pGoal);
  void InitializeSolveState(GameBoard* pBoard, bool pRestrictToToppleSearchRegion);
  void RefreshSolveSearchOrder();
  bool TrySolveBoard(GameBoard* pBoard, DeterministicGoal pGoal);
  bool TrySolveBoard_NoMatches(GameBoard* pBoard);
  bool TrySolveBoard_PlantedMatch(GameBoard* pBoard);
  bool TrySolveBoard_NoMatchesWithLegalMove(GameBoard* pBoard);
  bool TryConstructDeterministicBoard_NoMatches(GameBoard* pBoard);
  bool TryConstructDeterministicBoard_NoMatchesThenCreateLegalMove(GameBoard* pBoard);
  bool TryConstructDeterministicBoard_NoMatchesWithLegalMove(GameBoard* pBoard);
  bool PatternTouchesSolveSearchRegion(int pFlat0, int pFlat1, int pFlat2, int pFlatTail) const;
  bool SolverTryPattern(GameBoard* pBoard, int pFlat0, int pFlat1, int pFlat2, int pFlatTail, int pBSlot);
  bool SolverTrySeed(GameBoard* pBoard, int pFlat0, int pFlat1, int pFlat2, int pFlatTail, int pBSlot);
  bool ReportImpossible(GameBoard* pBoard);

  std::array<SolveCell, GameBoard::kGridSize> mSolveCells{};
  std::array<SolveFrame, GameBoard::kGridSize> mSolveFrames{};
  MatchBehavior mSolveMatchBehavior = kStreak;
  int mSolveTypeCount = 0;
  int mSolveSearchCount = 0;
  bool mSolveSearchRestricted = false;
};

class GamePlayDirector_TapStyle final : public GamePlayDirector {
 public:
  bool ResolveBoardState_PostSpawn(GameBoard* pBoard) override;
  bool ResolveBoardState_PostTopple(GameBoard* pBoard) override;
};

class GamePlayDirector_MatchThreeStyle final : public GamePlayDirector {
 public:
  bool ResolveBoardState_PostSpawn(GameBoard* pBoard) override;
  bool ResolveBoardState_PostTopple(GameBoard* pBoard) override;
};

}  // namespace peanutbutter::games

#endif  // BREAD_SRC_GAMES_GAMEPLAYDIRECTOR_HPP_
