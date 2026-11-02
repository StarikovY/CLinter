cmake_minimum_required(VERSION 3.20)
project(CLinter LANGUAGES C CXX)

# All build definitions live under builds/cmake_config.
get_filename_component(CLINTER_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CLINTER_SRC_DIR "${CLINTER_REPO_ROOT}/CLinter")
if(NOT DEFINED CLINTER_DROP_ROOT)
  set(CLINTER_DROP_ROOT "${CLINTER_REPO_ROOT}/builds/drops/local")
endif()
set(CLINTER_SOURCES
  "${CLINTER_SRC_DIR}/basic_repl.cpp"
  "${CLINTER_SRC_DIR}/basic_fileio.cpp"
  "${CLINTER_SRC_DIR}/basic_assignment.cpp"
  "${CLINTER_SRC_DIR}/data_table.cpp"
  "${CLINTER_SRC_DIR}/help.cpp"
  "${CLINTER_SRC_DIR}/main.cpp"
  "${CLINTER_SRC_DIR}/Parse.cpp"
  "${CLINTER_SRC_DIR}/printfunc.cpp"
  "${CLINTER_SRC_DIR}/strcon.cpp"
  "${CLINTER_SRC_DIR}/wxecut.cpp"
)
set_source_files_properties(
  "${CLINTER_SRC_DIR}/Parse.cpp"
  "${CLINTER_SRC_DIR}/wxecut.cpp"
  "${CLINTER_SRC_DIR}/basic_fileio.cpp"
  "${CLINTER_SRC_DIR}/basic_assignment.cpp"
  PROPERTIES LANGUAGE C
)
add_executable(CLinter ${CLINTER_SOURCES})
target_include_directories(CLinter PRIVATE "${CLINTER_SRC_DIR}")
set_target_properties(CLinter PROPERTIES
  C_STANDARD 17 C_STANDARD_REQUIRED YES
  RUNTIME_OUTPUT_DIRECTORY "${CLINTER_DROP_ROOT}/bin/$<CONFIG>"
)
if(MSVC)
  target_compile_definitions(CLinter PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_options(CLinter PRIVATE /W3)
else()
  target_compile_options(CLinter PRIVATE -Wall -Wextra)
endif()

# CLinter regression tests (configuration files remain in builds/cmake_config).
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  set(CLINTER_REGRESSION_RUNNER "${CLINTER_REPO_ROOT}/tests/regression/runner.py")
  foreach(case_name IN ITEMS core_flow data_read string_ops math strings if arrays arrays_extended basic_1 factorial star regression eof)
    add_test(NAME "clinter_${case_name}"
      COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REGRESSION_RUNNER}"
        --exe "$<TARGET_FILE:CLinter>" --case "${case_name}")
    set_tests_properties("clinter_${case_name}" PROPERTIES TIMEOUT 20 LABELS "CLinter;regression")
  endforeach()
endif()
