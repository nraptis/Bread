#ifndef BREAD_SRC_TABLES_TABLESTIMING_HPP_
#define BREAD_SRC_TABLES_TABLESTIMING_HPP_

#include <cstddef>

#include "src/Tables/Tables.hpp"

namespace peanutbutter::tables::timing {

enum class TimingMode : unsigned char {
  kSoftware = 0,
  kArmNeon = 1,
};

struct TimingProfile {
  double mL1SeedMilliseconds = 0.0;
  double mL2SeedMilliseconds = 0.0;
  double mL3SeedMilliseconds = 0.0;
  double mGameBlockMilliseconds = 0.0;
  double mMazeBlockMilliseconds = 0.0;
  double mFinalizeMilliseconds = 0.0;
  double mFastFillMegabytesPerSecond = 0.0;
};

struct WorkEstimate {
  double mSeedMilliseconds = 0.0;
  double mGameMilliseconds = 0.0;
  double mMazeMilliseconds = 0.0;
  double mFinalizeMilliseconds = 0.0;
  double mTotalMilliseconds = 0.0;
  std::size_t mGameBlockCount = 0U;
  std::size_t mMazeBlockCount = 0U;
};

TimingMode ActiveTimingMode();
const TimingProfile& ActiveTimingProfile();
double EstimateSeedMilliseconds(std::size_t pTableSize, bool pIsFastMode);
WorkEstimate BuildWorkEstimate(GameStyle pGameStyle,
                               MazeStyle pMazeStyle,
                               bool pIsFastMode);

}  // namespace peanutbutter::tables::timing

#endif  // BREAD_SRC_TABLES_TABLESTIMING_HPP_
