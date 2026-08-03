## What kind of change is this?

One topic per PR: one Finding, one correction topic, or one exercise.

- [ ] Finding (Chapter 25)
- [ ] Correction
- [ ] New exercise (+ solution + `build_all.sh` entry)
- [ ] New chapter (opened an issue first — see ROADMAP.md)
- [ ] Tooling / docs

## Where does it belong?

<!-- Which chapter, exercise, or file — and for Findings: which exercise
     surfaced it, and confirm you hit the mistake yourself. Lived experience
     is what makes a Finding worth reading. -->

## Checklist

- [ ] `./scripts/build_all.sh` prints **ALL GREEN** locally
- [ ] Book edits: `./scripts/build_book.sh` builds and `--check-nav` passes
      (run `--write-nav` if you added or renamed a chapter file), and
      `./scripts/check_markup.sh` passes
- [ ] No existing chapter or Finding numbers changed — appending is fine;
      renumbering is a MAJOR version event (open an issue first)
- [ ] Findings follow the Chapter 25 shape exactly:
      **Found in / The theory / broken vs fixed code / Habit**
      (template in CONTRIBUTING.md)
- [ ] New chapters, exercises, or appendix sections walked once against
      CONTRIBUTING's "The questions every piece of material answers" —
      each applicable question answered by a mechanism in the material
- [ ] New key principles are mirrored in Appendix B in the same PR
- [ ] Matches the book's voice: first-person curator, C# comparisons,
      British-neutral English; solutions use the standard library only
