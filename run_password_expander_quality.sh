#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${ROOT_DIR}/build_release"
BLOCKS="${1:-100}"
PASSWORD="${2:-hotdog}"
OUTPUT_PATH="${3:-${ROOT_DIR}/password_expander_quality_report.html}"
CONFUSION_PATH="${4:-${ROOT_DIR}/password_expander_confusion_matrix.html}"

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

echo "[STEP] Building password expander report targets"
cmake --build "${BUILD_DIR}" \
  --target test_password_expander password_expander_quality_report password_expander_confusion_matrix \
  -j"$(detect_jobs)"

echo "[STEP] Verifying password expander behavior"
"${BUILD_DIR}/test_password_expander"

echo "[STEP] Running password_expander_quality_report blocks=${BLOCKS} password=${PASSWORD}"
"${BUILD_DIR}/password_expander_quality_report" "${BLOCKS}" "${PASSWORD}" "${OUTPUT_PATH}"

echo "[STEP] Running password_expander_confusion_matrix blocks=${BLOCKS} password=${PASSWORD}"
"${BUILD_DIR}/password_expander_confusion_matrix" "${BLOCKS}" "${PASSWORD}" "${CONFUSION_PATH}"

echo "[PASS] Combined report written to ${OUTPUT_PATH}"
echo "[PASS] Confusion-only report written to ${CONFUSION_PATH}"
