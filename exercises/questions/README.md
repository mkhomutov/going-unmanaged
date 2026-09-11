# exercises/questions/

The field questions' listings — not an exercise, in the shape of
`../cost/` and `../choosing/`: one translation unit per question, `q<N>.cpp`
for row Q<N> of `book/M-field-questions.md`'s index, included by the
chapter section that answers the question, and nothing to attempt cold.

The rules, each held by a script:

- **The answering section includes the file.** Each `q<N>.cpp` fences the unit the entry
  shows between `answer` section-marker comments (the shape every listing in
  the book uses — see CONTRIBUTING.md, "The Question template"; the syntax
  is spelled there, not here), and the section's one cpp fence includes
  it. Edit the code here and the page follows; `scripts/check_verbatim.sh`
  refuses a marked section no page includes and a fence that copies one.
- **`main()` judges the claim.** Below the marked section, a `main()`
  asserts what the entry says, with a `CHECK`-style judge that counts
  failures and sets the exit code — never `assert`, which a Release build
  compiles away — so an answer that stops being true stops the build.
- **Nothing to register.** `scripts/build_all.sh` globs this directory and
  builds and runs every `q*.cpp` under the canonical flags. That is the one
  place in the repository a source is picked up without a line naming it,
  because the entries here are appended unattended by the `kb.yml`
  workflow, and a forgotten line would leave a listing the page shows and
  nothing compiles.
- **Standard library only**, like `solutions/`.
