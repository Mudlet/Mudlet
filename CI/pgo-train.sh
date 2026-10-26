#!/bin/bash
# Usage: pgo-train.sh <build-dir>
#
# The training step of a profile-guided build (cmake/ProfileGuidedOptimization.cmake):
# runs the workload through a tree configured with -DMUDLET_PGO=GENERATE and leaves
# the profile in its MUDLET_PGO_DIR, ready for the -DMUDLET_PGO=USE rebuild.
#
# The workload is PipelineBenchmark - text decoding, the trigger engine, the
# default packages and painting the console - because that is the per-line path
# a busy game hammers. Functions it never enters are optimised as if there were
# no profile under GCC, but Clang treats them as cold. MapRenderBenchmark and
# PathfindBenchmark then train the mapper on the Achaea map the Lua specs use:
# without them, GCC 12 loaded, audited and built the pathfinding graph of a big
# map slower than with no profile at all.

set -euo pipefail

BUILD_DIR="${1:-}"
if [ $# -ne 1 ] || [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
  echo "usage: $(basename "$0") <build-dir configured with -DMUDLET_PGO=GENERATE>" >&2
  exit 2
fi

cache_value() {
  sed -n "s/^$1:[A-Z]*=//p" "${BUILD_DIR}/CMakeCache.txt"
}

PGO_STAGE="$(cache_value MUDLET_PGO | tr "[:lower:]" "[:upper:]")"
if [ "${PGO_STAGE}" != "GENERATE" ]; then
  echo "${BUILD_DIR} is not configured with -DMUDLET_PGO=GENERATE, so running it records no profile" >&2
  exit 2
fi
PROFILE_DIR="$(cache_value MUDLET_PGO_DIR)"
COMPILER="$(sed -n 's/^set(CMAKE_CXX_COMPILER_ID "\(.*\)")$/\1/p' "${BUILD_DIR}"/CMakeFiles/*/CMakeCXXCompiler.cmake | head -1)"
if [ -z "${PROFILE_DIR}" ] || [ -z "${COMPILER}" ]; then
  echo "could not read MUDLET_PGO_DIR or the compiler from ${BUILD_DIR}" >&2
  exit 1
fi

# The Windows preset gathers every executable into CMAKE_RUNTIME_OUTPUT_DIRECTORY
find_benchmark() {
  local dir candidate
  for dir in "$(cache_value CMAKE_RUNTIME_OUTPUT_DIRECTORY)" "${BUILD_DIR}/test/functional_tests"; do
    for candidate in "${dir}/$1" "${dir}/$1.exe"; do
      if [ -n "${dir}" ] && [ -x "${candidate}" ]; then
        echo "${candidate}"
        return
      fi
    done
  done
  echo "$1 is missing from ${BUILD_DIR} - build that target (it needs BUILD_TESTING=ON)" >&2
  return 1
}

# Runs one benchmark, then fails unless it printed every metric listed after its
# name: a skipped slot exits 0 and still writes a profile - one of startup alone,
# if initTestCase() skips - which would leave that slot's code untrained.
train() {
  local benchmark log metric
  benchmark="$(find_benchmark "$1")"
  log="${PROFILE_DIR}/training-$1.log"
  shift
  echo "Training with ${benchmark}"
  # MUDLET_TEST_MODE as ctest and compare-perf-baseline.py set it: a release or PTB
  # build would otherwise look for updates, and install mpkg, which updates itself
  # from the network part way through a slot
  if ! QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" MUDLET_TEST_MODE="${MUDLET_TEST_MODE:-1}" "${benchmark}" 2>&1 | tee "${log}"; then
    echo "=== ERROR: the training run failed - see the $(basename "${benchmark}") output above ===" >&2
    exit 1
  fi
  for metric in "$@"; do
    if ! grep -q "^METRIC ${metric} " "${log}"; then
      echo "=== ERROR: the training run reported no ${metric}, so that slot did not run and the profile would leave its code cold ===" >&2
      exit 1
    fi
  done
}

# Clang names each raw profile after the binary that wrote it, so an older
# build's would be merged into this one; GCC would add to the counts of a
# .gcda left by an earlier run of this same build.
mkdir -p "${PROFILE_DIR}"
find "${PROFILE_DIR}" -type f \( -name '*.gcda' -o -name '*.profraw' -o -name '*.profdata' \) -delete

train PipelineBenchmark text_best_pass_ms trigger_best_pass_ms defaults_text_best_pass_ms display_paint_ms

# The benchmarks convert the XML to the binary format themselves, so TMap::restore() is trained too.
# The routes are Mapper_spec.lua's two longest, both ways: 96 steps between areas, and the 186-step diagonal.
# Under the profile directory rather than mktemp's, as the benchmarks need a path Windows can open.
MAP_DIR="${PROFILE_DIR}/training-map"
rm -rf "${MAP_DIR}"
mkdir -p "${MAP_DIR}"
trap 'rm -rf "${MAP_DIR}"' EXIT
MAP_ARCHIVE="$(cd "$(dirname "$0")/.." && pwd)/src/mudlet-lua/tests/fixtures/maps/achaea-map.zip"
(cd "${MAP_DIR}" && cmake -E tar xf "${MAP_ARCHIVE}")
export MUDLET_BENCH_MAP="${MAP_DIR}/achaea-map.xml"
train MapRenderBenchmark map_restore_seconds render_fit_ms
MUDLET_BENCH_ROUTES="107:1192,1192:107,6595:18690,18690:6595" train PathfindBenchmark init_graph_ms path_route_18690_6595_ms

if [[ "${COMPILER}" == *Clang* ]]; then
  # The merged profile has to be in the format of the compiler that reads it, so
  # on macOS ask Xcode for Apple's own rather than any Homebrew LLVM on PATH
  PROFDATA="${LLVM_PROFDATA:-}"
  if [ -z "${PROFDATA}" ] && [ "$(uname)" = "Darwin" ]; then
    PROFDATA="$(xcrun --find llvm-profdata 2>/dev/null || true)"
  fi
  if [ -z "${PROFDATA}" ]; then
    PROFDATA="$(command -v llvm-profdata || true)"
  fi
  if [ -z "${PROFDATA}" ]; then
    echo "llvm-profdata not found - install it or point LLVM_PROFDATA at the one matching the compiler" >&2
    exit 1
  fi
  shopt -s nullglob
  RAW_PROFILES=("${PROFILE_DIR}"/*.profraw)
  if [ ${#RAW_PROFILES[@]} -eq 0 ]; then
    echo "the training run wrote no .profraw files to ${PROFILE_DIR}" >&2
    exit 1
  fi
  "${PROFDATA}" merge -output="${PROFILE_DIR}/mudlet.profdata" "${RAW_PROFILES[@]}"
  rm -f "${RAW_PROFILES[@]}"
  echo "Profile written to ${PROFILE_DIR}/mudlet.profdata"
else
  PROFILE_COUNT="$(find "${PROFILE_DIR}" -name '*.gcda' | wc -l)"
  if [ "${PROFILE_COUNT}" -eq 0 ]; then
    echo "the training run wrote no .gcda files to ${PROFILE_DIR}" >&2
    exit 1
  fi
  echo "Profile written to ${PROFILE_DIR} (${PROFILE_COUNT} .gcda files)"
fi
