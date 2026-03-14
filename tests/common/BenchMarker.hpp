#ifndef BREAD_TESTS_BENCHMARKER_HPP_
#define BREAD_TESTS_BENCHMARKER_HPP_

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

class BenchMarker {
 public:
  struct Stats {
    std::uint64_t mBest = 0;
    std::uint64_t mWorst = 0;
    double mMean = 0.0;
    std::size_t mSampleCount = 0;
  };

  void BenchMarkStartJob() {
    mPassNanoseconds.clear();
    mJobStarted = true;
    mPassStarted = false;
  }

  void BenchMarkStartPass() {
    if (!mJobStarted) {
      return;
    }
    mPassStarted = true;
    mPassStart = Clock::now();
  }

  void BenchMarkEndPass() {
    if (!mJobStarted || !mPassStarted) {
      return;
    }
    const auto aNow = Clock::now();
    const std::uint64_t aNs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(aNow - mPassStart).count());
    mPassNanoseconds.push_back(aNs);
    mPassStarted = false;
  }

  void BenchMarkEndJob() {
    mJobStarted = false;
    mPassStarted = false;
  }

  void BenchMarkLogBest(std::size_t pCount, const std::string& pLabel) const {
    if (mPassNanoseconds.empty()) {
      std::cout << "[BENCH] " << pLabel << " | no samples\n";
      return;
    }
    std::vector<std::uint64_t> aSorted = mPassNanoseconds;
    std::sort(aSorted.begin(), aSorted.end());
    const std::size_t aTake = std::min<std::size_t>(pCount, aSorted.size());
    std::cout << "[BENCH] " << pLabel << " | best " << aTake << " runs (ns): ";
    for (std::size_t aIndex = 0; aIndex < aTake; ++aIndex) {
      if (aIndex != 0) {
        std::cout << ", ";
      }
      std::cout << aSorted[aIndex];
    }
    std::cout << "\n";
  }

  void BenchMarkLogWorse(std::size_t pCount, const std::string& pLabel) const {
    if (mPassNanoseconds.empty()) {
      std::cout << "[BENCH] " << pLabel << " | no samples\n";
      return;
    }
    std::vector<std::uint64_t> aSorted = mPassNanoseconds;
    std::sort(aSorted.begin(), aSorted.end());
    const std::size_t aTake = std::min<std::size_t>(pCount, aSorted.size());
    std::cout << "[BENCH] " << pLabel << " | worst " << aTake << " runs (ns): ";
    for (std::size_t aIndex = 0; aIndex < aTake; ++aIndex) {
      if (aIndex != 0) {
        std::cout << ", ";
      }
      std::cout << aSorted[aSorted.size() - aTake + aIndex];
    }
    std::cout << "\n";
  }

  void BenchMarkLogMeanDiscardingOutliers(std::size_t pDiscardEachSide, const std::string& pLabel) const {
    if (mPassNanoseconds.empty()) {
      std::cout << "[BENCH] " << pLabel << " | no samples\n";
      return;
    }
    std::vector<std::uint64_t> aSorted = mPassNanoseconds;
    std::sort(aSorted.begin(), aSorted.end());

    if ((pDiscardEachSide * 2) >= aSorted.size()) {
      pDiscardEachSide = 0;
    }

    const std::size_t aStart = pDiscardEachSide;
    const std::size_t aEnd = aSorted.size() - pDiscardEachSide;
    const std::size_t aCount = aEnd - aStart;
    const std::uint64_t aSum = std::accumulate(aSorted.begin() + static_cast<std::ptrdiff_t>(aStart),
                                               aSorted.begin() + static_cast<std::ptrdiff_t>(aEnd),
                                               static_cast<std::uint64_t>(0));
    const double aMean = (aCount == 0U) ? 0.0 : static_cast<double>(aSum) / static_cast<double>(aCount);
    std::cout << "[BENCH] " << pLabel << " | mean(ns), discard_low=" << pDiscardEachSide
              << ", discard_high=" << pDiscardEachSide << " => " << aMean << "\n";
  }

  std::size_t SampleCount() const {
    return mPassNanoseconds.size();
  }

  Stats ComputeStatsDiscardingOutliers(std::size_t pDiscardEachSide) const {
    Stats aStats;
    if (mPassNanoseconds.empty()) {
      return aStats;
    }
    std::vector<std::uint64_t> aSorted = mPassNanoseconds;
    std::sort(aSorted.begin(), aSorted.end());
    if ((pDiscardEachSide * 2) >= aSorted.size()) {
      pDiscardEachSide = 0;
    }
    const std::size_t aStart = pDiscardEachSide;
    const std::size_t aEnd = aSorted.size() - pDiscardEachSide;
    const std::size_t aCount = aEnd - aStart;
    if (aCount == 0) {
      return aStats;
    }
    const std::uint64_t aSum = std::accumulate(aSorted.begin() + static_cast<std::ptrdiff_t>(aStart),
                                               aSorted.begin() + static_cast<std::ptrdiff_t>(aEnd),
                                               static_cast<std::uint64_t>(0));
    aStats.mBest = aSorted[aStart];
    aStats.mWorst = aSorted[aEnd - 1];
    aStats.mMean = static_cast<double>(aSum) / static_cast<double>(aCount);
    aStats.mSampleCount = aCount;
    return aStats;
  }

 private:
  using Clock = std::chrono::steady_clock;

  bool mJobStarted = false;
  bool mPassStarted = false;
  Clock::time_point mPassStart = Clock::now();
  std::vector<std::uint64_t> mPassNanoseconds;
};

#endif  // BREAD_TESTS_BENCHMARKER_HPP_
