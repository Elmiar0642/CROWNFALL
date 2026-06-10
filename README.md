# crownfall_engine

Playable C engine scaffold for **CrownFall: Dice Court** with CLI notation play, JSONL logging, snapshots, a GTK3/Glade GUI board, and API stubs for future agents.

## Build

```sh
make
```

Output:

```sh
./bin/crownfall
```

GTK3 is required by default. The Makefile checks `pkg-config gtk+-3.0` and links the GUI build. For a CLI-only fallback build, use `make REQUIRE_GTK=0`.

## Run

```sh
./bin/crownfall
```

The default launch opens the GTK/Glade GUI setup screen. Console setup is available explicitly:

```sh
./bin/crownfall --cli
```

GUI startup collects:

- mode: GUI or notation
- team count: `2`, `4`, `6`, or `8`
- time travel: `enable` or `disable`
- active Houses
- King, Left House, and Right House player names per House

The GUI provides House defaults, deterministic role-aware random names, and blank-name autofill so 8-team setup does not require typing all 24 player names.

Agent stdin protocol:

```sh
./bin/crownfall --agent
```

Supported first-pass commands: `STATE`, `LEGAL <player_id>`, `ROLL`, `MOVE <from> <to>`, `APPLY <move_json>`, `BRANCH <turn_id>`, `QUIT`.

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

Each log line is a JSON event with timestamp, session id, mode, team count, current player, current House, player role, player name, and event details. Implemented event emission includes session start/end, config, House/player registration, turn start, dice roll, legal move generation, move attempt/success, capture, Bloodfall, Widow Freeze, branch creation, and session end. The GUI side pane mirrors human-readable events live. Advanced rule events are represented as clean extension points.

## Implementation Notes

- 2-team board: fully playable 9x9.
- 4-team board: 15x15 cross board with four 3x3 corners unplayable, using South/East/North/West court-arm legion placement.
- 8-team board: two 4-team cross-board layers shown side by side in GUI, `378` playable squares.
- 6-team board: experimental WIP notice, no crash.
- Canonical Houses are configured in `config/houses.json`; missing House PNGs in `assets/houses/` fall back to text/color placeholders.
- GUI court arms show House names, mottos, and House colors.
- Sliding pieces use dice sum after `roll`; if unrolled, a conservative limit of `1` is used.
- GUI board rendering uses `GtkDrawingArea` and draws valid squares, invalid holes, court-zone coloring, coordinates, pieces, selected piece, legal moves, and last move.
- GUI movement supports mouse select, legal move highlights, click-to-move, dice warning for sliding pieces, and live log refresh.
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
- `src/gui.c`, `src/gui.h`, `ui/crownfall.glade`: GTK3 GUI setup wizard, DrawingArea board, mouse movement, and live log pane.

More detailed path/line/column anchors are in `docs/IMPLEMENTATION_INDEX.md`.
