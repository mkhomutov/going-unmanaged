# third_party/ — vendored dependencies

Chapter 27's first strategy, practised: a dependency's source copied into
the tree, with the two things that chapter says to write down beside it —
the version, and whether anything was patched.

| Directory | What | Version | License | Local patches |
|---|---|---|---|---|
| `nlohmann/` | [JSON for Modern C++](https://github.com/nlohmann/json), the single-header `json.hpp` | 3.12.0 | MIT — `nlohmann/LICENSE.MIT` beside the header | none |

Two rules keep this honest. **Nothing under `solutions/` includes anything
here** — CONTRIBUTING's ground rule stands: solutions are standard-library
only, and this directory exists for the one cookbook translation unit
(`exercises/cookbook/json.cpp`, Recipes 25–26) that needs a library the
standard does not ship; `build_all.sh` greps `solutions/` for it. And it is
included with `-isystem`, so the canonical `-Wall -Wextra` keep meaning what
they mean for our code while a vendored header's own warnings, if a version
ever has any, stay the vendor's.

Updating: replace the header with the new release's `single_include/nlohmann/json.hpp`
and `LICENSE.MIT` with the release's, change the version in the table (the
build asserts the two agree), and if you patched it, say what and why in
this file before anything else.
