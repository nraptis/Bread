#ifndef BREAD_TESTS_COMMON_TEST_GRADE_DIGEST_PREDICTABILITY_NTRIALS_HPP_
#define BREAD_TESTS_COMMON_TEST_GRADE_DIGEST_PREDICTABILITY_NTRIALS_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <vector>

#include "tests/common/Ledger.hpp"

inline void GradePredictability(Ledger& pLedger, int pBatchLength, int pBatchCount, int pTrialCount) {
  if (pLedger.Digest() == nullptr) {
    pLedger.Log("predictability: missing digest");
    return;
  }
  if (pBatchLength <= 1 || pBatchCount <= 0 || pTrialCount <= 0) {
    pLedger.Log("predictability: invalid parameters");
    return;
  }

  constexpr int kAlphabet = 256;
  constexpr double kExpectedEqualRate = 1.0 / 256.0;
  const int aChunkLength = std::max(1, std::min(1024, pBatchLength));
  std::vector<unsigned char> aChunk(static_cast<std::size_t>(aChunkLength));

  double aScoreSum = 0.0;
  double aWorstScore = 100.0;
  double aEqualRateSum = 0.0;
  double aCorrelationAbsSum = 0.0;
  double aConditionalEntropySum = 0.0;
  std::uint64_t aTotalPairsAcrossTrials = 0;

  for (int aTrial = 0; aTrial < pTrialCount; ++aTrial) {
    std::array<std::uint64_t, kAlphabet * kAlphabet> aTransitions = {};
    std::array<std::uint64_t, kAlphabet> aRowTotals = {};

    std::uint64_t aPairCount = 0;
    std::uint64_t aEqualPairs = 0;
    long double aSumX = 0.0L;
    long double aSumY = 0.0L;
    long double aSumXX = 0.0L;
    long double aSumYY = 0.0L;
    long double aSumXY = 0.0L;
    bool aHasPrevious = false;
    unsigned char aPrevious = 0U;

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
          const unsigned char aCurrent = aChunk[static_cast<std::size_t>(aIndex)];
          if (aHasPrevious) {
            const std::uint64_t aPrevU = static_cast<std::uint64_t>(aPrevious);
            const std::uint64_t aCurrU = static_cast<std::uint64_t>(aCurrent);
            const std::size_t aTransitionIndex = static_cast<std::size_t>((aPrevU << 8U) | aCurrU);

            ++aTransitions[aTransitionIndex];
            ++aRowTotals[static_cast<std::size_t>(aPrevU)];
            ++aPairCount;
            if (aPrevious == aCurrent) {
              ++aEqualPairs;
            }

            const long double aX = static_cast<long double>(aPrevU);
            const long double aY = static_cast<long double>(aCurrU);
            aSumX += aX;
            aSumY += aY;
            aSumXX += aX * aX;
            aSumYY += aY * aY;
            aSumXY += aX * aY;
          }
          aPrevious = aCurrent;
          aHasPrevious = true;
        }
        aRemaining -= aTake;
      }
    }

    if (aPairCount == 0U) {
      continue;
    }

    double aConditionalEntropy = 0.0;
    for (int aRow = 0; aRow < kAlphabet; ++aRow) {
      const std::uint64_t aRowCount = aRowTotals[static_cast<std::size_t>(aRow)];
      if (aRowCount == 0U) {
        continue;
      }
      const double aRowCountD = static_cast<double>(aRowCount);
      const double aPairCountD = static_cast<double>(aPairCount);
      for (int aCol = 0; aCol < kAlphabet; ++aCol) {
        const std::uint64_t aCount = aTransitions[static_cast<std::size_t>((aRow << 8) | aCol)];
        if (aCount == 0U) {
          continue;
        }
        const double aCountD = static_cast<double>(aCount);
        const double aPxy = aCountD / aPairCountD;
        const double aPyGivenX = aCountD / aRowCountD;
        aConditionalEntropy -= aPxy * std::log2(aPyGivenX);
      }
    }

    const long double aN = static_cast<long double>(aPairCount);
    const long double aNumerator = (aN * aSumXY) - (aSumX * aSumY);
    const long double aDenomA = (aN * aSumXX) - (aSumX * aSumX);
    const long double aDenomB = (aN * aSumYY) - (aSumY * aSumY);
    double aCorrelation = 0.0;
    if (aDenomA > 0.0L && aDenomB > 0.0L) {
      aCorrelation = static_cast<double>(aNumerator / std::sqrt(aDenomA * aDenomB));
    }

    const double aEqualRate = static_cast<double>(aEqualPairs) / static_cast<double>(aPairCount);
    const double aPredictabilityBits = std::max(0.0, 8.0 - aConditionalEntropy);

    const double aPenaltyEqualRate = std::abs(aEqualRate - kExpectedEqualRate) * 25600.0;
    const double aPenaltyCorrelation = std::abs(aCorrelation) * 250.0;
    const double aPenaltyPredictabilityBits = aPredictabilityBits * 45.0;
    double aScore = 100.0 - (aPenaltyEqualRate + aPenaltyCorrelation + aPenaltyPredictabilityBits);
    aScore = std::max(0.0, std::min(100.0, aScore));

    aScoreSum += aScore;
    aWorstScore = std::min(aWorstScore, aScore);
    aEqualRateSum += aEqualRate;
    aCorrelationAbsSum += std::abs(aCorrelation);
    aConditionalEntropySum += aConditionalEntropy;
    aTotalPairsAcrossTrials += aPairCount;
  }

  const double aTrialCountD = static_cast<double>(pTrialCount);
  const double aMeanScore = aScoreSum / aTrialCountD;
  const double aMeanEqualRate = aEqualRateSum / aTrialCountD;
  const double aMeanAbsCorrelation = aCorrelationAbsSum / aTrialCountD;
  const double aMeanConditionalEntropy = aConditionalEntropySum / aTrialCountD;

  pLedger.AddMetric("predictability_mean_score", aMeanScore);
  pLedger.AddMetric("predictability_worst_score", aWorstScore);
  pLedger.AddMetric("predictability_mean_equal_rate", aMeanEqualRate);
  pLedger.AddMetric("predictability_mean_abs_correlation", aMeanAbsCorrelation);
  pLedger.AddMetric("predictability_mean_cond_entropy_bits", aMeanConditionalEntropy);

  std::cout << std::fixed << std::setprecision(4);
  pLedger.Log("Predictability metric = score[0..100] from equal-rate drift + lag-1 correlation + conditional entropy");
  pLedger.Log("Predictability summary: trials=" + std::to_string(pTrialCount) + ", batch_len=" +
              std::to_string(pBatchLength) + ", batch_count=" + std::to_string(pBatchCount) +
              ", total_pairs=" + std::to_string(aTotalPairsAcrossTrials));
}

#endif  // BREAD_TESTS_COMMON_TEST_GRADE_DIGEST_PREDICTABILITY_NTRIALS_HPP_
