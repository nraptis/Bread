#include "GamePowerUp.hpp"

#include "GameBoard.hpp"

namespace peanutbutter::games {

namespace {

bool IsInBounds(int pX, int pY) {
  return pX >= 0 && pX < GameBoard::kGridWidth && pY >= 0 && pY < GameBoard::kGridHeight;
}

void MarkMatched(GameBoard* pBoard, int pX, int pY) {
  if (pBoard == nullptr || !IsInBounds(pX, pY)) {
    return;
  }
  pBoard->MarkTileMatched(pX, pY);
}

}  // namespace

void GamePowerUp_ZoneBomb::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  for (int aY = pGridY - 1; aY <= pGridY + 1; ++aY) {
    for (int aX = pGridX - 1; aX <= pGridX + 1; ++aX) {
      MarkMatched(pBoard, aX, aY);
    }
  }
}

void GamePowerUp_Rocket::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
    MarkMatched(pBoard, aX, pGridY);
  }
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    MarkMatched(pBoard, pGridX, aY);
  }
}

void GamePowerUp_ColorBomb::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  if (pBoard == nullptr || !IsInBounds(pGridX, pGridY)) {
    return;
  }
  const GameTile* aCenter = pBoard->mGrid[pGridX][pGridY];
  if (aCenter == nullptr) {
    return;
  }

  const unsigned char aTargetType = aCenter->mType;
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      const GameTile* aTile = pBoard->mGrid[aX][aY];
      if (aTile != nullptr && aTile->mType == aTargetType) {
        MarkMatched(pBoard, aX, aY);
      }
    }
  }
}

void GamePowerUp_CrossBomb::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  MarkMatched(pBoard, pGridX, pGridY);

  int aX = pGridX - 1;
  int aY = pGridY - 1;
  while (IsInBounds(aX, aY)) {
    MarkMatched(pBoard, aX, aY);
    --aX;
    --aY;
  }

  aX = pGridX + 1;
  aY = pGridY - 1;
  while (IsInBounds(aX, aY)) {
    MarkMatched(pBoard, aX, aY);
    ++aX;
    --aY;
  }

  aX = pGridX - 1;
  aY = pGridY + 1;
  while (IsInBounds(aX, aY)) {
    MarkMatched(pBoard, aX, aY);
    --aX;
    ++aY;
  }

  aX = pGridX + 1;
  aY = pGridY + 1;
  while (IsInBounds(aX, aY)) {
    MarkMatched(pBoard, aX, aY);
    ++aX;
    ++aY;
  }
}

void GamePowerUp_PlasmaBeamV::Apply(GameBoard* pBoard, int pGridX, int /*pGridY*/) {
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    MarkMatched(pBoard, pGridX - 1, aY);
    MarkMatched(pBoard, pGridX, aY);
    MarkMatched(pBoard, pGridX + 1, aY);
  }
}

void GamePowerUp_PlasmaBeamH::Apply(GameBoard* pBoard, int /*pGridX*/, int pGridY) {
  for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
    MarkMatched(pBoard, aX, pGridY - 1);
    MarkMatched(pBoard, aX, pGridY);
    MarkMatched(pBoard, aX, pGridY + 1);
  }
}

void GamePowerUp_PlasmaBeamQuad::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    MarkMatched(pBoard, pGridX - 1, aY);
    MarkMatched(pBoard, pGridX, aY);
    MarkMatched(pBoard, pGridX + 1, aY);
  }
  for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
    MarkMatched(pBoard, aX, pGridY - 1);
    MarkMatched(pBoard, aX, pGridY);
    MarkMatched(pBoard, aX, pGridY + 1);
  }
}

void GamePowerUp_Nuke::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  for (int aY = pGridY - 2; aY <= pGridY + 2; ++aY) {
    for (int aX = pGridX - 2; aX <= pGridX + 2; ++aX) {
      MarkMatched(pBoard, aX, aY);
    }
  }
}

void GamePowerUp_CornerBomb::Apply(GameBoard* pBoard, int /*pGridX*/, int /*pGridY*/) {
  for (int aY = 0; aY < 4; ++aY) {
    for (int aX = 0; aX < 4; ++aX) {
      MarkMatched(pBoard, aX, aY);
      MarkMatched(pBoard, GameBoard::kGridWidth - 1 - aX, aY);
      MarkMatched(pBoard, aX, GameBoard::kGridHeight - 1 - aY);
      MarkMatched(pBoard, GameBoard::kGridWidth - 1 - aX, GameBoard::kGridHeight - 1 - aY);
    }
  }
}

void GamePowerUp_VerticalBombs::Apply(GameBoard* pBoard, int /*pGridX*/, int /*pGridY*/) {
  if (pBoard == nullptr) {
    return;
  }
  const int aParity = static_cast<int>(pBoard->GetRand(2));
  for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
    if ((aX & 1) != aParity) {
      continue;
    }
    for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
      MarkMatched(pBoard, aX, aY);
    }
  }
}

void GamePowerUp_HorizontalBombs::Apply(GameBoard* pBoard, int /*pGridX*/, int /*pGridY*/) {
  if (pBoard == nullptr) {
    return;
  }
  const int aParity = static_cast<int>(pBoard->GetRand(2));
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    if ((aY & 1) != aParity) {
      continue;
    }
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      MarkMatched(pBoard, aX, aY);
    }
  }
}

}  // namespace peanutbutter::games
