#!/usr/bin/env bash
# Build the tests with coverage, run them, report.
#
# Line coverage alone is misleading here: most of clap is inline or templated,
# so code that is never called is never emitted and never counted. Branch and
# function coverage are the numbers to watch.
#
#   tools/coverage.sh                          summary + HTML report

set -euo pipefail

cd "$(dirname "$0")/.."

BUILD=build-cov
TRACE=$BUILD/cov.json
REPORT=$BUILD/cov/index.html

cmake -B $BUILD -DCLAP_COVERAGE=ON >/dev/null
cmake --build $BUILD --target clap_tests -j >/dev/null

find $BUILD -name '*.gcda' -delete
./$BUILD/clap_tests

gcovr -r . --filter 'include/' $BUILD --json $TRACE >/dev/null

gcovr -a $TRACE --csv | awk -F, 'NR==2 {
    printf "\n[RESULT]  line %5.1f%% (%d/%d)   branch %5.1f%% (%d/%d)   func %5.1f%% (%d/%d)\n\n",
        $4*100, $3, $2, $7*100, $6, $5, $10*100, $9, $8 }'

mkdir -p "$(dirname $REPORT)"
gcovr -a $TRACE --html-details $REPORT --html-theme github.dark-green >/dev/null

echo "  file://$PWD/$REPORT"
echo "  uncovered functions: file://$PWD/${REPORT%.html}.functions.html"
echo

exit 0
