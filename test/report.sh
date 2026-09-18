#!/bin/bash

PROJECT_NAME=$1
list=$(ls "$PROJECT_NAME"_*)
for t in ${list[@]}; do
  ./$t
  if [ $? -ne 0 ]; then
    exit 1
fi
done

lcov -t "{$PROJECT_NAME}" -o "$PROJECT_NAME".info -c -d . --rc lcov_branch_coverage=1 --rc lcov_function_coverage=1
lcov  --rc lcov_branch_coverage=1 --remove "$PROJECT_NAME".info '/usr/*' '*/test/src/*' '*/etl/*' '*/math/*' -o "$PROJECT_NAME".info
genhtml -o report "$PROJECT_NAME".info  --rc lcov_branch_coverage=1 --rc lcov_function_coverage=1
