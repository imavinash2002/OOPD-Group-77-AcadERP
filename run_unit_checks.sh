#!/bin/sh
# group-77
set -eu
cd "$(dirname "$0")"
root=$(pwd)
output=$(mktemp)
sandbox=$(mktemp -d)
trap 'rm -f "$output"; rm -rf "$sandbox"' EXIT HUP INT TERM
mkdir -p "$sandbox/tests"
for source in tests/grp_77_unit_checks.cpp tests/grp_77_bonus_unit_checks.cpp; do
    ${CXX:-g++} -std=c++17 -Wall -Wextra -pedantic -Werror -O2 -Isrc \
        "$source" src/grp_77_CommandParser.cpp src/grp_77_Course.cpp src/grp_77_Grade.cpp \
        src/grp_77_Registrar.cpp src/grp_77_Student.cpp src/grp_77_StudentAllocator.cpp src/grp_77_ThesisRecord.cpp \
        -o "$output"
    (cd "$sandbox" && "$output")
done
