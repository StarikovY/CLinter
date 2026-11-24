# CLinter 1.0 — macOS ARM64

CLinter is a classic BASIC interpreter inspired by the original LINTER project (1986–1988), with RT-11-style BASIC features, MSX2-inspired graphics, sprites, and music.

Project: https://github.com/StarikovY/CLinter

## Requirements
- Apple Silicon Mac (ARM64)
- macOS compatible with the supplied Release build
- Terminal for interactive BASIC input

## Installation
1. Open `CLinter-1.0.0-macos-arm64.dmg`.
2. Drag `CLinter.app` to Applications.
3. Open `CLinter.app` to start an interactive Terminal session.
4. Browse the included Examples and Documentation folders. Copy them to a permanent location if they are outside the app bundle.

If macOS blocks an unnotarized build, consult Apple's security instructions and verify the download's origin before proceeding.

## Running programs
From CLinter's interactive prompt, use LOAD and RUN as described in the manual. You can also run a program by passing its filename to the executable in Terminal:

    CLinter /path/to/Examples/AI/Eliza.bas

The `CLinter` shell command is available only if the executable directory has been added to PATH; installing the app alone does not automatically configure PATH.

## Included examples
- `AI/Eliza.bas` — classic conversational chatbot demonstration.
- `Graphics/Nebula.bas` — colorful MSX2-style nebula.
- `Games/DuckHunt720p.bas` — high-resolution DuckHunt.
- `Games/DuckHuntD1.bas` — low-resolution DuckHunt.
- `Games/DuckHuntText.bas` — text-mode DuckHunt.
- `BASIC/RegTest.bas` — BASIC language demonstration/regression.

## Documentation
`Documentation/CLinter_BASIC_Manual_EN.pdf` — English BASIC reference.

## Support and source
Report issues and find source code at https://github.com/StarikovY/CLinter/issues

## License
See the accompanying `LICENSE` file. Any bundled third-party libraries or assets remain subject to their respective licenses.
