#ifndef BREAD_TESTS_COMMON_LEDGER_HPP_
#define BREAD_TESTS_COMMON_LEDGER_HPP_

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "src/Tables/rng/Digest.hpp"

class Ledger {
 public:
  struct Metric {
    std::string mName;
    double mValue = 0.0;
  };

  Ledger(const std::string& pName, peanutbutter::rng::Digest* pDigest, int pSeedSalt = 0)
      : mName(pName), mDigest(pDigest), mSeedSalt(pSeedSalt) {}

  peanutbutter::rng::Digest* Digest() const { return mDigest; }

  void SetDigest(peanutbutter::rng::Digest* pDigest) { mDigest = pDigest; }

  const std::string& Name() const { return mName; }

  int SeedSalt() const { return mSeedSalt; }

  void SetSeedSalt(int pSeedSalt) { mSeedSalt = pSeedSalt; }

  void AddMetric(const std::string& pName, double pValue) {
    mMetrics.push_back({pName, pValue});
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "[GRADE] " << mName << " | " << pName << "=" << pValue << "\n";
  }

  void Log(const std::string& pLine) const {
    std::cout << "[GRADE] " << mName << " | " << pLine << "\n";
  }

  const std::vector<Metric>& Metrics() const { return mMetrics; }

 private:
  std::string mName;
  peanutbutter::rng::Digest* mDigest = nullptr;
  int mSeedSalt = 0;
  std::vector<Metric> mMetrics;
};

inline std::vector<unsigned char> BuildLedgerPassword(int pSeedSalt, int pTrialIndex) {
  const int aLength = 1 + ((pSeedSalt + (pTrialIndex * 19)) % 96);
  std::vector<unsigned char> aPassword(static_cast<std::size_t>(aLength));

  for (int aIndex = 0; aIndex < aLength; ++aIndex) {
    unsigned int aValue = static_cast<unsigned int>(pSeedSalt + (pTrialIndex * 131) + (aIndex * 29));
    aValue ^= (aValue >> 3U);
    aValue += static_cast<unsigned int>((aIndex & 7) * (pTrialIndex + 11));
    aPassword[static_cast<std::size_t>(aIndex)] = static_cast<unsigned char>(aValue & 0xFFU);

    if (aIndex > 0 && (aIndex % 17) == 0) {
      aPassword[static_cast<std::size_t>(aIndex)] = 0x00U;
    }
    if (aIndex > 0 && (aIndex % 23) == 0) {
      aPassword[static_cast<std::size_t>(aIndex)] = static_cast<unsigned char>('/');
    }
    if (aIndex > 0 && (aIndex % 29) == 0) {
      aPassword[static_cast<std::size_t>(aIndex)] = static_cast<unsigned char>('\\');
    }
  }

  return aPassword;
}

#endif  // BREAD_TESTS_COMMON_LEDGER_HPP_
