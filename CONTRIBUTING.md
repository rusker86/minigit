# Contributing to minigit

Thank you for considering a contribution to `minigit`. This project is intentionally small and educational, so changes should favor clarity, predictable behavior, and a narrow scope.

## Before you start

1. Read the [README](README.md) to understand the current commands and limitations.
2. Search the existing source before adding a new helper or dependency.
3. Check the current working tree before making changes:

   ```sh
   git status --short
   ```

4. For a larger change, open an issue or discussion first so the design can be agreed on before implementation.

## Development setup

Install the required tools and libraries:

- A C11-compatible compiler.
- GNU Make or a compatible `make` implementation.
- OpenSSL development files for `libcrypto`.
- zlib development files for `libz`.

Build the project with warnings enabled:

```sh
make clean
make build
```

The build should complete without compiler errors. OpenSSL 3 may report deprecation warnings for the existing SHA-1 API; replacing that API is a separate task and should be treated as such.

## Project structure

- `src/main.c` contains command-line parsing, usage text, and command dispatch.
- `src/repository.c` contains repository initialization, index updates, object hashing, compression, and decompression.
- `src/repository.h` declares the repository operations used by the CLI.
- `makefile` defines the build, test, and clean commands.
- `test/` contains manual fixtures and sample repository data.
- `tests/run_tests.sh` contains the automated POSIX shell smoke-test suite.

Keep responsibilities in their existing modules unless a change clearly requires a new module. Avoid introducing a framework or abstraction for a single use case.

## Making a change

1. Create a focused branch from the current default branch.
2. Make the smallest change that solves the problem.
3. Preserve the existing command-line interface unless the change explicitly updates it.
4. Keep user-visible messages clear and consistent.
5. Avoid committing generated binaries, local `.minigit` directories, or temporary files.
6. Update the README when commands, formats, dependencies, or limitations change.
7. Add or update tests when the behavior can be tested. If no automated test exists for the change, document the manual verification you performed.

## C coding guidelines

- Use C11-compatible code.
- Compile with `-Wall -Wextra` and address warnings introduced by your change.
- Use descriptive names for public functions and keep internal helpers `static`.
- Check file operations, allocations, and path-building results.
- Free allocated memory and close files on both success and error paths.
- Keep functions focused and avoid unrelated refactors.
- Do not add comments that merely repeat the code. Use comments for non-obvious format or compatibility decisions.
- Keep the existing cross-platform handling for `_WIN32` and POSIX systems in mind.

## Validation checklist

Before submitting a change, run:

```sh
make clean
make build
make test
git diff --check
git status --short
```

`make test` builds the executable and runs the automated suite in a temporary directory. The suite requires a POSIX-compatible shell and standard utilities such as `mktemp`, `awk`, `cmp`, and `grep`. On Windows, run it from Git Bash or MSYS2.

For changes to repository behavior, also perform a smoke test in a temporary directory:

```sh
mkdir -p /tmp/minigit-contributing
cd /tmp/minigit-contributing
printf 'contribution test\n' > sample.txt
/path/to/minigit init
/path/to/minigit add sample.txt
cat .minigit/index
/path/to/minigit cat <object-hash>
```

Replace `<object-hash>` with the hash printed in `.minigit/index`. Verify that the final output matches the original file content.

If you modify argument parsing, check valid and invalid forms as well:

```sh
/path/to/minigit
/path/to/minigit init
/path/to/minigit add
/path/to/minigit unknown
```

## Commit messages

Use short, imperative commit subjects. Keep the subject concise and omit a body when the subject fully explains the change.

Good examples:

```text
Add object round-trip smoke test
Improve command usage output
Document repository object format
```

Avoid vague subjects such as `Fix stuff` or `Updates`.

## Pull requests

A pull request should include:

- A short explanation of the problem and solution.
- The commands used to validate the change.
- Any known limitations or follow-up work.
- Documentation updates when user-facing behavior changed.

Keep pull requests focused. Separate unrelated cleanup, formatting changes, or feature ideas into separate contributions when practical.

## Reporting bugs

Include the following information in a bug report:

- Operating system and compiler version.
- OpenSSL and zlib versions when relevant.
- The exact command that failed.
- The complete error output.
- A minimal reproduction case.
- Whether the problem occurs after a clean build.

Do not include private files or secrets from a local repository. If a test fixture is needed, use a small synthetic file.
