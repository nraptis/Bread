#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build_release"
OUTPUT_FILE="${ROOT_DIR}/run_maze_speed_benchmarks_output.txt"

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
  echo "[INFO] run_maze_speed_benchmarks started at $(date)"
  echo "[INFO] output file: ${OUTPUT_FILE}"
  echo "[INFO] knobs from tests/common/Tests.hpp:"
  rg -n "TEST_LOOP_COUNT|MAZE_TEST_DATA_LENGTH|TEST_SEED_GLOBAL|BENCHMARK_ENABLED" \
    "${ROOT_DIR}/tests/common/Tests.hpp" || true

  trials="$(read_constant_int TEST_LOOP_COUNT)"
  password_bytes="$(read_constant_int MAZE_TEST_DATA_LENGTH)"
  echo "[INFO] trials=${trials} password_bytes=${password_bytes}"
  echo

  echo "[STEP] Cleaning build directories"
  find "${ROOT_DIR}" -maxdepth 1 -type d -name 'build*' -exec rm -rf {} +

  echo "[STEP] Configuring Release build (optimized)"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

  echo "[STEP] Building benchmark_maze_speed_repeatability"
  cmake --build "${BUILD_DIR}" --target benchmark_maze_speed_repeatability -j"$(detect_jobs)"
  echo

  echo "[STEP] Running benchmark_maze_speed_repeatability"
  "${BUILD_DIR}/benchmark_maze_speed_repeatability"
  echo

  echo "[PASS] run_maze_speed_benchmarks completed at $(date)"
} | tee "${OUTPUT_FILE}"
