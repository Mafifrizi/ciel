TEMPEST_REPO ?= https://github.com/Mafifrizi/tempest.git
TANUKI_REPO  ?= https://github.com/Mafifrizi/tanuki.git
SHUNA_REPO   ?= https://github.com/Mafifrizi/shuna.git

TEMPEST_DIR  ?= $(abspath ../tempest)
SHUNA_DIR    ?= $(abspath ../shuna)
TANUKI_DIR   := $(shell if [ -d ../tanuki ]; then (cd ../tanuki && pwd); elif [ -d ../tanuki-repo ]; then (cd ../tanuki-repo && pwd); else echo "$(abspath ../tanuki)"; fi)
TANUKI_MODE  ?= python
PREFIX       ?= $(HOME)/.local

.PHONY: all deps install clean test test-go

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
	@if [ ! -f "$(SHUNA_DIR)/go.mod" ]; then \
		echo "[deps] Cloning Shuna from $(SHUNA_REPO) into $(SHUNA_DIR)..."; \
		git clone --depth 1 "$(SHUNA_REPO)" "$(SHUNA_DIR)"; \
	fi
	$(MAKE) -C "$(SHUNA_DIR)" build
	cp -f "$(SHUNA_DIR)/bin/shuna" bin/shuna
	chmod +x bin/shuna
	@if [ ! -f "$(TANUKI_DIR)/pyproject.toml" ]; then \
		echo "[deps] Cloning Tanuki from $(TANUKI_REPO) into $(TANUKI_DIR)..."; \
		git clone --depth 1 "$(TANUKI_REPO)" "$(TANUKI_DIR)"; \
	fi
	@if [ "$(TANUKI_MODE)" = "rust" ]; then \
		if [ ! -x "$(TANUKI_DIR)/target/release/tanuki" ]; then \
			echo "[deps] Error: TANUKI_MODE=rust requested but $(TANUKI_DIR)/target/release/tanuki not found or not executable"; \
			exit 1; \
		fi; \
		cp -f "$(TANUKI_DIR)/target/release/tanuki" bin/tanuki; \
	else \
		printf '#!/bin/sh\nPYTHONPATH="%s$${PYTHONPATH:+:$$PYTHONPATH}" exec python3 -m tanuki "$$@"\n' "$(TANUKI_DIR)" > bin/tanuki; \
	fi
	chmod +x bin/tanuki

bin/ciel-teamserver: teamserver/*.go teamserver/migrations/*.sql teamserver/go.mod | bin
	cd teamserver && go build -o ../bin/ciel-teamserver .

bin/ciel-client: client/CMakeLists.txt client/src/main.cpp | bin
	cmake -B build/client -S client
	cmake --build build/client -j$$(nproc)
	cp build/client/ciel-client bin/ciel-client
	cp -f bin/ciel-client bin/ciel

install: all
	mkdir -p "$(PREFIX)/bin"
	cp -f bin/ciel bin/ciel-teamserver bin/ciel-client bin/shuna bin/souei bin/chronoa bin/ranga bin/libranga.so bin/tanuki "$(PREFIX)/bin/"
	@echo "[install] Installed Ciel, Shuna, Tempest (souei, chronoa, ranga, libranga.so), and Tanuki into $(PREFIX)/bin"

test-go:
	cd teamserver && go test -race -count=1 -v ./...

test: all test-go
	python3 tests/test_ciel.py
	python3 tests/test_ecosystem_e2e.py

clean:
	rm -rf bin build
