#ifndef BREAD_SRC_TABLES_TABLESHELPERS_HPP_
#define BREAD_SRC_TABLES_TABLESHELPERS_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "src/Tables/Tables.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"

namespace peanutbutter::tables::helpers {

using peanutbutter::expansion::key_expansion::PasswordExpander;

enum class TableFillKind : std::uint8_t {
  kAesCounter = 0,
  kAriaCounter = 1,
  kChaChaCounter = 2,
  kPasswordExpander = 3,
};

struct FillInstruction {
  TableFillKind mKind = TableFillKind::kAesCounter;
  unsigned char mExpanderIndex = 0U;
};

std::string BuildLogMessage(const std::string& pMessage);
void LogStatus(Logger* pLogger, const std::string& pMessage);
void LogError(Logger* pLogger, const std::string& pMessage);
double ClampFraction(double pFraction);
std::string ResolveModeName(const char* pModeName);
std::size_t GameStride(GameStyle pStyle);
std::size_t MazeStride(MazeStyle pStyle);
bool ShouldCancel(const LaunchRequest& pRequest);
void BuildScratch(const LaunchRequest& pRequest);
std::array<unsigned char, PasswordExpander::kTypeCount> BuildExpanderStack();
std::array<FillInstruction, Tables::kL1TableCount> BuildL1Plan(
    const std::array<unsigned char, PasswordExpander::kTypeCount>& pExpanderStack,
    std::size_t* pExpanderIndex);
std::array<FillInstruction, Tables::kL2TableCount> BuildL2Plan(
    const std::array<unsigned char, PasswordExpander::kTypeCount>& pExpanderStack,
    std::size_t* pExpanderIndex);
std::array<FillInstruction, Tables::kL3TableCount> BuildL3Plan(
    const std::array<unsigned char, PasswordExpander::kTypeCount>& pExpanderStack,
    std::size_t* pExpanderIndex);
std::array<FillInstruction, Tables::kTableCount> BuildTablePlan();
bool FillTable(const FillInstruction& pInstruction,
               unsigned char* pPassword,
               int pPasswordLength,
               unsigned char* pDestination,
               std::size_t pDestinationLength);
bool FastFillTable(std::size_t pTableIndex,
                   const FillInstruction& pInstruction,
                   unsigned char* pDestination,
                   std::size_t pDestinationLength);
std::string FillLabel(const FillInstruction& pInstruction);
std::size_t CountProcessedBlocks(std::size_t pBlockCount, std::size_t pStride);
std::size_t CountGameBlocks(std::size_t pTableSize, GameStyle pStyle);
std::size_t CountMazeBlocks(std::size_t pTableSize, MazeStyle pStyle);
void ShufflePlayOrderFromBuffer(const unsigned char* pBuffer, std::size_t pLength, int* pPlayOrder, int pCount);

}  // namespace peanutbutter::tables::helpers

#endif  // BREAD_SRC_TABLES_TABLESHELPERS_HPP_
