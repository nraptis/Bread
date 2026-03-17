#ifndef BREAD_TESTS_REPEATABILITY_REPEATABILITYSUITE_CORE_HPP_
#define BREAD_TESTS_REPEATABILITY_REPEATABILITYSUITE_CORE_HPP_

#include <string_view>

namespace peanutbutter::tests::repeatability {

bool RunDigestRepeatabilitySuite(std::string_view pMode, int pLoops, int pDataLength, bool pRunFullGameVariants);

}  // namespace peanutbutter::tests::repeatability

#endif  // BREAD_TESTS_REPEATABILITY_REPEATABILITYSUITE_CORE_HPP_
