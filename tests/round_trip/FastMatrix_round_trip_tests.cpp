#include <iostream>

#include "src/matrices/FastMatrix.hpp"

namespace {

void FillPattern(bread::matrices::FastMatrix* pMatrix) {
  unsigned char* aData = pMatrix->Data();
  for (int aIndex = 0; aIndex < bread::matrices::FastMatrix::kSize; ++aIndex) {
    aData[aIndex] = static_cast<unsigned char>(aIndex);
  }
}

bool MatchBaseline(const bread::matrices::FastMatrix& pMatrix) {
  const unsigned char* aData = pMatrix.Data();
  for (int aIndex = 0; aIndex < bread::matrices::FastMatrix::kSize; ++aIndex) {
    if (aData[aIndex] != static_cast<unsigned char>(aIndex)) {
      return false;
    }
  }
  return true;
}

}  // namespace

int main() {
  bread::matrices::FastMatrix aMatrix;

  FillPattern(&aMatrix);
  aMatrix.ShiftRow(3, 2);
  aMatrix.ShiftRow(3, -2);
  aMatrix.ShiftColumn(5, 1);
  aMatrix.ShiftColumn(5, -1);
  aMatrix.ShiftRowRight1(1);
  aMatrix.ShiftRowLeft1(1);
  aMatrix.ShiftColumnDown1(2);
  aMatrix.ShiftColumnUp1(2);
  aMatrix.SwapRows(0, 7);
  aMatrix.SwapRows(0, 7);
  aMatrix.SwapColumns(1, 6);
  aMatrix.SwapColumns(1, 6);
  aMatrix.XorRows(2, 5);
  aMatrix.XorRows(2, 5);
  aMatrix.XorColumns(0, 7);
  aMatrix.XorColumns(0, 7);
  aMatrix.AddRowConstant(4, 19U);
  aMatrix.AddRowConstant(4, static_cast<unsigned char>(256 - 19));
  aMatrix.AddColumnConstant(3, 47U);
  aMatrix.AddColumnConstant(3, static_cast<unsigned char>(256 - 47));
  aMatrix.WeaveColumns(0, 1);
  aMatrix.WeaveColumns(0, 1);
  aMatrix.WeaveRows(2, 3);
  aMatrix.WeaveRows(2, 3);

  if (!MatchBaseline(aMatrix)) {
    std::cerr << "[FAIL] FastMatrix fast-op round-trip mismatch\n";
    return 1;
  }

  FillPattern(&aMatrix);
  aMatrix.RotateRight();
  aMatrix.RotateLeft();
  aMatrix.FlipH();
  aMatrix.FlipH();
  aMatrix.FlipV();
  aMatrix.FlipV();
  aMatrix.FlipDiagA();
  aMatrix.FlipDiagA();
  aMatrix.FlipDiagB();
  aMatrix.FlipDiagB();

  if (!MatchBaseline(aMatrix)) {
    std::cerr << "[FAIL] FastMatrix slow-op round-trip mismatch\n";
    return 1;
  }

  // Smoke check for non-recoverables (they should mutate state).
  FillPattern(&aMatrix);
  const unsigned char aBefore = aMatrix.Data()[0];
  aMatrix.MixRowsEven(0, 1);
  aMatrix.MixColumnsOdd(0, 1);
  if (aMatrix.Data()[0] == aBefore) {
    std::cerr << "[FAIL] FastMatrix non-recoverables did not mutate state\n";
    return 1;
  }

  std::cout << "[PASS] FastMatrix round-trip tests passed\n";
  return 0;
}
