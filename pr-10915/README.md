# Screenshots for pull request #10915

Taken on Linux under Xvfb with the `linux-debug-nosan` preset, on a throwaway
profile, with a demo package whose Lua does not work: `DragonHUD`, holding a
trigger whose body does not compile, a script that calls a function that is not
there, and one script that works.

| image | build | what it shows |
| --- | --- | --- |
| `before-console.png` | `development` (aad7c9c) | installing two such packages from a script: complete silence, and `installPackage()` answers a bare `true` |
| `repeat-on-profile-open.png` | the first cut of this branch | why the notice was narrowed: every profile open reinstalls its modules, so a broken module put the line back at the top of the console on every launch |
| `after-console-manager.png` | this branch | the install a person asked for - here from the package manager - names the parts that are not working and leaves the why to the editor |
| `after-console-script.png` | this branch | the same two packages installed from a script: the console stays quiet, and the reason comes back beside the `true` for the caller to report as it likes |
| `module-creator.png` | this branch | the module creator reports the export it did, and leaves the broken item to the editor, which is already showing it against the item itself |

This branch carries the images only, so that they can be linked from the pull
request without adding them to its diff. It can be deleted once #10915 is merged.
