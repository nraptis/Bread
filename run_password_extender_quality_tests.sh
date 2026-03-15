#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build_release"
OUTPUT_FILE="${ROOT_DIR}/run_password_extender_quality_tests.txt"

read_constant_int() {
  local constant_name="$1"
  awk -v target="${constant_name}" '
    $0 ~ ("inline constexpr int " target " = ") {
      match($0, /-?[0-9]+/)
      if (RSTART > 0) {
        print substr($0, RSTART, RLENGTH)
      }
      exit
    }
  ' "${ROOT_DIR}/tests/common/Tests.hpp"
}

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
  echo "[INFO] run_password_extender_quality_tests started at $(date)"
  echo "[INFO] output file: ${OUTPUT_FILE}"
  echo "[INFO] knobs from tests/common/Tests.hpp:"
  rg -n "TEST_LOOP_COUNT|GAME_TEST_DATA_LENGTH|TEST_SEED_GLOBAL|BENCHMARK_ENABLED" \
    "${ROOT_DIR}/tests/common/Tests.hpp" || true

  trials="$(read_constant_int TEST_LOOP_COUNT)"
  password_bytes="$(read_constant_int GAME_TEST_DATA_LENGTH)"
  if [[ $# -ge 1 && "$1" =~ ^[0-9]+$ && "$1" -gt 0 ]]; then
    trials="$1"
  fi
  if [[ $# -ge 2 && "$2" =~ ^[0-9]+$ && "$2" -gt 0 ]]; then
    password_bytes="$2"
  fi
  echo "[INFO] trials=${trials} password_bytes=${password_bytes}"
  echo

  echo "[STEP] Cleaning build directory"
  rm -rf "${BUILD_DIR}"

  echo "[STEP] Configuring Release build"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

  echo "[STEP] Building password_extender_quality"
  cmake --build "${BUILD_DIR}" --target password_extender_quality -j"$(detect_jobs)"
  echo

  echo "[STEP] Running password_extender_quality"
  "${BUILD_DIR}/password_extender_quality" "${trials}" "${password_bytes}"
  echo

  echo "[PASS] run_password_extender_quality_tests finished at $(date)"
} | tee "${OUTPUT_FILE}"
