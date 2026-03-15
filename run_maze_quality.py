#!/usr/bin/env python3
import os
import re
import subprocess
import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parent
BUILD_DIR = ROOT_DIR / "build_release"
TESTS_HPP = ROOT_DIR / "tests" / "common" / "Tests.hpp"


def read_constant(name: str) -> str:
    content = TESTS_HPP.read_text(encoding="utf-8")
    match = re.search(rf"inline constexpr (?:int|bool) {re.escape(name)} = ([^;]+);", content)
    return match.group(1).strip() if match else "unknown"


def run(cmd):
    print(f"[STEP] {' '.join(cmd)}")
    subprocess.run(cmd, check=True, cwd=ROOT_DIR)


def main() -> int:
    print(f"[INFO] run_maze_quality started in {ROOT_DIR}")
    print("[INFO] knobs from tests/common/Tests.hpp:")
    for name in ("TEST_LOOP_COUNT", "MAZE_TEST_DATA_LENGTH", "TEST_SEED_GLOBAL", "REPEATABILITY_ENABLED"):
        print(f"  {name} = {read_constant(name)}")
    print()

    build_jobs = str(os.cpu_count() or 4)

    run([
        "cmake",
        "-S",
        str(ROOT_DIR),
        "-B",
        str(BUILD_DIR),
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG",
    ])
    print()

    build_targets = (
        "maze_path_finding",
        "maze_edge_connected_quick",
        "maze_edge_connected",
        "maze_edge_connected_prim_quick",
        "maze_edge_connected_prim",
        "maze_edge_connected_kruskal_quick",
        "maze_edge_connected_kruskal",
    )
    for target in build_targets:
        run([
            "cmake",
            "--build",
            str(BUILD_DIR),
            "--target",
            target,
            f"-j{build_jobs}",
        ])
        print()

    run([str(BUILD_DIR / "maze_path_finding")])
    print()

    run([str(BUILD_DIR / "maze_edge_connected_quick")])
    print()

    run([str(BUILD_DIR / "maze_edge_connected")])
    print()

    run([str(BUILD_DIR / "maze_edge_connected_prim_quick")])
    print()

    run([str(BUILD_DIR / "maze_edge_connected_prim")])
    print()

    run([str(BUILD_DIR / "maze_edge_connected_kruskal_quick")])
    print()

    run([str(BUILD_DIR / "maze_edge_connected_kruskal")])
    print()

    print("[PASS] run_maze_quality finished successfully")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"[FAIL] command exited with status {exc.returncode}", file=sys.stderr)
        raise SystemExit(exc.returncode)
