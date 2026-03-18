#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "src/ArchiverCompatibility.hpp"
#include "src/PeanutButter.hpp"

namespace {

using peanutbutter::archiver::ExpansionStrength;
using peanutbutter::archiver::Logger;
using peanutbutter::archiver::ProgressInfo;
using peanutbutter::archiver::ProgressPhase;
using peanutbutter::archiver::ProgressProfileKind;

struct TableDescriptor {
  const char* mName;
  unsigned char* mData;
  std::size_t mSize;
};

const TableDescriptor kTables[] = {
    {"L1_A", gTableL1_A, BLOCK_SIZE_L1}, {"L1_B", gTableL1_B, BLOCK_SIZE_L1}, {"L1_C", gTableL1_C, BLOCK_SIZE_L1},
    {"L1_D", gTableL1_D, BLOCK_SIZE_L1}, {"L1_E", gTableL1_E, BLOCK_SIZE_L1}, {"L1_F", gTableL1_F, BLOCK_SIZE_L1},
    {"L1_G", gTableL1_G, BLOCK_SIZE_L1}, {"L1_H", gTableL1_H, BLOCK_SIZE_L1}, {"L2_A", gTableL2_A, BLOCK_SIZE_L2},
    {"L2_B", gTableL2_B, BLOCK_SIZE_L2}, {"L2_C", gTableL2_C, BLOCK_SIZE_L2}, {"L2_D", gTableL2_D, BLOCK_SIZE_L2},
    {"L2_E", gTableL2_E, BLOCK_SIZE_L2}, {"L2_F", gTableL2_F, BLOCK_SIZE_L2}, {"L3_A", gTableL3_A, BLOCK_SIZE_L3},
    {"L3_B", gTableL3_B, BLOCK_SIZE_L3}, {"L3_C", gTableL3_C, BLOCK_SIZE_L3}, {"L3_D", gTableL3_D, BLOCK_SIZE_L3},
};

class CaptureLogger final : public Logger {
 public:
  void LogStatus(const std::string& pMessage) override {
    mStatus.push_back(pMessage);
  }

  void LogError(const std::string& pMessage) override {
    mErrors.push_back(pMessage);
  }

  void LogProgress(const ProgressInfo& pInfo) override {
    mProgress.push_back(pInfo);
  }

  std::vector<std::string> mStatus;
  std::vector<std::string> mErrors;
  std::vector<ProgressInfo> mProgress;
};

struct CancelState {
  int mChecks = 0;
  int mTripAt = 0;
};

bool CancelAfterChecks(void* pUserData) {
  CancelState* aState = static_cast<CancelState*>(pUserData);
  ++aState->mChecks;
  return aState->mChecks >= aState->mTripAt;
}

std::uint64_t HashBytes(const unsigned char* pBytes, std::size_t pLength) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pBytes[aIndex]);
  }
  return aDigest;
}

std::uint64_t HashValue(std::uint64_t pDigest, std::uint64_t pValue) {
  for (int aShift = 0; aShift < 64; aShift += 8) {
    pDigest = (pDigest * 1099511628211ULL) ^ ((pValue >> aShift) & 0xFFU);
  }
  return pDigest;
}

std::uint64_t HashAllTables() {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (const TableDescriptor& aTable : kTables) {
    aDigest = HashValue(aDigest, HashBytes(aTable.mData, aTable.mSize));
  }
  return aDigest;
}

void FillTablesPattern(unsigned char pSeed) {
  for (std::size_t aTableIndex = 0; aTableIndex < (sizeof(kTables) / sizeof(kTables[0])); ++aTableIndex) {
    for (std::size_t aByteIndex = 0; aByteIndex < kTables[aTableIndex].mSize; ++aByteIndex) {
      kTables[aTableIndex].mData[aByteIndex] =
          static_cast<unsigned char>(pSeed + static_cast<unsigned char>(aTableIndex * 11U + aByteIndex));
    }
  }
}

bool ContainsToken(const std::vector<std::string>& pMessages, const std::string& pToken) {
  for (const std::string& aMessage : pMessages) {
    if (aMessage.find(pToken) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool ValidateProgressLog(const CaptureLogger& pLogger, const std::string& pModeName) {
  if (pLogger.mProgress.empty()) {
    return false;
  }
  double aLastFraction = -1.0;
  for (const ProgressInfo& aInfo : pLogger.mProgress) {
    if (aInfo.mModeName != pModeName) {
      return false;
    }
    if (aInfo.mPhase != ProgressPhase::kExpansion) {
      return false;
    }
    if (aInfo.mOverallFraction < 0.0 || aInfo.mOverallFraction > 1.0) {
      return false;
    }
    if (aInfo.mOverallFraction < aLastFraction) {
      return false;
    }
    aLastFraction = aInfo.mOverallFraction;
  }
  return pLogger.mProgress.back().mOverallFraction == 1.0;
}

}  // namespace

int main() {
  const std::string aPassword = "hotdog";

  CaptureLogger aLogger;
  if (!peanutbutter::archiver::Launch(reinterpret_cast<unsigned char*>(const_cast<char*>(aPassword.data())),
                                      static_cast<int>(aPassword.size()),
                                      peanutbutter::archiver::ExpanderLibraryVersion(),
                                      ExpansionStrength::kNormal,
                                      &aLogger,
                                      "Bundle",
                                      ProgressProfileKind::kBundle,
                                      nullptr,
                                      nullptr)) {
    std::cerr << "[FAIL] normal launch did not succeed\n";
    return 1;
  }

  const std::uint64_t aNormalDigestA = HashAllTables();
  if (aNormalDigestA == 0U) {
    std::cerr << "[FAIL] normal launch produced an empty digest\n";
    return 1;
  }
  if (!ValidateProgressLog(aLogger, "Bundle")) {
    std::cerr << "[FAIL] progress log was missing or malformed\n";
    return 1;
  }

  FillTablesPattern(0x5AU);
  const std::uint64_t aPoisonDigest = HashAllTables();
  CancelState aCancelState;
  aCancelState.mTripAt = 1;
  CaptureLogger aCancelLogger;
  if (peanutbutter::archiver::Launch(reinterpret_cast<unsigned char*>(const_cast<char*>(aPassword.data())),
                                     static_cast<int>(aPassword.size()),
                                     static_cast<std::uint8_t>(peanutbutter::archiver::ExpanderLibraryVersion() + 1U),
                                     ExpansionStrength::kNormal,
                                     &aCancelLogger,
                                     "Recover",
                                     ProgressProfileKind::kRecover,
                                     CancelAfterChecks,
                                     &aCancelState)) {
    std::cerr << "[FAIL] canceled launch unexpectedly succeeded\n";
    return 1;
  }
  if (HashAllTables() != aPoisonDigest) {
    std::cerr << "[FAIL] canceled launch modified global table state\n";
    return 1;
  }
  if (!ContainsToken(aCancelLogger.mStatus, "[Expansion][170]")) {
    std::cerr << "[FAIL] canceled launch did not emit the expected status log\n";
    return 1;
  }
  if (!ContainsToken(aCancelLogger.mStatus, "[Expansion][161]")) {
    std::cerr << "[FAIL] version mismatch did not emit the expected warning\n";
    return 1;
  }

  std::cout << "[PASS] archiver compatibility smoke tests passed"
            << " normal_digest=" << aNormalDigestA
            << " cancel_digest=" << aPoisonDigest << "\n";
  return 0;
}
