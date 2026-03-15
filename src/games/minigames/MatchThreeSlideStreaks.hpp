#ifndef BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SLIDE_STREAKS_HPP_
#define BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SLIDE_STREAKS_HPP_

#include "src/expansion/key_expansion/PasswordExpanderE.hpp"
#include "src/games/engine/GameBoard_Streak.hpp"

namespace bread::games {

class MatchThreeSlideStreaks final : public GameBoard_Slide, public GameBoard_Streak {
 public:
  explicit MatchThreeSlideStreaks(bread::rng::Counter* pCounter);
  void Seed(unsigned char* pPassword, int pPasswordLength) override;

 protected:
  bool AttemptMove() override;

 private:
  bread::expansion::key_expansion::PasswordExpanderE mOwnedPasswordExpander;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SLIDE_STREAKS_HPP_
