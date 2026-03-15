#ifndef BREAD_SRC_GAMES_ENGINE_GAMEBOARD_STREAK_HPP_
#define BREAD_SRC_GAMES_ENGINE_GAMEBOARD_STREAK_HPP_

#include "src/games/engine/GameBoard.hpp"

namespace bread::games {

class GameBoard_Streak : public virtual GameBoard {
 public:
  bool IsMatch(int pGridX, int pGridY) override;
  void Match() override;

 protected:
  GameBoard_Streak() = default;
  bool MatchMark(int pGridX, int pGridY) override;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_ENGINE_GAMEBOARD_STREAK_HPP_
