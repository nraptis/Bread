#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${ROOT_DIR}/build_release"
COMMAND="${1:-run}"

if [[ $# -gt 0 ]]; then
  shift
fi

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

configure_build() {
  local targets=("$@")
  echo "[STEP] Configuring Release build"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"

  echo "[STEP] Building archiver compatibility targets: ${targets[*]}"
  cmake --build "${BUILD_DIR}" \
    --target "${targets[@]}" \
    -j"$(detect_jobs)"
}

print_usage() {
  echo "Usage:"
  echo "  ${0} build"
  echo "  ${0} run [runner args]"
  echo "  ${0} test"
}

case "${COMMAND}" in
  build)
    configure_build archiver_compat_runner
    ;;
  run)
    configure_build archiver_compat_runner
    echo "[STEP] Running archiver compatibility runner"
    "${BUILD_DIR}/archiver_compat_runner" "$@"
    ;;
  test)
    configure_build archiver_compat_smoke_tests
    echo "[STEP] Running archiver compatibility smoke tests"
    ctest --test-dir "${BUILD_DIR}" -R ArchiverCompatibilitySmokeTests --output-on-failure
    ;;
  help|-h|--help)
    print_usage
    ;;
  *)
    echo "[FAIL] unknown command: ${COMMAND}" >&2
    print_usage
    exit 1
    ;;
esac
