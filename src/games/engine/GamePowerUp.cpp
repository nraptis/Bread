#include "src/games/engine/GamePowerUp.hpp"

#include "src/games/engine/GameBoard.hpp"

namespace bread::games {

namespace {

bool IsInBounds(int pX, int pY) {
  return pX >= 0 && pX < GameBoard::kGridWidth && pY >= 0 && pY < GameBoard::kGridHeight;
}

}  // namespace

void GamePowerUp_ZoneBomb::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };
  for (int aY = pGridY - 1; aY <= pGridY + 1; ++aY) {
    for (int aX = pGridX - 1; aX <= pGridX + 1; ++aX) {
      MarkTile(aX, aY);
    }
  }
}

void GamePowerUp_Rocket::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };
  for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
    MarkTile(aX, pGridY);
  }
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    MarkTile(pGridX, aY);
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
      GameTile* aTile = pBoard->mGrid[aX][aY];
      if (aTile != nullptr && aTile->mType == aTargetType) {
        aTile->mIsMatched = true;
        pBoard->mHasPendingMatches = true;
      }
    }
  }
}

void GamePowerUp_CrossBomb::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  if (pBoard == nullptr || !IsInBounds(pGridX, pGridY)) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };

  MarkTile(pGridX, pGridY);

  int aX = pGridX - 1;
  int aY = pGridY - 1;
  while (IsInBounds(aX, aY)) {
    MarkTile(aX, aY);
    --aX;
    --aY;
  }

  aX = pGridX + 1;
  aY = pGridY - 1;
  while (IsInBounds(aX, aY)) {
    MarkTile(aX, aY);
    ++aX;
    --aY;
  }

  aX = pGridX - 1;
  aY = pGridY + 1;
  while (IsInBounds(aX, aY)) {
    MarkTile(aX, aY);
    --aX;
    ++aY;
  }

  aX = pGridX + 1;
  aY = pGridY + 1;
  while (IsInBounds(aX, aY)) {
    MarkTile(aX, aY);
    ++aX;
    ++aY;
  }
}

void GamePowerUp_PlasmaBeamV::Apply(GameBoard* pBoard, int pGridX, int /*pGridY*/) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    MarkTile(pGridX - 1, aY);
    MarkTile(pGridX, aY);
    MarkTile(pGridX + 1, aY);
  }
}

void GamePowerUp_PlasmaBeamH::Apply(GameBoard* pBoard, int /*pGridX*/, int pGridY) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };
  for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
    MarkTile(aX, pGridY - 1);
    MarkTile(aX, pGridY);
    MarkTile(aX, pGridY + 1);
  }
}

void GamePowerUp_PlasmaBeamQuad::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    MarkTile(pGridX - 1, aY);
    MarkTile(pGridX, aY);
    MarkTile(pGridX + 1, aY);
  }
  for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
    MarkTile(aX, pGridY - 1);
    MarkTile(aX, pGridY);
    MarkTile(aX, pGridY + 1);
  }
}

void GamePowerUp_Nuke::Apply(GameBoard* pBoard, int pGridX, int pGridY) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };
  for (int aY = pGridY - 2; aY <= pGridY + 2; ++aY) {
    for (int aX = pGridX - 2; aX <= pGridX + 2; ++aX) {
      MarkTile(aX, aY);
    }
  }
}

void GamePowerUp_CornerBomb::Apply(GameBoard* pBoard, int /*pGridX*/, int /*pGridY*/) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };

  for (int aY = 0; aY < 4; ++aY) {
    for (int aX = 0; aX < 4; ++aX) {
      MarkTile(aX, aY);
      MarkTile(GameBoard::kGridWidth - 1 - aX, aY);
      MarkTile(aX, GameBoard::kGridHeight - 1 - aY);
      MarkTile(GameBoard::kGridWidth - 1 - aX, GameBoard::kGridHeight - 1 - aY);
    }
  }
}

void GamePowerUp_VerticalBombs::Apply(GameBoard* pBoard, int /*pGridX*/, int /*pGridY*/) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };

  const int aParity = static_cast<int>(pBoard->GetRand(2));  // 0 even, 1 odd
  for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
    if ((aX & 1) != aParity) {
      continue;
    }
    for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
      MarkTile(aX, aY);
    }
  }
}

void GamePowerUp_HorizontalBombs::Apply(GameBoard* pBoard, int /*pGridX*/, int /*pGridY*/) {
  if (pBoard == nullptr) {
    return;
  }
  auto MarkTile = [&](int pX, int pY) {
    if (!IsInBounds(pX, pY)) {
      return;
    }
    GameTile* aTile = pBoard->mGrid[pX][pY];
    if (aTile == nullptr) {
      return;
    }
    aTile->mIsMatched = true;
    pBoard->mHasPendingMatches = true;
  };

  const int aParity = static_cast<int>(pBoard->GetRand(2));  // 0 even, 1 odd
  for (int aY = 0; aY < GameBoard::kGridHeight; ++aY) {
    if ((aY & 1) != aParity) {
      continue;
    }
    for (int aX = 0; aX < GameBoard::kGridWidth; ++aX) {
      MarkTile(aX, aY);
    }
  }
}

}  // namespace bread::games
