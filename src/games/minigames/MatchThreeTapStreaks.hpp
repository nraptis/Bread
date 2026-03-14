#ifndef BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_STREAKS_HPP_
#define BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_STREAKS_HPP_

#include "src/games/engine/GameBoard.hpp"

namespace bread::games {

class MatchThreeTapStreaks final : public GameBoard {
 public:
  explicit MatchThreeTapStreaks(bread::rng::Counter* pCounter);
  void Seed(unsigned char* pPassword, int pPasswordLength) override;

 protected:
  bool AttemptMove() override;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_TAP_STREAKS_HPP_
