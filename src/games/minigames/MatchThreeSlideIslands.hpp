#ifndef BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SLIDE_ISLANDS_HPP_
#define BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SLIDE_ISLANDS_HPP_

#include "src/games/engine/GameBoard.hpp"

namespace bread::games {

class MatchThreeSlideIslands final : public GameBoard {
 public:
  explicit MatchThreeSlideIslands(bread::rng::Counter* pCounter);
  void Seed(unsigned char* pPassword, int pPasswordLength) override;

 protected:
  bool AttemptMove() override;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_MINIGAMES_MATCH_THREE_SLIDE_ISLANDS_HPP_
