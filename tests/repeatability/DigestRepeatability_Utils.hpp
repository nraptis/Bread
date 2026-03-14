#ifndef BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_UTILS_HPP_
#define BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_UTILS_HPP_

#include <array>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "src/rng/Digest.hpp"

namespace digest_repeatability {

inline bool RunDigestCase(bread::rng::Digest* pDigest,
                          const std::vector<unsigned char>& pPasswordBytes,
                          const std::string& pLabel,
                          std::string* pError) {
  if (pDigest == nullptr || pError == nullptr) {
    return false;
  }

  std::array<unsigned char, 512> aBulkA = {};
  std::array<unsigned char, 512> aBulkB = {};
  std::array<unsigned char, 512> aByteA = {};
  std::array<unsigned char, 512> aByteB = {};

  std::vector<unsigned char> aMutablePassword = pPasswordBytes;
  unsigned char* aPasswordPtr = aMutablePassword.empty() ? nullptr : aMutablePassword.data();
  const int aPasswordLength = static_cast<int>(aMutablePassword.size());

  pDigest->Seed(aPasswordPtr, aPasswordLength);
  pDigest->Get(aBulkA.data(), static_cast<int>(aBulkA.size()));

  pDigest->Seed(aPasswordPtr, aPasswordLength);
  pDigest->Get(aBulkB.data(), static_cast<int>(aBulkB.size()));

  if (aBulkA != aBulkB) {
    *pError = pLabel + ": bulk output mismatch between identical runs";
    return false;
  }

  pDigest->Seed(aPasswordPtr, aPasswordLength);
  for (std::size_t aIndex = 0; aIndex < aByteA.size(); ++aIndex) {
    pDigest->Get(&aByteA[aIndex], 1);
  }

  pDigest->Seed(aPasswordPtr, aPasswordLength);
  for (std::size_t aIndex = 0; aIndex < aByteB.size(); ++aIndex) {
    pDigest->Get(&aByteB[aIndex], 1);
  }

  if (aByteA != aByteB) {
    *pError = pLabel + ": byte output mismatch between identical runs";
    return false;
  }

  if (std::memcmp(aBulkA.data(), aByteA.data(), aBulkA.size()) != 0) {
    *pError = pLabel + ": bulk output differs from byte-by-byte output";
    return false;
  }

  return true;
}

inline std::vector<unsigned char> BuildPatternPassword(int pLength, int pSeed, unsigned char pBias) {
  std::vector<unsigned char> aOut(static_cast<std::size_t>(pLength), 0U);
  for (int aIndex = 0; aIndex < pLength; ++aIndex) {
    const unsigned int aValue =
        static_cast<unsigned int>(aIndex * 17 + pSeed * 13 + pBias + (aIndex % 5) * 29);
    aOut[static_cast<std::size_t>(aIndex)] = static_cast<unsigned char>(aValue & 0xFFU);
  }
  return aOut;
}

}  // namespace digest_repeatability

#endif  // BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_UTILS_HPP_
