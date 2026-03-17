#ifndef BREAD_TESTS_COMMON_GAME_CATALOG_HPP_
#define BREAD_TESTS_COMMON_GAME_CATALOG_HPP_

#include <array>

#include "src/Tables/games/engine/GameBoard.hpp"

namespace peanutbutter::tests::games {

struct GameCatalogEntry {
  peanutbutter::games::GameBoard::GameIndex mGameIndex;
  int mSalt;
};

inline constexpr std::array<GameCatalogEntry, peanutbutter::games::GameBoard::kGameCount> kAllGames = {{
    {peanutbutter::games::GameBoard::StreakSwapGreedy, 11},
    {peanutbutter::games::GameBoard::StreakSlideGreedy, 13},
    {peanutbutter::games::GameBoard::IslandSwapGreedy, 17},
    {peanutbutter::games::GameBoard::IslandSlideGreedy, 19},
    {peanutbutter::games::GameBoard::StreakTapGreedy, 23},
    {peanutbutter::games::GameBoard::IslandTapGreedy, 29},
    {peanutbutter::games::GameBoard::StreakSwapRandom, 31},
    {peanutbutter::games::GameBoard::StreakSlideRandom, 37},
    {peanutbutter::games::GameBoard::IslandSwapRandom, 41},
    {peanutbutter::games::GameBoard::IslandSlideRandom, 43},
    {peanutbutter::games::GameBoard::StreakTapRandom, 47},
    {peanutbutter::games::GameBoard::IslandTapRandom, 53},
    {peanutbutter::games::GameBoard::StreakSwapFirst, 59},
    {peanutbutter::games::GameBoard::StreakSlideFirst, 61},
    {peanutbutter::games::GameBoard::IslandSwapFirst, 67},
    {peanutbutter::games::GameBoard::IslandSlideFirst, 71},
}};

}  // namespace peanutbutter::tests::games

#endif  // BREAD_TESTS_COMMON_GAME_CATALOG_HPP_
