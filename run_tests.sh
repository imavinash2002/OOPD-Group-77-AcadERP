#!/bin/sh
# group-77
set -eu
cd "$(dirname "$0")"
make -s acaderp
root=$(pwd)
sandbox=$(mktemp -d)
trap 'rm -rf "$sandbox"' EXIT HUP INT TERM
mkdir -p "$sandbox/tests"
cp tests/corrupt.save tests/invalid_event.save "$sandbox/tests/"
count=0
passed=0
for file in tests/T*.in; do
    count=$((count + 1))
    expected="${file%.in}.out"
    if (cd "$sandbox" && "$root/acaderp" < "$root/$file" > output 2> errors) && \
       diff -u "$root/$expected" "$sandbox/output"; then
        printf 'PASS %s\n' "$(basename "$file")"
        passed=$((passed + 1))
    else
        printf 'FAIL %s\n' "$(basename "$file")"
    fi
done
printf 'Passed %s/%s tests\n' "$passed" "$count"
[ "$passed" -eq "$count" ]
