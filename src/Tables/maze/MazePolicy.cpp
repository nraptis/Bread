#include "MazePolicy.hpp"

namespace peanutbutter::maze {

namespace {

struct ProbeConfigRecord {
  const char* mName;
  ProbeConfig mConfig;
};

constexpr ProbeConfigRecord kProbeConfigs[kGameCount] = {
    {"Custom_R1_C1_S8", {GenerationMode::kCustom, 1, 8, 1}},
    {"Prim_R1_C2_S8", {GenerationMode::kPrim, 1, 8, 2}},
    {"Kruskal_R2_C1_S8", {GenerationMode::kKruskal, 2, 8, 1}},
    {"Custom_R2_C2_S8", {GenerationMode::kCustom, 2, 8, 2}},
    {"Custom_R3_C2_S8", {GenerationMode::kCustom, 3, 8, 2}},
    {"Prim_R3_C3_S8", {GenerationMode::kPrim, 3, 8, 3}},
    {"Kruskal_R4_C3_S8", {GenerationMode::kKruskal, 4, 8, 3}},
    {"Custom_R4_C4_S8", {GenerationMode::kCustom, 4, 8, 4}},
    {"Custom_R5_C3_S8", {GenerationMode::kCustom, 5, 8, 3}},
    {"Prim_R5_C5_S8", {GenerationMode::kPrim, 5, 8, 5}},
    {"Kruskal_R6_C3_S8", {GenerationMode::kKruskal, 6, 8, 3}},
    {"Custom_R6_C6_S8", {GenerationMode::kCustom, 6, 8, 6}},
    {"Custom_R7_C5_S8", {GenerationMode::kCustom, 7, 8, 5}},
    {"Prim_R7_C7_S8", {GenerationMode::kPrim, 7, 8, 7}},
    {"Kruskal_R8_C6_S8", {GenerationMode::kKruskal, 8, 8, 6}},
    {"Custom_R8_C8_S8", {GenerationMode::kCustom, 8, 8, 8}},
};

}  // namespace

int NormalizeGameIndex(int pGameIndex) {
  const int aModulo = pGameIndex % kGameCount;
  return (aModulo < 0) ? (aModulo + kGameCount) : aModulo;
}

const ProbeConfig& GetProbeConfigByIndex(int pGameIndex) {
  return kProbeConfigs[NormalizeGameIndex(pGameIndex)].mConfig;
}

const char* GetProbeConfigNameByIndex(int pGameIndex) {
  return kProbeConfigs[NormalizeGameIndex(pGameIndex)].mName;
}

}  // namespace peanutbutter::maze
