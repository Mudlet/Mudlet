# Screenshots for pull request #10915

Taken on Linux under Xvfb with the `linux-debug-nosan` preset, on a throwaway
profile, by installing two demo packages whose Lua does not work:

- `DragonHUD` - a trigger whose body does not compile and a script that calls a
  function that is not there, plus one script that does work
- `MoonMap` - an alias whose body does not compile, plus one script that does work

| image | build | what it shows |
| --- | --- | --- |
| `before-console.png` | `development` (aad7c9c) | both packages install in complete silence, `installPackage()` answers a bare `true` |
| `after-console.png` | this branch (3a19c09 merged with development) | each install names what in it is not working and why, and `installPackage()` hands the same text back as a second return value |
| `before-module-creator.png` | `development` (aad7c9c) | a module made out of the trigger that does not compile is reported as "created and installed successfully!" |
| `after-module-creator.png` | this branch | the same module is reported as installed with what in it is not working, shown as a problem rather than a clean success |

This branch carries the images only, so that they can be linked from the pull
request without adding them to its diff. It can be deleted once #10915 is merged.
