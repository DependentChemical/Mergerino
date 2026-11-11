# AGENTS.md

Mergerino — a desktop chat client that merges Twitch, Kick, YouTube and TikTok LIVE
into a single window. C++20 / Qt6 / CMake. Forked from `Fixlation/Mergerino`
(Chatterino-based) and maintained at `github.com/DependentChemical/Mergerino`.

## Setup commands

- Install deps: `conan install .. -s build_type=Release -c tools.cmake.cmaketoolchain:generator="Ninja" -b missing`
- Build: `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake" .. && ninja`
- Test: `cmake -DBUILD_TESTS=On -DBUILD_APP=OFF -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake" .. && ninja && ctest --output-on-failure`
- Lint: `scripts/check-format.sh` (clang-format), `scripts/check-clang-tidy.sh`, `scripts/check-line-endings.sh`
- CI: GitHub Actions builds and tests every push to `master`. A successful build
  publishes a release to every user of this fork — see [Releasing](#releasing).

## Project layout

- `src/` — application code (`common/`, `singletons/`, `widgets/`, `providers/`, `controllers/`, `util/`)
- `tests/` — GoogleTest suite, run with `ctest` (`tests/src/`)
- `cmake/` — CMake modules and templates (`cmake/resources/windows.rc.in` is the tracked `.rc` template; `resources/windows.rc.in` is generated — never edit it)
- `.github/workflows/` — `build.yml` (Windows build), `test-windows.yml` (tests), `release.yml` (publishes the rolling release)
- `resources/` — icons, translations, avatars
- `scripts/` — repo tooling (format/lint checks, asset updates)
- `docs/` — user-facing website and documentation
- `lib/` — vendored third-party code; never edit

## Code style

- `.clang-format` (Google-based, 4-space indent, 80 columns) and `.clang-tidy` define the rules — match the surrounding code
- Qt idioms: `QStringLiteral` for literals, `const` locals, early returns, RAII
- PascalCase types/methods, camelCase members, `snake_case` filenames, `UPPER_SNAKE` constants
- Keep files close to upstream `Fixlation/Mergerino` where possible: prefer additive edits over reformatting untouched code, so upstream merges stay clean

## Versioning

### The four places a version number lives

| File | Line | What it is | What it drives |
| --- | --- | --- | --- |
| `src/common/Version.hpp` | 37 | `CHATTERINO_VERSION` | Compiled into the binary. This is the "current version" the update check compares against. |
| `src/common/Version.hpp` | 38 | `CHATTERINO_DISPLAY_VERSION` | Title bar text, and the value the release workflow reads to name the release. |
| `CMakeLists.txt` | 71 | `project(mergerino VERSION ...)` | `@PROJECT_VERSION@` → Windows `FileVersion`/`ProductVersion` via `cmake/resources/windows.rc.in`. |
| `README.md` | 7 | `Current version: Mergerino X.Y.Z` | User-facing documentation. |

**All four must carry the same number.** Bumping the version means editing all four.

### Rules

1. Bump `CHATTERINO_VERSION` and `CHATTERINO_DISPLAY_VERSION` **together**.
   - `DISPLAY` newer than `VERSION` → the release is published under the new number but the
     installed app still reports the old one → users are prompted to update forever.
   - `VERSION` newer than `DISPLAY` → the release keeps the old name and the app looks like
     it is ahead of the release → no update is offered and the title bar shows the wrong number.
2. `CMakeLists.txt` only affects the Windows file properties — bump it to match so the exe
   metadata is not stale. It does not affect the update check.
3. Never change these look-alikes; they are **not** the app version:
   - `conanfile.py` (`libavif/1.3.0` and other dependency versions)
   - `tests/src/Updates.cpp` (deliberate semver fixtures)
   - comments citing upstream Chatterino `1.3.3` in `src/singletons/Theme.cpp` and `src/widgets/splits/SplitInput.cpp`
   - the sample output in `.github/ISSUE_TEMPLATE/a_make_a_report.yml` (it shows users what to write)
   - `cmake_minimum_required`, `CHATTERINO_MIN_QT_VERSION`, and Qt version checks
4. Keep `patchnotes.txt` in sync with user-visible changes: newest section first, headed by a
   `DD/MM/YYYY` date line (the post-update dialog parses exactly this format and shows the
   newest dated section).
5. After editing versions, verify consistency with:
   `git grep -n -E '1\.3\.[0-9]+' -- . ':(exclude)lib'`
   Every hit must be one of the four places above, or an intentional look-alike from rule 3.

### Releasing

`release.yml` runs after every successful `master` build and replaces the rolling `latest` release:

- the release name is read from `CHATTERINO_DISPLAY_VERSION` (fallback `CHATTERINO_VERSION`)
  in `src/common/Version.hpp` at the release commit → `Mergerino <version>`
- the release body records `Build commit: <sha>`, which the app uses to identify the build
- the tag is always `latest`; the asset is `Mergerino.zip`
- the app polls `api.github.com/repos/DependentChemical/Mergerino/releases/latest` —
  **never point this at another repository** without asking the user; that would redirect
  every user's update channel

Version comparison is semver, so a version bump is what produces a clean "update to X.Y.Z"
prompt. With an unchanged version number the app only offers a build when the published
commit differs *and* the release was published after the running build was created.

### Before pushing to `origin/master`

A push to `master` publishes a release to every user of this fork. Therefore:

- **Ask the user whether to bump the version** before pushing, and before completing a
  feature addition that changes user-visible behaviour. Do not bump it on your own.
- If the user says yes, apply all four edits from the table above, then verify with the
  `git grep` command in rule 5.
- Never push to `origin/master` without explicit confirmation.

## Testing instructions

- Unit tests: GoogleTest via `ctest --output-on-failure`; test sources live in `tests/src/`
- Add a test next to the existing ones for every new behaviour (`tests/src/Updates.cpp` for
  the updater, `tests/src/KickApi.cpp` for the Kick API, …)
- CI runs the full suite on every push — do not push with known failing tests

## PR & commit conventions

- Default branch is `master`; this fork pushes directly to it (no PR workflow)
- Commit messages: imperative summary line, then a body explaining *why*
- Keep upstream-mergeable files close to upstream `Fixlation/Mergerino` to avoid merge pain

## Security

- Never commit secrets. `MRAB` and `PHGTK` are repository secrets consumed by
  `.github/workflows/build.yml`; `src/providers/youtube/YouTubeOAuthSecrets.local.hpp` and
  `src/controllers/analytics/PostHogProjectToken.local.hpp` are generated at build time and
  are git-ignored — never commit them
- `update-emoji-data.sh` and `get-vcpkg-package-versions.sh` download third-party content;
  review it before committing the result
