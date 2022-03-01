SHELL = /bin/sh
.ONESHELL:
.SHELLFLAGS: -euo
.DEFAULT_GOAL: help
.NOTPARALLEL:
.POSIX:

#

CMAKE = cmake
RM = rm -rf

ifeq ($(OS),Windows_NT)
	NUM_CORES = $(NUMBER_OF_PROCESSORS)
	CMAKE_GENERATOR = Visual Studio 16 2019
else ifeq ($(shell uname -s),Darwin)
	NUM_CORES = $(shell sysctl hw.ncpu | awk '{print $$2}')
	CMAKE_GENERATOR = Xcode
else
	NUM_CORES = $(shell grep -c ^processor /proc/cpuinfo)
	CMAKE_GENERATOR = Ninja
endif

BUILDS = Builds
CACHE = Cache

#

override FOLEYS_ROOT := $(patsubst %/,%,$(strip $(dir $(realpath $(firstword $(MAKEFILE_LIST))))))

override THIS_MAKEFILE := $(FOLEYS_ROOT)/Makefile

override cmake_config = cd $(FOLEYS_ROOT) && $(CMAKE) -B $(BUILDS) -G "$(CMAKE_GENERATOR)" --log-level=DEBUG

override cmake_build_configuration = echo "Building $(1) configuration..."; $(CMAKE) --build $(BUILDS) -j $(NUM_CORES) --config $(1)

#

help:  ## Print this message
	@grep -E '^[a-zA-Z_-]+:.*?\#\# .*$$' $(THIS_MAKEFILE) | sort | awk 'BEGIN {FS = ":.*?\#\# "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

#

clean:  ## Removes the builds directory
	cd $(FOLEYS_ROOT) && $(RM) $(BUILDS)

wipe: clean  ## Removes the builds directory and the dependencies cache
	cd $(FOLEYS_ROOT) && $(RM) $(CACHE)

#

config:  ## Configure the tests
	$(call cmake_config) -D FOLEYS_BUILD_TESTS=1

tests: config  ## Build the tests
	@$(call cmake_build_configuration,Debug)

#

.PHONY: $(shell grep -E '^[a-zA-Z_-]+:.*?\#\# .*$$' $(THIS_MAKEFILE) | sed 's/:.*/\ /' | tr '\n' ' ')
