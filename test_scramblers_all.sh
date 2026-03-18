#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${ROOT_DIR}/build_release"
TRIALS="${1:-1000}"
REPORT_PATH="${2:-${ROOT_DIR}/test_all_scramblers_report.html}"

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

if [[ $# -gt 2 ]]; then
  echo "[FAIL] usage: $0 [trial_count] [report_path]" >&2
  exit 1
fi

echo "[STEP] Configuring Release build"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

echo "[STEP] Building TestAllScramblers"
cmake --build "${BUILD_DIR}" --target TestAllScramblers -j"$(detect_jobs)"

echo "[STEP] Running TestAllScramblers trials=${TRIALS} report=${REPORT_PATH}"
"${BUILD_DIR}/TestAllScramblers" "${TRIALS}" "${REPORT_PATH}"
echo "[PASS] Scrambler report written to ${REPORT_PATH}"
