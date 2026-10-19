#!/bin/bash
# Offline regression checks only: does not launch the app or contact the synth.
set -u
set -o pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
report_dir="$repo_dir/validation-results/$(date '+%Y%m%d-%H%M%S')-$$"
build_dir="$repo_dir/build"
mkdir -p "$report_dir" || exit 1
summary="$report_dir/summary.txt"
printf 'BSII Control validation\nStarted: %s\nCommit: %s\n' "$(date -Iseconds)" "$(git -C "$repo_dir" rev-parse --short HEAD 2>/dev/null || echo unknown)" > "$summary"
printf '\nWorking tree at start:\n' >> "$summary"
git -C "$repo_dir" status --short >> "$summary" 2>/dev/null

run_stage() {
    local label="$1" logfile="$2"
    shift 2
    printf '%s…\n' "$label"
    if "$@" > "$report_dir/$logfile" 2>&1; then
        printf 'PASS: %s\n' "$label" | tee -a "$summary"
    else
        local result=$?
        printf 'FAIL: %s (exit %s)\nLog: %s\n' "$label" "$result" "$report_dir/$logfile" | tee -a "$summary"
        tail -n 25 "$report_dir/$logfile"
        printf '\nReport: %s\n' "$summary"
        exit "$result"
    fi
}

if ! command -v cmake >/dev/null || ! command -v ctest >/dev/null; then
    printf 'FAIL: CMake and CTest must be installed.\n' | tee -a "$summary"
    exit 1
fi
# Reuse an existing build generator; default to Xcode for a fresh checkout.
if [ -f "$build_dir/CMakeCache.txt" ]; then
    run_stage 'Configure' configure.log cmake -S "$repo_dir" -B "$build_dir" -DBSII_BUILD_TESTS=ON
else
    run_stage 'Configure' configure.log cmake -S "$repo_dir" -B "$build_dir" -G Xcode -DBSII_BUILD_TESTS=ON
fi
build_extra=()
if /usr/bin/grep -q '^CMAKE_GENERATOR:INTERNAL=Xcode$' "$build_dir/CMakeCache.txt"; then
    build_extra=(-- CODE_SIGNING_ALLOWED=NO)
fi
run_stage 'Build standalone and regression tests' build.log cmake --build "$build_dir" --config Release --parallel 4 --target BSIIControl_Standalone BSIIMidiRoutingTest BSIIOsc1RangeTest BSIISnapshotRecallTest BSIIPatchInspect BSIITuningTableTool BSIIMicroTuningTest "${build_extra[@]}"
run_stage 'Regression tests' tests.log ctest --test-dir "$build_dir" -C Release --output-on-failure --no-tests=error
printf '\n' >> "$summary"
/usr/bin/grep -E 'tests passed|Total Test time' "$report_dir/tests.log" | tee -a "$summary"
printf '\nScope: standalone build and offline regressions. Does not verify live hardware, sound, signing, AU or VST3 hosts.\n' >> "$summary"
printf '\nReport: %s\n' "$summary"
