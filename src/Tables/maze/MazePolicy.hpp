#ifndef BREAD_SRC_MAZE_MAZEPOLICY_HPP_
#define BREAD_SRC_MAZE_MAZEPOLICY_HPP_

namespace peanutbutter::maze {

enum class GenerationMode {
  kRandom = -1,
  kCustom = 0,
  kPrim = 1,
  kKruskal = 2
};

struct ProbeConfig {
  GenerationMode mGenerationMode = GenerationMode::kCustom;
  int mRobotCount = 1;
  int mSharkCount = 8;
  int mCheeseCount = 1;
};

enum GameIndex {
  kCustom_R1_C1_S8 = 0,
  kPrim_R1_C2_S8,
  kKruskal_R2_C1_S8,
  kCustom_R2_C2_S8,
  kCustom_R3_C2_S8,
  kPrim_R3_C3_S8,
  kKruskal_R4_C3_S8,
  kCustom_R4_C4_S8,
  kCustom_R5_C3_S8,
  kPrim_R5_C5_S8,
  kKruskal_R6_C3_S8,
  kCustom_R6_C6_S8,
  kCustom_R7_C5_S8,
  kPrim_R7_C7_S8,
  kKruskal_R8_C6_S8,
  kCustom_R8_C8_S8,
  kGameCount
};

int NormalizeGameIndex(int pGameIndex);
const ProbeConfig& GetProbeConfigByIndex(int pGameIndex);
const char* GetProbeConfigNameByIndex(int pGameIndex);

}  // namespace peanutbutter::maze

#endif  // BREAD_SRC_MAZE_MAZEPOLICY_HPP_
