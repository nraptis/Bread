cmake_minimum_required(VERSION 3.16)

project(Bread LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

option(BREAD_BUILD_TOOLS "Build standalone Bread helper tools" OFF)
option(BREAD_BUILD_QT_APP "Build the Qt shell app when Qt is available" ON)
option(BREAD_BUILD_TESTS "Build Bread smoke and regression targets" OFF)

if(BREAD_BUILD_QT_APP)
  find_package(Qt5 COMPONENTS Widgets QUIET)
  if(NOT Qt5Widgets_FOUND)
    find_program(QMAKE_EXECUTABLE qmake)
    if(QMAKE_EXECUTABLE)
      execute_process(
        COMMAND ${QMAKE_EXECUTABLE} -query QT_INSTALL_PREFIX
        OUTPUT_VARIABLE QT_INSTALL_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      if(QT_INSTALL_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${QT_INSTALL_PREFIX}")
        find_package(Qt5 COMPONENTS Widgets QUIET)
      endif()
    endif()
  endif()
endif()

set(PEANUTBUTTER_CORE_SOURCES
  src/PeanutButter.cpp
  src/ArchiverCompatibility.cpp
  src/Tables/Tables.cpp
  src/Tables/TablesHelpers.cpp
  src/Tables/TablesTiming.cpp
  src/Tables/password_expanders/ByteTwister.cpp
  src/Tables/password_expanders/PasswordExpander.cpp
  src/Tables/counters/AESCounter.cpp
  src/Tables/counters/ARIA256Counter.cpp
  src/Tables/counters/ChaCha20Counter.cpp
  src/Tables/rng/Shuffler.cpp
  src/Tables/fast_rand/FastRand.cpp
  src/Tables/maze/MazeGrid.cpp
  src/Tables/maze/Maze.cpp
  src/Tables/maze/MazeCharacters.cpp
  src/Tables/maze/MazePolicy.cpp
  src/Tables/maze/MazeSpecialEvents.cpp
  src/Tables/maze/MazeDirector.cpp
  src/Tables/games/GameBoard.cpp
  src/Tables/games/GamePlayDirector.cpp
  src/Tables/games/GamePowerUp.cpp
)

add_library(peanutbutter_core
  ${PEANUTBUTTER_CORE_SOURCES}
)

add_library(peanutbutter_core_software
  ${PEANUTBUTTER_CORE_SOURCES}
)

target_include_directories(peanutbutter_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
  target_compile_options(peanutbutter_core PRIVATE /W4)
  target_compile_options(peanutbutter_core_software PRIVATE /W4)
else()
  target_compile_options(peanutbutter_core PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(peanutbutter_core_software PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(peanutbutter_core_software
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(peanutbutter_core_software PRIVATE PB_FORCE_SOFTWARE_MODE=1)

add_executable(bread_main
  src/core/main.cpp
)
target_link_libraries(bread_main PRIVATE peanutbutter_core)

if(BREAD_BUILD_TOOLS)
add_executable(maze_sim_matrix
  tools/Maze_sim_matrix.cpp
)
target_link_libraries(maze_sim_matrix PRIVATE peanutbutter_core)
target_include_directories(maze_sim_matrix
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(maze_sim_matrix PRIVATE /W4)
else()
  target_compile_options(maze_sim_matrix PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(archiver_compat_runner
  tools/Archiver_compat_runner.cpp
)
target_link_libraries(archiver_compat_runner PRIVATE peanutbutter_core)
target_include_directories(archiver_compat_runner
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(archiver_compat_runner PRIVATE /W4)
else()
  target_compile_options(archiver_compat_runner PRIVATE -Wall -Wextra -Wpedantic)
endif()
endif()

if(BREAD_BUILD_QT_APP AND Qt5Widgets_FOUND)
add_executable(main_qt MACOSX_BUNDLE WIN32
  main_qt.cpp
)
target_link_libraries(main_qt PRIVATE peanutbutter_core Qt5::Widgets)
target_include_directories(main_qt
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(main_qt PRIVATE /W4)
else()
  target_compile_options(main_qt PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(APPLE)
  get_filename_component(QT_LIBRARY_DIR "${Qt5Core_DIR}/../../" ABSOLUTE)

  set_target_properties(main_qt PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Bread"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.codex.bread"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
    BUILD_RPATH "${QT_LIBRARY_DIR}"
    INSTALL_RPATH "@executable_path/../Frameworks"
  )

  find_program(MACDEPLOYQT_EXECUTABLE
    NAMES macdeployqt
    HINTS
      "${QT_INSTALL_PREFIX}/bin"
  )

  if(MACDEPLOYQT_EXECUTABLE)
    add_custom_target(deploy_main_qt_bundle
      COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_BUNDLE_DIR:main_qt>" -always-overwrite -verbose=0
      COMMENT "Deploying Qt runtime into the main_qt app bundle"
      DEPENDS main_qt
      VERBATIM
    )
  else()
    message(STATUS "macdeployqt was not found; skipping the deploy_main_qt_bundle target.")
  endif()
endif()
endif()

if(BREAD_BUILD_TESTS AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests_smoke" OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests_regression"))
enable_testing()

add_executable(maze_pathfinding_tests
  tests_smoke/Maze_pathfinding_tests.cpp
)
target_link_libraries(maze_pathfinding_tests PRIVATE peanutbutter_core)
target_include_directories(maze_pathfinding_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(maze_pathfinding_tests PRIVATE /W4)
else()
  target_compile_options(maze_pathfinding_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME MazePathfindingTests COMMAND $<TARGET_FILE:maze_pathfinding_tests>)
set_tests_properties(MazePathfindingTests PROPERTIES LABELS "smoke;verification;round_trip")

add_executable(archiver_compat_smoke_tests
  tests_smoke/ArchiverCompatibility_smoke_tests.cpp
)
target_link_libraries(archiver_compat_smoke_tests PRIVATE peanutbutter_core)
target_include_directories(archiver_compat_smoke_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(archiver_compat_smoke_tests PRIVATE /W4)
else()
  target_compile_options(archiver_compat_smoke_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ArchiverCompatibilitySmokeTests COMMAND archiver_compat_smoke_tests)
set_tests_properties(ArchiverCompatibilitySmokeTests PROPERTIES LABELS "smoke;verification;compatibility")

add_executable(maze_path_finding
  tests_regression/MazePathFinding.cpp
)
target_link_libraries(maze_path_finding PRIVATE peanutbutter_core)
target_include_directories(maze_path_finding
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(maze_path_finding PRIVATE /W4)
else()
  target_compile_options(maze_path_finding PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME MazePathFinding COMMAND maze_path_finding)
set_tests_properties(MazePathFinding PROPERTIES
  LABELS "regression;verification;quality"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_maze_grid_validity
  tests_regression/TestMazeGridValidity.cpp
)
target_link_libraries(test_maze_grid_validity PRIVATE peanutbutter_core)
target_include_directories(test_maze_grid_validity
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(test_maze_grid_validity PRIVATE /W4)
else()
  target_compile_options(test_maze_grid_validity PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME TestMazeGridValidity COMMAND $<TARGET_FILE:test_maze_grid_validity>)
set_tests_properties(TestMazeGridValidity PROPERTIES
  LABELS "regression;verification;quality"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(maze_edge_connected_quick
  tests_regression/MazeEdgeConnectedQuick.cpp
)
target_link_libraries(maze_edge_connected_quick PRIVATE peanutbutter_core)
target_include_directories(maze_edge_connected_quick
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(maze_edge_connected_quick PRIVATE /W4)
else()
  target_compile_options(maze_edge_connected_quick PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME MazeEdgeConnectedQuick COMMAND $<TARGET_FILE:maze_edge_connected_quick>)
set_tests_properties(MazeEdgeConnectedQuick PROPERTIES
  LABELS "regression;verification;quality"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(maze_edge_connected_prim_quick
  tests_regression/MazeEdgeConnectedPrimQuick.cpp
)
target_link_libraries(maze_edge_connected_prim_quick PRIVATE peanutbutter_core)
target_include_directories(maze_edge_connected_prim_quick
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(maze_edge_connected_prim_quick PRIVATE /W4)
else()
  target_compile_options(maze_edge_connected_prim_quick PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME MazeEdgeConnectedPrimQuick COMMAND $<TARGET_FILE:maze_edge_connected_prim_quick>)
set_tests_properties(MazeEdgeConnectedPrimQuick PROPERTIES
  LABELS "regression;verification;quality"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(maze_edge_connected_kruskal_quick
  tests_regression/MazeEdgeConnectedKruskalQuick.cpp
)
target_link_libraries(maze_edge_connected_kruskal_quick PRIVATE peanutbutter_core)
target_include_directories(maze_edge_connected_kruskal_quick
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(maze_edge_connected_kruskal_quick PRIVATE /W4)
else()
  target_compile_options(maze_edge_connected_kruskal_quick PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME MazeEdgeConnectedKruskalQuick COMMAND $<TARGET_FILE:maze_edge_connected_kruskal_quick>)
set_tests_properties(MazeEdgeConnectedKruskalQuick PROPERTIES
  LABELS "regression;verification;quality"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(game_repeatability_tests
  tests_regression/Games_repeatability_tests.cpp
  tests_regression/RepeatabilitySuiteCore.cpp
  tests_regression/DigestRepeatability_Short_Passwords.cpp
  tests_regression/DigestRepeatability_Medium_Passwords.cpp
  tests_regression/DigestRepeatability_Oversized_Passwords.cpp
  tests_regression/DigestRepeatability_Contaminated_Passwords.cpp
  tests_regression/DigestRepeatability_Manual_Passwords.cpp
)
target_link_libraries(game_repeatability_tests PRIVATE peanutbutter_core)
target_include_directories(game_repeatability_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(game_repeatability_tests PRIVATE /W4)
else()
  target_compile_options(game_repeatability_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME GameRepeatabilityTests COMMAND game_repeatability_tests)
set_tests_properties(GameRepeatabilityTests PROPERTIES LABELS "regression;verification;repeatability")

add_executable(counter_repeatability_tests
  tests_regression/Counters_repeatability_tests.cpp
)
target_link_libraries(counter_repeatability_tests PRIVATE peanutbutter_core)
target_include_directories(counter_repeatability_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(counter_repeatability_tests PRIVATE /W4)
else()
  target_compile_options(counter_repeatability_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME CounterRepeatabilityTests COMMAND counter_repeatability_tests)
set_tests_properties(CounterRepeatabilityTests PROPERTIES LABELS "regression;verification;repeatability")

add_executable(password_expander_insane_tests
  tests_regression/PasswordExpanderInsaneTest.cpp
)
target_link_libraries(password_expander_insane_tests PRIVATE peanutbutter_core)
target_include_directories(password_expander_insane_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(password_expander_insane_tests PRIVATE /W4)
else()
  target_compile_options(password_expander_insane_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME PasswordExpanderInsaneTests COMMAND password_expander_insane_tests)
set_tests_properties(PasswordExpanderInsaneTests PROPERTIES LABELS "regression;verification;password_expander")

add_executable(avalanche_tests
  tests_regression/AvalancheTests.cpp
)
target_link_libraries(avalanche_tests PRIVATE peanutbutter_core)
target_include_directories(avalanche_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(avalanche_tests PRIVATE /W4)
else()
  target_compile_options(avalanche_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME AvalancheTests COMMAND avalanche_tests 2 4)
set_tests_properties(AvalancheTests PROPERTIES LABELS "regression;verification;avalanche")

add_executable(bit_inclusion_tests
  tests_regression/BitInclusionTests.cpp
)
target_link_libraries(bit_inclusion_tests PRIVATE peanutbutter_core)
target_include_directories(bit_inclusion_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(bit_inclusion_tests PRIVATE /W4)
else()
  target_compile_options(bit_inclusion_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME BitInclusionTests COMMAND bit_inclusion_tests 2 4)
set_tests_properties(BitInclusionTests PROPERTIES LABELS "regression;verification;bit_inclusion")

add_executable(fast_rand_repeatability_tests
  tests_regression/FastRand_repeatability_tests.cpp
)
target_link_libraries(fast_rand_repeatability_tests PRIVATE peanutbutter_core)
target_include_directories(fast_rand_repeatability_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(fast_rand_repeatability_tests PRIVATE /W4)
else()
  target_compile_options(fast_rand_repeatability_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME FastRandRepeatabilityTests COMMAND fast_rand_repeatability_tests)
set_tests_properties(FastRandRepeatabilityTests PROPERTIES LABELS "regression;verification;repeatability")

add_executable(byte_twister_poison_repeatability_tests
  tests_regression/ByteTwister_poison_repeatability_tests.cpp
)
target_link_libraries(byte_twister_poison_repeatability_tests PRIVATE peanutbutter_core)
target_include_directories(byte_twister_poison_repeatability_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(byte_twister_poison_repeatability_tests PRIVATE /W4)
else()
  target_compile_options(byte_twister_poison_repeatability_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ByteTwisterPoisonRepeatabilityTests COMMAND byte_twister_poison_repeatability_tests)
set_tests_properties(ByteTwisterPoisonRepeatabilityTests PROPERTIES LABELS "regression;verification;repeatability")

add_executable(table_fill_and_engine_benchmark
  tests_regression/Table_fill_and_engine_benchmark.cpp
)
target_link_libraries(table_fill_and_engine_benchmark PRIVATE peanutbutter_core)
target_include_directories(table_fill_and_engine_benchmark
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(table_fill_and_engine_benchmark PRIVATE /W4)
else()
  target_compile_options(table_fill_and_engine_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(tables_stage_benchmark_software
  tests_regression/Tables_stage_benchmark.cpp
)
target_link_libraries(tables_stage_benchmark_software PRIVATE peanutbutter_core_software)
target_include_directories(tables_stage_benchmark_software
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(tables_stage_benchmark_software PRIVATE PB_STAGE_BENCH_MODE=\"software\")
if(MSVC)
  target_compile_options(tables_stage_benchmark_software PRIVATE /W4)
else()
  target_compile_options(tables_stage_benchmark_software PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64)$")
add_executable(tables_stage_benchmark_arm_neon
  tests_regression/Tables_stage_benchmark.cpp
)
target_link_libraries(tables_stage_benchmark_arm_neon PRIVATE peanutbutter_core)
target_include_directories(tables_stage_benchmark_arm_neon
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(tables_stage_benchmark_arm_neon PRIVATE PB_STAGE_BENCH_MODE=\"arm-neon\")
if(MSVC)
  target_compile_options(tables_stage_benchmark_arm_neon PRIVATE /W4)
else()
  target_compile_options(tables_stage_benchmark_arm_neon PRIVATE -Wall -Wextra -Wpedantic)
endif()
endif()

add_executable(digest_repeatability_suite
  tests_regression/DigestRepeatabilitySuite.cpp
  tests_regression/RepeatabilitySuiteCore.cpp
  tests_regression/DigestRepeatability_Short_Passwords.cpp
  tests_regression/DigestRepeatability_Medium_Passwords.cpp
  tests_regression/DigestRepeatability_Oversized_Passwords.cpp
  tests_regression/DigestRepeatability_Contaminated_Passwords.cpp
  tests_regression/DigestRepeatability_Manual_Passwords.cpp
)
target_link_libraries(digest_repeatability_suite PRIVATE peanutbutter_core)
target_include_directories(digest_repeatability_suite
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(digest_repeatability_suite PRIVATE /W4)
else()
  target_compile_options(digest_repeatability_suite PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME DigestRepeatabilitySuite COMMAND digest_repeatability_suite)
set_tests_properties(DigestRepeatabilitySuite PROPERTIES LABELS "regression;verification;repeatability")

add_executable(test_password_expander
  tests_smoke/Test_PasswordExpander.cpp
)
target_link_libraries(test_password_expander PRIVATE peanutbutter_core)
target_include_directories(test_password_expander
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(test_password_expander PRIVATE /W4)
else()
  target_compile_options(test_password_expander PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME TestPasswordExpander COMMAND test_password_expander)
set_tests_properties(TestPasswordExpander PROPERTIES LABELS "smoke;verification;repeatability")

add_executable(game_perseverance_tests
  tests_regression/Games_perseverance_tests.cpp
  tests_regression/PerseveranceSuiteCore.cpp
)
target_link_libraries(game_perseverance_tests PRIVATE peanutbutter_core)
target_include_directories(game_perseverance_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(game_perseverance_tests PRIVATE /W4)
else()
  target_compile_options(game_perseverance_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME GamePerseveranceTests COMMAND game_perseverance_tests)
set_tests_properties(GamePerseveranceTests PROPERTIES LABELS "regression;verification;perseverance")

add_executable(game_stability_tests
  tests_regression/GameStabilityTests.cpp
)
target_link_libraries(game_stability_tests PRIVATE peanutbutter_core)
target_include_directories(game_stability_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(game_stability_tests PRIVATE /W4)
else()
  target_compile_options(game_stability_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME GameStabilityTests COMMAND game_stability_tests 2)
set_tests_properties(GameStabilityTests PROPERTIES LABELS "regression;verification;stability")

add_executable(maze_perseverance_tests
  tests_regression/Mazes_perseverance_tests.cpp
)
target_link_libraries(maze_perseverance_tests PRIVATE peanutbutter_core)
target_include_directories(maze_perseverance_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(maze_perseverance_tests PRIVATE /W4)
else()
  target_compile_options(maze_perseverance_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME MazePerseveranceTests COMMAND maze_perseverance_tests)
set_tests_properties(MazePerseveranceTests PROPERTIES LABELS "regression;verification;perseverance")

add_executable(maze_stability_tests
  tests_regression/MazeStabilityTests.cpp
)
target_link_libraries(maze_stability_tests PRIVATE peanutbutter_core)
target_include_directories(maze_stability_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(maze_stability_tests PRIVATE /W4)
else()
  target_compile_options(maze_stability_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME MazeStabilityTests COMMAND maze_stability_tests 2)
set_tests_properties(MazeStabilityTests PROPERTIES LABELS "regression;verification;stability")
else()
  message(STATUS "Skipping test/benchmark targets because tests/ directory is missing.")
endif()
