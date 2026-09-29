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
# no profile under GCC, but Clang treats them as cold.

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
BENCHMARK=""
for DIR in "$(cache_value CMAKE_RUNTIME_OUTPUT_DIRECTORY)" "${BUILD_DIR}/test/functional_tests"; do
  for CANDIDATE in "${DIR}/PipelineBenchmark" "${DIR}/PipelineBenchmark.exe"; do
    if [ -n "${DIR}" ] && [ -x "${CANDIDATE}" ]; then
      BENCHMARK="${CANDIDATE}"
      break 2
    fi
  done
done
if [ -z "${BENCHMARK}" ]; then
  echo "PipelineBenchmark is missing from ${BUILD_DIR} - build that target (it needs BUILD_TESTING=ON)" >&2
  exit 1
fi

# Clang names each raw profile after the binary that wrote it, so an older
# build's would be merged into this one; GCC would add to the counts of a
# .gcda left by an earlier run of this same build.
mkdir -p "${PROFILE_DIR}"
find "${PROFILE_DIR}" -type f \( -name '*.gcda' -o -name '*.profraw' -o -name '*.profdata' \) -delete

echo "Training with ${BENCHMARK}"
TRAINING_LOG="${PROFILE_DIR}/training.log"
# MUDLET_TEST_MODE as ctest and compare-perf-baseline.py set it: a release or PTB
# build would otherwise look for updates, and install mpkg, which updates itself
# from the network part way through a slot
if ! QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" MUDLET_TEST_MODE="${MUDLET_TEST_MODE:-1}" "${BENCHMARK}" 2>&1 | tee "${TRAINING_LOG}"; then
  echo "=== ERROR: the training run failed - see the PipelineBenchmark output above ===" >&2
  exit 1
fi
# A skipped slot exits 0 and still writes a profile - one of startup alone, if
# initTestCase() skips - so require a metric from each slot the training is for.
for METRIC in text_best_pass_ms trigger_best_pass_ms defaults_text_best_pass_ms display_paint_ms; do
  if ! grep -q "^METRIC ${METRIC} " "${TRAINING_LOG}"; then
    echo "=== ERROR: the training run reported no ${METRIC}, so that slot did not run and the profile would leave its code cold ===" >&2
    exit 1
  fi
done

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
