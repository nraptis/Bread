#include "src/Tables/TablesTiming.hpp"

#include "src/Tables/TablesHelpers.hpp"

namespace peanutbutter::tables::timing {

namespace {

constexpr TimingProfile kAverageProfile = {
    2.039,
    3.308,
    7.724,
    17.363,
    8.757,
    4.000,
    1050.0,
};

double BytesToMilliseconds(std::size_t pBytes, double pMegabytesPerSecond) {
  if (pMegabytesPerSecond <= 0.0) {
    return 0.0;
  }
  const double aBytesPerMillisecond = (pMegabytesPerSecond * 1000.0 * 1000.0) / 1000.0;
  return static_cast<double>(pBytes) / aBytesPerMillisecond;
}

}  // namespace

const TimingProfile& ActiveTimingProfile() {
  return kAverageProfile;
}

double EstimateSeedMilliseconds(std::size_t pTableSize, bool pIsFastMode) {
  const TimingProfile& aProfile = ActiveTimingProfile();
  if (pIsFastMode) {
    return BytesToMilliseconds(pTableSize, aProfile.mFastFillMegabytesPerSecond);
  }
  if (pTableSize == static_cast<std::size_t>(BLOCK_SIZE_L1)) {
    return aProfile.mL1SeedMilliseconds;
  }
  if (pTableSize == static_cast<std::size_t>(BLOCK_SIZE_L2)) {
    return aProfile.mL2SeedMilliseconds;
  }
  return aProfile.mL3SeedMilliseconds;
}

WorkEstimate BuildWorkEstimate(GameStyle pGameStyle, MazeStyle pMazeStyle, bool pIsFastMode) {
  WorkEstimate aEstimate;
  const TimingProfile& aProfile = ActiveTimingProfile();
  const auto& aTables = Tables::All();
  for (const auto& aTable : aTables) {
    aEstimate.mSeedMilliseconds += EstimateSeedMilliseconds(aTable.mSize, pIsFastMode);
    if (!pIsFastMode) {
      aEstimate.mGameBlockCount += helpers::CountGameBlocks(aTable.mSize, pGameStyle);
      aEstimate.mMazeBlockCount += helpers::CountMazeBlocks(aTable.mSize, pMazeStyle);
    }
  }

  if (!pIsFastMode) {
    aEstimate.mGameMilliseconds = aProfile.mGameBlockMilliseconds * static_cast<double>(aEstimate.mGameBlockCount);
    aEstimate.mMazeMilliseconds = aProfile.mMazeBlockMilliseconds * static_cast<double>(aEstimate.mMazeBlockCount);
  }
  aEstimate.mFinalizeMilliseconds = aProfile.mFinalizeMilliseconds;
  aEstimate.mTotalMilliseconds = aEstimate.mSeedMilliseconds + aEstimate.mGameMilliseconds +
                                 aEstimate.mMazeMilliseconds + aEstimate.mFinalizeMilliseconds;
  if (aEstimate.mTotalMilliseconds <= 0.0) {
    aEstimate.mTotalMilliseconds = 1.0;
  }
  return aEstimate;
}

}  // namespace peanutbutter::tables::timing
