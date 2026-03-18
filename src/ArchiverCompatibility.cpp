#include "src/ArchiverCompatibility.hpp"

#include "src/Tables/Tables.hpp"

namespace peanutbutter::archiver {

std::uint8_t ExpanderLibraryVersion() {
  return peanutbutter::tables::ExpanderLibraryVersion();
}

const char* ExpansionStrengthName(ExpansionStrength pStrength) {
  return peanutbutter::tables::ExpansionStrengthName(pStrength);
}

void ReportProgress(Logger& pLogger,
                    const std::string& pModeName,
                    ProgressProfileKind pProfile,
                    ProgressPhase pPhase,
                    double pPhaseFraction,
                    const std::string& pDetail) {
  peanutbutter::tables::ReportProgress(pLogger, pModeName, pProfile, pPhase, pPhaseFraction, pDetail);
}

bool Launch(unsigned char* pPassword,
            int pPasswordLength,
            std::uint8_t pExpanderVersion,
            ExpansionStrength pExpansionStrength,
            Logger* pLogger,
            const char* pModeName,
            ProgressProfileKind pProgressProfile,
            ExpansionCancelFn pShouldCancel,
            void* pCancelUserData) {
  LaunchRequest aRequest;
  aRequest.mPassword = pPassword;
  aRequest.mPasswordLength = pPasswordLength;
  aRequest.mExpanderVersion = pExpanderVersion;
  aRequest.mExpansionStrength = pExpansionStrength;
  aRequest.mLogger = pLogger;
  aRequest.mModeName = pModeName;
  aRequest.mProgressProfile = pProgressProfile;
  aRequest.mShouldCancel = pShouldCancel;
  aRequest.mCancelUserData = pCancelUserData;
  return peanutbutter::archiver::Launch(aRequest);
}

bool Launch(const LaunchRequest& pRequest) {
  return peanutbutter::tables::Launch(pRequest);
}

}  // namespace peanutbutter::archiver
