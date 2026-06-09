# crownfall_engine

Playable C engine scaffold for **CrownFall: Dice Court** with CLI notation play, JSONL logging, snapshots, a GTK3/Glade GUI shell, and API stubs for future agents.

## Build

```sh
make
```

Output:

```sh
./bin/crownfall
```

<<<<<<< HEAD
GTK3 is required by default. The Makefile checks `pkg-config gtk+-3.0` and links the GUI build. For a CLI-only fallback build, use `make REQUIRE_GTK=0`.
=======
GTK3 is optional at compile time. If `pkg-config gtk+-3.0` is available, the GUI is compiled in. Without GTK3 development headers, the binary still builds and text mode works.
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)

## Run

```sh
./bin/crownfall
```

<<<<<<< HEAD
The default launch opens the GTK/Glade GUI setup screen. Console setup is available explicitly:

```sh
./bin/crownfall --cli
```

GUI startup collects:

=======
Startup asks for:

- mode: `text` or `board-gui`
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)
- team count: `2`, `4`, `6`, or `8`
- time travel: `enable` or `disable`
- team names
- player names

Agent stdin protocol:

```sh
./bin/crownfall --agent
```

Supported first-pass commands: `STATE`, `ROLL`, `LEGAL <player>`, `MOVE <from> <to>`, `APPLY <move_json>`, `QUIT`.

## CLI Commands

- `help`
- `board`
- `roll`
- `moves <square>`
- `move <from> <to>`
- `state`
- `log`
- `branch <turn_id>`
- `quit`

Coordinates use `a1-i9` on 2-team boards, `a1-o15` on cross boards, and `L1:a1` / `L2:h7` on 8-team layered boards.

## Logs

Session logs are written automatically to:

```txt
logs/session_<timestamp>.jsonl
```

Snapshots are written to:

```txt
logs/snapshots/latest.json
```

<<<<<<< HEAD
Each log line is a JSON event with timestamp, session id, mode, team count, current player, and event details. Implemented event emission includes session start/end, config, team/player registration, turn start, dice roll, legal move generation, move attempt/success, capture, Bloodfall, Widow Freeze, branch creation, and session end. Advanced rule events are represented as clean extension points.
=======
Each log line is a JSON event. Implemented event emission includes session start/end, config, turn start, dice roll, legal move generation, move attempt/success, capture, Bloodfall, Widow Freeze, branch creation, and session end. Advanced rule events are represented as clean extension points.
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)

## Implementation Notes

- 2-team board: fully playable 9x9.
- 4-team board: 15x15 cross board with four 3x3 corners unplayable.
- 8-team board: two stacked 4-team layers, `378` playable squares.
- 6-team board: experimental WIP notice, no crash.
- Sliding pieces use dice sum after `roll`; if unrolled, a conservative limit of `1` is used.
- Full check/checkmate, Sacred Intercession, Ascension, deterministic replay, and undo are explicit TODO extension points in code.

## File Map

- `src/main.c`: launch flow and mode selection.
- `src/cli.c`, `src/cli.h`: text mode and stdin agent protocol.
- `src/engine.c`, `src/engine.h`: game state, setup, moves, captures, snapshots.
- `src/board.c`, `src/board.h`: layouts and coordinate parsing.
- `src/movegen.c`, `src/movegen.h`: movement generation.
- `src/rules.c`, `src/rules.h`: royal capture rules and advanced rule hooks.
- `src/dice.c`, `src/dice.h`: custom dice.
- `src/log.c`, `src/log.h`: JSONL logging.
- `src/replay.c`, `src/replay.h`: replay/time-travel stubs.
- `src/api.c`, `src/api.h`, `include/crownfall_api.h`: future agent API.
<<<<<<< HEAD
- `src/gui.c`, `src/gui.h`, `ui/crownfall.glade`: GTK3 GUI setup and board shell.
=======
- `src/gui.c`, `src/gui.h`, `ui/crownfall.glade`: GTK3 GUI shell.
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)

More detailed path/line/column anchors are in `docs/IMPLEMENTATION_INDEX.md`.
