#ifndef BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SWAP_ISLANDS_HPP_
#define BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SWAP_ISLANDS_HPP_

#include "src/expansion/key_expansion/PasswordExpanderD.hpp"
#include "src/games/engine/GameBoard_Island.hpp"

namespace bread::games {

class MatchThreeSwapIslands final : public GameBoard_Swap, public GameBoard_Island {
 public:
  explicit MatchThreeSwapIslands(bread::rng::Counter* pCounter);
  void Seed(unsigned char* pPassword, int pPasswordLength) override;

 protected:
  bool AttemptMove() override;

 private:
  bread::expansion::key_expansion::PasswordExpanderD mOwnedPasswordExpander;
  void SwapTiles(int pX1, int pY1, int pX2, int pY2);
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SWAP_ISLANDS_HPP_
