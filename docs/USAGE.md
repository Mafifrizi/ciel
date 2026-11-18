# Building, Installing, and Running

## Automatic All-in-One Install (Ciel + Tempest + Tanuki)

Running the bootstrap script inside a cloned Ciel repository automatically checks toolchain prerequisites, clones and compiles [Tempest](https://github.com/Mafifrizi/tempest), clones [Tanuki](https://github.com/Mafifrizi/tanuki), builds Ciel, and installs all executables into bin/ and ~/.local/bin:

    git clone https://github.com/Mafifrizi/ciel.git
    cd ciel
    chmod +x install.sh
    ./install.sh

If ~/.local/bin is not yet on your PATH, add it in your shell profile:

    export PATH="$HOME/.local/bin:$PATH"

## Manual Makefile Targets

1. Build Ciel and bundle Tempest operatives (souei, chronoa, ranga, libranga.so) plus tanuki launcher into bin/:

    make clean && make

2. Install all binaries (ciel-teamserver, ciel-client, souei, chronoa, ranga, libranga.so, tanuki) into PREFIX/bin (defaults to ~/.local/bin):

    make install

3. Override sibling repository directories if Tempest or Tanuki live in custom locations:

    make TEMPEST_DIR=/custom/tempest TANUKI_DIR=/custom/tanuki install

4. Run the full RPC, bundled operative, and Qt6 desktop self-test suite:

    make test

## Start a Live Session

Terminal 1 starts the Go teamserver (auto-detecting bundled Tempest binaries inside bin/):

    ciel-teamserver --addr 127.0.0.1:40056

Terminal 2 launches the native Qt6 desktop client over X11, Wayland, or WSLg:

    ciel-client --addr 127.0.0.1:40056

## Headless Self-Test Mode

Run automated end-to-end UI verification without an active display server:

    QT_QPA_PLATFORM=offscreen ./bin/ciel-client --addr 127.0.0.1:40056 --self-test --screenshot table.png --graph-screenshot graph.png
