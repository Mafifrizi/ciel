# Building, Installing, and Running

## Automatic All-in-One Install (Ciel + Tempest + Tanuki)

Running the bootstrap script inside a cloned Ciel repository checks toolchain prerequisites, clones and compiles [Tempest](https://github.com/Mafifrizi/tempest), clones [Tanuki](https://github.com/Mafifrizi/tanuki), builds Ciel, and installs all executables into bin/ and ~/.local/bin:

    git clone https://github.com/Mafifrizi/ciel.git
    cd ciel
    chmod +x install.sh
    ./install.sh

If ~/.local/bin is not yet on your PATH, add it in your shell profile:

    export PATH="$HOME/.local/bin:$PATH"

## Single-Command Launch

Run Ciel directly and it automatically starts the background teamserver, Tempest operatives, and Tanuki runtime, runs the initial telemetry sweep and audit matrix, and turns on live polling so everything is ready for immediate interaction:

    ciel

Or launch directly from the local build directory:

    ./bin/ciel

Closing the Ciel window cleanly shuts down the auto-started background teamserver. You can also connect to an already-running or remote teamserver by passing an address:

    ciel --addr 127.0.0.1:40056

## Manual Makefile Targets

1. Build Ciel and bundle Tempest operatives (souei, chronoa, ranga, libranga.so) plus tanuki launcher into bin/:

    make clean && make

2. Install all binaries (ciel, ciel-teamserver, ciel-client, souei, chronoa, ranga, libranga.so, tanuki) into PREFIX/bin (defaults to ~/.local/bin):

    make install

3. Override sibling repository directories if Tempest or Tanuki live in custom locations:

    make TEMPEST_DIR=/custom/tempest TANUKI_DIR=/custom/tanuki install

4. Run the full RPC, bundled operative, auto-spawn, and Qt6 desktop self-test suite:

    make test

## Headless Self-Test Mode

Run automated end-to-end UI verification without an active display server:

    QT_QPA_PLATFORM=offscreen ./bin/ciel --addr 127.0.0.1:40056 --self-test --screenshot table.png --graph-screenshot graph.png
