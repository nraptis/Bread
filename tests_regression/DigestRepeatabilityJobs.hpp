#ifndef BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_JOBS_HPP_
#define BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_JOBS_HPP_

namespace peanutbutter::tests::repeatability {

bool RunDigestRepeatabilityShort();
bool RunDigestRepeatabilityMedium();
bool RunDigestRepeatabilityOversized();
bool RunDigestRepeatabilityContaminated();
bool RunDigestRepeatabilityManual();

}  // namespace peanutbutter::tests::repeatability

#endif  // BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_JOBS_HPP_
