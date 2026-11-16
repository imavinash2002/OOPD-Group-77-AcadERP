<!-- group-77 -->
# AcadERP — Bonus Features Quick Review

The main requirements are unchanged; this optional extension addresses all three
bonus entries from page 9 of the assignment PDF. For assumptions and limitations,
see Section 9 of README.md.

## 1. SAVE / LOAD (+4)

**Code:** `Registrar::save`, `Registrar::load`, `Registrar::replayEvent` in
`src/grp_77_Registrar.cpp`. Successful mutations are recorded in a checksummed,
versioned command journal. A failed mutation is discarded by the RAII
`JournalGuard`. During LOAD, the complete journal is checked and replayed into a
separate Registrar before the current state is replaced.

**Demonstration:** `tests/T61`, `T68`, `T70`–`T74`, plus `bonus_example.in`.

## 2. DEAN-approved CHANGE with audit log (+3)

**Syntax:**

```
CHANGE <roll> <course> <start-term> <new-grade> DEAN <reason words...>
AUDIT <roll>
```

**Code:** `Registrar::changeGrade`, `Attempt::replaceFinalGrade`, and
`ThesisRecord::replaceFinalGrade`. Corrections apply to an existing final
regular/SU grade or final thesis letter. No-op edits and pending/incomplete
attempts are rejected. All accepted changes append an old/new/reason entry to
`audit.log` and the in-memory append-only audit record, which SAVE/LOAD also
preserves. The fixed literal DEAN is a coursework authorization convention,
not actual secure user authentication.

**Demonstration:** `tests/T62`–`T65`, `T68`, `T75`.

## 3. Student Allocator for expansion and contraction (+3)

**Code:** `src/grp_77_StudentAllocator.h` / `src/grp_77_StudentAllocator.cpp` and
`Registrar::addStudent` / `Registrar::removeStudent`. The allocator uses
64-object blocks, aligned object slots, placement-new and RAII handles.
It reuses empty slots and releases fully empty blocks when populations shrink.
The extra command `REMOVE_STUDENT <roll> DEAN <reason>` exposes shrinking
through the command-driven CLI.

**Demonstration:** `tests/grp_77_bonus_unit_checks.cpp` verifies 3,200 students occupy
50 blocks, returned slots are reused, all blocks are reclaimed after removal,
and 140 Registrar students shrink from 192 to 64 slots after removal of 130.

## Reproduce the tests

```bash
make clean && make
make test
./acaderp < bonus_example.in
```

Expected: **75/75** black-box tests; two additional direct C++ unit checks
pass; the bonus example matches `bonus_example.out`.

The project remains C++17 / standard library only, warning-free under
`-Wall -Wextra -pedantic -Werror`. Actual grading, including bonus award,
is at the course evaluator's discretion.
