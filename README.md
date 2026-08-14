# cognet-ls

> A small, self-contained reimplementation of the Unix `ls` command in C.

`cognet-ls` reproduces the everyday behavior of GNU `ls` — directory listings,
long format, hidden files, human-readable sizes, recursion, and time sorting —
in a single portable C file with no dependencies beyond the standard library.
It is useful as a reference for how `opendir`/`scandir`/`stat` fit together and
for environments where a minimal, auditable lister is preferred.

This project was fully written by Qwen 3.8.

## Features

- **Sorted listings** — entries sorted with `scandir` + `alphasort`
- **`-l` long format** — permissions (including setuid/setgid/sticky), link count, owner, group, size, and modification time
- **`-a`** — show dotfiles
- **`-h`** — human-readable sizes (K/M/G/T) with `-l`
- **`-R`** — recursive listing, directories marked with a trailing `/`
- **`-t`** — sort by modification time, newest first (ties broken by name)
- **`-r`** — reverse the sort order (combinable with `-t`)
- **Multiple paths** — per-path `name:` headers, like GNU `ls`
- **Symlink-safe** — uses `lstat`, so links are listed without being followed

## Getting Started

### Prerequisites

- A POSIX C compiler (gcc or clang)
- `make`

### Installation

```sh
git clone git@github.com:mycroft/cognet-ls.git
cd cognet-ls
make
```

## Build & Run

```sh
make            # builds ./ls
make clean      # removes the binary
```

### Usage

```sh
./ls [flags] [path ...]
```

Examples:

```sh
./ls -l          # long listing of the current directory
./ls -ah         # all files, long format with human-readable sizes
./ls -tR /var    # recursive, sorted newest first
./ls -l a.txt b  # multiple arguments
```

Flags combine freely (e.g. `-la`, `-lhR`, `-lt`).

### Tests

```sh
make
./test.sh
```

Runs the shell test suite covering flag behaviors and error handling (requires the built binary).

## References

- [ls(1) — GNU coreutils manual](https://www.gnu.org/software/coreutils/manual/html_node/ls-invocation.html) — the behavior this project emulates
- [scandir(3) — Linux man page](https://www.man7.org/linux/man-pages/man3/scandir.3.html) — directory reading and sorting API used by the implementation
- [stat(2) — Linux man page](https://www.man7.org/linux/man-pages/man2/stat.2.html) — file metadata behind the long format
