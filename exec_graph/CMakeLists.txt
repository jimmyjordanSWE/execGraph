cmake_minimum_required(VERSION 3.16)

project(exec_graph LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EG_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(EG_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(EG_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

function(eg_apply_sanitizers target_name)
  if(EG_ENABLE_TSAN)
    target_compile_options(${target_name} PRIVATE -fsanitize=thread)
    target_link_options(${target_name} PRIVATE -fsanitize=thread)
  endif()

  set(extra_sanitizers "")
  if(EG_ENABLE_ASAN)
    list(APPEND extra_sanitizers address)
  endif()
  if(EG_ENABLE_UBSAN)
    list(APPEND extra_sanitizers undefined)
  endif()
  if(extra_sanitizers)
    string(REPLACE ";" "," sanitizer_flags "${extra_sanitizers}")
    target_compile_options(${target_name} PRIVATE -fsanitize=${sanitizer_flags})
    target_link_options(${target_name} PRIVATE -fsanitize=${sanitizer_flags})
  endif()
endfunction()

add_library(eg_process_runtime
  src/runtime/process_runtime.cpp
)

target_include_directories(eg_process_runtime PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(eg_process_runtime PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

eg_apply_sanitizers(eg_process_runtime)

add_library(eg_graph_document
  src/graph/graph_document.cpp
)

target_include_directories(eg_graph_document PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(eg_graph_document PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

eg_apply_sanitizers(eg_graph_document)

add_library(eg_graph_core
  src/graph_core/graph_snapshot.cpp
)

target_include_directories(eg_graph_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(eg_graph_core PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(eg_graph_core PUBLIC eg_graph_document eg_process_runtime)

eg_apply_sanitizers(eg_graph_core)

add_executable(eg_demo_pipeline
  src/app/demo_pipeline.cpp
)

target_link_libraries(eg_demo_pipeline PRIVATE eg_graph_core eg_graph_document eg_process_runtime)
target_include_directories(eg_demo_pipeline PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(eg_demo_pipeline PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

eg_apply_sanitizers(eg_demo_pipeline)

enable_testing()

find_program(BASH_PROGRAM bash REQUIRED)

add_test(
  NAME smoke.toy_linux_workflow
  COMMAND
    ${BASH_PROGRAM}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/run_toy_workflow.sh
    $<TARGET_FILE:eg_demo_pipeline>
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/toy_linux.workflow
    "2 APPLE"
    "1 BANANA"
    "2 PEAR"
)

add_test(
  NAME smoke.unique_count_workflow
  COMMAND
    ${BASH_PROGRAM}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/run_toy_workflow.sh
    $<TARGET_FILE:eg_demo_pipeline>
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/unique_count.workflow
    "3"
)

add_test(
  NAME smoke.process_graph
  COMMAND
    ${BASH_PROGRAM}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/run_graph_workflow.sh
    $<TARGET_FILE:eg_demo_pipeline>
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/toy_process.graph
    "sink count:"
    "2 APPLE"
    "1 BANANA"
    "2 PEAR"
)

add_test(
  NAME smoke.invalid_process_graph
  COMMAND
    ${BASH_PROGRAM}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/run_invalid_graph.sh
    $<TARGET_FILE:eg_demo_pipeline>
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_cycle.graph
)

add_test(
  NAME smoke.failing_process_graph
  COMMAND
    ${BASH_PROGRAM}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/run_failing_graph.sh
    $<TARGET_FILE:eg_demo_pipeline>
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/failing_stderr.graph
)
