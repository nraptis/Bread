#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${ROOT_DIR}/build_release"
OUTPUT_FILE="${ROOT_DIR}/run_bit_inclusion_report.txt"
TRIALS="${1:-100}"
BLOCKS="${2:-200}"

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

{
  echo "[INFO] run_bit_inclusion_tests started at $(date)"
  echo "[INFO] output file: ${OUTPUT_FILE}"
  echo "[INFO] trials=${TRIALS} blocks=${BLOCKS}"
  echo

  echo "[STEP] Cleaning build directory"
  rm -rf "${BUILD_DIR}"

  echo "[STEP] Configuring Release build"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DBREAD_BUILD_TESTS=ON \
    -DBREAD_BUILD_QT_APP=OFF \
    -DBREAD_BUILD_TOOLS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

  echo "[STEP] Building bit_inclusion_tests"
  cmake --build "${BUILD_DIR}" --target bit_inclusion_tests -j"$(detect_jobs)"
  echo

  echo "[STEP] Running bit_inclusion_tests"
  "${BUILD_DIR}/bit_inclusion_tests" "${TRIALS}" "${BLOCKS}"
  echo

  echo "[PASS] run_bit_inclusion_tests finished at $(date)"
} | tee "${OUTPUT_FILE}"
