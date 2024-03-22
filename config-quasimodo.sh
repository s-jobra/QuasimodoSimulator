#!/bin/bash

# Initializes the submodules.

# Exit on failure
set -e

git clone https://github.com/trishullab/Quasimodo.git
cd Quasimodo/
git submodule update --init

cd cflobdd/cudd-complex-big/
autoupdate
autoreconf

sed -i 's/: ${CFLAGS="-Wall -Wextra -g -O3"}/: ${CFLAGS="-Wall -Wextra -g -O3 -fPIC"}/g' configure
sed -i 's/: ${CXXFLAGS="-Wall -Wextra -std=c++0x -g -O3"}/: ${CXXFLAGS="-Wall -Wextra -std=c++0x -g -O3 -fPIC"}/g' configure

./configure

make
cd ../..

cd python_pkg/
export BOOST_PATH="$HOME/boost_1_81_0"
invoke build-quasimodo