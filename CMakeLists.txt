cmake_minimum_required(VERSION 3.16)

project(Bread LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bread_core
  src/core/BreadCore.cpp
  src/core/Glyph.cpp
  src/expansion/password_inflation/Inflater_Twenty.cpp
  src/expansion/key_expansion/PasswordExpanderA.cpp
  src/expansion/key_expansion/PasswordExpanderB.cpp
  src/expansion/key_expansion/PasswordExpanderC.cpp
  src/expansion/key_expansion/PasswordExpanderD.cpp
  src/expansion/key_expansion/PasswordExpanderE.cpp
  src/expansion/key_expansion/PasswordExpanderF.cpp
  src/expansion/key_expansion/PasswordExpanderG.cpp
  src/expansion/key_expansion/PasswordExpanderH.cpp
  src/expansion/key_expansion/PasswordBalloonerA.cpp
  src/counters/AESCounter.cpp
  src/counters/ARIA256Counter.cpp
  src/counters/ChaCha20Counter.cpp
  src/counters/MersenneCounter.cpp
  src/matrices/FastMatrix.cpp
  src/maze/Maze.cpp
  src/games/engine/GameBoard.cpp
  src/games/minigames/MatchThreeTapIslands.cpp
  src/games/minigames/MatchThreeTapStreaks.cpp
  src/games/minigames/MatchThreeSwapIslands.cpp
  src/games/minigames/MatchThreeSwapStreaks.cpp
  src/games/minigames/MatchThreeSlideIslands.cpp
  src/games/minigames/MatchThreeSlideStreaks.cpp
)

target_include_directories(bread_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/counters
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto
)

if(MSVC)
  target_compile_options(bread_core PRIVATE /W4)
else()
  target_compile_options(bread_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(bread_main
  src/core/main.cpp
)
target_link_libraries(bread_main PRIVATE bread_core)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
enable_testing()

add_executable(fast_matrix_round_trip_tests
  tests/round_trip/FastMatrix_round_trip_tests.cpp
)
target_link_libraries(fast_matrix_round_trip_tests PRIVATE bread_core)
target_include_directories(fast_matrix_round_trip_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(fast_matrix_round_trip_tests PRIVATE /W4)
else()
  target_compile_options(fast_matrix_round_trip_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME FastMatrixRoundTripTests COMMAND fast_matrix_round_trip_tests)
set_tests_properties(FastMatrixRoundTripTests PROPERTIES LABELS "round_trip")

add_executable(game_boards_round_trip_tests
  tests/round_trip/GameBoards_round_trip_tests.cpp
)
target_link_libraries(game_boards_round_trip_tests PRIVATE bread_core)
target_include_directories(game_boards_round_trip_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(game_boards_round_trip_tests PRIVATE /W4)
else()
  target_compile_options(game_boards_round_trip_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME GameBoardsRoundTripTests COMMAND game_boards_round_trip_tests)
set_tests_properties(GameBoardsRoundTripTests PROPERTIES LABELS "round_trip")

add_executable(maze_pathfinding_tests
  tests/round_trip/Maze_pathfinding_tests.cpp
)
target_link_libraries(maze_pathfinding_tests PRIVATE bread_core)
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
set_tests_properties(MazePathfindingTests PROPERTIES LABELS "round_trip")

add_executable(benchmark_counters_matrix_tests
  tests/benchmark/Benchmark_CountersAndMatrix.cpp
)
target_link_libraries(benchmark_counters_matrix_tests PRIVATE bread_core)
target_include_directories(benchmark_counters_matrix_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(benchmark_counters_matrix_tests PRIVATE /W4)
else()
  target_compile_options(benchmark_counters_matrix_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME BenchmarkCountersAndMatrix COMMAND benchmark_counters_matrix_tests)
set_tests_properties(BenchmarkCountersAndMatrix PROPERTIES
  LABELS "benchmark"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(benchmark_counter_speed_comparison
  tests/benchmark/Benchmark_CounterSpeedComparison.cpp
)
target_link_libraries(benchmark_counter_speed_comparison PRIVATE bread_core)
target_include_directories(benchmark_counter_speed_comparison
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(benchmark_counter_speed_comparison PRIVATE /W4)
else()
  target_compile_options(benchmark_counter_speed_comparison PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(benchmark_counter_uniformity_baseline
  tests/benchmark/Benchmark_CounterUniformityBaseline.cpp
)
target_link_libraries(benchmark_counter_uniformity_baseline PRIVATE bread_core)
target_include_directories(benchmark_counter_uniformity_baseline
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(benchmark_counter_uniformity_baseline PRIVATE /W4)
else()
  target_compile_options(benchmark_counter_uniformity_baseline PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME BenchmarkCounterUniformityBaseline COMMAND benchmark_counter_uniformity_baseline)
set_tests_properties(BenchmarkCounterUniformityBaseline PROPERTIES
  LABELS "benchmark"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(benchmark_counter_uniformity_all
  tests/benchmark/Benchmark_CounterUniformityAll.cpp
)
target_link_libraries(benchmark_counter_uniformity_all PRIVATE bread_core)
target_include_directories(benchmark_counter_uniformity_all
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(benchmark_counter_uniformity_all PRIVATE /W4)
else()
  target_compile_options(benchmark_counter_uniformity_all PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME BenchmarkCounterUniformityAll COMMAND benchmark_counter_uniformity_all)
set_tests_properties(BenchmarkCounterUniformityAll PROPERTIES
  LABELS "benchmark"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(benchmark_counter_predictability_all
  tests/benchmark/Benchmark_CounterPredictabilityAll.cpp
)
target_link_libraries(benchmark_counter_predictability_all PRIVATE bread_core)
target_include_directories(benchmark_counter_predictability_all
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(benchmark_counter_predictability_all PRIVATE /W4)
else()
  target_compile_options(benchmark_counter_predictability_all PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME BenchmarkCounterPredictabilityAll COMMAND benchmark_counter_predictability_all)
set_tests_properties(BenchmarkCounterPredictabilityAll PROPERTIES
  LABELS "benchmark"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(benchmark_password_expander_quality_speed
  tests/benchmark/Benchmark_PasswordExpanders_QualityAndSpeed.cpp
)
target_link_libraries(benchmark_password_expander_quality_speed PRIVATE bread_core)
target_include_directories(benchmark_password_expander_quality_speed
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(benchmark_password_expander_quality_speed PRIVATE /W4)
else()
  target_compile_options(benchmark_password_expander_quality_speed PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME BenchmarkPasswordExpanderQualityAndSpeed COMMAND benchmark_password_expander_quality_speed)
set_tests_properties(BenchmarkPasswordExpanderQualityAndSpeed PROPERTIES
  LABELS "benchmark"
  RUN_SERIAL TRUE
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(benchmark_games_speed_repeatability
  tests/benchmark/Benchmark_Games_Speed_Repeatability.cpp
)
target_link_libraries(benchmark_games_speed_repeatability PRIVATE bread_core)
target_include_directories(benchmark_games_speed_repeatability
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(benchmark_games_speed_repeatability PRIVATE /W4)
else()
  target_compile_options(benchmark_games_speed_repeatability PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(benchmark_minigames_speed_8192
  tests/benchmark/Benchmark_Minigames_Speed8192.cpp
)
target_link_libraries(benchmark_minigames_speed_8192 PRIVATE bread_core)
target_include_directories(benchmark_minigames_speed_8192
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(benchmark_minigames_speed_8192 PRIVATE /W4)
else()
  target_compile_options(benchmark_minigames_speed_8192 PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(counter_repeatability_tests
  tests/repeatability/Counters_repeatability_tests.cpp
)
target_link_libraries(counter_repeatability_tests PRIVATE bread_core)
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
set_tests_properties(CounterRepeatabilityTests PROPERTIES LABELS "repeatability")

add_executable(digest_repeatability_suite
  tests/repeatability/DigestRepeatabilitySuite.cpp
  tests/repeatability/DigestRepeatability_Short_Passwords.cpp
  tests/repeatability/DigestRepeatability_Medium_Passwords.cpp
  tests/repeatability/DigestRepeatability_Oversized_Passwords.cpp
  tests/repeatability/DigestRepeatability_Contaminated_Passwords.cpp
  tests/repeatability/DigestRepeatability_Manual_Passwords.cpp
)
target_link_libraries(digest_repeatability_suite PRIVATE bread_core)
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
set_tests_properties(DigestRepeatabilitySuite PROPERTIES LABELS "repeatability")

add_executable(test_password_expander
  tests/repeatability/Test_PasswordExpander.cpp
)
target_link_libraries(test_password_expander PRIVATE bread_core)
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
set_tests_properties(TestPasswordExpander PROPERTIES LABELS "repeatability")
else()
  message(STATUS "Skipping test/benchmark targets because tests/ directory is missing.")
endif()
