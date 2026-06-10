# CrownFall Engine v0.3.0

Release date: 2026-06-10

## Included

- GTK3/Glade GUI build in `bin/crownfall`.
- Canonical House metadata in `config/houses.json`.
- Asset-ready House directory at `assets/houses/`.
- Corrected legion pawn file placement across 2-House, 4-House, and 8-House boards.
- House-aware JSONL logging and GUI side-log summaries.
- House-aware setup defaults and deterministic role-aware random names.

## Binary

- Build locally with `make`.
- `bin/crownfall` is intentionally not committed because the GitHub repository LFS budget is exhausted.

## Known Issues

- GUI movement and side-log polish still need another gameplay pass.
- Piece graphics/sprites are not added yet; current GUI uses colored text pieces.
- Time-travel replay and undo remain stubs.
