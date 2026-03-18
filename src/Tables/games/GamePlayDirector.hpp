#ifndef BREAD_SRC_GAMES_GAMEPLAYDIRECTOR_HPP_
#define BREAD_SRC_GAMES_GAMEPLAYDIRECTOR_HPP_

namespace peanutbutter::games {

class GameBoard;

class GamePlayDirector {
 public:
  virtual ~GamePlayDirector() = default;
  virtual bool BuildExploreList(GameBoard* pBoard) = 0;
  virtual bool ResolveBoardState_PostSpawn(GameBoard* pBoard) = 0;
  virtual bool ResolveBoardState_PostTopple(GameBoard* pBoard) = 0;

  static GamePlayDirector* CollapseStyle();
  static GamePlayDirector* MatchThreeStyle();

 protected:
  static bool BuildExploreList_AllTiles(GameBoard* pBoard);
  static bool BuildExploreList_NewTiles(GameBoard* pBoard);
  static bool BuildExploreList_MatchedTiles(GameBoard* pBoard);
  static void RandomizeAllTileTypes(GameBoard* pBoard);
  static void RandomFlipOneTileType(GameBoard* pBoard, int pX, int pY);
  static bool GuaranteeMatchDoesNotExist(GameBoard* pBoard);
  static bool GuaranteeMatchDoesExist(GameBoard* pBoard);
  static bool GuaranteeMoveExistsForNewTiles_MatchDoesExist(GameBoard* pBoard);
  static bool GuaranteeMoveExistsForNewTiles_MatchDoesNotExist(GameBoard* pBoard);
};

class GamePlayDirector_CollapseStyle final : public GamePlayDirector {
 public:
  bool BuildExploreList(GameBoard* pBoard) override;
  bool ResolveBoardState_PostSpawn(GameBoard* pBoard) override;
  bool ResolveBoardState_PostTopple(GameBoard* pBoard) override;
};

class GamePlayDirector_MatchThreeStyle final : public GamePlayDirector {
 public:
  bool BuildExploreList(GameBoard* pBoard) override;
  bool ResolveBoardState_PostSpawn(GameBoard* pBoard) override;
  bool ResolveBoardState_PostTopple(GameBoard* pBoard) override;
};

}  // namespace peanutbutter::games

#endif  // BREAD_SRC_GAMES_GAMEPLAYDIRECTOR_HPP_
