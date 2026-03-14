#ifndef BREAD_SRC_MATRICES_FAST_MATRIX_HPP_
#define BREAD_SRC_MATRICES_FAST_MATRIX_HPP_

#include <cstddef>
#include <cstdint>

namespace bread::matrices {

class FastMatrix final {
 public:
  static constexpr int kWidth = 8;
  static constexpr int kHeight = 8;
  static constexpr int kSize = 64;

  FastMatrix();
  ~FastMatrix() = default;

  unsigned char* Data();
  const unsigned char* Data() const;

  unsigned char (&Rows())[kHeight][kWidth];
  const unsigned char (&Rows() const)[kHeight][kWidth];

  void Clear();
  void Fill(unsigned char pValue);

  // ----- Fast Ops (8x8 tuned) -----
  void ShiftRow(int pIndex, int pAmount);
  void ShiftRowRight1(int pIndex);
  void ShiftRowLeft1(int pIndex);
  void ShiftColumn(int pIndex, int pAmount);
  void ShiftColumnDown1(int pIndex);
  void ShiftColumnUp1(int pIndex);
  void SwapRows(int pIndex1, int pIndex2);
  void SwapColumns(int pIndex1, int pIndex2);
  void XorRows(int pIndex1, int pIndex2);
  void XorColumns(int pIndex1, int pIndex2);
  void AddRowConstant(int pIndex, unsigned char pValue);
  void AddColumnConstant(int pIndex, unsigned char pValue);
  void WeaveColumns(int pIndex1, int pIndex2);
  void WeaveRows(int pIndex1, int pIndex2);

  // ----- Non-Recoverable Ops -----
  void MixRowsEven(int pIndex1, int pIndex2);
  void MixColumnsOdd(int pIndex1, int pIndex2);

  // ----- Slow Ops (whole-matrix transforms) -----
  void RotateRight();
  void RotateLeft();
  void FlipH();
  void FlipV();
  void FlipDiagA();
  void FlipDiagB();

 protected:
  unsigned char mData[8][8];
};

}  // namespace bread::matrices

#endif  // BREAD_SRC_MATRICES_FAST_MATRIX_HPP_
