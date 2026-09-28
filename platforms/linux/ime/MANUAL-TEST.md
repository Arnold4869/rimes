# RIMES Linux IME — real-hardware checklist

Automated CI covers librime smoke, in-process Fcitx5 `testfrontend`, and a
headless DBus/Xvfb path. It does **not** replace a real desktop. Run this on
at least two distros after installing the addon.

## Machines

Record distro, desktop, session type, and host app for each row.

| Distro | Session | Desktop | Host | Result |
|---|---|---|---|---|
| Ubuntu 24.04 | X11 | GNOME | gedit (GTK) | |
| Ubuntu 24.04 | Wayland | GNOME | gnome-text-editor | |
| Ubuntu 24.04 | X11 | KDE Plasma | kate (Qt) | |
| Fedora (current) | Wayland | KDE Plasma | kwrite | |
| Fedora or Arch | Wayland | GNOME | Firefox | |
| Any | X11 or Wayland | any | Chromium / Electron (VS Code, Slack) | |
| Any | either | any | GNOME Terminal / Konsole | |
| Any | either | any | LibreOffice | |

Also try one machine with stock `fcitx5-rime` **still installed**: RIMES must
appear as a separate IM, and `~/.local/share/rimes` must stay distinct from
`~/.local/share/fcitx5/rime`.

## Per-host cases

For each host above:

1. Switch to **RIMES**. Default schema should be 雾凇全拼 (`rime_ice`).
2. Type `nihao` and confirm preedit plus a candidate list in the Fcitx5 panel.
3. Press Space. The field must contain `你好` and no leftover `nihao`.
4. Type `ni`, press `2` (or another number). A non-first candidate commits.
5. Type a short syllable with many pages (`a` or `ni`), press Page Down, then
   Page Up. The highlighted page must change.
6. Type `nihao`, press Escape. Composition clears; nothing commits.
7. Press F4 (or the Fcitx5 IM menu) and switch to 自然码 / 小鹤 / 五笔 / 英文
   if those schemas deployed. Type a short sample in at least one other schema.
8. Click into a password field. RIMES must not leak preedit into the password
   widget in a surprising way; note the toolkit behaviour.
9. Switch away mid-composition, then back. No stuck preedit, no double commit.

## Session / compositor notes

- GNOME + Wayland uses the Fcitx5 Wayland IM protocol. If only XWayland apps
  work, record that — IBus would need the same matrix later.
- KDE usually uses `fcitx5-frontend-qt5` / Qt6. Confirm both Qt and GTK apps.
- Electron/Chromium sometimes need `GTK_IM_MODULE=fcitx` exported in the
  desktop environment, not only the shell that launched the test.

## What this checklist does not cover

Buffer, Capsule, Mailbox, custom candidate chrome, and IBus. Those are later
Linux steps.
