#ifndef BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_JOBS_HPP_
#define BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_JOBS_HPP_

namespace bread::tests::repeatability {

bool RunDigestRepeatabilityShort();
bool RunDigestRepeatabilityMedium();
bool RunDigestRepeatabilityOversized();
bool RunDigestRepeatabilityContaminated();
bool RunDigestRepeatabilityManual();

}  // namespace bread::tests::repeatability

#endif  // BREAD_TESTS_REPEATABILITY_DIGEST_REPEATABILITY_JOBS_HPP_
