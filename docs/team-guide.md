# Smart Dumbbell Team Repository Guide

Repository: [juliazhang1015/smart-dumbbell](https://github.com/juliazhang1015/smart-dumbbell)

Use this workflow: update your copy, claim a task, create a branch, edit and validate, open a pull request, get a teammate's review, merge, and update again. The repository contains planning materials, prototype code and initial bench records; these do not establish full acceptance-test completion.

## 1. Git vocabulary

| Term | Meaning here |
|---|---|
| Repository | Project files and their change history |
| `main` | The team's accepted version, not a claim that every feature works |
| Branch | A separate line of changes for one task |
| Commit | A described set of changes saved locally |
| Push | Upload local commits to GitHub |
| Pull | Bring remote changes into your local copy |
| Issue | A task or problem with an owner, goal and completion criteria |
| Pull request (PR) | A request to review and merge a branch |
| Merge | Incorporate accepted changes into the shared version |

Saving a file, committing, pushing and merging are separate steps.

## 2. Joining the repository

The owner collects teammates' GitHub usernames and invites them through **Settings → Collaborators**. Each teammate accepts using their own account. Public visibility does not give everyone write access. Never share account passwords or tokens. These instructions do not automatically add collaborators or enable branch protection.

Read the root README, this guide and the specification. Use a separate local clone for code, CAD and slides; the web editor is suitable for small text changes. Do not share a synchronized working directory containing `.git`.

```bash
git clone https://github.com/juliazhang1015/smart-dumbbell.git
cd smart-dumbbell
git status
```

If you already have a clone, check its remote before using it. Files outside the clone do not automatically synchronize. Configure your own author identity if needed:

```bash
git config user.name "Your Name"
git config user.email "YOUR_GITHUB_NOREPLY_EMAIL"
```

Use the noreply address in GitHub **Settings → Emails** if you want to keep your email private. Commit identity and authentication for pushing are separate settings.

## 3. File locations and authority

| Content | Location |
|---|---|
| Overview and current links | `README.md` |
| Working requirements and acceptance criteria | `docs/specification.md` |
| Dated design decisions | `docs/decisions.md` |
| Components, budget, measured dimensions | `docs/hardware.md` |
| Project handoff and known issues | `docs/handoff.md` |
| Research | `docs/research-plan.md` and related documents |
| Specification exports | `docs/exports/` |
| CAD, generators and dimension assumptions | `hardware/cad/` |
| Appearance studies | `hardware/renders/` |
| Schematics and pin assignments | `hardware/wiring/` |
| Pitch and matching narration | `presentation/` |
| Microcontroller code | `firmware/` |
| Computer interface and logging | `software/` |
| Data processing, training and evaluation | `ml/` |
| Test methods and results | `tests/` |
| Data-handling guidance | `data/` |

Update the specification before changing functionality; update the BOM and decision log for hardware changes, then affected exports and presentation materials. Verbal discussions are not automatically a new baseline. The 80 mm head width is only a candidate; committed CAD v0.1 has a 100 mm maximum width.

## 4. Claim a focused task

Use **Issues → New issue → Task**. Include the problem and affected modules, one owner and reviewer, a due date, completion criteria linked to `FR-xx` or `AT-xx`, and dependencies. Track `Todo / In progress / In review / Done / Blocked` in the body without relying on configured labels. Software tasks normally close after merging; experiments need a result record.

Work areas include electronics/power, firmware, ML/data, and enclosure/computer interface. These areas are not automatic assignments. Prefer a task such as “record six-axis IMU data” over “finish all firmware.”

## 5. Editing on GitHub

1. Start from `main` and create a branch such as `docs/sensor-research`.
2. Confirm the selected branch, edit the intended Markdown file, and commit with a specific message.
3. Choose **Compare & pull request**, with base `main` and your task branch as compare.
4. Fill out the PR template and ask a teammate to review.

Upload binary artifacts on a task branch too. Check the destination and filename first; use local Git for bulk changes.

## 6. Local workflow

Start with a clean working tree:

```bash
git status
git switch main
git pull --ff-only
git switch -c docs/sensor-research
```

If you have unfinished edits, save them on your task branch first. Do not discard them with a forced switch or `reset --hard`. Investigate a failed fast-forward instead of forcing it.

After editing and checking:

```bash
git diff
git add docs/hardware.md
git diff --cached
git commit -m "docs: confirm motion sensor availability"
git push -u origin docs/sensor-research
```

Stage only intended files. The example message is appropriate only after actually confirming availability. A PR should explain the problem, changes, related requirements, checks, untested behavior and any changes to interfaces, hardware, budget, dimensions or acceptance criteria. Use Draft for unfinished work.

To incorporate teammates' work after committing your own:

```bash
git fetch origin
git merge origin/main
```

After your PR is merged:

```bash
git switch main
git pull --ff-only
git branch -d docs/sensor-research
```

Delete only a merged local branch. If deletion is refused, investigate instead of using `-D`. The remote branch can be deleted from the merged PR page.

## 7. Review and merge

Normal changes enter `main` through a PR reviewed by at least one teammate other than the author. Review the actual diff and validation, including affected specifications and presentation claims. The maintainer or agreed merge owner merges after resolving conflicts; the initial convention is a merge commit. These are team conventions, not automatically enforced rules.

Repository initialization and first-time collaboration setup are bootstrap exceptions. Explicit user instructions can adjust the workflow; keep an explanation of changes.

## 8. CAD, slides and PDF collaboration

Assign one editor per binary artifact/version. Start from current `main`, use versioned names such as `Smart_Dumbbell_Concept_v02.dwg`, and provide a preview or change list. Screenshots do not replace editable sources. Keep CAD notes and generators aligned, or state when the generator cannot reproduce the new model. Update matching narration and timing for slides and identify the specification version behind PDF exports. Update README links when a version is accepted.

Concept rendering details are not manufacturing dimensions. Use reviewed CAD and measured parts for fabrication.

## 9. Code, interfaces and test handoff

Module READMEs should record dependencies, tool versions, run instructions and reproducible checks. Initial wiring records also need module versions, levels, pins and axis orientation. Example branches: `firmware/lsm6dsox-readout`, `firmware/curl-counter`, `ml/curl-idle-other`, `software/session-logger`, `hardware/grip-contact-test`.

Changes to Wi-Fi fields, units, timestamps or invalid values must update the interface specification and explain how both ends adapt. Mark unimplemented interfaces as proposals.

Copy the [test template](../tests/templates/test-record.md) into `tests/results/` with a date and test name. Record the commit, hardware/model versions, steps, data source, criteria, actual results and limitations. Compilation, simulation, offline evaluation and hardware measurements are distinct evidence. A CAD audit does not establish mechanical strength or clearance.

Split ML datasets by participant or complete session before windowing. Report heart-rate coverage and error together.

## 10. Public repository boundaries

Commit project code, reviewed documents, BOMs, CAD, presentations, approved anonymous results and explicitly labelled synthetic data. Do not commit credentials, private contact lists, identifiable health data, unapproved course materials or unrelated files. Raw data needs explicit authorization.

`data/raw/`, `data/private/` and `data/session_logs/` are ignored by default. Ignore rules do not protect already tracked files or erase history; do not casually force-add data. Revoke leaked credentials and notify the owner; deleting the latest copy is insufficient. Coordinate any personal-data incident before further distribution.

## 11. Troubleshooting

| Problem | Action |
|---|---|
| Push permission denied | Check account, accepted invitation and remote |
| Local edits block switching | Save on the task branch; do not overwrite |
| Fast-forward pull fails | Inspect status and recent graph; do not force push |
| Text conflict | Reconcile both intentions, remove markers, validate and commit |
| Abort an unresolved merge | Use `git merge --abort`, then coordinate |
| DWG/PPTX conflict | Ask the owner to reconcile in the native application |
| Teammate cannot see edits | Check push status and selected branch |
| Uploaded file absent from main | Check merge status and README links |
| Bad change already merged | Use a corrective commit or PR; preserve public history |

## 12. End-of-task checklist

- [ ] Clear task, owner and current baseline.
- [ ] Only intended changes staged on the task branch.
- [ ] Affected requirements, interfaces, BOM, CAD and narration updated or follow-up documented.
- [ ] Actual checks and unverified behavior recorded.
- [ ] No credentials or unapproved personal data.
- [ ] Branch pushed and PR targets `main`.
- [ ] Reviewer knows what to check.

Reference: [GitHub flow](https://docs.github.com/en/get-started/using-github/github-flow).
