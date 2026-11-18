TEMPEST_REPO ?= https://github.com/Mafifrizi/tempest.git
TANUKI_REPO  ?= https://github.com/Mafifrizi/tanuki.git

TEMPEST_DIR  ?= $(abspath ../tempest)
TANUKI_DIR   := $(shell if [ -d ../tanuki ]; then (cd ../tanuki && pwd); elif [ -d ../tanuki-repo ]; then (cd ../tanuki-repo && pwd); else echo "$(abspath ../tanuki)"; fi)
PREFIX       ?= $(HOME)/.local

.PHONY: all deps install clean test

all: deps bin/ciel-teamserver bin/ciel-client

bin:
	mkdir -p bin

deps: | bin
	@if [ ! -f "$(TEMPEST_DIR)/Makefile" ]; then \
		echo "[deps] Cloning Tempest from $(TEMPEST_REPO) into $(TEMPEST_DIR)..."; \
		git clone --depth 1 "$(TEMPEST_REPO)" "$(TEMPEST_DIR)"; \
	fi
	$(MAKE) -C "$(TEMPEST_DIR)"
	cp -f "$(TEMPEST_DIR)/bin/souei" "$(TEMPEST_DIR)/bin/chronoa" "$(TEMPEST_DIR)/bin/ranga" "$(TEMPEST_DIR)/bin/libranga.so" bin/
	@if [ ! -f "$(TANUKI_DIR)/pyproject.toml" ]; then \
		echo "[deps] Cloning Tanuki from $(TANUKI_REPO) into $(TANUKI_DIR)..."; \
		git clone --depth 1 "$(TANUKI_REPO)" "$(TANUKI_DIR)"; \
	fi
	@if [ -x "$(TANUKI_DIR)/target/release/tanuki" ]; then \
		cp -f "$(TANUKI_DIR)/target/release/tanuki" bin/tanuki; \
	else \
		printf '#!/bin/sh\nPYTHONPATH="%s$${PYTHONPATH:+:$$PYTHONPATH}" exec python3 -m tanuki "$$@"\n' "$(TANUKI_DIR)" > bin/tanuki; \
	fi
	chmod +x bin/tanuki

bin/ciel-teamserver: teamserver/main.go teamserver/go.mod | bin
	cd teamserver && go build -o ../bin/ciel-teamserver .

bin/ciel-client: client/CMakeLists.txt client/src/main.cpp | bin
	cmake -B build/client -S client
	cmake --build build/client -j$$(nproc)
	cp build/client/ciel-client bin/ciel-client
	cp -f bin/ciel-client bin/ciel

install: all
	mkdir -p "$(PREFIX)/bin"
	cp -f bin/ciel bin/ciel-teamserver bin/ciel-client bin/souei bin/chronoa bin/ranga bin/libranga.so bin/tanuki "$(PREFIX)/bin/"
	@echo "[install] Installed Ciel, Tempest (souei, chronoa, ranga, libranga.so), and Tanuki into $(PREFIX)/bin"

test: all
	python3 tests/test_ciel.py

clean:
	rm -rf bin build
