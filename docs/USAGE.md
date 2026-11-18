# Building and Running

## Prerequisites

1. Build [Tempest](https://github.com/Mafifrizi/tempest) alongside Ciel at ../tempest so the compiled binaries live in ../tempest/bin (or point TEMPEST_BIN_DIR to your Tempest bin directory).
2. Install the Linux / WSL2 toolchain packages:

    sudo apt-get update && sudo apt-get install -y golang-go qt6-base-dev cmake pkg-config fonts-dejavu-core python3-pil

## Compile and Verify

Run the strict -Wall -Wextra -Werror build and full RPC + headless Qt6 test suite from the repository root:

    make clean && make
    make test

## Start a Live Session

Terminal 1 starts the Go teamserver (resolving ../tempest/bin automatically or via TEMPEST_BIN_DIR):

    ./bin/ciel-teamserver --addr 127.0.0.1:40056

Terminal 2 launches the native Qt6 desktop client over X11, Wayland, or WSLg:

    ./bin/ciel-client --addr 127.0.0.1:40056

## Headless Self-Test Mode

Run automated end-to-end UI verification and regenerate showcase screenshots without an active display server:

    QT_QPA_PLATFORM=offscreen ./bin/ciel-client --addr 127.0.0.1:40056 --self-test --screenshot table.png --graph-screenshot graph.png
