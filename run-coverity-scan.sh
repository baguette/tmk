#!/bin/sh

echo "Running bootstrap build"
TMAKE_BUILD=$(./bootstrap.sh |grep gcc |grep 'tmake\.c')

echo
echo "Using build command: $TMAKE_BUILD"
echo

echo "Building tmake only with Coverity"
cov-build --dir cov-int $TMAKE_BUILD

#echo "Coverity build log:"
#tail cov-int/build-log.txt

echo
echo "Creating archive for submission to Coverity Scan"
echo
tar czf tmake.tgz cov-int

echo "Submit tmake.tgz to Coverity Scan for analysis"
echo "http://scan.coverity.com/"
echo

