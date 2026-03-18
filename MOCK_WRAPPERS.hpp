#ifndef BREAD_MOCK_WRAPPERS_HPP_
#define BREAD_MOCK_WRAPPERS_HPP_

#include <cstdint>
#include <string>

namespace peanutbutter {

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

}  // namespace peanutbutter

#endif  // BREAD_MOCK_WRAPPERS_HPP_
