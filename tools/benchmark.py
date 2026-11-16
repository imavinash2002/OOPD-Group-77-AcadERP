#!/usr/bin/env python3
# group-77
"""Generate and validate a deterministic multi-student AcadERP workload.

Run after `make` as: python3 tools/benchmark.py --students 3200
Uses only the Python standard library; not required for core grading.
"""
import argparse
from pathlib import Path
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--students', type=int, default=3200)
    args = parser.parse_args()
    if args.students < 1:
        parser.error('students must be >= 1')

    binary = Path(__file__).resolve().parent.parent / 'acaderp'
    if not binary.exists():
        parser.error('binary does not exist; first run make')

    lines = [
        'COURSE CS1 REGULAR 4 CORE',
        'COURSE CS2 REGULAR 4 CORE',
        'COURSE CS3 REGULAR 3 ELECTIVE',
    ]
    for i in range(args.students):
        roll = f'S{i:05d}'
        lines.extend((
            f'STUDENT {roll} User{i}',
            f'REGISTER {roll} CS1 2026-1',
            f'GRADE {roll} CS1 2026-1 A',
            f'REGISTER {roll} CS2 2026-1',
            f'GRADE {roll} CS2 2026-1 B',
            f'REGISTER {roll} CS3 2026-1',
            f'GRADE {roll} CS3 2026-1 C',
            f'GPA {roll}',
            f'AVG {roll}',
            f'GRAD {roll}',
            f'TRANSCRIPT {roll}',
        ))
    payload = '\n'.join(lines) + '\n'
    start = time.perf_counter()
    run = subprocess.run([str(binary)], input=payload, text=True,
                         capture_output=True, check=True)
    duration = time.perf_counter() - start
    output = run.stdout.splitlines()
    expected_count = args.students * 4
    if len(output) != expected_count:
        raise SystemExit(f'Expected {expected_count} outputs, got {len(output)}')
    for i in range(args.students):
        roll = f'S{i:05d}'
        rows = output[4*i:4*i+4]
        # (4*10 + 4*8 + 3*6) / 11 = 90/11 = 8.18
        if rows[0] != f'GPA {roll} 8.18':
            raise SystemExit(f'Wrong GPA output: {rows[0]}')
        if rows[1] != f'AVG {roll} B':
            raise SystemExit(f'Wrong AVG output: {rows[1]}')
        if rows[2] != f'GRAD {roll} NO R1,R3,R5':
            raise SystemExit(f'Wrong GRAD output: {rows[2]}')
        if not rows[3].startswith(f'TRANSCRIPT {roll} User{i} | '):
            raise SystemExit(f'Wrong transcript: {rows[3]}')
    print(f'Validated {args.students} students, {len(lines)} input commands, '
          f'{len(output)} query outputs in {duration:.3f} seconds')
    print('Benchmark depends on hardware and is not a latency guarantee.')


if __name__ == '__main__':
    main()
