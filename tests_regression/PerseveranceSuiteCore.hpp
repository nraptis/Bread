#ifndef BREAD_TESTS_PERSEVERANCE_CORE_HPP_
#define BREAD_TESTS_PERSEVERANCE_CORE_HPP_

#include <string_view>

namespace peanutbutter::tests::perseverance {

bool RunDigestPerseveranceSuite(std::string_view pMode, int pLoops, int pDataLength);

}  // namespace peanutbutter::tests::perseverance

#endif  // BREAD_TESTS_PERSEVERANCE_CORE_HPP_
