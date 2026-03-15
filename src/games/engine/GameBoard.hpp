#ifndef BREAD_SRC_GAMES_ENGINE_GAMEBOARD_HPP_
#define BREAD_SRC_GAMES_ENGINE_GAMEBOARD_HPP_

#include <cstddef>
#include <cstdint>

#include "src/expansion/key_expansion/PasswordExpander.hpp"
#include "src/fast_rand/FastRand.hpp"
#include "src/games/engine/GameTile.hpp"
#include "src/rng/Counter.hpp"
#include "src/rng/Digest.hpp"

namespace bread::games {

class GamePlayDirector;
class GamePlayDirector_CollapseStyle;
class GamePlayDirector_BejeweledStyle;
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

enum PlayTypeMode {
  kSlide,
  kSwap,
  kTap
};

class GameBoard : public bread::rng::Digest {
 public:
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
  static constexpr int kSeedBufferCapacity = PASSWORD_EXPANDED_SIZE;
  static constexpr int kMaxLockedThreshold = 32;
  static constexpr int kSuspendedThreshold = 4;
  static constexpr unsigned char kPowerUpSpawnChance = 4U;
  static constexpr int kMoveListCapacity =
      (kGridWidth * (kGridHeight - 1)) + (kGridHeight * (kGridWidth - 1));

  GameBoard(bread::rng::Counter* pCounter,
            bread::expansion::key_expansion::PasswordExpander* pPasswordExpander);
  ~GameBoard() override;

  using bread::rng::Digest::Get;
  virtual void Seed(unsigned char* pPassword, int pPasswordLength) override = 0;
  void Get(unsigned char* pDestination, int pDestinationLength) override;

  bool SeedCanDequeue() const;
  unsigned char SeedDequeue();
  bool Empty() const;
  bool HasAnyMatches();
  virtual bool IsMatch(int pGridX, int pGridY);
  virtual int GetMatches();
  RuntimeStats GetRuntimeStats() const;

  void Stuck();
  virtual void Match();
  void Topple();
  void Spawn();
  void Play();
  virtual bool MakeMoves();

  int SuccessfulMoveCount() const;
  int BrokenCount() const;

  void SlideRow(int pRowIndex, int pAmount);
  void SlideColumn(int pColumnIndex, int pAmount);

  void InvalidateNew();
  void InvalidateMatches();
  void InvalidateToppleFlags();
  void Cascade();

  void MoveListClear();
  bool MoveListPush(int pX, int pY, bool pHorizontal, int pDir);
  int MoveListPickIndex();

 protected:
  friend class GamePlayDirector;
  friend class GamePlayDirector_CollapseStyle;
  friend class GamePlayDirector_BejeweledStyle;
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
  void SetCounter(bread::rng::Counter* pCounter);
  void SetPasswordExpander(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander);
  void SetTypeCount(int pTypeCount);
  void SetPlayTypeMode(PlayTypeMode pPlayTypeMode);
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
  virtual bool MatchMark(int pGridX, int pGridY);
  virtual bool AttemptMove() = 0;
  bool HasAnyTiles() const;
  int ActiveTileCount() const;
  void ToppleStep();
  bool CascadeStep();
  bool PlayMainLoop();

  GameTile* GenerateTile(int pGridX, int pGridY);
  unsigned char GetRand(int pMax);
  unsigned char NextTypeByte();
  void EnqueueByte(unsigned char pByte);
  void ShuffleSeedBuffer();
  void DragonAttack();
  void RiddlerAttack();
  void ClearBoard();

  GameTile* mGrid[kGridWidth][kGridHeight];
  bread::rng::Counter* mCounter;
  bread::fast_rand::FastRand mFastRand;
  GamePlayDirector* mGamePlayDirector;
  PlayTypeMode mPlayTypeMode;
  bool mIsCascading;

  unsigned char mSeedBuffer[kSeedBufferCapacity];
  unsigned char* mResultBuffer;
  unsigned int mResultBufferWriteIndex;
  unsigned int mCataclysmWriteIndex;
  unsigned int mApocalypseWriteIndex;
  std::uint64_t mResultBufferWriteProgress;
  unsigned int mResultBufferReadIndex;
  unsigned int mResultBufferLength;
  unsigned int mSeedBytesRemaining;
  bool mHasPendingMatches;

  int mSuccessfulMoveCount;
  int mBrokenCount;

  int mMoveListX[kMoveListCapacity];
  int mMoveListY[kMoveListCapacity];
  bool mMoveListHorizontal[kMoveListCapacity];
  int mMoveListDir[kMoveListCapacity];
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

class GameBoard_Slide : public virtual GameBoard {
 protected:
  GameBoard_Slide();
};

class GameBoard_Swap : public virtual GameBoard {
 protected:
  GameBoard_Swap();
};

class GameBoard_Tap : public virtual GameBoard {
 protected:
  GameBoard_Tap();
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_ENGINE_GAMEBOARD_HPP_
