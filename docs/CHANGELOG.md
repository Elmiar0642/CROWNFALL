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

## 2026-06-09 GUI Launch and Logging Fixes

### Changed Files

- `Makefile:4:1`
  - Added `REQUIRE_GTK ?= 1`.
  - Default builds now require GTK3 development files and link GTK3/Glade support when `pkg-config gtk+-3.0` is present.
  - CLI-only fallback remains available with `make REQUIRE_GTK=0`.
- `src/main.c:8:1`
  - Default `./bin/crownfall` now opens the GUI path.
  - Console startup now uses `./bin/crownfall --cli`.
  - Agent protocol remains `./bin/crownfall --agent`.
- `src/gui.c:1:1`
  - Replaced fallback-only GUI shell with a GTK setup screen and board screen.
  - GUI setup collects team count, time travel, team names, and player names without using CLI prompts.
  - GUI board screen supports roll dice, legal moves, move input, branch input, board rendering, and session log display.
- `ui/crownfall.glade:1:1`
  - Added setup tab, board tab, rules tab, setup controls, board text view, log text view, and move/branch controls.
- `src/log.c:29:1`
  - Ensures `logs/` and `logs/snapshots/` exist before creating a session log.
  - Every JSONL event now includes timestamp, session id, mode, team count, and current player metadata.
- `src/engine.c:103:1`
  - Session start logs include engine version and log path.
  - Config logs include playable square count and time-travel state.
  - Team and player registration events are emitted for both GUI and CLI sessions.
- `README.md:5:1`
  - Documented default GUI launch, explicit CLI launch, GTK build requirement, and richer log metadata.

## 2026-06-09 Canonical Court Placement and DrawingArea GUI

### Changed Files

- `src/engine.c:63:1`
  - Replaced horizontal-only team setup with a canonical legion template.
  - 2-team mode now uses the exact 9x9 South/North ranks.
  - 4-team mode rotates the legion into South, East, North, and West court arms.
  - 8-team mode repeats the 4-team placement on L1 for Teams 1-4 and L2 for Teams 5-8.
- `src/gui.c:1:1`
  - Replaced text-board GUI rendering with GTK `DrawingArea` rendering.
  - GUI now draws valid squares, invalid 3x3 corner holes, court zones, coordinates, pieces, selection, legal moves, and last move.
  - Added mouse selection and click-to-move.
  - Added dice warning before sliding moves when dice have not been rolled.
  - Added default names, deterministic random names, live side log, and Help content.
- `ui/crownfall.glade:1:1`
  - Rebuilt GUI layout around setup controls, two side-by-side layer drawing areas, right-side log pane, and Help tab.
- `src/board.c:49:1`, `src/board.h:30:1`
  - Added named board geometry helper wrappers: `board_init_2team`, `board_init_4team`, `board_init_8team`, `board_is_valid_square`, and `board_rotate_template`.
- `include/crownfall_api.h:10:1`, `src/api.c:11:1`
  - Added future-agent JSON API spellings: `cf_get_state_json`, `cf_get_legal_moves_json`, and `cf_branch_from_turn`.
- `src/cli.c:129:1`
  - Added stdin protocol support for `BRANCH <turn_id>`.
- `README.md:1:1`
  - Documented corrected court placement, DrawingArea GUI, mouse movement, side pane, name helpers, and updated agent protocol.
- `ui/crownfall.glade:137:19`, `src/gui.c:445:9`
  - Fixed collapsed board rendering by replacing the board/log `GtkPaned` with a horizontal box and giving both layer DrawingAreas stable 520x520 minimum sizes.

## 2026-06-10 House-Based Playable GUI Pass

### Changed Files

- `src/engine.h:20:1`, `src/engine.c:11:1`
  - Added canonical House metadata, House colors, mottos, emblems, asset paths, and active House lookup helpers.
  - Added explicit left/right piece roles for Queens, Princes, Bishops, Rooks, and Knights.
- `config/houses.json:1:1`
  - Added asset-ready default House configuration for Lion, Eagle, Dragon, Wolf, Cobra, Crocodile, Owl, and Albatross.
- `assets/houses/.gitkeep:1:1`
  - Added tracked placeholder directory for future PNG House banners.
- `src/gui.c:1:1`, `ui/crownfall.glade:1:1`
  - Updated setup to use House defaults and House-aware deterministic random player names.
  - Added click-to-select/move side-log messages, dice duplicate-roll guard, branch dialog, House court labels, House colors, piece tooltips, side-log filters, and disabled Undo placeholder.
- `src/log.c:34:1`, `src/cli.c:91:1`
  - JSONL events now include `event_type`, timestamp, current House, player role, player name, dice fields, and human-readable summaries without duplicate JSON keys.
- `src/api.c:8:1`, `include/crownfall_api.h:10:1`
  - Agent state JSON now exposes House metadata and piece identities.
