cmake_minimum_required(VERSION 3.20)
project(CLinter LANGUAGES C CXX)

# All build definitions live under builds/cmake_config.
get_filename_component(CLINTER_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CLINTER_SRC_DIR "${CLINTER_REPO_ROOT}/CLinter")
if(NOT DEFINED CLINTER_DROP_ROOT OR CLINTER_DROP_ROOT STREQUAL "")
  if(APPLE)
    set(CLINTER_PLATFORM "macos-arm64")
  elseif(WIN32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
      set(CLINTER_PLATFORM "win-arm64")
    else()
      set(CLINTER_PLATFORM "win-amd64")
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CLINTER_PLATFORM "linux-x64")
  else()
    message(FATAL_ERROR "Unsupported platform: specify CLINTER_DROP_ROOT")
  endif()
  set(CLINTER_DROP_ROOT "${CLINTER_REPO_ROOT}/builds/drops/${CLINTER_PLATFORM}")
endif()
set(CLINTER_SOURCES
  "${CLINTER_SRC_DIR}/basic_repl.cpp"
  "${CLINTER_SRC_DIR}/basic_fileio.cpp"
  "${CLINTER_SRC_DIR}/basic_assignment.cpp"
  "${CLINTER_SRC_DIR}/data_table.cpp"
  "${CLINTER_SRC_DIR}/help.cpp"
  "${CLINTER_SRC_DIR}/matrix.cpp"
  "${CLINTER_SRC_DIR}/pycompat.cpp"
  "${CLINTER_SRC_DIR}/screen.cpp"
  "${CLINTER_SRC_DIR}/graphics.cpp"
  "${CLINTER_SRC_DIR}/music.cpp"
  "${CLINTER_SRC_DIR}/keyboard.cpp"
  "${CLINTER_SRC_DIR}/main.cpp"
  "${CLINTER_SRC_DIR}/Parse.cpp"
  "${CLINTER_SRC_DIR}/printfunc.cpp"
  "${CLINTER_SRC_DIR}/strcon.cpp"
  "${CLINTER_SRC_DIR}/wxecut.cpp"
)
set_source_files_properties(
  "${CLINTER_SRC_DIR}/Parse.cpp"
  "${CLINTER_SRC_DIR}/matrix.cpp"
  "${CLINTER_SRC_DIR}/pycompat.cpp"
  "${CLINTER_SRC_DIR}/screen.cpp"
  "${CLINTER_SRC_DIR}/graphics.cpp"
  "${CLINTER_SRC_DIR}/music.cpp"
  "${CLINTER_SRC_DIR}/keyboard.cpp"
  "${CLINTER_SRC_DIR}/wxecut.cpp"
  "${CLINTER_SRC_DIR}/basic_fileio.cpp"
  "${CLINTER_SRC_DIR}/basic_assignment.cpp"
  PROPERTIES LANGUAGE C
)
add_executable(CLinter ${CLINTER_SOURCES})
# SDL2 supplies a native graphics window. Headless framebuffer mode is always
# available for deterministic CTest runs and build environments without SDL2.
find_package(SDL2 QUIET CONFIG)
if(NOT TARGET SDL2::SDL2)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(SDL2_PC QUIET IMPORTED_TARGET sdl2)
  endif()
endif()
if(TARGET SDL2::SDL2)
  target_link_libraries(CLinter PRIVATE SDL2::SDL2)
  target_compile_definitions(CLinter PRIVATE CLINTER_HAVE_SDL2=1)
elseif(TARGET PkgConfig::SDL2_PC)
  target_link_libraries(CLinter PRIVATE PkgConfig::SDL2_PC)
  target_compile_definitions(CLinter PRIVATE CLINTER_HAVE_SDL2=1)
else()
  message(WARNING "SDL2 not found: graphics work in headless framebuffer mode; install SDL2 for native window")
endif()

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
  set(CLINTER_REGRESSION_RUNNER "${CLINTER_REPO_ROOT}/Tests/regression/runner.py")
  add_test(NAME clinter_mat COMMAND "$<TARGET_FILE:CLinter>" "${CLINTER_REPO_ROOT}/Tests/regression/auto/MatTest.bas")
  set_tests_properties(clinter_mat PROPERTIES PASS_REGULAR_EXPRESSION "MAT PASS" TIMEOUT 20 LABELS "CLinter;regression")
  foreach(case_name IN ITEMS core_flow data_read string_ops math strings if arrays arrays_extended basic_1 factorial regression)
    add_test(NAME "clinter_${case_name}"
      COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REGRESSION_RUNNER}"
        --exe "$<TARGET_FILE:CLinter>" --case "${case_name}")
    set_tests_properties("clinter_${case_name}" PROPERTIES TIMEOUT 20 LABELS "CLinter;regression")
    if(case_name STREQUAL "basic_1")
      # BAS1.bas requires interactive INKEY$, not yet supported by CLinter.
      # Keep the test registered, but do not run an infinite loop unattended.
      set_tests_properties(clinter_basic_1 PROPERTIES DISABLED TRUE)
    endif()
  endforeach()
endif()

# Day 4 MAT I/O and negative regression cases.
if(BUILD_TESTING)
  foreach(mat_case IN ITEMS read input singular dimension_add dimension_multiply dimension_transpose dimension_inverse input_invalid)
    add_test(NAME "clinter_mat_${mat_case}"
      COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/mat_cases.py"
        --exe "$<TARGET_FILE:CLinter>" --case "${mat_case}")
    set_tests_properties("clinter_mat_${mat_case}" PROPERTIES TIMEOUT 20 LABELS "CLinter;regression;MAT")
  endforeach()
endif()

# PYLinter Stage 1 compatibility regression.
if(BUILD_TESTING)
  add_test(NAME clinter_pycompat_stage1
    COMMAND "$<TARGET_FILE:CLinter>" "${CLINTER_REPO_ROOT}/Tests/regression/auto/PyFunctionsTest.bas")
  set_tests_properties(clinter_pycompat_stage1 PROPERTIES
    PASS_REGULAR_EXPRESSION "PYFUNCTIONS PASS" TIMEOUT 20 LABELS "CLinter;regression;PYLinter")
endif()

# PYLinter Stage 2: ANSI-backed MSX text screen, including original user fixture.
if(BUILD_TESTING)
  add_test(NAME clinter_screen_colorscreen1
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/screen_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case colorscreen1
      --file "${CLINTER_REPO_ROOT}/Tests/regression/auto/ColorScreen1.bas")
  add_test(NAME clinter_screen_invalid
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/screen_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case invalid)
  set_tests_properties(clinter_screen_colorscreen1 clinter_screen_invalid PROPERTIES
    TIMEOUT 20 LABELS "CLinter;regression;PYLinter;SCREEN")
endif()

# PYLinter Stage 3: INKEY$ and PAUSE noninteractive smoke test.
if(BUILD_TESTING)
  add_test(NAME clinter_pycompat_inkey
    COMMAND "$<TARGET_FILE:CLinter>" "${CLINTER_REPO_ROOT}/Tests/regression/auto/InkeyTest.bas")
  set_tests_properties(clinter_pycompat_inkey PROPERTIES PASS_REGULAR_EXPRESSION "INKEY PASS" TIMEOUT 20 LABELS "CLinter;regression;PYLinter;INKEY")
endif()

# Day 5 graphics: deterministic pixel tests, no display server required.
if(BUILD_TESTING)
  add_test(NAME clinter_graphics_primitives
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case primitives)
  add_test(NAME clinter_graphics_invalid
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case invalid)
  set_tests_properties(clinter_graphics_primitives clinter_graphics_invalid PROPERTIES
    TIMEOUT 20 LABELS "CLinter;regression;GRAPHICS")
endif()

# Day 5 continuation: MSX2 graphics text and PLAY/MML, headless deterministic check.
if(BUILD_TESTING)
  add_test(NAME clinter_msx2_text_play
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/msx2_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case valid)
  add_test(NAME clinter_msx2_play_invalid
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/msx2_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case invalid)
  set_tests_properties(clinter_msx2_text_play clinter_msx2_play_invalid PROPERTIES
    TIMEOUT 20 LABELS "CLinter;regression;GRAPHICS;PLAY")
endif()
