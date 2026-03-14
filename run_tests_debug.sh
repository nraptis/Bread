#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build_debug"

detect_jobs() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
    return
  fi
  if command -v getconf >/dev/null 2>&1; then
    getconf _NPROCESSORS_ONLN
    return
  fi
  if command -v sysctl >/dev/null 2>&1; then
    local ncpu
    ncpu="$(sysctl -n hw.ncpu 2>/dev/null || true)"
    if [[ -n "${ncpu}" ]]; then
      echo "${ncpu}"
      return
    fi
  fi
  echo 4
}

triage_test_failure() {
  local log_file="$1"
  local last_log="${BUILD_DIR}/Testing/Temporary/LastTest.log"

  echo
  echo "[TRIAGE] ctest reported failures."
  echo "[TRIAGE] Failed test lines:"
  grep -E "Test #[0-9]+: .*\\*\\*\\*Failed" "${log_file}" || true
  echo
  echo "[TRIAGE] Summary section:"
  awk '/The following tests FAILED:/{flag=1} flag{print}' "${log_file}" || true

  if [[ -f "${last_log}" ]]; then
    echo
    echo "[TRIAGE] Tail of LastTest.log:"
    tail -n 200 "${last_log}" || true
  fi
}

echo "[STEP] Cleaning build directories"
find "${ROOT_DIR}" -maxdepth 1 -type d -name 'build*' -exec rm -rf {} +

echo "[STEP] Configuring Debug build"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug

echo "[STEP] Building Debug"
cmake --build "${BUILD_DIR}" -j"$(detect_jobs)"

echo "[STEP] Running test suites (Debug)"
CTEST_LOG="${BUILD_DIR}/ctest_debug_output.log"
set +e
ctest --test-dir "${BUILD_DIR}" --output-on-failure 2>&1 | tee "${CTEST_LOG}"
CTEST_STATUS=${PIPESTATUS[0]}
set -e

if [[ ${CTEST_STATUS} -ne 0 ]]; then
  triage_test_failure "${CTEST_LOG}"
  exit "${CTEST_STATUS}"
fi

echo "[PASS] Debug test suites completed"
