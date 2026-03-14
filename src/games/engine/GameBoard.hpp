#ifndef BREAD_SRC_GAMES_ENGINE_GAMEBOARD_HPP_
#define BREAD_SRC_GAMES_ENGINE_GAMEBOARD_HPP_

#include <cstddef>

#include "src/games/engine/GameTile.hpp"
#include "src/rng/Counter.hpp"
#include "src/rng/Digest.hpp"

namespace bread::games {

enum MatchRequiredMode {
  kRequiredToExist,
  kRequiredToNotExist
};

enum MatchTypeMode {
  kStreak,
  kIsland
};

class GameBoard : public bread::rng::Digest {
 public:
  static constexpr int kGridWidth = 8;
  static constexpr int kGridHeight = 8;
  static constexpr int kGridSize = kGridWidth * kGridHeight;
  static constexpr int kSeedBufferCapacity = 8192;

  explicit GameBoard(bread::rng::Counter* pCounter,
                     MatchRequiredMode pMatchRequiredMode,
                     MatchTypeMode pMatchType);
  ~GameBoard() override;

  using bread::rng::Digest::Get;
  virtual void Seed(unsigned char* pPassword, int pPasswordLength) override = 0;
  void Get(unsigned char* pDestination, int pDestinationLength) override;
  unsigned char Get();

  bool SeedCanDequeue() const;
  unsigned char SeedDequeue();
  bool Empty() const;
  bool HasAnyMatches();

  void Stuck();
  void Match();
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

  bool IsMatchStreak(int pGridX, int pGridY);
  bool IsMatchIsland(int pGridX, int pGridY);

  void Spawn_EnsureNoMatchesExistStreak();
  void Spawn_EnsureNoMatchesExistIsland();
  void Topple_EnsureNoMatchesExistStreak();
  void Topple_EnsureNoMatchesExistIsland();
  void Topple_EnsureMatchesExistStreak();
  void Topple_EnsureMatchesExistIsland();
  void MoveListClear();
  bool MoveListPush(int pX, int pY, bool pHorizontal, int pDir);
  int MoveListPickIndex() const;

 protected:
  void MatchesBegin();
  bool MatchCheckStreak(int pGridX, int pGridY);
  bool MatchCheckIsland(int pGridX, int pGridY);
  int MatchesCommit();
  bool HasPendingMatches() const;

  void InitializeSeed(unsigned char* pPassword, int pPasswordLength);
  void SetTypeCount(int pTypeCount);
  virtual bool AttemptMove() = 0;
  bool HasAnyTiles() const;

  GameTile* GenerateTile(int pGridX, int pGridY);
  unsigned char NextTypeByte();
  void EnqueueByte(unsigned char pByte);
  void ClearBoard();

  GameTile* mGrid[kGridWidth][kGridHeight];
  bread::rng::Counter* mCounter;
  int mTypeCount;
  MatchRequiredMode mMatchRequiredMode;
  MatchTypeMode mMatchType;

  unsigned char mBuffer[kSeedBufferCapacity];
  int mBufferLength;
  int mGetCursor;
  int mPutCursor;
  bool mHasPendingMatches;

  int mSuccessfulMoveCount;
  int mBrokenCount;

  int mMoveListX[8 * 8 * 4];
  int mMoveListY[8 * 8 * 4];
  bool mMoveListHorizontal[8 * 8 * 4];
  int mMoveListDir[8 * 8 * 4];
  int mMoveListCount;

  int mMatchCheckStack[kGridSize];
  int mMatchComponent[kGridSize];
  bool mMatchVisited[kGridSize];
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_ENGINE_GAMEBOARD_HPP_
