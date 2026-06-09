# Changelog

## 2026-06-09 Initial Engine Scaffold

Created `crownfall_engine` as a C11/Makefile project.

### Added Files

- `Makefile:1:1`
  - Builds `./bin/crownfall`.
  - Uses GTK3 automatically when `pkg-config gtk+-3.0` is available.
- `README.md:1:1`
  - Build, run, CLI, logging, file-map, and implementation notes.
- `include/crownfall_api.h:1:1`
  - Public API declarations for game creation, state access, legal moves, dice, move application, snapshot, replay, branch, and agent callback registration.
- `src/main.c:1:1`
  - Startup path for normal CLI/GUI flow and `--agent` stdin protocol.
- `src/cli.c:1:1`, `src/cli.h:1:1`
  - Text command loop and setup prompts.
- `src/engine.c:1:1`, `src/engine.h:1:1`
  - Central game state, turn order, piece setup, move application, capture handling, Bloodfall logging, and snapshot writing.
- `src/board.c:1:1`, `src/board.h:1:1`
  - 2-team, 4-team, 6-team WIP, and 8-team board layout support.
- `src/movegen.c:1:1`, `src/movegen.h:1:1`
  - Dice-limited sliding, king/love one-step movement, knight jumps, prince hybrid movement, and pawn movement.
- `src/rules.c:1:1`, `src/rules.h:1:1`
  - Same-side capture prevention, Prince-vs-Prince ban, and Mercy Pact capture blocking.
- `src/dice.c:1:1`, `src/dice.h:1:1`
  - Custom dice A `{1,2,3,4,1,2}` and B `{1,2,3,1,2,3}`.
- `src/log.c:1:1`, `src/log.h:1:1`
  - JSONL event logger.
- `src/replay.c:1:1`, `src/replay.h:1:1`
  - Replay and branch placeholders.
- `src/api.c:1:1`, `src/api.h:1:1`
  - Local API wrapper implementation.
- `src/gui.c:1:1`, `src/gui.h:1:1`
  - GTK3/Glade GUI shell with turn label, dice label, roll button, log panel, and rules tab.
- `ui/crownfall.glade:1:1`
  - Main window definition.

### Current TODO Extension Points

- `src/rules.c:25:5`
  - Implement full check detection.
- `src/replay.c:7:5`
  - Parse JSONL logs into deterministic state.
- `src/replay.c:14:5`
  - Implement log replay to create branches.
