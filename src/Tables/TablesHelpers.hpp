#ifndef BREAD_SRC_TABLES_TABLESHELPERS_HPP_
#define BREAD_SRC_TABLES_TABLESHELPERS_HPP_

#include <cstddef>
#include <string>

#include "src/Tables/Tables.hpp"

namespace peanutbutter::tables::helpers {

std::string BuildLogMessage(const std::string& pMessage);
void LogStatus(Logger* pLogger, const std::string& pMessage);
void LogError(Logger* pLogger, const std::string& pMessage);
double ClampFraction(double pFraction);
std::size_t GameStride(GameStyle pStyle);
std::size_t MazeStride(MazeStyle pStyle);
bool ShouldCancel(const LaunchRequest& pRequest);
void SeedFastRand(const LaunchRequest& pRequest);
void ShuffleFillOrder();
bool FillTable(TableFillKind pKind,
               unsigned char* pPassword,
               int pPasswordLength,
               unsigned char* pDestination,
               std::size_t pDestinationLength);
bool FastFillTable(std::size_t pTableIndex, TableFillKind pKind, unsigned char* pDestination, std::size_t pDestinationLength);
std::string FillLabel(TableFillKind pKind);
std::size_t CountProcessedBlocks(std::size_t pBlockCount, std::size_t pStride);
std::size_t CountGameBlocks(std::size_t pTableSize, GameStyle pStyle);
std::size_t CountMazeBlocks(std::size_t pTableSize, MazeStyle pStyle);

}  // namespace peanutbutter::tables::helpers

#endif  // BREAD_SRC_TABLES_TABLESHELPERS_HPP_
