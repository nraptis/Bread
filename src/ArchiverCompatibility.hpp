#ifndef BREAD_SRC_ARCHIVER_COMPATIBILITY_HPP_
#define BREAD_SRC_ARCHIVER_COMPATIBILITY_HPP_

#include <cstdint>
#include <string>

namespace peanutbutter::archiver {

enum class ExpansionStrength : std::uint8_t {
  kLow = 0,
  kNormal = 1,
  kHigh = 2,
  kExtreme = 3,
};

enum class ProgressPhase : std::uint8_t {
  kPreflight = 0,
  kExpansion = 1,
  kFinalize = 2,
};

enum class ProgressProfileKind : std::uint8_t {
  kBundle = 0,
  kUnbundle = 1,
  kRecover = 2,
  kUnknown = 3,
};

struct ProgressInfo {
  std::string mModeName;
  ProgressPhase mPhase = ProgressPhase::kPreflight;
  double mOverallFraction = 0.0;
  std::string mDetail;
};

class Logger {
 public:
  virtual ~Logger() = default;
  virtual void LogStatus(const std::string& pMessage) = 0;
  virtual void LogError(const std::string& pMessage) = 0;
  virtual void LogProgress(const ProgressInfo&) {}
};

using ExpansionCancelFn = bool (*)(void* pUserData);

struct LaunchRequest {
  unsigned char* mPassword = nullptr;
  int mPasswordLength = 0;
  std::uint8_t mExpanderVersion = 0;
  ExpansionStrength mExpansionStrength = ExpansionStrength::kNormal;
  Logger* mLogger = nullptr;
  const char* mModeName = "Bundle";
  ProgressProfileKind mProgressProfile = ProgressProfileKind::kBundle;
  ExpansionCancelFn mShouldCancel = nullptr;
  void* mCancelUserData = nullptr;
};

std::uint8_t ExpanderLibraryVersion();
const char* ExpansionStrengthName(ExpansionStrength pStrength);

void ReportProgress(Logger& pLogger,
                    const std::string& pModeName,
                    ProgressProfileKind pProfile,
                    ProgressPhase pPhase,
                    double pPhaseFraction,
                    const std::string& pDetail = std::string());

bool Launch(unsigned char* pPassword,
            int pPasswordLength,
            std::uint8_t pExpanderVersion,
            ExpansionStrength pExpansionStrength,
            Logger* pLogger,
            const char* pModeName,
            ProgressProfileKind pProgressProfile,
            ExpansionCancelFn pShouldCancel,
            void* pCancelUserData);

bool Launch(const LaunchRequest& pRequest);

}  // namespace peanutbutter::archiver

#endif  // BREAD_SRC_ARCHIVER_COMPATIBILITY_HPP_
