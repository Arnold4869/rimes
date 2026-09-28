# RIMES Fcitx5 input method (Linux)

This is a real RIMES frontend for [Fcitx5](https://fcitx-im.org/): a C++ addon
that links `librime` (with `librime-lua` and OpenCC) and deploys the reviewed
RIMES `rime-data` set. It lives next to the existing
[Linux data preview](../README.md), which still installs into stock
`fcitx5-rime` / `ibus-rime` and is unchanged.

This is step 1 of the Linux port (IME only). Buffer, Capsule, and Mailbox are
out of scope. See `src/engine/rime_hooks.hpp` for how those layers should hook
in later.

## What works

- Preedit (client + panel) and Fcitx5 candidate UI
- Space commit (`nihao` + Space → `你好` on `rime_ice`)
- Number keys 1–9 select the current page
- Page Up / Page Down
- Escape cancels composition without committing
- Isolated user directory: `$XDG_DATA_HOME/rimes` (not `…/fcitx5/rime`)
- Shared data: `$prefix/share/rimes/data` (policy-staged 55-file closure)

macOS-only behaviour that is **not** reproduced here: custom candidate chrome,
Buffer / Capsule / Mailbox, cross-batch chord pairing, `Delivery.insert`.

## Dependencies

See `scripts/deps.sh --print` for Debian/Ubuntu, Arch, and Fedora package
lists. On Ubuntu 24.04:

```bash
platforms/linux/ime/scripts/deps.sh --install
```

`librime-plugin-lua` and a distro OpenCC `opencc/s2t.json` are required. The
addon will not bundle those.

## Build and install

```bash
platforms/linux/ime/scripts/build.sh
platforms/linux/ime/scripts/install.sh          # default prefix: ~/.local
# or
sudo RIMES_IME_PREFIX=/usr platforms/linux/ime/scripts/install.sh
fcitx5 -r
```

Then add **RIMES** in Fcitx5 configuration. Restart the compositor/session if
the new input method does not appear.

Environment overrides used by tests and unusual layouts:

| Variable | Meaning |
|---|---|
| `RIMES_SHARED_DIR` | Absolute SharedSupport root (`default.yaml`) |
| `RIMES_USER_DIR` | Isolated user dir (must not be the stock fcitx5-rime path) |
| `RIMES_LOG_DIR` | librime log directory |

## Packaging

```bash
platforms/linux/ime/scripts/package-deb.sh /tmp/rimes-deb
```

writes `fcitx5-rimes_<version>_<arch>.deb`. Flatpak notes are in
`packaging/flatpak/README.md`. The data preview tarball is a separate artifact
and stays data-only.

## Tests

```bash
platforms/linux/ime/scripts/run-tests.sh
```

1. CTest unit tests (keysyms, UTF-8 snapshot, path isolation)
2. Live `librime` smoke: `nihao` + Space, number selection, paging, Escape
3. In-process Fcitx5 `testfrontend` (no display)
4. Optional Xvfb + live Fcitx5 DBus virtual input context
5. GTK/Qt hosts via `xdotool` (best-effort: headless Xvfb has no window
   manager, so toolkit IM modules may not commit even when DBus does)
6. Weston headless is probed when installed; injecting keys into a
   seat-less compositor is a documented limitation

GitHub Actions workflow `.github/workflows/linux-ime.yml` runs the same
sequence on `ubuntu-latest`. It is **not** a macOS merge/release gate.

## IBus follow-up

An IBus engine can reuse `RimeEngine`, `EngineSnapshot`, and the path resolver.
It would be a new `ibus-rimes` component XML plus a C++ `IBusEngine` that maps
IBus keysyms (already X11) into `ProcessKey` and commits with
`ibus_engine_commit_text`. Estimate: similar size to this addon, no Fcitx5 UI.
Not started in this tree.

## Later Buffer / Capsule / Mailbox

See `src/engine/rime_hooks.hpp`. The single commit path is
`RimesIme::commitText` → `InputContext::commitString`. One Rime session per
Fcitx5 input context. A Linux Buffer should attach as another
`InputContextProperty` or a process-local panel; it must not start a second
librime runtime.
