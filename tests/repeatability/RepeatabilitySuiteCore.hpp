#ifndef BREAD_TESTS_REPEATABILITY_REPEATABILITYSUITE_CORE_HPP_
#define BREAD_TESTS_REPEATABILITY_REPEATABILITYSUITE_CORE_HPP_

#include <string_view>

namespace bread::tests::repeatability {

bool RunDigestRepeatabilitySuite(std::string_view pMode, int pLoops, int pDataLength, bool pRunFullGameVariants);

}  // namespace bread::tests::repeatability

#endif  // BREAD_TESTS_REPEATABILITY_REPEATABILITYSUITE_CORE_HPP_
