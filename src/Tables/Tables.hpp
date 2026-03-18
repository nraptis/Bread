#ifndef BREAD_SRC_TABLES_TABLES_HPP_
#define BREAD_SRC_TABLES_TABLES_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "src/PeanutButter.hpp"

namespace peanutbutter::tables {

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

enum class GameStyle : std::uint8_t {
  kNone = 0,
  kSparse = 1,
  kFull = 2,
};

enum class MazeStyle : std::uint8_t {
  kNone = 0,
  kSparse = 1,
  kFull = 2,
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
  GameStyle mGameStyle = GameStyle::kNone;
  MazeStyle mMazeStyle = MazeStyle::kNone;
  bool mIsFastMode = false;
  Logger* mLogger = nullptr;
  const char* mModeName = "Bundle";
  ProgressProfileKind mProgressProfile = ProgressProfileKind::kBundle;
  ExpansionCancelFn mShouldCancel = nullptr;
  void* mCancelUserData = nullptr;
};

std::uint8_t ExpanderLibraryVersion();
const char* ExpansionStrengthName(ExpansionStrength pStrength);
const char* GameStyleName(GameStyle pStyle);
const char* MazeStyleName(MazeStyle pStyle);
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

class Tables final {
 public:
  struct TableDescriptor {
    const char* mName;
    unsigned char* mData;
    std::size_t mSize;
  };

  static constexpr std::size_t kL1TableCount = 12u;
  static constexpr std::size_t kL2TableCount = 6u;
  static constexpr std::size_t kL3TableCount = 4u;
  static constexpr std::size_t kTableCount = kL1TableCount + kL2TableCount + kL3TableCount;
  static constexpr std::size_t kScratchSize = kTableCount;

  static unsigned char gTableL1_A[BLOCK_SIZE_L1];
  static unsigned char gTableL1_B[BLOCK_SIZE_L1];
  static unsigned char gTableL1_C[BLOCK_SIZE_L1];
  static unsigned char gTableL1_D[BLOCK_SIZE_L1];
  static unsigned char gTableL1_E[BLOCK_SIZE_L1];
  static unsigned char gTableL1_F[BLOCK_SIZE_L1];
  static unsigned char gTableL1_G[BLOCK_SIZE_L1];
  static unsigned char gTableL1_H[BLOCK_SIZE_L1];
  static unsigned char gTableL1_I[BLOCK_SIZE_L1];
  static unsigned char gTableL1_J[BLOCK_SIZE_L1];
  static unsigned char gTableL1_K[BLOCK_SIZE_L1];
  static unsigned char gTableL1_L[BLOCK_SIZE_L1];

  static unsigned char gTableL2_A[BLOCK_SIZE_L2];
  static unsigned char gTableL2_B[BLOCK_SIZE_L2];
  static unsigned char gTableL2_C[BLOCK_SIZE_L2];
  static unsigned char gTableL2_D[BLOCK_SIZE_L2];
  static unsigned char gTableL2_E[BLOCK_SIZE_L2];
  static unsigned char gTableL2_F[BLOCK_SIZE_L2];

  static unsigned char gTableL3_A[BLOCK_SIZE_L3];
  static unsigned char gTableL3_B[BLOCK_SIZE_L3];
  static unsigned char gTableL3_C[BLOCK_SIZE_L3];
  static unsigned char gTableL3_D[BLOCK_SIZE_L3];

  static unsigned char mScratch[kScratchSize];
  static int mRandomTableIndex;
  static int mRandomByteIndex;
  static int mPlayOrder[16];

  static const std::array<TableDescriptor, kTableCount>& All();
  static void ResetRandomCursor();
  static unsigned char Get();
  static unsigned char Get(int pMax);
  static void ShufflePlayOrder();
  static bool Launch(const LaunchRequest& pRequest);
};

}  // namespace peanutbutter::tables

#endif  // BREAD_SRC_TABLES_TABLES_HPP_
