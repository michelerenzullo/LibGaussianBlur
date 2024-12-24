#!/bin/sh

# This script generates a coverage report for the project. Run from the root directory of the project.

# Ensure the script exits if any command fails
set -e

# Define directories
BUILD_DIR="build"
COVERAGE_DIR="coverage"
LCOV_INFO="$COVERAGE_DIR/lcov.info"
HTML_REPORT_DIR="$COVERAGE_DIR/html"

# Create coverage directory if it doesn't exist
mkdir -p $COVERAGE_DIR

# Reset coverage data
lcov --zerocounters --directory $BUILD_DIR

# Run tests to generate .gcda files
ctest --test-dir $BUILD_DIR

# Capture coverage data using lcov
lcov --base-directory . --directory $BUILD_DIR --capture -o $LCOV_INFO --no-external

# Remove coverage data for external libraries (e.g., Google Test, pffft_pommier)
lcov --remove $LCOV_INFO '*/googletest/*' '*/pffft_pommier/*' -o $LCOV_INFO

# Generate HTML report
genhtml $LCOV_INFO --output-directory $HTML_REPORT_DIR

# Print the location of the HTML report
echo "HTML report generated at $HTML_REPORT_DIR/index.html"