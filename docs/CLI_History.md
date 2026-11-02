# CLinter interactive CLI and history

The interactive terminal supports Left/Right, Up/Down history recall, Home/End,
Backspace/Delete, Ctrl+A/E, Ctrl+U, Ctrl+C cancellation, and Ctrl+D on an empty line.

`HISTORY` prints retained input entries with monotonically increasing event numbers;
`HISTORY 10` prints the latest 10; `HISTORY CLEAR` empties the history.
`!5` recalls and **executes** history event 5. Re-executed lines are recorded as
new events; invalid or expired event numbers produce an error. History holds the
latest 100 nonempty submitted BASIC lines and immediate statements. HISTORY
meta-commands themselves are not recorded. `NEW` does not clear history.

Non-interactive stdin retains fgets-based input for scripts and regression tests.
History is in-memory only. On a terminal, Ctrl+C cancels the current input;
while running BASIC, the interpreter's existing SIGINT handling applies.

Build using the existing CMake configuration or Visual Studio project.
