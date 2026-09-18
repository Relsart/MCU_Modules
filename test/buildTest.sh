#!/bin/bash
PROC=$(nproc --all)
dos2unix report.sh
chmod +x report.sh
rm -rf buildTest
cmake -H. -BbuildTest
cmake --build ./buildTest -j $PROC
STATUS=$?
exit $STATUS