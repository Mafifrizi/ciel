.PHONY: all clean test

all: bin/ciel-teamserver bin/ciel-client

bin:
	mkdir -p bin

bin/ciel-teamserver: teamserver/main.go teamserver/go.mod | bin
	cd teamserver && go build -o ../bin/ciel-teamserver .

bin/ciel-client: client/CMakeLists.txt client/src/main.cpp | bin
	cmake -B build/client -S client
	cmake --build build/client -j$$(nproc)
	cp build/client/ciel-client bin/ciel-client

test: all
	python3 tests/test_ciel.py

clean:
	rm -rf bin build
