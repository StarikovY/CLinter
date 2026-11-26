CLinter V1.0 — Official Release
Release date: November 25, 2026

CLinter is a cross-platform BASIC interpreter that combines the classic BASIC programming experience with modern graphics, sound, floating-point arithmetic and interactive programming capabilities.
Version 1.0 is the official stable release.

Supported Platforms
	•	Windows AMD64 (64-bit Intel and AMD)
	•	Windows ARM64
	•	macOS ARM64 (Apple Silicon)
Key Features
	•	Classic BASIC programming language support.
	•	Interactive command-line interpreter.
	•	Execution of BASIC source files.
	•	Interactive program editing, loading, saving and execution.
	•	Floating-point arithmetic and mathematical functions.
	•	String operations and multidimensional arrays.
	•	MAT matrix operations.
	•	DATA, READ and RESTORE statements.
	•	Text and graphics screen modes.
	•	MSX2-inspired graphics and sprite capabilities.
	•	High-resolution graphics up to 1280 × 720 with 256 colors.
	•	Graphics primitives with automatic floating-point coordinate and color rounding.
	•	Music and sound programming.
	•	Interactive keyboard controls and command history.
	•	Cross-platform graphics powered by SDL2.
Included Sample Programs
Program
Description
RegTest.bas
BASIC language functionality demonstration and regression test
DuckHuntText.bas
Classic text-mode Duck Hunt game
DuckHuntD1.bas
Low-resolution graphical Duck Hunt game
DuckHunt720p.bas
High-resolution graphical Duck Hunt game
Nebula.bas
Graphics demonstration
Spiral.bas
High-resolution rainbow spiral demonstration
StairwayCloudsDemo.bas
Graphics and music demonstration
Documentation
Each distribution package includes:
	•	Detailed BASIC Programming Manual — English.
	•	Detailed BASIC Programming Manual — Russian.
	•	README_FIRST.txt — Getting started.
	•	README.md — Project overview.
	•	License information.
Downloads
Platform
Distribution package
Windows AMD64
CLinter-1.0-win-amd64.zip
Windows ARM64
CLinter-1.0-win-arm64.zip
macOS ARM64
CLinter-1.0-macos-arm64.zip
All distributions are portable and include the required non-system runtime libraries.
SHA-256 checksums are provided for archive integrity verification.
Quality Assurance
All three Release configurations have successfully completed automated regression testing, manual BASIC regression testing, interactive testing and standalone distribution verification.
Validation
Windows AMD64
Windows ARM64
macOS ARM64
Automated CTests
56/56
56/56
57/57
BASIC regression tests
13/13
13/13
13/13
Interactive testing
PASS
PASS
PASS
Distribution verification
PASS
PASS
PASS
Total: 169 automated tests and 39 BASIC regression tests passed across three platforms.
One automated test is intentionally disabled on each Windows platform.
Installation
	1	Download the distribution archive for your platform.
	2	Extract the archive into a directory of your choice.
	3	Read README_FIRST.txt.
	4	Launch CLinter from a terminal or command prompt.
	5	Explore the included BASIC sample programs.
The macOS distribution uses ad-hoc code signing and is not Apple-notarized. macOS security approval may be required before execution.
Source Code
The complete source code, build scripts and regression tests are available in the public GitHub repository.
https://github.com/StarikovY/CLinter
Future Development
Future releases are planned to extend BASIC dialect compatibility, interactive editing and debugging capabilities, and platform support.
CLinter V1.0 establishes the stable foundation for future development.
Enjoy programming in BASIC again!
