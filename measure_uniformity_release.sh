#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build_release"

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

triage_measure_failure() {
  local log_file="$1"
  echo
  echo "[TRIAGE] uniformity benchmark failed."
  if [[ -f "${log_file}" ]]; then
    echo "[TRIAGE] Tail of benchmark log:"
    tail -n 200 "${log_file}" || true
  fi
}

echo "[STEP] Cleaning build directories"
find "${ROOT_DIR}" -maxdepth 1 -type d -name 'build*' -exec rm -rf {} +

echo "[STEP] Configuring Release build (optimized)"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

echo "[STEP] Building Release"
cmake --build "${BUILD_DIR}" -j"$(detect_jobs)"

echo "[STEP] Running benchmark_counter_uniformity_all"
MEASURE_LOG="${BUILD_DIR}/uniformity_all_release.log"
set +e
"${BUILD_DIR}/benchmark_counter_uniformity_all" 2>&1 | tee "${MEASURE_LOG}"
MEASURE_STATUS=${PIPESTATUS[0]}
set -e

if [[ ${MEASURE_STATUS} -ne 0 ]]; then
  triage_measure_failure "${MEASURE_LOG}"
  exit "${MEASURE_STATUS}"
fi

echo "[PASS] Uniformity release measurement completed"
