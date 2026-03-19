#ifndef BREAD_TESTS_COMMON_GAME_CATALOG_HPP_
#define BREAD_TESTS_COMMON_GAME_CATALOG_HPP_

#include <array>

#include "src/Tables/games/GameBoard.hpp"

namespace peanutbutter::tests::games {

struct GameCatalogEntry {
  peanutbutter::games::GameBoard::GameIndex mGameIndex;
  int mSalt;
};

inline constexpr std::array<GameCatalogEntry, peanutbutter::games::GameBoard::kGameCount> kAllGames = {{
    {peanutbutter::games::GameBoard::IslandSwapFourGreedy, 23},
    {peanutbutter::games::GameBoard::IslandTapFourGreedy, 29},
    {peanutbutter::games::GameBoard::StreakSwapFourGreedy, 31},
    {peanutbutter::games::GameBoard::StreakTapFourGreedy, 37},
    {peanutbutter::games::GameBoard::IslandSwapFiveGreedy, 11},
    {peanutbutter::games::GameBoard::IslandTapFiveGreedy, 13},
    {peanutbutter::games::GameBoard::StreakSwapFiveGreedy, 17},
    {peanutbutter::games::GameBoard::StreakTapFiveGreedy, 19},
    {peanutbutter::games::GameBoard::IslandSwapFourRandom, 59},
    {peanutbutter::games::GameBoard::IslandTapFourRandom, 61},
    {peanutbutter::games::GameBoard::StreakSwapFourRandom, 67},
    {peanutbutter::games::GameBoard::StreakTapFourRandom, 71},
    {peanutbutter::games::GameBoard::IslandSwapFiveRandom, 41},
    {peanutbutter::games::GameBoard::IslandTapFiveRandom, 43},
    {peanutbutter::games::GameBoard::StreakSwapFiveRandom, 47},
    {peanutbutter::games::GameBoard::StreakTapFiveRandom, 53},
}};

}  // namespace peanutbutter::tests::games

#endif  // BREAD_TESTS_COMMON_GAME_CATALOG_HPP_
