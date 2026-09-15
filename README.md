# minigit

`minigit` is a small educational version-control tool written in C. It implements a minimal content-addressed object store inspired by Git, using SHA-1 to identify file objects and zlib to compress them.

The project is intentionally small. It is useful for learning how repository initialization, object hashing, compression, and a simple index fit together; it is not intended to be a drop-in replacement for Git.

## Features

- Initialize a local repository.
- Store files as compressed blob objects addressed by their SHA-1 hash.
- Record added files in a simple index.
- Read a stored object back by its hash.
- Build on Unix-like systems and Windows toolchains that provide the required C libraries.

## Current status

The following commands are available:

| Command | Description |
| --- | --- |
| `minigit init` | Creates `.minigit/` and `.minigit/objects/` in the current directory. |
| `minigit add <file>` | Hashes, compresses, and stores a file, then appends its filename and hash to `.minigit/index`. |
| `minigit cat <hash>` | Decompresses and prints the content of an object from `.minigit/objects/`. |
| `minigit commit` | Reserved for future commit functionality; it currently performs no operation. |

`minigit` currently has no branch management, commit history, tree persistence, status command, or automatic duplicate detection. The index is append-only, so adding the same file more than once creates another index entry.

## Requirements

The following tools and libraries are required:

- A C11-compatible compiler, such as GCC or Clang.
- GNU Make or a compatible `make` implementation.
- OpenSSL development headers and libraries, including `libcrypto`.
- zlib development headers and libraries, including `libz`.

On Debian or Ubuntu, the dependencies can be installed with:

```sh
sudo apt update
sudo apt install build-essential libssl-dev zlib1g-dev
```

On other operating systems, install the equivalent compiler, OpenSSL, and zlib development packages using the platform's package manager.

## Building

From the project root, run:

```sh
make build
```

This produces the `minigit` executable in the project root. To remove the generated executable:

```sh
make clean
```

The default build uses:

- `-Wall -Wextra` for compiler warnings.
- `-std=c11` for the C language standard.
- `-lcrypto -lz` for OpenSSL and zlib.

You can override the compiler or compiler flags when invoking `make`, for example:

```sh
make build CC=clang
make build CFLAGS="-Wall -Wextra -std=c11 -g"
```

## Quick start

Run the commands from the directory that should become the repository root:

```sh
make build

./minigit init
printf 'Hello, minigit!\n' > example.txt
./minigit add example.txt
```

`add` prints no hash itself. The generated index contains the filename and object hash:

```sh
cat .minigit/index
```

Use the hash from that file to read the object:

```sh
./minigit cat <object-hash>
```

For example, if the index contains:

```text
example.txt 0123456789abcdef0123456789abcdef01234567
```

then run:

```sh
./minigit cat 0123456789abcdef0123456789abcdef01234567
```

## Repository layout

```text
.
├── makefile
├── README.md
├── CONTRIBUTING.md
├── src/
│   ├── main.c          # Command-line parsing and dispatch
│   ├── repository.c    # Repository and object-store implementation
│   └── repository.h    # Public repository operations
├── test/               # Manual test fixtures and sample repository data
└── tests/
    └── run_tests.sh    # Automated smoke-test suite
```

Running `minigit init` creates this directory inside the current working directory:

```text
.minigit/
├── index
└── objects/
    └── <sha-1-hash>
```

Each stored blob contains a small `blob <size>` header followed by the file data, and the complete blob is compressed before it is written to the object store.

## Testing

The project includes a lightweight automated smoke-test suite written in POSIX shell. It builds the executable, runs each test in a temporary directory, and removes the temporary files when it finishes.

Run it with:

```sh
make test
```

The suite covers:

- Usage and invalid-command errors.
- Repository initialization.
- Index creation.
- SHA-1 hash generation.
- Compressed object creation.
- Restoring file content with `cat`.
- Recording filenames in the index.

The test runner requires a POSIX-compatible shell and standard utilities such as `mktemp`, `awk`, `cmp`, and `grep`. On Windows, run it from a POSIX-compatible environment such as Git Bash or MSYS2.

A manual smoke test can also be run in a temporary directory:

```sh
mkdir -p /tmp/minigit-smoke
cd /tmp/minigit-smoke
printf 'sample data\n' > sample.txt
/path/to/minigit init
/path/to/minigit add sample.txt
cat .minigit/index
/path/to/minigit cat <object-hash>
```

## Known limitations

This project is deliberately incomplete. In particular:

- `commit` is a placeholder.
- Error handling is basic and most command functions return no status to the CLI.
- Error handling is basic and most command functions return no status to the CLI.
- Paths are limited by fixed-size buffers.
- SHA-1 APIs used by OpenSSL are deprecated in OpenSSL 3; migrating to the EVP interface is future work.
- The object format and index format are internal and may change.

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a change. Small, focused contributions are preferred over broad rewrites.
