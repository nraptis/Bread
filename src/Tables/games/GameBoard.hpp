#ifndef BREAD_SRC_GAMES_GAMEBOARD_HPP_
#define BREAD_SRC_GAMES_GAMEBOARD_HPP_

#include <cstddef>
#include <cstdint>

#include "../fast_rand/FastRand.hpp"
#include "GameTile.hpp"
#include "../rng/Shuffler.hpp"

namespace peanutbutter::games {

class GamePlayDirector;
class GamePowerUp;
class GamePowerUp_ZoneBomb;
class GamePowerUp_Rocket;
class GamePowerUp_ColorBomb;
class GamePowerUp_CrossBomb;
class GamePowerUp_PlasmaBeamH;
class GamePowerUp_PlasmaBeamV;
class GamePowerUp_PlasmaBeamQuad;
class GamePowerUp_Nuke;
class GamePowerUp_CornerBomb;
class GamePowerUp_VerticalBombs;
class GamePowerUp_HorizontalBombs;

enum MoveBehavior {
  kSlide,
  kSwap,
  kTap
};

enum MovePolicy {
  kGreedy,
  kRandom,
  kFirst
};

enum MatchBehavior {
  kStreak,
  kIsland
};

class GameBoard : public peanutbutter::rng::Shuffler {
 public:
  enum GameIndex {
    StreakSwapGreedy = 0,
    StreakSlideGreedy,
    IslandSwapGreedy,
    IslandSlideGreedy,
    StreakTapGreedy,
    IslandTapGreedy,
    StreakSwapRandom,
    StreakSlideRandom,
    IslandSwapRandom,
    IslandSlideRandom,
    StreakTapRandom,
    IslandTapRandom,
    StreakSwapFirst,
    StreakSlideFirst,
    IslandSwapFirst,
    IslandSlideFirst
  };

  struct RuntimeStats {
    std::uint64_t mStuck = 0U;
    std::uint64_t mTopple = 0U;
    std::uint64_t mUserMatch = 0U;
    std::uint64_t mCascadeMatch = 0U;
    std::uint64_t mGameStateOverflowCatastrophic = 0U;
    std::uint64_t mGameStateOverflowCataclysmic = 0U;
    std::uint64_t mGameStateOverflowApocalypse = 0U;
    std::uint64_t mPowerUpSpawned = 0U;
    std::uint64_t mPowerUpConsumed = 0U;
    std::uint64_t mPasswordExpanderWraps = 0U;
    std::uint64_t mDragonAttack = 0U;
    std::uint64_t mRiddlerAttack = 0U;
  };

  static constexpr int kGridWidth = 8;
  static constexpr int kGridHeight = 8;
  static constexpr int kGridSize = kGridWidth * kGridHeight;
  static constexpr int kTypeCount = 4;
  static constexpr int kGameCount = 16;
  static constexpr int kSeedBufferCapacity = PASSWORD_EXPANDED_SIZE;
  static constexpr int kMaxLockedThreshold = 32;
  static constexpr int kSuspendedThreshold = 4;
  static constexpr unsigned char kPowerUpSpawnChance = 4U;
  static constexpr int kTilePoolCapacity = kGridSize * 2;
  static constexpr int kMoveListCapacity =
      (kGridWidth * (kGridHeight - 1)) + (kGridHeight * (kGridWidth - 1));

  GameBoard();
  ~GameBoard() override;

  void Seed(unsigned char* pPassword, int pPasswordLength) override;

  bool Empty() const;
  bool HasAnyMatches();
  virtual bool IsMatch(int pGridX, int pGridY);
  virtual int GetMatches();
  RuntimeStats GetRuntimeStats() const;
  const char* GetName() const;

  void Stuck();
  virtual void Match();
  void Topple();
  void Spawn();
  void Play();
  virtual bool MakeMoves();

  int SuccessfulMoveCount() const;
  int BrokenCount() const;
  void SetGame(int pGameIndex);
  void SetMoveBehavior(MoveBehavior pMoveBehavior);
  void SetMovePolicy(MovePolicy pMovePolicy);
  void SetMatchBehavior(MatchBehavior pMatchBehavior);

  void SlideRow(int pRowIndex, int pAmount);
  void SlideColumn(int pColumnIndex, int pAmount);

  void InvalidateNew();
  void InvalidateMatches();
  void InvalidateToppleFlags();
  void Cascade();

  void MoveListClear();
  bool MoveListPush(int pX, int pY, bool pHorizontal, int pDir);

 protected:
  friend class GamePlayDirector;
  friend class GamePowerUp;
  friend class GamePowerUp_ZoneBomb;
  friend class GamePowerUp_Rocket;
  friend class GamePowerUp_ColorBomb;
  friend class GamePowerUp_CrossBomb;
  friend class GamePowerUp_PlasmaBeamH;
  friend class GamePowerUp_PlasmaBeamV;
  friend class GamePowerUp_PlasmaBeamQuad;
  friend class GamePowerUp_Nuke;
  friend class GamePowerUp_CornerBomb;
  friend class GamePowerUp_VerticalBombs;
  friend class GamePowerUp_HorizontalBombs;

  void MatchesBegin();
  int MatchesCommit();
  bool HasPendingMatches() const;
  void ShuffleXY(int* pListX, int* pListY, int pCount);

  void InitializeSeed(unsigned char* pPassword, int pPasswordLength);
  void SetIsCascading(bool pIsCascading);
  void SetGamePlayDirector(GamePlayDirector* pGamePlayDirector);
  bool ResolveBoardState_PostSpawn();
  bool ResolveBoardState_PostTopple();
  void RecordGameStateOverflowCatastrophic();
  void RecordGameStateOverflowCataclysmic();
  void RecordGameStateOverflowApocalypse();
  void ShuffleAllTiles();
  void ApocalypseScenario();
  bool TriggerMatchedPowerUps();
  bool HasAnyLegalMove();
  bool MatchMark(int pGridX, int pGridY);
  bool HasAnyTiles() const;
  int ActiveTileCount() const;
  void ToppleStep();
  bool CascadeStep();
  bool PlayMainLoop();
  GameTile* AcquireTile();
  void ReleaseTile(GameTile* pTile);
  bool MatchMarkStreak(int pGridX, int pGridY);
  bool MatchMarkIsland(int pGridX, int pGridY);
  bool IsMatchStreak(int pGridX, int pGridY);
  bool IsMatchIsland(int pGridX, int pGridY);
  int CountMarkedTiles() const;
  int EvaluateCurrentMatches();
  void SwapTiles(int pX1, int pY1, int pX2, int pY2);
  void ApplyMoveAt(int pMoveIndex);
  void ResetMoveList();
  bool PushMoveCandidate(int pX, int pY, bool pHorizontal, int pDir, int pScore);
  int SelectMoveIndex();
  void CollectSlideMoves();
  void CollectSwapMoves();
  void CollectTapMoves();
  bool EnumerateMoves();

  GameTile* GenerateTile(int pGridX, int pGridY);
  unsigned char GetRand(int pMax);
  unsigned char NextTypeByte();
  void ShuffleSeedBuffer();
  void DragonAttack();
  void RiddlerAttack();
  void ClearBoard();
  void ConfigureGameByIndex(int pGameIndex);

  GameTile* mGrid[kGridWidth][kGridHeight];
  GameTile* mOwnedTiles;
  GameTile* mTileStack;
  peanutbutter::fast_rand::FastRand mFastRand;
  GamePlayDirector* mGamePlayDirector;
  MoveBehavior mMoveBehavior;
  MovePolicy mMovePolicy;
  MatchBehavior mMatchBehavior;
  bool mIsCascading;

  unsigned int mCataclysmWriteIndex;
  unsigned int mApocalypseWriteIndex;
  bool mHasPendingMatches;

  int mSuccessfulMoveCount;
  int mBrokenCount;
  int mGameIndex;
  const char* mGameName;

  int mMoveListX[kMoveListCapacity];
  int mMoveListY[kMoveListCapacity];
  bool mMoveListHorizontal[kMoveListCapacity];
  int mMoveListDir[kMoveListCapacity];
  int mMoveListScore[kMoveListCapacity];
  int mMoveListCount;

  int mExploreListX[kGridSize];
  int mExploreListY[kGridSize];
  int mExploreListCount;
  int mMatchListX[kGridSize];
  int mMatchListY[kGridSize];
  int mStackX[kGridSize];
  int mStackY[kGridSize];
  int mComponentX[kGridSize];
  int mComponentY[kGridSize];
  unsigned char mVisited[kGridSize];
  RuntimeStats mRuntimeStats;
};

}  // namespace peanutbutter::games

#endif  // BREAD_SRC_GAMES_GAMEBOARD_HPP_
