# asio

asio is Boost.Asio natively, and in the browser Boost.Asio's own
`io_context` suspended into the page's event loop, in C++20, one of the
libraries of [webcpp](https://github.com/webcpporg/webcpp). A program
written against Boost.Asio, `main()` and `io_context::run()` included, runs
unchanged in a browser: compiled with Emscripten, asio adds only what Boost
lacks there, the configuration Asio needs to build, what wakes `run` from a
browser event, and a limit on the handlers one turn runs, so that the page
keeps rendering. The program's code runs only when the browser delivers an
event (a message, a promise, a timer), never by polling, and its stack and
objects stay as they were. Timers are Boost's on both targets, so that a
library written on asio, and every Boost library built on Asio, never knows
where it runs.

## Building and testing

asio is developed inside the webcpp superproject, as a Boost library is
developed inside Boost:

    git clone --recursive https://github.com/webcpporg/webcpp
    cd webcpp

From there, the four lanes of the continuous integration build and run
asio's tests and examples:

    b2 libs/asio/test libs/asio/example
    b2 toolset=emscripten libs/asio/test libs/asio/example
    b2 toolset=emscripten libs/asio/test//asyncify libs/asio/example//asyncify
    b2 toolset=emscripten libs/asio/test/browser//browser

The first builds them natively, runs every test and every example, and
compares each example's output with the `.expected` file beside it. The
second does the same on emscripten, run by node, with the toolset that
`user-config.jam` registers against emsdk, in the default suspension
variant, JSPI. The third, the lane `asyncify`, runs every browser test again
under Asyncify, the other variant, and writes the size table of both. The
fourth, the lane `browser`, runs the loader that picks a variant under node,
loading both built artifacts and checking their output. It also runs idle,
animation and loader checks in headless Chrome, on JSPI and Asyncify.
`CHROME` names the executable; on macOS the driver can use the newest
Playwright headless shell already installed in its cache. b2 compares timestamps, so a
result that counts is built from scratch, with `b2 -a`, and the CI gives
each lane a `--build-dir` of its own. A b2 project uses asio through
`/webcpp/asio//asio`.

A page imports `load` from `webcpp-asio-loader.mjs`, shipped in
`include/webcpp/asio/`, and calls `load('page_program.variants.json')`.
The loader detects JSPI support, selects JSPI or Asyncify from the build's
manifest and returns `{variant, module}` when its factory resolves. A JSPI
factory can await `main`, so an indefinitely running program must expose
its readiness through its module callbacks instead of awaiting `load`. The manifest
and its artifacts keep the relative layout produced by `webcpp.variants`.

## Documentation

asio's page, with its API reference, is published at
<https://webcpporg.github.io/webcpp/libs/asio/>; `b2 libs/asio/doc` builds
it into `doc/html/index.html`.

## License

Distributed under the [Boost Software License, Version 1.0](LICENSE_1_0.txt).
