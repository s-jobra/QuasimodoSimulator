#!/bin/bash

# Initializes Quasimodo (as stated in Quasimodo's README).
# It is assumed that the script is run from the repository's home folder.

# Exit on failure
set -e

git clone https://github.com/trishullab/Quasimodo.git
cd Quasimodo/
git submodule update --init

wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz .
tar -xvf boost_1_81_0.tar.gz
export BOOST_PATH="$(pwd)/boost_1_81_0"

cd cflobdd/cudd-complex-big/
autoupdate
autoreconf

sed -i 's/: ${CFLAGS="-Wall -Wextra -g -O3"}/: ${CFLAGS="-Wall -Wextra -g -O3 -fPIC"}/g' configure
sed -i 's/: ${CXXFLAGS="-Wall -Wextra -std=c++0x -g -O3"}/: ${CXXFLAGS="-Wall -Wextra -std=c++0x -g -O3 -fPIC"}/g' configure

./configure

make
cd ../..

cd python_pkg/
invoke build-quasimodo