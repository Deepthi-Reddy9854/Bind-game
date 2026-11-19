# Bind-game

## Brain Match — Test Your Memory

Brain Match is a graphical sequence-memory game written in C with Raylib. It
opens in its own Windows application window; game screens are not drawn in a
terminal.

## Project files

```text
main.c
game.c / game.h       Sequence gameplay and generated sound effects
menu.c / menu.h       Home screen, difficulty, instructions, and score screens
score.c / score.h     Persistent top-10 leaderboard
assets/               Raylib-drawn visuals; no image files required
sounds/               Runtime-generated tones; no sound files required
.vscode/              GCC build, launch, and IntelliSense settings
highscores.txt        Created automatically when it does not exist
```

## Requirements and installation

1. Install [MSYS2](https://www.msys2.org/) and Visual Studio Code.
2. Open the **MSYS2 UCRT64** terminal and install GCC, Raylib, and GDB:

   ```sh
   pacman -Syu
   ```

   If MSYS2 asks you to close the terminal, close it, reopen **MSYS2 UCRT64**,
   then run:

   ```sh
   pacman -Su
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-gdb
   ```

3. Add `C:\msys64\ucrt64\bin` to the Windows `PATH`. This lets Windows find
   `gcc.exe`, Raylib's DLL, and its runtime dependencies.
4. In VS Code, install the **C/C++** extension and open the project folder.

The `.vscode` settings assume the default MSYS2 installation path
`C:\msys64`. If MSYS2 is installed elsewhere, update the compiler and debugger
paths in `.vscode/tasks.json`, `.vscode/launch.json`, and
`.vscode/c_cpp_properties.json`.

## Build and run

From the project directory in PowerShell or the VS Code terminal:

```powershell
gcc -std=c99 -O2 -Wall -Wextra -Wpedantic -mwindows main.c game.c menu.c score.c -o brain_match_gui.exe -lraylib -lopengl32 -lgdi32 -lwinmm -lm
.\brain_match_gui.exe
```

The `-mwindows` option starts the GUI without a console window. The same build
is available with **Terminal → Run Build Task** (Ctrl+Shift+B); press F5 to
build and launch with the VS Code debugger.

The GUI build uses `brain_match_gui.exe` so an existing `brain_match.exe` is
not overwritten. If that older executable is present, it may still be the
previous console version; use the GUI-named executable for this project.

## Play

- Click **Start Game**, enter a name, and watch the sequence.
- Type the symbols in order. Press Enter or click **Submit**. Backspace edits
  the answer and **Clear** removes it.
- Correct answers increase the score and add a symbol to the next sequence.
- Wrong answers and expired answer timers each cost one of three lives.
- Easy uses digits at a relaxed pace; Medium uses letters; Hard mixes letters
  and digits at the fastest pace. Answer time shortens as levels rise.
- The game-over screen offers **Play Again** and **Return Home**.
- The leaderboard stores the top 10 scores in `highscores.txt`, relative to the
  current working directory. Its record format is
  `score|name|difficulty|level|seconds`. Existing five-column Brain Match
  score records remain readable.

Graphics are drawn with Raylib shapes and the built-in font. Short click,
correct, and incorrect tones are synthesized when the audio device is
available; the game still works if audio initialization fails.

## Troubleshooting

- **`raylib.h: No such file`**: install `mingw-w64-ucrt-x86_64-raylib` from
  the **MSYS2 UCRT64** terminal, not the MSYS shell.
- **`cannot find -lraylib`**: use the UCRT64 GCC at
  `C:\msys64\ucrt64\bin\gcc.exe` and confirm the Raylib package is installed.
- **`libraylib.dll was not found`**: add `C:\msys64\ucrt64\bin` to `PATH`,
  then reopen VS Code or the terminal.
- **The app exits on launch**: run the build from the VS Code task or terminal
  to see compiler errors. Confirm the Windows display driver supports OpenGL.
- **No sound**: check the Windows output device. Audio is optional; gameplay
  does not depend on it.
