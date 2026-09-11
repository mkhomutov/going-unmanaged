## What kind of change is this?

One topic per PR: one Finding, one correction topic, or one exercise.

- [ ] Finding (Chapter 25)
- [ ] Correction
- [ ] New exercise (+ solution + `build_all.sh` entry)
- [ ] New chapter (opened an issue first — see ROADMAP.md)
- [ ] Question answered (from a Question issue; indexed in Appendix M)
- [ ] Tooling / docs

## Where does it belong?

<!-- Which chapter, exercise, or file — and for Findings: which exercise
     surfaced it, and confirm you hit the mistake yourself. Lived experience
     is what makes a Finding worth reading. -->

## Checklist

- [ ] `./scripts/build_all.sh` prints **ALL GREEN** locally
- [ ] Book edits: `./scripts/build_site.sh` (strict) and
      `./scripts/check_markup.sh` pass
- [ ] No existing chapter or Finding numbers changed — appending is fine;
      renumbering is a MAJOR version event (open an issue first)
- [ ] Findings follow the Chapter 25 shape exactly:
      **Symptom / The theory / broken vs fixed code / Habit**
      (template in CONTRIBUTING.md)
- [ ] Recipes follow the Appendix F shape exactly:
      **In C# / The recipe / Why it looks like this / Trap**, and the
      listing is included from `exercises/cookbook/` by a marked section
      (template in CONTRIBUTING.md)
- [ ] Question answers live in the owning chapter (or a Recipe / Finding),
      have their Appendix M row and `search_queries.tsv` line, and include
      the listing from `exercises/questions/` (template in CONTRIBUTING.md)
- [ ] New chapters, exercises, or appendix sections walked once against
      CONTRIBUTING's "The questions every piece of material answers" —
      each applicable question answered by a mechanism in the material
- [ ] New key principles are mirrored in Appendix B in the same PR
- [ ] Matches the book's voice: first-person curator, C# comparisons,
      British-neutral English; solutions use the standard library only
