# DIFCVerilog

DIFCVerilog is a Verilog compiler extension for static decentralized information-flow checking. The compiler front end is based on Icarus Verilog 0.9.6 and earlier SecVerilog security-typing code. DIFCVerilog adds DIFC analysis support and uses a separate fixed Prolog backend maintained as part of this project by Yubo Shi. The original copyright and GPL notices are retained in the inherited source files.

## Requirements

The supported build environment is 64-bit Ubuntu 22.04 or a compatible Linux distribution. Other recent Debian/Ubuntu versions should also work if the same packages are available.

Required packages:

- GCC and G++: <https://gcc.gnu.org/>
- GNU Make: <https://www.gnu.org/software/make/>
- GNU binutils (`ar`, `ld`, `ranlib`, `strip`): <https://www.gnu.org/software/binutils/>
- Autoconf: <https://www.gnu.org/software/autoconf/>
- Bison: <https://www.gnu.org/software/bison/>
- Flex: <https://github.com/westes/flex>
- gperf: <https://www.gnu.org/software/gperf/>
- SWI-Prolog: <https://www.swi-prolog.org/>
- zlib development library: <https://zlib.net/>
- Common POSIX shell tools (`sh`, `sed`, `awk`, `head`, `tail`, `grep`, `mkdir`, `install`)

Optional packages:

- Git, if you want version tags in local builds: <https://git-scm.com/>
- Perl, for installed helper scripts: <https://www.perl.org/>
- Python 3, for installed helper scripts: <https://www.python.org/>
- Readline development library: <https://tiswww.case.edu/php/chet/readline/rltop.html>
- termcap/ncurses development library: <https://invisible-island.net/ncurses/>
- bzip2 development library: <https://sourceware.org/bzip2/>
- man and Ghostscript tools for local manual-page generation

On Ubuntu or Debian:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential binutils autoconf bison flex gperf swi-prolog \
  zlib1g-dev libreadline-dev libncurses-dev libbz2-dev \
  git perl python3 man-db ghostscript
```

This source package does not include local build products such as object files, generated parser files, or compiler binaries. They are regenerated during the normal build. The dependency set follows the same general pattern as the original SecVerilog and Icarus Verilog 0.9.x build environments, with SWI-Prolog required for the DIFC backend.

## Build

Build the compiler without installing it system-wide:

```bash
cd DIFCVerilog
autoconf
./configure --prefix="$PWD/../build"
make
make check
make install
cd ..
```

The compiler is installed under `build/bin`. A system-wide installation can use the default prefix and `sudo make install` instead.

If `configure` reports a missing tool, install the corresponding package above and rerun `autoconf` and `./configure`.

## Basic usage

In declarations, security labels follow the data type and width. For example:

```verilog
wire [7:0] {1| |} public_value;
wire [7:0] {2| |} secret_value;
```

The `examples/` directory contains a small multi-file design that is separate from the evaluation benchmarks.

From the repository root, run:

```bash
build/bin/iverilog -z -s example_top -f examples/example_design.f
swipl -q -s DIFCVerilog/difc_backend.pl -- --stem example_top
```

The Prolog step should finish with `Analysis complete.` for this example.

For designs with many Verilog files or a deep module hierarchy, put the source file paths in a file list and pass it with `-f`:

```text
# your_design.f
relative/or/absolute/source/file.v
another/source/file.v
more/source/files.v
```

```bash
build/bin/iverilog -z -s top -f your_design.f
swipl -q -s DIFCVerilog/difc_backend.pl -- --stem top
```

The compiler creates the following files in the working directory:

- `<top>_nodes.txt`: labeled signals
- `<top>_steps.txt`: recorded flow edges
- `<top>_portcheck.txt`: port checks

The fixed Prolog backend writes `<top>_main.out` by default. You can also pass explicit files:

```bash
swipl -q -s DIFCVerilog/difc_backend.pl -- \
  --nodes example_top_nodes.txt \
  --steps example_top_steps.txt \
  --out example_top_main.out
```

Useful compiler options are:

```text
-z              run DIFC type checking
-s MODULE       optional: select the top module
-f FILE         read source files and options from a file list
```

## Repository layout

```text
DIFCVerilog/       compiler source, fixed Prolog backend, and GPL license
examples/          small standalone usage examples
experiment/        benchmark source links and local reproduction notes
```

The `experiment/` directory records links to Hack@DAC, GHOST, TrustHub, and OpenCores sources. Benchmark RTL files and generated outputs are not included.

## License and attribution

This project is distributed under GNU GPL v2; see [`LICENSE`](LICENSE) and [`DIFCVerilog/COPYING`](DIFCVerilog/COPYING).

- Icarus Verilog: Stephen Williams and contributors
- Earlier security-typing code: Danfeng Zhang
- DIFCVerilog front-end modifications and fixed Prolog backend: Yubo Shi, 2026

Prior copyright notices are preserved. The Yubo Shi notice identifies later DIFCVerilog code and does not replace upstream authorship.

## Release notes

This release includes the DIFCVerilog source, a standalone usage example, and benchmark source links. Benchmark RTL and generated evaluation files are not included in this repository and should be regenerated locally from the referenced sources.

## Contributing

Before submitting a change, run:

```bash
cd DIFCVerilog
make check
```

Keep changes focused, preserve existing copyright notices, and do not commit build products or benchmark output files.
