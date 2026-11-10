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
  "${CLINTER_SRC_DIR}/basic_b1b2.cpp"
  "${CLINTER_SRC_DIR}/basic_b3.cpp"
  "${CLINTER_SRC_DIR}/data_table.cpp"
  "${CLINTER_SRC_DIR}/help.cpp"
  "${CLINTER_SRC_DIR}/matrix.cpp"
  "${CLINTER_SRC_DIR}/pycompat.cpp"
  "${CLINTER_SRC_DIR}/screen.cpp"
  "${CLINTER_SRC_DIR}/graphics.cpp"
  "${CLINTER_SRC_DIR}/sprite.cpp"
  "${CLINTER_SRC_DIR}/filename.cpp"
  "${CLINTER_SRC_DIR}/music.cpp"
  "${CLINTER_SRC_DIR}/music_psg.cpp"
  "${CLINTER_SRC_DIR}/msx_events.cpp"
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
  "${CLINTER_SRC_DIR}/sprite.cpp"
  "${CLINTER_SRC_DIR}/filename.cpp"
  "${CLINTER_SRC_DIR}/music.cpp"
  "${CLINTER_SRC_DIR}/music_psg.cpp"
  "${CLINTER_SRC_DIR}/msx_events.cpp"
  "${CLINTER_SRC_DIR}/keyboard.cpp"
  "${CLINTER_SRC_DIR}/wxecut.cpp"
  "${CLINTER_SRC_DIR}/basic_fileio.cpp"
  "${CLINTER_SRC_DIR}/basic_assignment.cpp"
  "${CLINTER_SRC_DIR}/basic_b1b2.cpp"
  "${CLINTER_SRC_DIR}/basic_b3.cpp"
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
    if(case_name STREQUAL "basic_1" AND UNIX)
      add_test(NAME clinter_basic_1
        COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/basic1_pty.py"
          "$<TARGET_FILE:CLinter>" "${CLINTER_REPO_ROOT}/Tests/regression/legacy/Tests/BAS1.bas")
    else()
      add_test(NAME "clinter_${case_name}"
        COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REGRESSION_RUNNER}"
          --exe "$<TARGET_FILE:CLinter>" --case "${case_name}")
    endif()
    set_tests_properties("clinter_${case_name}" PROPERTIES TIMEOUT 20 LABELS "CLinter;regression")
    if(case_name STREQUAL "basic_1" AND NOT UNIX)
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

# MSX2 Graphics Phase G1: extended drawing and invalid syntax regression.
if(Python3_EXECUTABLE)
  add_test(NAME clinter_graphics_g1_valid
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_g1_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case valid)
  add_test(NAME clinter_graphics_g1_invalid
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_g1_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case invalid)
  set_tests_properties(clinter_graphics_g1_valid clinter_graphics_g1_invalid
    PROPERTIES LABELS "CLinter;GRAPHICS;regression")
endif()

# MSX2 Phase G2: page selection and COPY, deterministic headless tests.
if(Python3_Interpreter_FOUND)
  add_test(NAME clinter_graphics_g2_valid
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_g2_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case valid)
  add_test(NAME clinter_graphics_g2_invalid
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_g2_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --case invalid)
  set_tests_properties(clinter_graphics_g2_valid clinter_graphics_g2_invalid PROPERTIES LABELS "CLinter;regression;GRAPHICS" TIMEOUT 20)
endif()

# MSX SCREEN 2 compatibility: original fixture remains unchanged, infinite loop
# replaced only in a temporary automated-test copy.
if(BUILD_TESTING)
  add_test(NAME clinter_msx_blocks1
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/blocks1_cases.py"
      --exe "$<TARGET_FILE:CLinter>" --file "${CLINTER_REPO_ROOT}/Tests/regression/auto/blocks1.bas")
  set_tests_properties(clinter_msx_blocks1 PROPERTIES TIMEOUT 20 LABELS "CLinter;regression;GRAPHICS;SCREEN")
endif()

# MSX2 graphics G3: SCREEN 6/8, color ranges, and interruptible infinite loops.
if(BUILD_TESTING)
  foreach(g3_case IN ITEMS valid invalid interrupt)
    add_test(NAME "clinter_msx2_g3_${g3_case}"
      COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_g3_cases.py"
        --exe "$<TARGET_FILE:CLinter>" --case "${g3_case}")
    set_tests_properties("clinter_msx2_g3_${g3_case}" PROPERTIES TIMEOUT 20 LABELS "CLinter;regression;GRAPHICS;G3")
  endforeach()
endif()

# MSX2 graphics G4: palette programming, SCREEN 3/4, and DRAW movement.
if(BUILD_TESTING)
  foreach(g4_case IN ITEMS valid invalid)
    add_test(NAME "clinter_msx2_g4_${g4_case}"
      COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_g4_cases.py"
        --exe "$<TARGET_FILE:CLinter>" --case "${g4_case}")
    set_tests_properties("clinter_msx2_g4_${g4_case}" PROPERTIES TIMEOUT 20 LABELS "CLinter;regression;GRAPHICS;G4")
  endforeach()
endif()

# G5: graphics primitives, page isolation, and syntax validation.
if(BUILD_TESTING)
  foreach(g5_case IN ITEMS valid invalid)
    add_test(NAME "clinter_msx2_g5_${g5_case}"
      COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_g5_cases.py"
        --exe "$<TARGET_FILE:CLinter>" --case "${g5_case}")
    set_tests_properties("clinter_msx2_g5_${g5_case}" PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;GRAPHICS;G5")
  endforeach()
endif()

# G6: MSX2 sprites (headless, deterministic).
if(BUILD_TESTING)
  add_test(NAME clinter_msx2_g6_sprite
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/sprite_g6_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_msx2_g6_sprite PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;GRAPHICS;G6")
endif()

# G7 sprite magnification, collision controls and unquoted LOAD.
if(Python3_EXECUTABLE)
  add_test(NAME clinter_msx2_g7_sprite
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/sprite_g7_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_msx2_g7_sprite PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;GRAPHICS;G7")
endif()

# G8: extended MSX2 SCREEN 6/7/8 framebuffer pages and color boundaries.
if(BUILD_TESTING AND Python3_EXECUTABLE)
  add_test(NAME clinter_msx2_g8_extended
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/graphics_g8_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_msx2_g8_extended PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;GRAPHICS;G8")
endif()

# M1: simultaneous three-voice MSX BASIC PLAY/MML.
if(BUILD_TESTING AND Python3_EXECUTABLE)
  add_test(NAME clinter_msx2_m1_music
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/music_m1_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_msx2_m1_music PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;MUSIC;M1")
endif()

# M2: numeric notes and independent articulation per PLAY voice.
if(BUILD_TESTING AND Python3_EXECUTABLE)
  add_test(NAME clinter_msx2_m2_music
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/music_m2_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_msx2_m2_music PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;MUSIC;M2")
endif()

# G10 sprite collision interrupt dispatch + M3 PSG/PLAY events.
if(BUILD_TESTING AND Python3_EXECUTABLE)
  add_test(NAME clinter_msx2_g10_m3
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/g10_m3_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_msx2_g10_m3 PROPERTIES TIMEOUT 30 LABELS "CLinter;regression;GRAPHICS;MUSIC;G10;M3")
endif()

# B1+B2 BASIC language compatibility regression.
if(BUILD_TESTING AND Python3_EXECUTABLE)
  add_test(NAME clinter_b1b2_language
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/b1b2_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_b1b2_language PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;B1;B2")
endif()

# B3: variable type declarations and BASIC error-handler control flow.
if(Python3_EXECUTABLE)
  add_test(NAME clinter_b3_language
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/b3_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_b3_language PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;B3")
endif()

# B4: cross-subsystem MSX2 BASIC compatibility (headless, noninteractive).
if(BUILD_TESTING AND Python3_EXECUTABLE)
  add_test(NAME clinter_b4_integration
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/b4_integration_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_b4_integration PROPERTIES TIMEOUT 30 LABELS "CLinter;regression;B4;GRAPHICS;MUSIC")
endif()

# G11 + M4: sprite pattern variables and background music status.
if(BUILD_TESTING AND Python3_EXECUTABLE)
  add_test(NAME clinter_g11_m4_integration
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/g11_m4_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_g11_m4_integration PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;G11;M4;GRAPHICS;MUSIC")
endif()

if(BUILD_TESTING AND Python3_EXECUTABLE)
  add_test(NAME clinter_d1_duck_integration
    COMMAND "${Python3_EXECUTABLE}" "${CLINTER_REPO_ROOT}/Tests/regression/d1_duck_cases.py" "$<TARGET_FILE:CLinter>")
  set_tests_properties(clinter_d1_duck_integration PROPERTIES TIMEOUT 25 LABELS "CLinter;regression;D1;GRAPHICS;MUSIC")
endif()

# CLinter extension: 1280x720 indexed 256-color SCREEN 20.
if(BUILD_TESTING)
  add_test(NAME clinter_screen20_720p
    COMMAND "$<TARGET_FILE:CLinter>" "${CLINTER_REPO_ROOT}/Tests/regression/auto/Screen20AutoTest.bas")
  set_tests_properties(clinter_screen20_720p PROPERTIES
    PASS_REGULAR_EXPRESSION "SCREEN20 PASS"
    FAIL_REGULAR_EXPRESSION "SCREEN20 FAIL;ERROR:"
    TIMEOUT 20 LABELS "CLinter;regression;GRAPHICS;SCREEN20")
endif()
