#ifndef BREAD_SRC_ARCHIVER_COMPATIBILITY_HPP_
#define BREAD_SRC_ARCHIVER_COMPATIBILITY_HPP_

#include "src/Tables/Tables.hpp"

namespace peanutbutter::archiver {

using ExpansionStrength = peanutbutter::tables::ExpansionStrength;
using ProgressPhase = peanutbutter::tables::ProgressPhase;
using ProgressProfileKind = peanutbutter::tables::ProgressProfileKind;
using GameStyle = peanutbutter::tables::GameStyle;
using MazeStyle = peanutbutter::tables::MazeStyle;
using ProgressInfo = peanutbutter::tables::ProgressInfo;
using Logger = peanutbutter::tables::Logger;
using ExpansionCancelFn = peanutbutter::tables::ExpansionCancelFn;
using LaunchRequest = peanutbutter::tables::LaunchRequest;

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
