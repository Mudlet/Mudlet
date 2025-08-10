Here’s how you can start contributing to the Mudlet project (Mudlet/Mudlet):

1. Get in touch and explore
- Join the community: Discord (badge in README) is the fastest way to discuss ideas with the core team.
- Browse existing issues: https://github.com/Mudlet/Mudlet/issues
  - Look for labels like good first issue, help wanted, or 💎 Bounty / $ amounts (Algora) if you’re interested in incentivized tasks.
- Read the vision and roadmap (linked in the README) to align ideas with project direction.

2. Pick what you want to contribute
You don’t have to start with C++:
- Code (C++/Qt and Lua internals)
- Lua scripting examples or starter packages
- Bug fixes & small UX improvements
- Documentation (wiki pages, clarifying tooltips, README sections)
- Internationalization (Crowdin-managed translations; auto-generated translation PRs are routinely merged)
- Testing (add or improve unit tests under src/mudlet-lua/tests/)
- Performance profiling or refactors (split into small, focused PRs)
- Packaging/build improvements (follow existing CI conventions)

3. Set up your development environment
- Follow the Compiling guide: https://wiki.mudlet.org/w/Compiling_Mudlet
- Ensure you have Qt, Lua, and other dependencies the build guide lists.
- Run the application locally to confirm a clean baseline build before making changes.

4. Follow core coding guidelines (summarized from CONTRIBUTING.md)
C++:
- Use clang-format with the repo’s src/.clang-format config.
- Use clang-tidy and clazy for linting.
- Always use braces {} around conditionals—even single-line bodies.
- Wrap string literals with qsl() (QStringLiteral helper) unless you’d create an empty QString (then prefer QString()).
- Escape dynamic UI label content with .toHtmlEscaped() before injecting into rich text.
Internationalization:
- Mark user-visible GUI text for translation.
- Do not translate API elements (function names, constants).
- Provide full sentences (avoid assembling fragments).
- Avoid assumptions about pluralization, quotes, or punctuation conventions.
Tooltips:
- Keep them as complete sentences with proper punctuation.
- For multi-paragraph tooltips, supply proper rich text (see examples in CONTRIBUTING.md); utils::richText(...) helps insert needed tags cleanly.

5. Good PR hygiene
- Keep PRs small and logically scoped (avoid “mega PRs”).
- Refactors or formatting-only changes should be separate from feature/bug fix PRs.
- Don’t add lingering TODO comments—open an issue instead if something needs future work.
- PR titles must begin with one of: fix, improve, add, infra (enforced by Danger bot).
- Prefer “Squash and merge” (adds PR number in history). Use rebase only if you intentionally want to preserve commits. Avoid merge commits.
- If you tackle something large, discuss design first on Discord to avoid rework.

6. Naming and clarity
- Use intuitive, self-explanatory names; check against known linguistic anti-patterns (see link in CONTRIBUTING.md).
- Keep user-facing strings simple and translation-friendly.

7. Testing & validation
- Add or update unit tests (especially for Lua logic under src/mudlet-lua).
- Manually verify UI changes on at least one additional platform if feasible (e.g., Windows + Linux).
- For features affecting internationalization, confirm translatable strings appear correctly (they’ll later flow through Crowdin).

8. Contributing Lua-side logic
- Lua “source of truth” is in src/mudlet-lua/lua with LuaGlobal.lua as the loader.
- Maintain structure and doc conventions (LuaDoc material is present; see luadoc-guide.txt).
- Regenerate docs only if necessary (script: genDoc.sh) and usually in a separate PR if it creates large diffs.

9. Translation workflow
- Translations are sourced via Crowdin; auto-generated PRs can be merged quickly.
- If you notice a translation issue, raise it via Crowdin rather than editing strings in code directly.

10. Picking your first issue
A practical progression:
- Start with a good first issue label.
- Ask in Discord if it’s still current (some older issues may need reconfirmation).
- Outline your intended approach in an issue comment before coding; maintainers can validate direction early.

11. Bounties
- Open bounties are visible via the badge (Algora). If claiming one, announce intent early to avoid duplicated effort.
- Deliverables should clearly close or advance the issue’s acceptance criteria.

12. Submitting your PR
Checklist before opening:
- Builds cleanly (no new warnings).
- clang-format & lint tools run.
- PR title uses required prefix.
- Added tests (if relevant) and/or manual test notes in the PR description.
- No unrelated file churn.
- Screenshots or GIFs for UI changes (helps reviewers).
- Mention closes #ISSUE_NUMBER to auto-link closure if it resolves an issue.

13. Non-code contributions
- Report bugs with clear reproduction steps, platform info, and expected vs actual behavior.
- Suggest UX improvements with mockups or examples.
- Improve documentation (clarify ambiguous build steps; add examples to wiki/manual).
- Create reusable Lua modules and share them (even if not core code).

14. Staying aligned
- Periodically pull/rebase against development (or the repo’s main active branch) to avoid merge conflicts.
- Communicate if you go inactive on an issue so others can pick it up.

Quick “Day 1” starter path
1. Build Mudlet locally.
2. Pick a small labeled issue (typo fix, tooltip improvement, minor bug).
3. Make change, run formatting/linting, test.
4. Open PR with proper title (e.g., fix: correct tooltip punctuation in mapper dialog).
5. Get feedback and iterate.

If you tell me what area interests you (UI, mapping engine, scripting API, performance, translations, docs), I can suggest a more targeted next step.

Ready to begin? Let me know what you’d like to work on and I can help you narrow down an actionable first issue.

Coding guidelines
If you're a first-timer, don't worry about conforming to all of these! We'll show you the ropes.

Code style
Naming things? Check against antipatterns
Check https://www.linguistic-antipatterns.com when naming anything to help ensure it can be understood intuitively.

C++
use clang-format for formatting your code with src/.clang-format settings. To get started, check out Clang Format in the Setting up IDE's section.
use clang-tidy linting with .clang-tidy settings. To get started, check out Clang Tidy in the Setting up IDE's section
additionally, use clazy for linting as well
use braces {} around all statements (ie, if's and so on), even if they are one line
use qsl() to wrap Qt strings, this ensures they're created at compile time
at the same time, don't use a blank qsl("") - use QString() in that case (source)
escape dynamic label information with .toHtmlEscaped() to ensure safe display (example).
Internationalization do's and don'ts
Do:

enable strings visible in the Mudlet GUI to be translateable
minimise use of HTML styling tags in strings to be translated
enable users to use language-specific Mudlet object names (triggers, aliases, labels, etc)
Don't:

translate the Mudlet API: functions, events, error messages or constants (e.g. main console)
use numbers in the API - English words are preferred instead
try to assemble a sentence on the fly - English grammar does not translate into other languages. Present the full sentence to translators instead
assume English-centric plural forms, other languages do not necessarily have the simple add an "s"/"es" for more/less than the singular case.
assume universal quote and number punctuation formats. There are languages that use « and » instead of " for "quoting" words or phrases. Qt can provide Locale specific displays of numbers/dates/times.
Tooltip tips:
Tooltips are (ideally) short pieces of text that can give additional hints or help with a control or setting. As such they are sentences so should end with the appropriate punctuation, usually a period (a.k.a. full-stop).
To avoid long single line tooltips that sprawl across the screen it is necessary to signal to the Qt libraries that the text is "rich-text", and this is done by the inclusion of HTML-like tags in the text. At a bare minimum this can be done by surrounding the text with a pair of paragraph tags: <p>...</p>.
To help with the above there is a static helper functon defined in the utils class called richText(...) that can be put around the text that will insert those tags. As such text is user facing as part of the User Interface (UI) it must be put through the translation system - and thus will likely be inside the QObject class's tr() method.
So as to reduce the need for translators to have to deal with HTML-like tags in the texts they have to work on, the richText function will eliminate the need for them to remember a pair of <p>...</p> around a single paragraph of text; however when more than one paragraph is used it is clearer to NOT use the utils::richText(...) and include the paragraph tags around each of them. The on-line translation system we use (CrowdIn) can be set to handle/hide HTML tags but it needs to see matched pairs to be able to make sense of them, so:
Do:

Single paragraph:
    widget->setToolTip(utils::richText(tr("A single sentence or paragraph that is a tool-tip.")));
More than one paragraph:
    widget->setToolTip(tr("<p>The first paragraph that is a tool-tip.</p>"
                          "<p>Another paragraph, maybe in a different style, e.g. <i>italics</i> or <b>bold</b>.</p>")));
Don't:

More than one paragraph:
    widget->setToolTip(utils::richText(tr("The first paragraph that is a tool-tip.</p>"
                                          "<p>Another paragraph, maybe in a different style, e.g. <i>italics</i> or <b>bold</b>.")));
TODO's
Avoid adding TODO's to code - file an issue or fix it with a separate pull request instead. Practice has shown that TODO's get added to the codebase but seldomly get resolved.

Refactoring
Refactors for linting or formatting should be their own PRs
Do not change code in code paths which are not a part of the PR
If it needs to be refactored, it deserves to be its own PR
Danger enforced PR requirements
PR Title must start with fix, improve, add, or infra
This facilitates automatic changelog gathering and categorization
Cannot merge until it is fixed: core team can always adjust it before merging
Danger will also give a heads up if the PR title is long, or if more than 10 source files are changed in a single PR. These are not blocked but the warnings should serve to draw attention to something which may require a double check. More info below.

Mega PRs
Pull Requests that overhaul large pieces of functionality at once will not be accepted: through experience, they bring more pain than they are worth. Being really difficult to discuss, test, and reason about, they are banned.

That does not mean we don't welcome large overhauls: we do! Just make sure to send it in as separate, logically broken-down improvements that implement the functionality you'd like to have in a step process.

Of course, before embarking on such a journey, discuss with the core team your ideas first so we can guide you on the best design!

Git commit guidelines for core team
The preferred order of merging PRs is:

Prefer squash and merge for a clean history and added PR numbers for details of discussion for future comparison.
Else rebase and merge if you'd like to keep the history, but know this will not link to the PR in public test builds' (PTB) changelogs, etc.
Avoid creating a merge commit.
Merging auto-generated translation PRs
PRs auto-opened by mudlet-machine-account with new translation strings can be approved and merged right away by anyone on the core team.

The idea is to use Crowdin as a single source of truth for translation - if there's an issue with a translation, let's discuss it in Crowdin.