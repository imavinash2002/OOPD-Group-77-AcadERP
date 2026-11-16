<!-- group-77 -->
# AcadERP — Mini Academic Records & Graduation Audit System

**OOPD course project | C++17 | Required milestones 1–4 + all three bonus features**

AcadERP is a command-driven academic records and graduation audit application. It accepts one command per line on standard input, sends the exact specified results to standard output, and prints human-readable error details to standard error. An invalid command prints `ERROR <line> <CODE>` and **does not change any academic state**, then execution continues.

The core system implements the required features from the nine-page *Mini-ERP: Academic Records & Graduation Audit System* assignment: Regular/SU/Thesis courses, grades, course registrations and repeats, thesis state changes, integer GPA rounding, average letter grades, registration eligibility E1–E7, graduation audit R1–R7, all typed errors, and `TRANSCRIPT`.

> **Bonus implemented:** `SAVE`/`LOAD` (+4), `CHANGE` with DEAN authorization and persistent audit log (+3), and `StudentAllocator` with slab growth, reuse and shrink (+3). These features are worth **up to 10 additional marks** at the grader's discretion; local testing cannot guarantee the awarded bonus.

## 1. Build and run

### Linux / WSL / macOS with g++ and make

```bash
make
./acaderp < assignment_sample.in
make test
```

The `make test` target executes all 75 black-box input/output pairs and two compiled C++ unit checks (exception safety/deep copies and bonus behavior including 3,200-object allocator stress). The supplied `run_tests.sh` separately runs the input/output comparisons in the required submission format.

Clean compiled files with `make clean`.

### Windows PowerShell / VS Code

From the folder containing `src`:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Werror -O2 .\src\grp_77_CommandParser.cpp .\src\grp_77_Course.cpp .\src\grp_77_Grade.cpp .\src\grp_77_Registrar.cpp .\src\grp_77_Student.cpp .\src\grp_77_StudentAllocator.cpp .\src\grp_77_ThesisRecord.cpp .\src\grp_77_main.cpp -o acaderp.exe
Get-Content .\assignment_sample.in | .\acaderp.exe
powershell -ExecutionPolicy Bypass -File .\run_tests.ps1
```

For the required C++ unit check on Windows (also run the second test by replacing `grp_77_unit_checks.cpp` with `grp_77_bonus_unit_checks.cpp`):

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Werror -O2 -I.\src .\tests\grp_77_unit_checks.cpp .\src\grp_77_CommandParser.cpp .\src\grp_77_Course.cpp .\src\grp_77_Grade.cpp .\src\grp_77_Registrar.cpp .\src\grp_77_Student.cpp .\src\grp_77_StudentAllocator.cpp .\src\grp_77_ThesisRecord.cpp -o unit_checks.exe
.\unit_checks.exe
```

No external libraries are needed. **Compilation flags** are `-std=c++17 -Wall -Wextra -pedantic -Werror`. The Makefile also enables `-O2`.

### Worked sample from the assignment

```text
ERROR 14 E_GRADE_TYPE
GPA 2023CS01 8.71
ELIGIBLE 2023CS01 CS499 NO E4
GRAD 2023CS01 NO R1,R2,R5
GPA 2023CS01 8.05
AVG 2023CS01 B
GRAD 2023CS01 NO R1
ELIGIBLE 2023CS01 CS101 NO E2
```

A human-readable message is also sent to stderr for invalid commands, but it is *not* part of the graded stdout.

## 2. Commands

| Command | Meaning |
| --- | --- |
| `PROGRAM total=20 elective=3 mingpa=5.00 thesis_min_grade=C thesis_min_credits=10 thesis_min_gpa=6.00 maxload=12` | Optional rules before the first COURSE; all 7 settings or use defaults |
| `COURSE CS101 REGULAR 4 CORE` | Define a course |
| `COURSE CS102 REGULAR 4 CORE PRE=CS101:C` | Define a prerequisite with minimum grade |
| `COURSE CS201 REGULAR 4 ELECTIVE ANTI=CS202` | Define an anti-requisite |
| `STUDENT MT26033 Avinash` | Create a student |
| `REGISTER MT26033 CS101 2026-1` | Register a course after checking E1–E7 |
| `GRADE MT26033 CS101 2026-1 A` | Assign a grade or a thesis progress symbol |
| `CONVERT MT26033 CS499 B` | Convert an IN_PROGRESS thesis to a final letter |
| `GPA MT26033` | Exact GPA rounded half-up to 2 decimals |
| `AVG MT26033` | GPA-equivalent average grade |
| `ELIGIBLE MT26033 CS102 2026-2` | Show eligibility or earliest failing E-code |
| `GRAD MT26033` | Graduation decision or all failing R-rules |
| `TRANSCRIPT MT26033` | Deterministic transcript; documented below |
| `SAVE records.acaderp` | Bonus: atomically save replayable commands and audit history |
| `LOAD records.acaderp` | Bonus: validate and restore full state without losing the old state on failure |
| `CHANGE MT26033 CS101 2026-1 A DEAN corrected_final_grade` | Bonus: authorized correction of an already-final grade, with required reason |
| `AUDIT MT26033` | Bonus: query that student's append-only correction/deletion history |
| `REMOVE_STUDENT MT26033 DEAN records_retention_completed` | Bonus: DEAN-token-gated demonstration removal to exercise allocator shrinking |

Empty lines and comment lines starting with `#` are ignored, but still contribute to error line numbers. Both LF and CRLF are accepted.

## 3. Transcript format (Milestone 4)

The assignment permits any **documented transcript format**. This implementation prints **exactly one line**:

```text
TRANSCRIPT <roll> <name> | <course>@<term> grade=<value> counting=<YES|NO> improvement=<YES|NO> | ...
```

- Registrations are listed chronologically by registration **term**, then **course code**; multiple attempts for one course are retained.
- `grade=PENDING` means regular/SU registration is not graded. `I` is shown as `grade=I`.
- `counting=YES` identifies the latest **completed** attempt for that course, even if it is failed/terminated; only eligible final letter grades enter the GPA.
- `improvement=YES` identifies a one-time C-/D improvement registration.
- Thesis uses `grade=THESIS(<STATE>;PROGRESS=<term>:<S|X>[,...])`. State is one of `REGISTERED`, `IN_PROGRESS`, `AT_RISK`, `TERMINATED`, or `FINAL:<letter>`. With no progress, use `PROGRESS=NONE`.
- Students with no registrations have `TRANSCRIPT <roll> <name> EMPTY`.
- Unknown roll numbers throw `E_UNKNOWN_STUDENT`.

For example, after an A in CS1 and an ungraded thesis started in 2026-2:

```text
TRANSCRIPT M Mia | CS1@2026-1 grade=A counting=YES improvement=NO | TH@2026-2 grade=THESIS(REGISTERED;PROGRESS=NONE) counting=NO improvement=NO
```

The string is completely assembled **before** printing, so exceptions cannot produce half-written transcript lines.

## 4. Domain behavior

- **REGULAR:** A, A-, B, B-, C, C-, D, E, F, I. Final A–D pass; E/F fail.
- **SU:** S, X, I. S earns credits but never enters GPA.
- **THESIS:** Progress only S or X using `GRADE`; an IN_PROGRESS thesis converts to a final A–F through `CONVERT`. Two consecutive progress X submissions terminate it. Progress terms must be strictly increasing and not earlier than the registration term.
- **Counting attempts:** latest completed attempt for a course, even if a later fail replaces an earlier pass. Open/I attempts are not counting attempts.
- **Improvement:** once only, for a REGULAR course whose latest final grade is C- or D. Its use is recorded on successful registration. After a failing counting attempt, a standard repeat may be allowed.
- **GPA:** includes final REGULAR and converted THESIS grades, including E/F, weighted by credits. Uses **64-bit integer arithmetic only**, rounded to two decimals using `(200 * numerator + denominator)/(2 * denominator)`; no `float`/`double` in the GPA path. A zero eligible denominator throws `E_NO_GPA`.
- **AVG:** letter band of the **rounded** GPA, not the unrounded rational.

### Eligibility (first failure wins)

| Check | Description | REGISTER exception |
| --- | --- | --- |
| E1 | Student, then course, must exist | `E_UNKNOWN_STUDENT` / `E_UNKNOWN_COURSE` |
| E2 | Course not passed, unless unused eligible improvement | `E_ALREADY_PASSED` |
| E3 | No existing open/I/same-term course attempt | `E_DUP_REG` |
| E4 | All prerequisites completed at required grade | `E_PREREQ` |
| E5 | No passed anti-requisite | `E_ANTIREQ` |
| E6 | New registered load within maximum for that term | `E_CREDIT_LIMIT` |
| E7 | Thesis entry GPA/credits met; no other thesis open/passed | `E_THESIS_NOT_ALLOWED` |

`ELIGIBLE` reports `YES` or `NO E2` to `NO E7`, but still throws on E1 and malformed input.

### Graduation (report **every** failed rule in R1–R7 order)

| Rule | Requirement |
| --- | --- |
| R1 | Total earned credits meets threshold |
| R2 | Every CORE course in catalogue passed |
| R3 | Passed ELECTIVE credits meets threshold |
| R4 | Every MANDATORY_SU course has S |
| R5 | At least one FINAL thesis with sufficient converted letter |
| R6 | Rounded GPA meets threshold; missing GPA data fails |
| R7 | No pending/I REGULAR or SU registrations |

## 5. Object-oriented design

- `Grade`: immutable value object with factory validation (`Grade::create`).
- `Course`: abstract base class. `RegularCourse`, `SuCourse`, and `ThesisCourse` override virtual grade validation, GPA eligibility, and credit eligibility.
- `Attempt`: one course registration with term, grade or thesis record, and improvement marker.
- `ThesisRecord`: encapsulates the five-state thesis lifecycle; only this class changes its thesis state.
- `Student`: roll, name, owned attempts; counting and open-attempt lookup.
- `ProgramRules`: validated thresholds with defaults.
- `Registrar`: facade exposing all required mutations and queries.
- `CommandParser`: converts input lines to validated command kind plus arguments.
- `ErpException` hierarchy: `InputException`, `EntityException`, `GradeException`, `RegistrationException`, and `ComputationException` with typed subclasses and fixed codes.
- `CourseMaterial`: abstract `clone()` returning `std::unique_ptr<CourseMaterial>`. A course owns a clone of its template, and `copyMaterialsForInstructor()` gives each faculty member a separate, editable, deep copy of syllabus, notes, and assignments. Tested with independently modified clones.

## 6. Exception safety and testing

The project uses *throw by value, catch by reference*; the command loop has one `catch (const ErpException&)`. Only `main()` has the permitted outer catch-all guard printing `FATAL` and exiting 2.

**Strong exception guarantee at mutation boundaries:**

1. `PROGRAM` is validated in a temporary `ProgramRules`, then committed through a non-throwing swap.
2. Course and student additions validate before inserting into owning maps.
3. `REGISTER` completes E1–E7 before inserting an attempt; a new attempt is constructed before vector insertion.
4. `GRADE` for REGULAR/SU allocates a replacement immutable grade before swapping it into the attempt.
5. `ThesisRecord::submitProgress` validates state/type/term and pushes history **before** switching the state. `convert` allocates the final grade before changing state.
6. Query result strings (including TRANSCRIPT) are fully created before printing.

**Automated coverage:** `tests/T01.in`–`T75.in` with exact `.out` expectations cover grade validity, GPA half-up, incomplete grades, thesis states and bad transitions, eligibility precedence, repeats/improvements, graduation rules, transcript order/content, parse failures, and state after rejected commands.

**Additional direct C++ assertions:** `tests/grp_77_unit_checks.cpp` compares transcript snapshots and GPA before/after invalid registrations, grades, final-grade rewrites, and illegal thesis conversions. It also verifies that instructor material clones have independent strings, notes, and assignments. `grp_77_bonus_unit_checks.cpp` exercises 3,200 pooled student allocations, slab reuse and reclamation, full SAVE/LOAD restoration, audit history, and authorization.

`make test` runs both sets; if any assertion or output diff fails, the command returns a nonzero exit code. The official sample is checked separately by running `./acaderp < assignment_sample.in` and comparing with `assignment_sample.out`.

## 7. Assumptions

Rules left open by the assignment are resolved as follows:

1. `PROGRAM` contains exactly seven distinct keys in any order; an absent PROGRAM uses the given defaults (160, 24, 5.00, C, 100, 6.00, 24). A repeated valid PROGRAM before the first COURSE replaces earlier rules.
2. Terms follow exactly `YYYY-1`, `YYYY-2`, or `YYYY-3`, ordered lexicographically by year and term. A new registration may refer to an earlier term than a different course's registration; the *same course and term* cannot be registered twice.
3. Names, roll numbers, and course codes are single non-whitespace tokens.
4. For the latest completed counting attempt, 'latest' means latest **registration term**, not the command submission order. An unconverted thesis is not a final result. A TERMINATED thesis is a final failed attempt with no GPA letter or earned credits.
5. I grades can be replaced exactly once with a grade valid for their course type; final regular/SU grades cannot change.
6. Thesis progress can first be entered during the start term, then only in strictly later terms. 'Consecutive X' refers to adjacent progress **submissions**, even if those submissions are not in consecutive terms.
7. A C-/D improvement slot is consumed at successful registration, not after its final grade is submitted. Failed counting results may be repeated; a repeat after failure is not treated as an additional improvement.
8. Anti-requisites may reference a not-yet-defined course because only prerequisites are explicitly required to exist at COURSE definition time. Unknown anti-requisites do not count as passed; self anti-requisites are disallowed.
9. In E7, `thesis_min_gpa=0.00` still requires that GPA data **exists**. An absent GPA is not treated as zero.
10. R2 checks **every CORE course currently defined** in the catalogue, registered or not. R5 requires at least one passing, converted thesis; without any such thesis, R5 fails.
11. The same course's attempts contribute to term load in their original registration term whether or not graded. An ongoing thesis adds load only in its start term.
12. TRANSCRIPT's invented format is documented in Section 3 and counts as one query output line.
13. Bonus commands are additional extensions and do not alter the required commands' output or grading. `CHANGE` accepts a final grade only, not a pending, I or terminated thesis, and refuses a no-op replacement. Thesis corrections require a final letter grade; SU corrections allow S/X. Any nonempty space-separated reason is accepted.
14. For `CHANGE` and `REMOVE_STUDENT`, the literal `DEAN` is a **demonstration authorization token**, not cryptographic authentication. A real college would use authenticated identities and role-based access control; the spec does not provide a secret or authentication protocol.
15. `SAVE`/`LOAD` paths are **single tokens** relative to the current working directory. Files use a versioned, line-oriented, checksummed command journal. This is deliberately a coursework persistence format, not a cryptographically signed or confidential database. `LOAD` validates in a separate Registrar before replacing live state. External changes to course materials are not exposed as commands and therefore are not part of the CLI save format.
16. Audit records are stored in-memory, exported by `AUDIT <roll>`, written to `audit.log` on each authorized change/removal, serialized through the command journal, and rebuilt on LOAD. The audit log contains educational test data; do not place real personal grades or secrets in this demonstration app.
17. `REMOVE_STUDENT` is an extra bonus-only command. It removes one student's registrations from the live system and releases completely unused allocator slabs; its audit history remains available.

## 8. Performance — workload above 3,000 students/hour

Let **N** = number of students, **C** = number of courses in the catalogue, and **A** = number of attempts for a particular student.

| Operation | Current complexity | Explanation |
| --- | --- | --- |
| Student/course lookup | `O(log N)` / `O(log C)` | Ordered `std::map` with stable pooled student pointers |
| Counting/open attempt | `O(A)` | Scan one student's attempts |
| GPA / GRAD | `O(C*A)` | For each catalogue course, inspect counting attempt |
| Eligibility | Typically `O(A + P*A)` plus optional GPA/credits scans | P prerequisites, anti-requisites, and term load; thesis adds GPA/earned-credit work |
| TRANSCRIPT | `O(A log A)` expected (plus hashing) | Cache counting records, stable sort attempts |
| Storage | `O(N + C + total attempts + progress records + J)` | Student slabs plus course data and J saved journal events |

At a 3,000-student/hour scale, the above is practical for ordinary catalogue and transcript sizes, but the per-course scan inside GPA/GRAD may become costly with very large histories. A natural optimization would maintain `courseCode -> latest completed attempt` and a per-term registered-credit index for each student. To keep correctness, invalidate/update indices only **after** a successful registration, final grade, thesis termination, or conversion; never on an error. Precomputing course dependencies also reduces repeated lookups. A cached GPA must be invalidated when a grade becomes final or a counting attempt changes. Parallel processing of independent students is possible in a service deployment, but this specified CLI is intentionally single-threaded to preserve command order and stdout determinism.

An optional reproducible workload driver is provided:

```bash
make
python3 tools/benchmark.py --students 3200
```

It generates **3,200 students**, **35,203 commands**, and **12,800 query outputs**, then validates GPA/AVG/GRAD and transcript results. A run in the development environment completed in about **0.06 seconds** (hardware-dependent; not a real-world service throughput guarantee). Since input is already in memory, this measures parsing and processing rather than disk/network delivery.


## 9. Bonus features — detailed explanation

### Bonus A: SAVE / LOAD (+4)

`SAVE <file>` saves a **versioned journal of successful mutating commands**. The journal includes full course definitions, students, registrations and improvements, regular and thesis grade histories, DEAN changes, removals, and the audit trail. Failed commands are never recorded. A count and 64-bit FNV-1a checksum catch truncation and accidental damage. The app first writes to a sibling temporary file and renames it after a successful flush (best-effort replacement on Windows).

`LOAD <file>` checks the header and checksum, replays every command into a **temporary Registrar**, and commits with nonthrowing swaps only if the entire replay succeeds. Missing, malformed, or inconsistent input raises `E_STORAGE`, leaving the prior live academic state untouched. On successful LOAD the on-disk `audit.log` is regenerated from the restored history. Paths do not allow spaces in this console grammar. The demo journal is not encrypted; never store real private academic data in it.

```text
SAVE my_backup.acaderp
LOAD my_backup.acaderp
```

### Bonus B: CHANGE / DEAN / audit log (+3)

Syntax:

```text
CHANGE <roll> <course> <registration-term> <new-grade> DEAN <reason...>
AUDIT <roll>
```

`CHANGE` works only on an already-final Regular or SU grade or a converted Thesis letter. It validates the new grade against the course, records **old -> new**, and recalculates GPA and graduation using the corrected counting attempt. Unapproved token = `E_AUTH`; pending/I or identical grade = `E_CHANGE_STATE`; grade scale/type and registration failures retain the existing typed error codes. No audit line is written for rejected commands. The append-only log is stored in `audit.log` in the current directory and rebuilt from the snapshot on LOAD. `AUDIT` prints matching entries as a single, documented line.

The assignment does not define real dean credentials, so the case-sensitive word `DEAN` is a **demonstration role token only**, not a security control suitable for deployment.

### Bonus C: growing and shrinking StudentAllocator (+3)

`StudentAllocator` (`grp_77_StudentAllocator.h` / `.cpp`) is integrated into `Registrar`, not merely described. It owns **64-student aligned slabs** and creates `Student` instances with placement-new only when a slot is needed. The Registrar's `StudentHandle` RAII pointer returns each student to the pool upon deletion. Empty slots are reused, student addresses remain stable despite growth, and completely empty slabs are released by `releaseUnusedBlocks()` when the privileged `REMOVE_STUDENT` command runs. The pool destroys any remaining live objects when it is itself destroyed.

```text
REMOVE_STUDENT MT26033 DEAN retention_completed
```

The direct unit check allocates **3,200** students (50 blocks), tests slot reuse, deallocates them, and checks that capacity shrinks to zero. It also adds 140 students to a Registrar, removes 130, and verifies capacity decreases from 192 to 64 slots. The allocator is not thread-safe: the requested CLI processes commands sequentially.

### Sample bonus workflow

```bash
make
./acaderp < bonus_example.in
```

Expected output is in `bonus_example.out`. New bonus codes: `E_STORAGE`, `E_AUTH` and `E_CHANGE_STATE`. All original fixed error codes are unchanged.

## 10. Submission contents

```text
AcadERP_Final/
  src/                  All required .cpp/.h files
  tests/                T01..T75.in/.out, grp_77_unit_checks.cpp, grp_77_bonus_unit_checks.cpp
  Makefile              Produces the executable "acaderp"
  run_tests.sh          Compares test inputs to expected outputs
  run_tests.ps1         PowerShell version of the same tests
  run_unit_checks.sh    Direct C++ tests of exception safety/cloning
  README.md             Build, design, assumptions, performance
  assignment_sample.in  Assignment worked example
  assignment_sample.out Expected example output
  bonus_example.in      Demonstrates SAVE, CHANGE, LOAD and AUDIT
  bonus_example.out     Expected bonus example stdout
  tools/benchmark.py    Optional performance driver
```

No third-party dependencies, generated object files, or precompiled binaries are required in the final archive. Test runners create isolated temporary directories, so an existing `audit.log` in your project folder is preserved.

## Group 77 submission conventions

All C++ source/header files, including direct unit checks, use `grp_77_` filename prefixes and start with `// group-77`. Markdown files begin with `<!-- group-77 -->`; script comments follow the interpreter shebang. The `Makefile` and `.in`/`.out` files follow the stated exemptions.

Submission archive: **`group-77.zip`**. Only one member should upload it. Both members should contribute to one meaningfully organized GitHub repository. **Never commit or upload executables or compiled objects.** Build locally and clean with `make clean` before creating any replacement ZIP.
