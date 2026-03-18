#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "src/ArchiverCompatibility.hpp"
#include "src/PeanutButter.hpp"
#include "src/Tables/Tables.hpp"

namespace {

using peanutbutter::archiver::ExpansionStrength;
using peanutbutter::archiver::Logger;
using peanutbutter::archiver::ProgressInfo;
using peanutbutter::archiver::ProgressPhase;
using peanutbutter::archiver::ProgressProfileKind;

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
  for (const auto& aTable : peanutbutter::tables::Tables::All()) {
    aDigest = HashValue(aDigest, HashBytes(aTable.mData, aTable.mSize));
  }
  return aDigest;
}

void FillTablesPattern(unsigned char pSeed) {
  const auto& aTables = peanutbutter::tables::Tables::All();
  for (std::size_t aTableIndex = 0; aTableIndex < aTables.size(); ++aTableIndex) {
    for (std::size_t aByteIndex = 0; aByteIndex < aTables[aTableIndex].mSize; ++aByteIndex) {
      aTables[aTableIndex].mData[aByteIndex] =
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
    if (aInfo.mPhase != ProgressPhase::kExpansion && aInfo.mPhase != ProgressPhase::kFinalize) {
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
  peanutbutter::archiver::LaunchRequest aRequest;
  aRequest.mPassword = reinterpret_cast<unsigned char*>(const_cast<char*>(aPassword.data()));
  aRequest.mPasswordLength = static_cast<int>(aPassword.size());
  aRequest.mExpanderVersion = peanutbutter::archiver::ExpanderLibraryVersion();
  aRequest.mExpansionStrength = ExpansionStrength::kNormal;
  aRequest.mLogger = &aLogger;
  aRequest.mModeName = "Bundle";
  aRequest.mProgressProfile = ProgressProfileKind::kBundle;
  if (!peanutbutter::archiver::Launch(aRequest)) {
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
  peanutbutter::archiver::LaunchRequest aCancelRequest;
  aCancelRequest.mPassword = reinterpret_cast<unsigned char*>(const_cast<char*>(aPassword.data()));
  aCancelRequest.mPasswordLength = static_cast<int>(aPassword.size());
  aCancelRequest.mExpanderVersion =
      static_cast<std::uint8_t>(peanutbutter::archiver::ExpanderLibraryVersion() + 1U);
  aCancelRequest.mExpansionStrength = ExpansionStrength::kNormal;
  aCancelRequest.mLogger = &aCancelLogger;
  aCancelRequest.mModeName = "Recover";
  aCancelRequest.mProgressProfile = ProgressProfileKind::kRecover;
  aCancelRequest.mShouldCancel = CancelAfterChecks;
  aCancelRequest.mCancelUserData = &aCancelState;
  if (peanutbutter::archiver::Launch(aCancelRequest)) {
    std::cerr << "[FAIL] canceled launch unexpectedly succeeded\n";
    return 1;
  }
  if (HashAllTables() != aPoisonDigest) {
    std::cerr << "[FAIL] canceled launch modified global table state\n";
    return 1;
  }

  peanutbutter::archiver::LaunchRequest aFastRequest;
  aFastRequest.mPassword = reinterpret_cast<unsigned char*>(const_cast<char*>(aPassword.data()));
  aFastRequest.mPasswordLength = static_cast<int>(aPassword.size());
  aFastRequest.mExpanderVersion = peanutbutter::archiver::ExpanderLibraryVersion();
  aFastRequest.mExpansionStrength = ExpansionStrength::kLow;
  aFastRequest.mIsFastMode = true;
  if (!peanutbutter::archiver::Launch(aFastRequest)) {
    std::cerr << "[FAIL] fast launch did not succeed\n";
    return 1;
  }
  const std::uint64_t aFastDigest = HashAllTables();
  if (aFastDigest == 0U || aFastDigest == aNormalDigestA) {
    std::cerr << "[FAIL] fast launch did not produce a distinct digest\n";
    return 1;
  }
  if (!ContainsToken(aCancelLogger.mStatus, "[Expansion] Table generation canceled.")) {
    std::cerr << "[FAIL] canceled launch did not emit the expected status log\n";
    return 1;
  }
  if (!ContainsToken(aCancelLogger.mStatus,
                     "[Expansion] Warning: expander library version mismatch; continuing with local implementation.")) {
    std::cerr << "[FAIL] version mismatch did not emit the expected warning\n";
    return 1;
  }

  std::cout << "[PASS] archiver compatibility smoke tests passed"
            << " normal_digest=" << aNormalDigestA
            << " cancel_digest=" << aPoisonDigest
            << " fast_digest=" << aFastDigest << "\n";
  return 0;
}
