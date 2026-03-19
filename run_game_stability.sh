#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${ROOT_DIR}/build_release"
RUNS="${1:-100}"
OUTPUT_PATH="${2:-${ROOT_DIR}/game_stability_report.txt}"

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

echo "[STEP] Configuring Release build"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

echo "[STEP] Building game_stability_tests"
cmake --build "${BUILD_DIR}" --target game_stability_tests -j"$(detect_jobs)"

echo "[STEP] Running game_stability_tests runs=${RUNS}"
"${BUILD_DIR}/game_stability_tests" "${RUNS}" | tee "${OUTPUT_PATH}"

echo "[PASS] Game stability report written to ${OUTPUT_PATH}"
