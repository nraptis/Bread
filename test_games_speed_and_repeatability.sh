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

triage_failure() {
  local log_file="$1"
  echo
  echo "[TRIAGE] game speed/repeatability run failed."
  if [[ -f "${log_file}" ]]; then
    echo "[TRIAGE] Tail of run log:"
    tail -n 200 "${log_file}" || true
  fi
}

echo "[STEP] Configuring Release build (optimized)"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

echo "[STEP] Building Release"
cmake --build "${BUILD_DIR}" -j"$(detect_jobs)"

echo "[STEP] Running benchmark_games_speed_repeatability"
RUN_LOG="${BUILD_DIR}/games_speed_repeatability_release.log"
set +e
"${BUILD_DIR}/benchmark_games_speed_repeatability" 2>&1 | tee "${RUN_LOG}"
RUN_STATUS=${PIPESTATUS[0]}
set -e

if [[ ${RUN_STATUS} -ne 0 ]]; then
  triage_failure "${RUN_LOG}"
  exit "${RUN_STATUS}"
fi

echo "[PASS] Game speed and repeatability run completed"
