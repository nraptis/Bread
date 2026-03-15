#ifndef BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_ISLANDS_HPP_
#define BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_ISLANDS_HPP_

#include "src/expansion/key_expansion/PasswordExpanderB.hpp"
#include "src/games/engine/GameBoard_Island.hpp"

namespace bread::games {

class MatchThreeTapIslands final : public GameBoard_Tap, public GameBoard_Island {
 public:
  explicit MatchThreeTapIslands(bread::rng::Counter* pCounter);
  void Seed(unsigned char* pPassword, int pPasswordLength) override;

 protected:
  bool AttemptMove() override;

 private:
  bread::expansion::key_expansion::PasswordExpanderB mOwnedPasswordExpander;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_ISLANDS_HPP_
