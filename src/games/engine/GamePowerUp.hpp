#ifndef BREAD_SRC_GAMES_ENGINE_GAMEPOWERUP_HPP_
#define BREAD_SRC_GAMES_ENGINE_GAMEPOWERUP_HPP_

namespace bread::games {

class GameBoard;

class GamePowerUp {
 public:
  virtual ~GamePowerUp() = default;
  virtual void Apply(GameBoard* pBoard, int pGridX, int pGridY) = 0;
};

class GamePowerUp_ZoneBomb final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_Rocket final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_ColorBomb final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_CrossBomb final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_PlasmaBeamH final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_PlasmaBeamV final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_PlasmaBeamQuad final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_Nuke final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_CornerBomb final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_VerticalBombs final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

class GamePowerUp_HorizontalBombs final : public GamePowerUp {
 public:
  void Apply(GameBoard* pBoard, int pGridX, int pGridY) override;
};

}  // namespace bread::games

#endif  // BREAD_SRC_GAMES_ENGINE_GAMEPOWERUP_HPP_
