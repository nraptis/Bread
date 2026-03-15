#ifndef BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SWAP_STREAKS_HPP_
#define BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SWAP_STREAKS_HPP_

#include "src/expansion/key_expansion/PasswordExpanderC.hpp"
#include "src/games/engine/GameBoard_Streak.hpp"

namespace bread::games {

class MatchThreeSwapStreaks final : public GameBoard_Swap, public GameBoard_Streak {
 public:
  explicit MatchThreeSwapStreaks(bread::rng::Counter* pCounter);
  void Seed(unsigned char* pPassword, int pPasswordLength) override;

 protected:
  bool AttemptMove() override;

 private:
  bread::expansion::key_expansion::PasswordExpanderC mOwnedPasswordExpander;
  void SwapTiles(int pX1, int pY1, int pX2, int pY2);
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SWAP_STREAKS_HPP_
