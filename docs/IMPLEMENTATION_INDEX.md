# Implementation Index

Generated for the initial scaffold on 2026-06-09. Column anchors use `:1` unless otherwise noted because each referenced symbol begins at the first non-indented token on that line.

## Startup and Modes

- `src/main.c:8:1` - `main` handles normal startup and `--agent`.
- `src/main.c:19:1` - default startup routes to GUI; `--cli` routes to console setup.
- `src/cli.c:27:1` - `cf_prompt_config` asks for mode, team count, time travel, team names, and player names.
- `src/cli.c:68:1` - `cf_cli_run` implements text/notation commands.
- `src/gui.c:38:1` - default and deterministic random name support.
- `src/gui.c:87:1` - GUI setup helpers copy names into config safely.
- `src/gui.c:182:1` - DrawingArea renderer draws canonical engine board state.
- `src/gui.c:279:1` - mouse click handler selects pieces, highlights legal moves, and applies moves.
- `src/gui.c:404:1` - GTK3 GUI entry loads Glade and connects controls.

## Board and Coordinates

- `src/board.c:19:1` - `cf_board_init` creates 2-team, 4-team, 6-team WIP, and 8-team layouts.
- `src/board.c:49:1` - named board init wrappers expose 2-team, 4-team, and 8-team geometry.
- `src/board.c:87:1` - `cf_board_is_playable` rejects out-of-bounds and removed cross-board corners.
- `src/board.c:55:1` - `cf_parse_coord` supports `a1` and layered `L1:a1` notation.
- `src/board.c:78:1` - `cf_coord_to_string` serializes coordinates.
- `src/board.c:87:1` - `cf_board_playable_count` confirms 81, 189, and 378-square layouts.

## Engine State

- `src/engine.h:12:1` - piece type enum.
- `src/engine.h:31:1` - `CfPiece` data model.
- `src/engine.h:60:1` - `CfMercy` relation tracking.
- `src/engine.h:67:1` - `CfGame` full state model.
- `src/engine.c:103:1` - `cf_engine_new` allocates, configures, logs, and starts a session.
- `src/engine.c:123:1` - `cf_engine_free` closes logs and releases memory.
- `src/engine.c:130:1` - `cf_engine_start_turn` emits `turn_start`.
- `src/engine.c:210:1` - `cf_engine_next_turn` advances through generated player order.

## Pieces and Movement

- `src/engine.c:26:1` - `add_piece` creates a piece safely.
- `src/engine.c:43:1` - `setup_players` creates King, Left House, Right House per team.
- `src/engine.c:77:1` - canonical legion template defines K, QL/QR, PL/PR, L, R, N, B, and p1-p6.
- `src/engine.c:90:1` - court rotation maps legion template into South, East, North, and West arms.
- `src/engine.c:130:1` - `setup_pieces` places 2-team, 4-team, and 8-team legions on the canonical board.
- `src/movegen.c:9:1` - `add_move` validates destination and capture.
- `src/movegen.c:27:1` - `slide` applies dice-sum range limits.
- `src/movegen.c:52:1` - `cf_movegen_for_piece` implements King, Queen, Rook, Bishop, Knight, Prince, Love Interest, and Pawn movement.
- `src/engine.c:152:1` - `cf_engine_generate_moves` logs legal move generation.
- `src/engine.c:179:1` - `cf_engine_apply_move` validates, logs, captures, snapshots, and advances turns.

## Rules

- `src/rules.c:6:1` - `cf_rules_can_capture` prevents same-side captures, Prince-on-Prince captures, and pact-blocked Prince-on-Queen captures.
- `src/rules.c:21:1` - `cf_rules_team_in_check` TODO hook for complete check detection.
- `src/rules.c:28:1` - `cf_rules_repetition_legal` limits repeated full game-state keys to three.
- `src/engine.c:165:1` - `handle_capture` logs captures, Bloodfall, and Widow Freeze hooks.

## Dice and Logging

- `src/dice.c:6:1` - `cf_dice_seed`.
- `src/dice.c:10:1` - `cf_roll_custom_dice` uses Die A `{1,2,3,4,1,2}` and Die B `{1,2,3,1,2,3}`.
- `src/log.c:30:1` - `cf_log_open` creates log directories and `logs/session_<timestamp>.jsonl`.
- `src/log.c:42:1` - `cf_log_event` writes timestamped JSONL events with session metadata.
- `src/log.c:50:1` - `cf_log_close`.
- `src/engine.c:248:1` - `cf_engine_snapshot` writes full snapshot JSON.

## Time Travel, Replay, and Agent API

- `src/engine.c:270:1` - `cf_engine_branch` logs branch creation when time travel is enabled.
- `src/replay.c:5:1` - `cf_replay_log_file` TODO replay hook.
- `src/replay.c:11:1` - `cf_branch_from_log_file` TODO branch replay hook.
- `include/crownfall_api.h:10:1` - public API begins.
- `src/api.c:9:1` - public API implementation begins.
- `src/cli.c:129:1` - simple stdin protocol begins.
- `src/cli.c:144:1` - stdin protocol supports `BRANCH <turn_id>`.
- `include/crownfall_api.h:13:1` - JSON state and legal move API spellings begin.

## GUI and Glade

- `ui/crownfall.glade:4:3` - main GTK window.
- `ui/crownfall.glade:15:15` - setup tab grid.
- `ui/crownfall.glade:29:19` - team count selector.
- `ui/crownfall.glade:36:19` - time-travel check button.
- `ui/crownfall.glade:87:19` - Start Session button.
- `ui/crownfall.glade:41:19` - team count selector.
- `ui/crownfall.glade:102:19` - Randomize All Names button.
- `ui/crownfall.glade:108:19` - Use Default Names button.
- `ui/crownfall.glade:114:19` - Continue button.
- `ui/crownfall.glade:122:15` - board tab.
- `ui/crownfall.glade:133:35` - Roll Dice button.
- `ui/crownfall.glade:134:35` - Branch From Turn button.
- `ui/crownfall.glade:152:51` - Layer L1 DrawingArea.
- `ui/crownfall.glade:160:51` - Layer L2 DrawingArea.
- `ui/crownfall.glade:170:44` - side log pane.
- `ui/crownfall.glade:180:44` - Help tab text view.
