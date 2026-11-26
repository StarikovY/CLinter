# CLinter 1.0 — Windows AMD64 / ARM64

CLinter is a classic BASIC interpreter inspired by the original LINTER project (1986–1988), with RT-11-style BASIC features, MSX2-inspired graphics, sprites, and music.

Project: https://github.com/StarikovY/CLinter

## Requirements
- Windows PC with an AMD64 (x64) or ARM64 processor
- Windows version compatible with the supplied Release build
- Command Prompt, PowerShell, or Windows Terminal for interactive BASIC input
- Microsoft Visual C++ Redistributable, if required by the supplied build

## Installation
1. Download the ZIP package corresponding to your Windows architecture:
   - `CLinter-1.0.0-win-amd64.zip` — Intel and AMD 64-bit processors.
   - `CLinter-1.0.0-win-arm64.zip` — Windows on ARM processors.
2. Extract the entire ZIP package to a permanent location, such as `C:\CLinter`.
3. Double-click `CLinter.exe` to start an interactive BASIC session.
4. Browse the included Examples and Documentation folders.

Keep all supplied DLL files, including `SDL2.dll`, in the same directory as `CLinter.exe`.

If Windows displays a Microsoft Defender SmartScreen warning for an unsigned build, consult Microsoft's security instructions and verify the download's origin before proceeding.

## Running programs
From CLinter's interactive prompt, use LOAD and RUN as described in the manual. You can also run a program by passing its filename to the executable in Command Prompt:

    CLinter.exe C:\CLinter\Examples\AI\Eliza.bas

When using PowerShell from the executable directory, use:

    .\CLinter.exe C:\CLinter\Examples\AI\Eliza.bas

The `CLinter` shell command is available from other directories only if the executable directory has been added to the Windows PATH environment variable. Extracting the ZIP package does not automatically configure PATH.

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
