cmake_minimum_required(VERSION 3.16)

project(Bread LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(peanutbutter_core
  src/PeanutButter.cpp
  src/ArchiverCompatibility.cpp
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
  src/Tables/maze/MazeHelpers.cpp
  src/Tables/maze/MazePolicy.cpp
  src/Tables/maze/MazeSpecialEvents.cpp
  src/Tables/maze/MazeDirector.cpp
  src/Tables/games/GameBoard.cpp
  src/Tables/games/GamePlayDirector.cpp
  src/Tables/games/GamePowerUp.cpp
)

target_include_directories(peanutbutter_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
  target_compile_options(peanutbutter_core PRIVATE /W4)
else()
  target_compile_options(peanutbutter_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(bread_main
  src/core/main.cpp
)
target_link_libraries(bread_main PRIVATE peanutbutter_core)

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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests_smoke" OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests_regression")
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

add_executable(game_confusion_matrix
  tests_regression/Game_confusion_matrix.cpp
)
target_link_libraries(game_confusion_matrix PRIVATE peanutbutter_core)
target_include_directories(game_confusion_matrix
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(game_confusion_matrix PRIVATE /W4)
else()
  target_compile_options(game_confusion_matrix PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(password_expander_quality_report
  tests_regression/PasswordExpander_quality_report.cpp
)
target_link_libraries(password_expander_quality_report PRIVATE peanutbutter_core)
target_include_directories(password_expander_quality_report
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(password_expander_quality_report PRIVATE /W4)
else()
  target_compile_options(password_expander_quality_report PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(password_expander_confusion_matrix
  tests_regression/PasswordExpander_confusion_matrix.cpp
)
target_link_libraries(password_expander_confusion_matrix PRIVATE peanutbutter_core)
target_include_directories(password_expander_confusion_matrix
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(password_expander_confusion_matrix PRIVATE /W4)
else()
  target_compile_options(password_expander_confusion_matrix PRIVATE -Wall -Wextra -Wpedantic)
endif()

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

add_executable(TestAllScramblers
  tests_regression/TestAllScramblers.cpp
)
target_link_libraries(TestAllScramblers PRIVATE peanutbutter_core)
target_include_directories(TestAllScramblers
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(TestAllScramblers PRIVATE /W4)
else()
  target_compile_options(TestAllScramblers PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME TestAllScramblers
  COMMAND $<TARGET_FILE:TestAllScramblers> 10 ${CMAKE_CURRENT_BINARY_DIR}/TestAllScramblers_report.html
)
set_tests_properties(TestAllScramblers PROPERTIES
  LABELS "regression;verification;scramblers"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME PasswordExpanderQualityReport
  COMMAND password_expander_quality_report 2 hotdog password_expander_quality_report.html
)
set_tests_properties(PasswordExpanderQualityReport PROPERTIES
  LABELS "regression;verification;quality"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME PasswordExpanderConfusionMatrix
  COMMAND password_expander_confusion_matrix 2 hotdog password_expander_confusion_matrix.html
)
set_tests_properties(PasswordExpanderConfusionMatrix PROPERTIES
  LABELS "regression;verification;quality"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

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
