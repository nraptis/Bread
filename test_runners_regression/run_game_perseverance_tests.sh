#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build_release"
OUTPUT_FILE="${ROOT_DIR}/run_game_perseverance_tests_report.txt"

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
  echo "[INFO] run_game_perseverance_tests started at $(date)"
  echo "[INFO] output file: ${OUTPUT_FILE}"
  echo "[INFO] knobs from tests/common/Tests.hpp:"
  rg -n "TEST_LOOP_COUNT|GAME_TEST_DATA_LENGTH|TEST_SEED_GLOBAL|REPEATABILITY_ENABLED" \
    "${ROOT_DIR}/tests/common/Tests.hpp" || true

  trials="$(read_constant_int TEST_LOOP_COUNT)"
  password_bytes="$(read_constant_int GAME_TEST_DATA_LENGTH)"
  echo "[INFO] trials=${trials} password_bytes=${password_bytes}"
  echo

  echo "[STEP] Cleaning build directory"
  rm -rf "${BUILD_DIR}"

  echo "[STEP] Configuring Release build"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

  echo "[STEP] Building game_perseverance_tests"
  cmake --build "${BUILD_DIR}" --target game_perseverance_tests -j"$(detect_jobs)"
  echo

  echo "[STEP] Running game_perseverance_tests"
  "${BUILD_DIR}/game_perseverance_tests"
  echo

  echo "[PASS] run_game_perseverance_tests finished at $(date)"
} | tee "${OUTPUT_FILE}"
