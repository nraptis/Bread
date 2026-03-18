#include "src/PeanutButter.hpp"

#include <cstring>

namespace peanutbutter {

namespace {

bool CopyContiguous(unsigned char* pDestination,
                    std::size_t pDestinationLength,
                    const unsigned char* pSource,
                    std::size_t pSourceLength,
                    std::size_t pExpectedLength) {
  if (pDestination == nullptr || pSource == nullptr || pDestinationLength != pExpectedLength ||
      pSourceLength != pExpectedLength) {
    return false;
  }

  std::memcpy(pDestination, pSource, pExpectedLength);
  return true;
}

}  // namespace

bool BuildL1FromExpandedChunks(unsigned char* pDestination, const unsigned char* pChunks, std::size_t pChunkCount) {
  return CopyContiguous(pDestination,
                        kBlockSizeL1,
                        pChunks,
                        pChunkCount * kPasswordExpandedSize,
                        kBlockSizeL1);
}

bool BuildL1FromBalloonedChunks(unsigned char* pDestination, const unsigned char* pChunks, std::size_t pChunkCount) {
  return CopyContiguous(pDestination,
                        kBlockSizeL1,
                        pChunks,
                        pChunkCount * kPasswordBalloonedSize,
                        kBlockSizeL1);
}

bool BuildL2FromL1Tables(unsigned char* pDestination, const unsigned char* pTableA, const unsigned char* pTableB) {
  if (pDestination == nullptr || pTableA == nullptr || pTableB == nullptr) {
    return false;
  }

  std::memcpy(pDestination, pTableA, kBlockSizeL1);
  std::memcpy(pDestination + kBlockSizeL1, pTableB, kBlockSizeL1);
  return true;
}

bool BuildL3FromL2Tables(unsigned char* pDestination, const unsigned char* pTableA, const unsigned char* pTableB) {
  if (pDestination == nullptr || pTableA == nullptr || pTableB == nullptr) {
    return false;
  }

  std::memcpy(pDestination, pTableA, kBlockSizeL2);
  std::memcpy(pDestination + kBlockSizeL2, pTableB, kBlockSizeL2);
  return true;
}

}  // namespace peanutbutter
