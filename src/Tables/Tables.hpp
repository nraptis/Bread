#ifndef BREAD_SRC_TABLES_TABLES_HPP_
#define BREAD_SRC_TABLES_TABLES_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "MOCK_WRAPPERS.hpp"
#include "src/PeanutButter.hpp"
#include "src/Tables/fast_rand/FastRand.hpp"

class AESCounter;
class ARIA256Counter;
class ChaCha20Counter;

namespace peanutbutter::games {
class GameBoard;
}

namespace peanutbutter::maze {
class MazeDirector;
}

namespace peanutbutter::tables {

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

enum class TableFillKind : std::uint8_t {
  kAesCounter1 = 0,
  kAesCounter2 = 1,
  kAesCounter3 = 2,
  kAriaCounter = 3,
  kChaChaCounter = 4,
  kPasswordExpander00 = 5,
  kPasswordExpander01 = 6,
  kPasswordExpander02 = 7,
  kPasswordExpander03 = 8,
  kPasswordExpander04 = 9,
  kPasswordExpander05 = 10,
  kPasswordExpander06 = 11,
  kPasswordExpander07 = 12,
  kPasswordExpander08 = 13,
  kPasswordExpander09 = 14,
  kPasswordExpander10 = 15,
  kPasswordExpander11 = 16,
  kPasswordExpander12 = 17,
  kPasswordExpander13 = 18,
  kPasswordExpander14 = 19,
  kPasswordExpander15 = 20,
};

struct LaunchRequest {
  unsigned char* mPassword = nullptr;
  int mPasswordLength = 0;
  std::uint8_t mExpanderVersion = 0;
  GameStyle mGameStyle = GameStyle::kNone;
  MazeStyle mMazeStyle = MazeStyle::kNone;
  bool mIsFastMode = false;
  Logger* mLogger = nullptr;
  ExpansionCancelFn mShouldCancel = nullptr;
  void* mCancelUserData = nullptr;
};

std::uint8_t ExpanderLibraryVersion();
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
  static constexpr std::size_t kBaseFillKindCount = 21u;

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

  static peanutbutter::fast_rand::FastRand gFastRand;
  static TableFillKind gFillOrder[kTableCount];
  static unsigned char gExpanderWorker[PASSWORD_EXPANDED_SIZE];
  static int mRandomTableIndex;
  static int mRandomByteIndex;
  static AESCounter* gAesCounter;
  static ARIA256Counter* gAriaCounter;
  static ChaCha20Counter* gChaChaCounter;
  static peanutbutter::games::GameBoard* gGameBoard;
  static peanutbutter::maze::MazeDirector* gMazeDirector;

  static const std::array<TableDescriptor, kTableCount>& All();
  static void EnsureRuntimeObjects();
  static void ResetRandomCursor();
  static unsigned char Get();
  static unsigned char Get(int pMax);
  static bool Launch(const LaunchRequest& pRequest);
};

}  // namespace peanutbutter::tables

#endif  // BREAD_SRC_TABLES_TABLES_HPP_
