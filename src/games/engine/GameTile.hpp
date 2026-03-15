#ifndef BREAD_SRC_GAMES_ENGINE_GAMETILE_HPP_
#define BREAD_SRC_GAMES_ENGINE_GAMETILE_HPP_

namespace bread::games {

enum class GamePowerUpType : unsigned char {
  kNone = 0U,
  kZoneBomb = 1U,
  kRocket = 2U,
  kColorBomb = 3U,
  kCrossBomb = 4U,
  kPlasmaBeamH = 5U,
  kPlasmaBeamV = 6U,
  kPlasmaBeamQuad = 7U,
  kNuke = 8U,
  kCornerBomb = 9U,
  kVerticalBombs = 10U,
  kHorizontalBombs = 11U
};

class GameTile {
 public:
  GameTile() = default;

  GameTile(int pGridX,
           int pGridY,
           unsigned char pByte,
           unsigned char pType,
           GamePowerUpType pPowerUpType = GamePowerUpType::kNone)
      : mGridX(pGridX),
        mGridY(pGridY),
        mByte(pByte),
        mType(pType),
        mPowerUpType(pPowerUpType),
        mIsMatched(false),
        mIsNew(false),
        mDidTopple(false) {}

  int mGridX = 0;
  int mGridY = 0;
  unsigned char mByte = 0U;
  unsigned char mType = 0U;
  GamePowerUpType mPowerUpType = GamePowerUpType::kNone;
  bool mIsMatched = false;
  bool mIsNew = false;
  bool mDidTopple = false;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_ENGINE_GAMETILE_HPP_
