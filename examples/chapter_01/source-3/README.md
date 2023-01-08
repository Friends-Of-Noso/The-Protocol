# The C/C++ Examples Folder

## pingpong

- Requirements

Requires `libevent`

Install `libevent` in ubuntu/linux `apt install libevent-dev`

Install `libevent` in macos `brew install libevent`

- Build

`c++ src/pingpong/pingpong.cpp -opingpong -levent_core`

Replace `c++` by `g++` or `clang++` if need

- Running

`./pingpong remote-node-ipv4 [port]`
