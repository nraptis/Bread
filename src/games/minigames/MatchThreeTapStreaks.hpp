#ifndef BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_STREAKS_HPP_
#define BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_STREAKS_HPP_

#include "src/expansion/key_expansion/PasswordExpanderA.hpp"
#include "src/games/engine/GameBoard_Streak.hpp"

namespace bread::games {

class MatchThreeTapStreaks final : public GameBoard_Tap, public GameBoard_Streak {
 public:
  explicit MatchThreeTapStreaks(bread::rng::Counter* pCounter);
  void Seed(unsigned char* pPassword, int pPasswordLength) override;

 protected:
  bool AttemptMove() override;

 private:
  bread::expansion::key_expansion::PasswordExpanderA mOwnedPasswordExpander;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_STREAKS_HPP_
