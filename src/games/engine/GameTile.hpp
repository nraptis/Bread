#ifndef BREAD_SRC_GAMES_ENGINE_GAMETILE_HPP_
#define BREAD_SRC_GAMES_ENGINE_GAMETILE_HPP_

namespace bread::games {

class GameTile {
 public:
  GameTile() = default;

  GameTile(int pGridX, int pGridY, unsigned char pByte, unsigned char pType)
      : mGridX(pGridX), mGridY(pGridY), mByte(pByte), mType(pType), mIsMatched(false), mIsNew(false) {}

  int mGridX = 0;
  int mGridY = 0;
  unsigned char mByte = 0U;
  unsigned char mType = 0U;
  bool mIsMatched = false;
  bool mIsNew = false;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_ENGINE_GAMETILE_HPP_
