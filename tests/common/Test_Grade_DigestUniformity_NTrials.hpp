#ifndef BREAD_TESTS_COMMON_TEST_GRADE_DIGEST_UNIFORMITY_NTRIALS_HPP_
#define BREAD_TESTS_COMMON_TEST_GRADE_DIGEST_UNIFORMITY_NTRIALS_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <vector>

#include "tests/common/Ledger.hpp"

inline void GradeUniformity(Ledger& pLedger, int pBatchLength, int pBatchCount, int pTrialCount) {
  if (pLedger.Digest() == nullptr) {
    pLedger.Log("uniformity: missing digest");
    return;
  }
  if (pBatchLength <= 0 || pBatchCount <= 0 || pTrialCount <= 0) {
    pLedger.Log("uniformity: invalid parameters");
    return;
  }

  constexpr int kAlphabet = 256;
  const int aChunkLength = std::max(1, std::min(1024, pBatchLength));
  std::vector<unsigned char> aChunk(static_cast<std::size_t>(aChunkLength));

  double aScoreSum = 0.0;
  double aWorstScore = 100.0;
  double aEntropySum = 0.0;
  double aReducedChi2Sum = 0.0;
  double aWorstMaxDeviationPercent = 0.0;
  std::uint64_t aTotalBytesAcrossTrials = 0;

  for (int aTrial = 0; aTrial < pTrialCount; ++aTrial) {
    std::array<std::uint64_t, kAlphabet> aCounts = {};
    std::uint64_t aTotalBytes = 0;

    std::vector<unsigned char> aPassword = BuildLedgerPassword(pLedger.SeedSalt(), aTrial);
    unsigned char* aPasswordPtr = aPassword.empty() ? nullptr : aPassword.data();
    const int aPasswordLength = static_cast<int>(aPassword.size());
    pLedger.Digest()->Seed(aPasswordPtr, aPasswordLength);

    for (int aBatch = 0; aBatch < pBatchCount; ++aBatch) {
      int aRemaining = pBatchLength;
      while (aRemaining > 0) {
        const int aTake = std::min(aChunkLength, aRemaining);
        pLedger.Digest()->Get(aChunk.data(), aTake);
        for (int aIndex = 0; aIndex < aTake; ++aIndex) {
          const unsigned int aByte = static_cast<unsigned int>(aChunk[static_cast<std::size_t>(aIndex)]);
          ++aCounts[aByte];
        }
        aTotalBytes += static_cast<std::uint64_t>(aTake);
        aRemaining -= aTake;
      }
    }

    if (aTotalBytes == 0U) {
      continue;
    }

    const double aExpected = static_cast<double>(aTotalBytes) / static_cast<double>(kAlphabet);
    double aChi2 = 0.0;
    double aEntropy = 0.0;
    double aMaxDeviationPercent = 0.0;

    for (int aByte = 0; aByte < kAlphabet; ++aByte) {
      const double aObserved = static_cast<double>(aCounts[static_cast<std::size_t>(aByte)]);
      const double aDelta = aObserved - aExpected;
      aChi2 += (aDelta * aDelta) / aExpected;

      const double aDeviationPercent = (std::abs(aDelta) / aExpected) * 100.0;
      if (aDeviationPercent > aMaxDeviationPercent) {
        aMaxDeviationPercent = aDeviationPercent;
      }

      if (aObserved > 0.0) {
        const double aP = aObserved / static_cast<double>(aTotalBytes);
        aEntropy -= aP * std::log2(aP);
      }
    }

    const double aReducedChi2 = aChi2 / 255.0;
    const double aPenaltyChi2 = std::abs(aReducedChi2 - 1.0) * 35.0;
    const double aPenaltyDeviation = aMaxDeviationPercent * 0.9;
    const double aPenaltyEntropy = std::max(0.0, 8.0 - aEntropy) * 60.0;
    double aScore = 100.0 - (aPenaltyChi2 + aPenaltyDeviation + aPenaltyEntropy);
    aScore = std::max(0.0, std::min(100.0, aScore));

    aScoreSum += aScore;
    aEntropySum += aEntropy;
    aReducedChi2Sum += aReducedChi2;
    aWorstScore = std::min(aWorstScore, aScore);
    aWorstMaxDeviationPercent = std::max(aWorstMaxDeviationPercent, aMaxDeviationPercent);
    aTotalBytesAcrossTrials += aTotalBytes;
  }

  const double aTrialCountD = static_cast<double>(pTrialCount);
  const double aMeanScore = aScoreSum / aTrialCountD;
  const double aMeanEntropy = aEntropySum / aTrialCountD;
  const double aMeanReducedChi2 = aReducedChi2Sum / aTrialCountD;

  pLedger.AddMetric("uniformity_mean_score", aMeanScore);
  pLedger.AddMetric("uniformity_worst_score", aWorstScore);
  pLedger.AddMetric("uniformity_mean_entropy_bits", aMeanEntropy);
  pLedger.AddMetric("uniformity_mean_reduced_chi2", aMeanReducedChi2);
  pLedger.AddMetric("uniformity_worst_max_dev_pct", aWorstMaxDeviationPercent);

  std::cout << std::fixed << std::setprecision(4);
  pLedger.Log("Uniformity metric = score[0..100] from reduced-chi2 + max deviation + entropy");
  pLedger.Log("Uniformity summary: trials=" + std::to_string(pTrialCount) + ", batch_len=" +
              std::to_string(pBatchLength) + ", batch_count=" + std::to_string(pBatchCount) +
              ", total_bytes=" + std::to_string(aTotalBytesAcrossTrials));
}

#endif  // BREAD_TESTS_COMMON_TEST_GRADE_DIGEST_UNIFORMITY_NTRIALS_HPP_
