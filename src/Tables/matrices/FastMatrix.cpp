#include "FastMatrix.hpp"

#include <cstdint>
#include <cstring>

namespace peanutbutter::matrices {

namespace {

bool IsValidIndex(int pIndex) {
  return (pIndex >= 0 && pIndex < 8);
}

}  // namespace

FastMatrix::FastMatrix() : mData{} {
  std::memset(mData, 0, sizeof(mData));
}

unsigned char* FastMatrix::Data() {
  return &mData[0][0];
}

const unsigned char* FastMatrix::Data() const {
  return &mData[0][0];
}

unsigned char (&FastMatrix::Rows())[kHeight][kWidth] {
  return mData;
}

const unsigned char (&FastMatrix::Rows() const)[kHeight][kWidth] {
  return mData;
}

void FastMatrix::Clear() {
  std::memset(mData, 0, sizeof(mData));
}

void FastMatrix::Fill(unsigned char pValue) {
  for (int aY = 0; aY < 8; ++aY) {
    for (int aX = 0; aX < 8; ++aX) {
      mData[aY][aX] = pValue;
    }
  }
}

void FastMatrix::ShiftRow(int pIndex, int pAmount) {
  if (!IsValidIndex(pIndex)) {
    return;
  }
  int aShift = pAmount % 8;
  if (aShift < 0) {
    aShift += 8;
  }
  if (aShift == 0) {
    return;
  }
  unsigned char aTemp[8];
  for (int aX = 0; aX < 8; ++aX) {
    aTemp[(aX + aShift) & 7] = mData[pIndex][aX];
  }
  for (int aX = 0; aX < 8; ++aX) {
    mData[pIndex][aX] = aTemp[aX];
  }
}

void FastMatrix::ShiftRowRight1(int pIndex) {
  if (!IsValidIndex(pIndex)) {
    return;
  }
  const unsigned char aTail = mData[pIndex][7];
  mData[pIndex][7] = mData[pIndex][6];
  mData[pIndex][6] = mData[pIndex][5];
  mData[pIndex][5] = mData[pIndex][4];
  mData[pIndex][4] = mData[pIndex][3];
  mData[pIndex][3] = mData[pIndex][2];
  mData[pIndex][2] = mData[pIndex][1];
  mData[pIndex][1] = mData[pIndex][0];
  mData[pIndex][0] = aTail;
}

void FastMatrix::ShiftRowLeft1(int pIndex) {
  if (!IsValidIndex(pIndex)) {
    return;
  }
  const unsigned char aHead = mData[pIndex][0];
  mData[pIndex][0] = mData[pIndex][1];
  mData[pIndex][1] = mData[pIndex][2];
  mData[pIndex][2] = mData[pIndex][3];
  mData[pIndex][3] = mData[pIndex][4];
  mData[pIndex][4] = mData[pIndex][5];
  mData[pIndex][5] = mData[pIndex][6];
  mData[pIndex][6] = mData[pIndex][7];
  mData[pIndex][7] = aHead;
}

void FastMatrix::ShiftColumn(int pIndex, int pAmount) {
  if (!IsValidIndex(pIndex)) {
    return;
  }
  int aShift = pAmount % 8;
  if (aShift < 0) {
    aShift += 8;
  }
  if (aShift == 0) {
    return;
  }
  unsigned char aTemp[8];
  for (int aY = 0; aY < 8; ++aY) {
    aTemp[(aY + aShift) & 7] = mData[aY][pIndex];
  }
  for (int aY = 0; aY < 8; ++aY) {
    mData[aY][pIndex] = aTemp[aY];
  }
}

void FastMatrix::ShiftColumnDown1(int pIndex) {
  if (!IsValidIndex(pIndex)) {
    return;
  }
  const unsigned char aTail = mData[7][pIndex];
  mData[7][pIndex] = mData[6][pIndex];
  mData[6][pIndex] = mData[5][pIndex];
  mData[5][pIndex] = mData[4][pIndex];
  mData[4][pIndex] = mData[3][pIndex];
  mData[3][pIndex] = mData[2][pIndex];
  mData[2][pIndex] = mData[1][pIndex];
  mData[1][pIndex] = mData[0][pIndex];
  mData[0][pIndex] = aTail;
}

void FastMatrix::ShiftColumnUp1(int pIndex) {
  if (!IsValidIndex(pIndex)) {
    return;
  }
  const unsigned char aHead = mData[0][pIndex];
  mData[0][pIndex] = mData[1][pIndex];
  mData[1][pIndex] = mData[2][pIndex];
  mData[2][pIndex] = mData[3][pIndex];
  mData[3][pIndex] = mData[4][pIndex];
  mData[4][pIndex] = mData[5][pIndex];
  mData[5][pIndex] = mData[6][pIndex];
  mData[6][pIndex] = mData[7][pIndex];
  mData[7][pIndex] = aHead;
}

void FastMatrix::SwapRows(int pIndex1, int pIndex2) {
  if (!IsValidIndex(pIndex1) || !IsValidIndex(pIndex2) || pIndex1 == pIndex2) {
    return;
  }
  std::uint64_t aRow1 = 0U;
  std::uint64_t aRow2 = 0U;
  std::memcpy(&aRow1, mData[pIndex1], sizeof(aRow1));
  std::memcpy(&aRow2, mData[pIndex2], sizeof(aRow2));
  std::memcpy(mData[pIndex1], &aRow2, sizeof(aRow2));
  std::memcpy(mData[pIndex2], &aRow1, sizeof(aRow1));
}

void FastMatrix::SwapColumns(int pIndex1, int pIndex2) {
  if (!IsValidIndex(pIndex1) || !IsValidIndex(pIndex2) || pIndex1 == pIndex2) {
    return;
  }
  for (int aY = 0; aY < 8; ++aY) {
    const unsigned char aTmp = mData[aY][pIndex1];
    mData[aY][pIndex1] = mData[aY][pIndex2];
    mData[aY][pIndex2] = aTmp;
  }
}

void FastMatrix::XorRows(int pIndex1, int pIndex2) {
  if (!IsValidIndex(pIndex1) || !IsValidIndex(pIndex2) || pIndex1 == pIndex2) {
    return;
  }
  for (int aX = 0; aX < 8; ++aX) {
    mData[pIndex1][aX] = static_cast<unsigned char>(mData[pIndex1][aX] ^ mData[pIndex2][aX]);
  }
}

void FastMatrix::XorColumns(int pIndex1, int pIndex2) {
  if (!IsValidIndex(pIndex1) || !IsValidIndex(pIndex2) || pIndex1 == pIndex2) {
    return;
  }
  for (int aY = 0; aY < 8; ++aY) {
    mData[aY][pIndex1] = static_cast<unsigned char>(mData[aY][pIndex1] ^ mData[aY][pIndex2]);
  }
}

void FastMatrix::AddRowConstant(int pIndex, unsigned char pValue) {
  if (!IsValidIndex(pIndex) || pValue == 0U) {
    return;
  }
  for (int aX = 0; aX < 8; ++aX) {
    mData[pIndex][aX] = static_cast<unsigned char>(mData[pIndex][aX] + pValue);
  }
}

void FastMatrix::AddColumnConstant(int pIndex, unsigned char pValue) {
  if (!IsValidIndex(pIndex) || pValue == 0U) {
    return;
  }
  for (int aY = 0; aY < 8; ++aY) {
    mData[aY][pIndex] = static_cast<unsigned char>(mData[aY][pIndex] + pValue);
  }
}

void FastMatrix::WeaveColumns(int pIndex1, int pIndex2) {
  if (!IsValidIndex(pIndex1) || !IsValidIndex(pIndex2) || pIndex1 == pIndex2) {
    return;
  }
  for (int aY = 1; aY < 8; aY += 2) {
    const unsigned char aTmp = mData[aY][pIndex1];
    mData[aY][pIndex1] = mData[aY][pIndex2];
    mData[aY][pIndex2] = aTmp;
  }
}

void FastMatrix::WeaveRows(int pIndex1, int pIndex2) {
  if (!IsValidIndex(pIndex1) || !IsValidIndex(pIndex2) || pIndex1 == pIndex2) {
    return;
  }
  for (int aX = 1; aX < 8; aX += 2) {
    const unsigned char aTmp = mData[pIndex1][aX];
    mData[pIndex1][aX] = mData[pIndex2][aX];
    mData[pIndex2][aX] = aTmp;
  }
}

void FastMatrix::MixRowsEven(int pIndex1, int pIndex2) {
  if (!IsValidIndex(pIndex1) || !IsValidIndex(pIndex2) || pIndex1 == pIndex2) {
    return;
  }
  for (int aX = 0; aX < 8; aX += 2) {
    const unsigned char aMix = static_cast<unsigned char>(mData[pIndex1][aX] + mData[pIndex2][aX]);
    mData[pIndex1][aX] = aMix;
    mData[pIndex2][aX] = aMix;
  }
}

void FastMatrix::MixColumnsOdd(int pIndex1, int pIndex2) {
  if (!IsValidIndex(pIndex1) || !IsValidIndex(pIndex2) || pIndex1 == pIndex2) {
    return;
  }
  for (int aY = 1; aY < 8; aY += 2) {
    const unsigned char aMix = static_cast<unsigned char>(mData[aY][pIndex1] + mData[aY][pIndex2]);
    mData[aY][pIndex1] = aMix;
    mData[aY][pIndex2] = aMix;
  }
}

void FastMatrix::RotateRight() {
  FlipDiagA();
  FlipH();
}

void FastMatrix::RotateLeft() {
  FlipDiagA();
  FlipV();
}

void FastMatrix::FlipH() {
  for (int aY = 0; aY < 8; ++aY) {
    unsigned char aTmp = mData[aY][0];
    mData[aY][0] = mData[aY][7];
    mData[aY][7] = aTmp;
    aTmp = mData[aY][1];
    mData[aY][1] = mData[aY][6];
    mData[aY][6] = aTmp;
    aTmp = mData[aY][2];
    mData[aY][2] = mData[aY][5];
    mData[aY][5] = aTmp;
    aTmp = mData[aY][3];
    mData[aY][3] = mData[aY][4];
    mData[aY][4] = aTmp;
  }
}

void FastMatrix::FlipV() {
  SwapRows(0, 7);
  SwapRows(1, 6);
  SwapRows(2, 5);
  SwapRows(3, 4);
}

void FastMatrix::FlipDiagA() {
  for (int aY = 0; aY < 8; ++aY) {
    for (int aX = aY + 1; aX < 8; ++aX) {
      const unsigned char aTmp = mData[aY][aX];
      mData[aY][aX] = mData[aX][aY];
      mData[aX][aY] = aTmp;
    }
  }
}

void FastMatrix::FlipDiagB() {
  for (int aY = 0; aY < 8; ++aY) {
    for (int aX = 0; aX < 8; ++aX) {
      const int aMY = 7 - aX;
      const int aMX = 7 - aY;
      if ((aY * 8 + aX) < (aMY * 8 + aMX)) {
        const unsigned char aTmp = mData[aY][aX];
        mData[aY][aX] = mData[aMY][aMX];
        mData[aMY][aMX] = aTmp;
      }
    }
  }
}

}  // namespace peanutbutter::matrices
