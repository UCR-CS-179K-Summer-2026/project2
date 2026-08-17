# Convenience wrapper around the CMake commands everyone keeps retyping.
# Usage:
#   make build      : configure + compile the normal dev build
#   make test       : build + run all 32 GoogleTest cases through ctest
#   make coverage   : configure + build + test + generate coverage report
#   make clean      : remove both build directories entirely

.PHONY: build test coverage clean

build:
	cmake -B build
	cmake --build build

test: build
	ctest --test-dir build --output-on-failure

coverage:
	cmake -B build-coverage -DCMAKE_BUILD_TYPE=Coverage
	cmake --build build-coverage
	ctest --test-dir build-coverage --output-on-failure
	./scripts/coverage.sh

clean:
	rm -rf build build-coverage