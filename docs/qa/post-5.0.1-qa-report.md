# QA report: development since 5.0.1

Status: in progress (batch 1 of 4 complete, verification of batch 1 running).
Plan: `post-5.0.1-qa-plan.md`. Every item below was either run by the coordinator
or reproduced by an independent verifier; agent claims that were not reproduced
are listed in their own section, not among the findings.

## Trees under test

| Batch | Tree | Notes |
| --- | --- | --- |
| 1 | `dbbf040c3` | development head when the campaign started |
| 2 onwards | `85d814292` | development `12b373743` (two commits later) merged in: #10858 keychain test harness fix, #10392 negative wrap indent |

## Automated baselines

| Tree | ctest (linux-debug-nosan, offscreen) | busted specs |
| --- | --- | --- |
| `dbbf040c3` | 218 / 220 passed. `CredentialManagerKeychainTest` SEGFAULT; `HomeUntouchedTest` failed only because it runs that binary as a child | 4698 passed, 0 failed, 0 errors, 86 pending |
| `85d814292` | 220 / 220 passed | 4702 passed, 0 failed, 0 errors, 86 pending |

The keychain crash was a use-after-free inside the test's own job staller, not
in `CredentialManager`; #10858 (merged during the campaign) removes it, and the
rebuilt tree passes both tests. The running application handles the same
keychain-less environment by falling back to the encrypted file.

## Confirmed findings

Ranked by severity. "Confirmed" means the coordinator re-ran the reproduction
and saw the reported behaviour.

### 1. Major: an ESC that ends a stalled read still loses its colour code (efe9414f2, fix incomplete)

Commit efe9414f2 stops the carriage-return marker that Mudlet injects after a
300 ms network pause from corrupting a multi-byte character or a colour code
split mid-parameter. It does not cover the case where the ESC is the last byte
before the pause: the ESC is discarded and the next read prints the rest of the
sequence as text. Reproduced on `dbbf040c3` and the code is unchanged on
`85d814292`:

```lua
local E = string.char(27)
feedTelnet("whole1:"..E.."[1;31mRED1:end\n")
tempTimer(0.2,  function() feedTelnet("csiA:"..E) end)
tempTimer(0.65, function() feedTelnet("[1;31mRED1:end\n") end)
tempTimer(1.1,  function() feedTelnet("csiB:") end)
tempTimer(1.55, function() feedTelnet(E.."[1;31mRED1:end\n") end)
```

Result: `whole1:RED1:end` and `csiB:` / `RED1:end` are right; `csiA:` is
followed by the literal line `[1;31mRED1:end`. The `mGotESC` block in
`src/TBuffer.cpp` lacks the `localBufferDecodableLength` guard the `mGotCSI`
block received. Same visible defect as #10766, in a one-byte window. Found by
agent B1 (F-B1-1), re-run by the coordinator.

### 2. Minor, pre-existing: `mudlet --version` aborts when there is no display

`env -u DISPLAY mudlet --version` exits with SIGABRT after Qt fails to load a
platform plugin; with `QT_QPA_PLATFORM=offscreen` it prints the version. The
option is handled after the `QApplication` is built, in 5.0.1 as on
development, so this is not a regression. Found by agent E1 (F-E1-4), re-run by
the coordinator.

## Fixed during the campaign

- `CredentialManagerKeychainTest` SEGFAULT and the consequent `HomeUntouchedTest`
  failure (agent E1, F-E1-1 and F-E1-2): fixed by #10858, verified by re-running
  both tests on the rebuilt tree (pass).

## Environment notes for anyone repeating this

- Shared ccache: `/etc/ccache.conf` with `base_dir=/home/user`; a clean
  rebuild of the app target from a second worktree ran in 13 s with all 367
  compiles served from cache.
- Two batch-1 agents killed each other's Mudlet and X server while cleaning
  up by process name. The agent brief now requires recording PIDs at launch
  and killing only those.
