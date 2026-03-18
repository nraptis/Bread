#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

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
  const unsigned char* mData;
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

struct Options {
  std::string mPassword = "hotdog";
  std::uint8_t mVersion = peanutbutter::archiver::ExpanderLibraryVersion();
  ExpansionStrength mStrength = ExpansionStrength::kNormal;
  std::string mModeName = "Bundle";
  ProgressProfileKind mProfile = ProgressProfileKind::kBundle;
  bool mQuiet = false;
};

class StdoutLogger final : public Logger {
 public:
  void LogStatus(const std::string& pMessage) override {
    std::cout << pMessage << "\n";
  }

  void LogError(const std::string& pMessage) override {
    std::cerr << pMessage << "\n";
  }

  void LogProgress(const ProgressInfo& pInfo) override {
    std::cout << "[Progress]"
              << " mode=" << pInfo.mModeName
              << " phase=" << PhaseName(pInfo.mPhase)
              << " fraction=" << pInfo.mOverallFraction
              << " detail=" << pInfo.mDetail << "\n";
  }

 private:
  static const char* PhaseName(ProgressPhase pPhase) {
    switch (pPhase) {
      case ProgressPhase::kPreflight:
        return "Preflight";
      case ProgressPhase::kExpansion:
        return "Expansion";
      case ProgressPhase::kFinalize:
        return "Finalize";
    }
    return "Unknown";
  }
};

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

void PrintUsage(const char* pProgramName) {
  std::cout << "Usage: " << pProgramName
            << " [--password TEXT] [--strength low|normal|high|extreme] [--version N]"
            << " [--mode Bundle|Unbundle|Recover] [--profile bundle|unbundle|recover] [--quiet]\n";
}

bool ParseStrength(const std::string& pText, ExpansionStrength* pOutStrength) {
  if (pText == "low") {
    *pOutStrength = ExpansionStrength::kLow;
    return true;
  }
  if (pText == "normal" || pText == "medium") {
    *pOutStrength = ExpansionStrength::kNormal;
    return true;
  }
  if (pText == "high") {
    *pOutStrength = ExpansionStrength::kHigh;
    return true;
  }
  if (pText == "extreme") {
    *pOutStrength = ExpansionStrength::kExtreme;
    return true;
  }
  return false;
}

bool ParseProfile(const std::string& pText, ProgressProfileKind* pOutProfile) {
  if (pText == "bundle") {
    *pOutProfile = ProgressProfileKind::kBundle;
    return true;
  }
  if (pText == "unbundle") {
    *pOutProfile = ProgressProfileKind::kUnbundle;
    return true;
  }
  if (pText == "recover") {
    *pOutProfile = ProgressProfileKind::kRecover;
    return true;
  }
  return false;
}

bool ParseVersion(const std::string& pText, std::uint8_t* pOutVersion) {
  char* aEnd = nullptr;
  const unsigned long aValue = std::strtoul(pText.c_str(), &aEnd, 10);
  if (aEnd == nullptr || *aEnd != '\0' || aValue > 255UL) {
    return false;
  }
  *pOutVersion = static_cast<std::uint8_t>(aValue);
  return true;
}

bool ParseArgs(int pArgc, char** pArgv, Options* pOptions) {
  for (int aIndex = 1; aIndex < pArgc; ++aIndex) {
    const std::string aArg = pArgv[aIndex];
    if (aArg == "--help" || aArg == "-h") {
      PrintUsage(pArgv[0]);
      return false;
    }
    if (aArg == "--quiet") {
      pOptions->mQuiet = true;
      continue;
    }
    if ((aArg == "--password" || aArg == "--strength" || aArg == "--version" || aArg == "--mode" ||
         aArg == "--profile") &&
        (aIndex + 1) >= pArgc) {
      std::cerr << "[FAIL] missing value for " << aArg << "\n";
      return false;
    }
    if (aArg == "--password") {
      pOptions->mPassword = pArgv[++aIndex];
      continue;
    }
    if (aArg == "--strength") {
      if (!ParseStrength(pArgv[++aIndex], &pOptions->mStrength)) {
        std::cerr << "[FAIL] unsupported strength value\n";
        return false;
      }
      continue;
    }
    if (aArg == "--version") {
      if (!ParseVersion(pArgv[++aIndex], &pOptions->mVersion)) {
        std::cerr << "[FAIL] invalid version value\n";
        return false;
      }
      continue;
    }
    if (aArg == "--mode") {
      pOptions->mModeName = pArgv[++aIndex];
      continue;
    }
    if (aArg == "--profile") {
      if (!ParseProfile(pArgv[++aIndex], &pOptions->mProfile)) {
        std::cerr << "[FAIL] unsupported profile value\n";
        return false;
      }
      continue;
    }
    std::cerr << "[FAIL] unknown argument: " << aArg << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main(int pArgc, char** pArgv) {
  Options aOptions;
  if (!ParseArgs(pArgc, pArgv, &aOptions)) {
    return (pArgc > 1) ? 2 : 0;
  }

  StdoutLogger aLogger;
  Logger* aLoggerPtr = aOptions.mQuiet ? nullptr : &aLogger;
  const auto aStartedAt = std::chrono::steady_clock::now();
  if (!peanutbutter::archiver::Launch(reinterpret_cast<unsigned char*>(aOptions.mPassword.data()),
                                      static_cast<int>(aOptions.mPassword.size()),
                                      aOptions.mVersion,
                                      aOptions.mStrength,
                                      aLoggerPtr,
                                      aOptions.mModeName.c_str(),
                                      aOptions.mProfile,
                                      nullptr,
                                      nullptr)) {
    std::cerr << "[FAIL] archiver compatibility runner failed\n";
    return 1;
  }
  const auto aEndedAt = std::chrono::steady_clock::now();
  const auto aElapsedMilliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(aEndedAt - aStartedAt).count();
  const double aElapsedSeconds = static_cast<double>(aElapsedMilliseconds) / 1000.0;

  std::uint64_t aDigest = 1469598103934665603ULL;
  for (const TableDescriptor& aTable : kTables) {
    const std::uint64_t aTableDigest = HashBytes(aTable.mData, aTable.mSize);
    aDigest = HashValue(aDigest, aTableDigest);
    std::cout << "[TABLE] name=" << aTable.mName
              << " size=" << aTable.mSize
              << " digest=" << aTableDigest << "\n";
  }

  std::cout << "[PASS] archiver compatibility runner complete"
            << " version=" << static_cast<unsigned int>(peanutbutter::archiver::ExpanderLibraryVersion())
            << " requested_version=" << static_cast<unsigned int>(aOptions.mVersion)
            << " strength=" << peanutbutter::archiver::ExpansionStrengthName(aOptions.mStrength)
            << " elapsed_ms=" << aElapsedMilliseconds
            << " elapsed_s=" << std::fixed << std::setprecision(3) << aElapsedSeconds
            << " digest=" << aDigest << "\n";
  return 0;
}
