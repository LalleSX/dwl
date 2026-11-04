# VARBS dwl — first iteration

Based on the imported dwl **0.10-dev** tree (`2f0b144` before customization),
whose changelog covers 0.9 and whose build requires **wlroots-0.20**. The fork
has no upstream release tags or original upstream commit ancestry, so an exact
upstream commit cannot be established from its history. Do not mix wlroots API
versions or assume an arbitrary dwl patch applies to this development snapshot.

## Build and configure

On Arch, provision `base-devel`, `pkgconf`, `wayland`, `wayland-protocols`,
`libinput`, `libxkbcommon`, **wlroots0.20** (must provide `wlroots-0.20.pc`),
`libxcb`, `xcb-util-wm`, and runtime `xorg-xwayland`. Package availability and
names depend on the Arch snapshot; check the pkg-config identity before building.
wlroots must include DRM/libinput and XWayland support. No dependency was upgraded
in the source tree.

```sh
pkg-config --modversion wlroots-0.20 wayland-server wayland-protocols
make
make check
```

The first build copies tracked `config.def.h` to local ignored `config.h`.
Customize `config.h` and rebuild; for VARBS deployment, change `config.def.h`.
An existing `config.h` is intentionally preserved, including by `make clean`.
When updating an old checkout, merge the new defaults into it, or back it up and
copy `config.def.h` over it before rebuilding. `make clean && make` forces a full
rebuild. Restart the entire session to apply changes (save work first).

XWayland is enabled by default. To omit it: `make clean`, then
`make XWAYLAND= XLIBS=` (use the same overrides for `make check`).
Nothing installs automatically. Review `make install` paths before installing
through VARBS. The build environment's binary is for its Linux toolchain;
rebuild on Arch before deploying.

## Behavior and appearance

- Nine tags, tile / floating / monocle, one master using 55% of usable width.
- Click to focus and raise. Moving the pointer alone does not steal focus.
- Super-drag floats a tiled window automatically; fullscreen windows cannot be
  dragged. Upstream edge snapping and minimum/maximum size hints remain in use.
- Parent dialogs inherit their parent's monitor/tags and float; fixed-size and
  supported X11 dialog/modal types float automatically. No broad app-specific
  floating rules or forced tag assignments.
- Dark neutral background, muted blue focused border, warm urgent border,
  2 logical-pixel borders, no new effects. Gaps are deferred: upstream tile math
  is retained, including size-hint handling and small-output behavior.
- Flat pointer acceleration, default speed, repeat 40 Hz after 300 ms. All live
  in `config.h`; unsupported input-device options are handled by libinput.

## Controls

Super is the Windows/logo key. Keysyms are modifier-independent in this tree.

| Shortcut | Action |
|---|---|
| Super + Return / D | Launch foot / fuzzel |
| Super + J / K | Focus next / previous visible window |
| Super + Shift + J / K | Move tiled window next / previous, wrapping at ends |
| Super + H / L | Shrink / grow master by 5%, bounded to 10–90% |
| Super + [ / ] | Decrease / increase master count (minimum zero) |
| Super + Shift + Return | Promote tiled window to master / swap master |
| Super + Space | Cycle tile → floating → monocle → tile |
| Super + T / V / M | Select tile / floating / monocle directly |
| Super + Shift + Space | Toggle focused window floating |
| Super + F | Toggle true fullscreen (covers the bar; no borders) |
| Super + 1–9 | View tag |
| Super + Shift + 1–9 | Move focused window to tag |
| Super + Ctrl + 1–9 | Toggle tag in current view |
| Super + Ctrl + Shift + 1–9 | Toggle focused window's membership in tag |
| Super + Tab | Previous tag view |
| Super + 0 / Shift + 0 | View all tags / put window on all tags |
| Super + Shift + Q | Close window |
| Super + Shift + Escape | End session (applications may lose unsaved work) |
| Ctrl + Alt + F1–F12 | Switch VT |
| Ctrl + Alt + Backspace | Upstream emergency end-session binding |

Original monitor bindings are retained: Super + comma/period focuses the
left/right monitor; add Shift to send a window. No additional monitor controls
or monitor-management dependencies were added.

Stack movement skips floating, hidden, fullscreen and other-monitor windows,
preserves keyboard focus, and does nothing in the floating layout. Monocle
fills the usable area and keeps the bar; true fullscreen fills the output.

| Mouse | Action |
|---|---|
| Click window | Focus and raise |
| Super + left drag | Move / float window |
| Super + right drag | Resize / float window from bottom-right corner |
| Super + middle click | Toggle floating |

Client-side titlebar move/resize requests remain an upstream limitation; use
Super-drag. Resize warps the pointer to the bottom-right, as upstream does.

## Display: 4K / 240 Hz / VRR

The catch-all `monrules` entry avoids connector names, uses scale **1.5**
(2560×1440 logical at 3840×2160), and requests the **highest advertised refresh
at 3840×2160**. 239.97 Hz is accepted naturally; refresh is not compared to an
exact 240000 value. This also chooses higher rates if the monitor offers them.
If that resolution is absent, the backend's preferred mode is used. Virtual
outputs with no advertised modes retain their backend defaults.

VRR is attempted when wlroots reports support. If initialization fails, dwl
retries with VRR off, then the preferred mode if different. Failures go to
stderr. If every attempt fails, the output remains unavailable; there is no
invented modeline. This is a startup/hotplug policy, not a guarantee that every
advertised mode will work. Output-management clients can override it afterwards.

Change `monrules` width/height to `0, 0` for the backend preferred mode; change
its last field to `0` to disable startup VRR. Set scale to `1.0f` for native
logical pixels or `2.0f` for integer scaling. Fractional scaling can soften
legacy XWayland clients; compare 1.5 and 2 on your actual monitor. Native Wayland
fractional-scale/viewport support is preserved. No global toolkit scale or
renderer environment variables are forced.

Optional `wlr-randr` can inspect actual modes/scale/adaptive sync and change
these at runtime through `zwlr_output_manager_v1`. Use the connector reported
by that tool, e.g. `wlr-randr --output <name> --mode 3840x2160@240Hz --scale 1.5`;
use the exact advertised rate if different. `--adaptive-sync enabled` or
`disabled` controls VRR. These tools are optional, not build dependencies.

## Applications and status bar

Optional defaults: **foot**, **fuzzel**, **somebar**, **mako**. Only terminal and
launcher are compiled into dwl; `DWL_BAR` and `DWL_NOTIFY` select executables
(no arguments; use a wrapper if needed). Empty values disable each service.
VARBS should provision them and a font; none are installed by this project.

Run from a logind-managed VT with a valid `XDG_RUNTIME_DIR`, for example:

```sh
dbus-run-session ./dwl -s "exec \"$PWD/scripts/dwl-startup\""
```

The startup script forwards dwl's status stdin to the **original stdin-driven
somebar** and starts mako if available. It supervises its own child processes,
stops them at session end, and drains status if the bar is absent or exits.
It does not import user-service environments, configure portals, or install a
session entry. The upstream `dwl.desktop` still starts bare dwl; VARBS can later
install a session wrapper with an absolute script path. Do not also start mako
through this script if another session service already manages it.

Layer-shell and xdg-output support are already present. `printstatus()` sends
monitor name plus title/appid, selected monitor, layout, fullscreen/floating,
and four tag masks: **occupied, viewed, focused-client, urgent**. The original
somebar parses tags, layout and title from this stream without a dwl patch.
Configure its nine tag labels and colors separately in `src/config.hpp`.
Somebar build dependencies are C++, Meson/Ninja, Wayland, Cairo and Pango;
these stay outside dwl.

**Clickable tags are not enabled in this iteration.** Original somebar's
`contrib/ipc.patch` requires the matching dwl `wayland-ipc` extension; its patched
version stops consuming stdin. Other somebar forks may use different IPC. Do
not apply an old patch blindly to this wlroots-0.20 tree or run an IPC-only bar
with the supplied stdin startup script. Pin a bar revision and port/test both
sides together in a later iteration. Reference inspected:
[somebar README and IPC requirements](https://github.com/xihh87/somebar/blob/master/README.md)
and its `src/main.cpp` stdin parser. No IPC extension was added here.

## NVIDIA and validation

Use the Arch driver packages appropriate to the installed kernel and RTX 4090,
with DRM/KMS active (`cat /sys/module/nvidia_drm/parameters/modeset` should say
`Y`). Use matching current NVIDIA userspace/kernel components and a modern
Xwayland. This tree already creates `linux-drm-syncobj-v1` when both renderer
and backend expose timeline support. That conditional path is preserved;
compilation does not establish that the NVIDIA driver exposes or uses it.
No `WLR_NO_HARDWARE_CURSORS`, GBM vendor override, EGLStreams workaround,
forced software renderer, tearing patch or NVIDIA-specific bypass was added.

This environment has no physical NVIDIA GPU/display or Xwayland executable.
See the recorded build/test results below; still verify on hardware:

1. Confirm actual 3840×2160 mode and ~240 Hz using `wlr-randr`, monitor OSD,
   and logs; verify driver, cable bandwidth, monitor settings and DSC support
   if 240 Hz is missing. Test suspend/DPMS/reconnect and fallback behavior.
2. Check `wayland-info` for `wp_linux_drm_syncobj_manager_v1`, then test native
   Wayland and XWayland/Steam/Proton games for flicker, corruption and latency.
   Check fullscreen geometry, input capture, alt-tab and returning to tiling.
3. Test VRR enabled/disabled for flicker, frame pacing and monitor OSD behavior;
   compare fixed 240 Hz. A protocol global alone is not proof of correct sync.
4. Try scaling 1.5 and 2, foot/fuzzel/mako, browser popups, dialogs, tag changes,
   stack wraparound and Super mouse grabs. Check bar reservation/fullscreen.
5. Confirm original somebar shows active/occupied/urgent tags, layout and title.
   Later: pin and port clickable-tag IPC, evaluate small gaps, and add a VARBS
   session wrapper/portals/lock/idle policy as separate, tested components.

`make check` exercises actual functions with synthetic mode/client lists,
checks expanded keybinding collisions, and tests layout cycling and stack
movement guards. Disabled test outputs deliberately avoid rendering; it cannot
verify pixels, input delivery, real clients, or DRM commit rejection paths.

### Recorded checks (2026-11-04)

- Built with GCC 14.2 and isolated wlroots **0.20.2**, Wayland **1.26.0**,
  wayland-protocols **1.49**; default warning flags produced no warnings.
- `make check` passed with XWayland enabled and disabled. The latter is a
  separate clean build; no X11 client execution was possible here.
- `python3 tests/startup.py` (optional Python 3 check) passed all four cases:
  status forwarding, missing bar, exited bar, and SIGTERM cleanup of children.
- `sh -n scripts/dwl-startup` and `git diff --check` passed.
- Headless/Pixman dwl served a `wayland-info` client on a 1280×720 virtual
  output and shut down cleanly. Physical DRM modes, rejection/retry paths,
  GPU rendering, actual somebar/mako and interactive windows were not tested.
- GCC `-O2 -fanalyzer` completed with four inherited diagnostics: one possible
  NULL `old_l` dereference in `focusclient`, and three `strncpy` truncation
  warnings for layout symbols. The same diagnostics reproduce in the original
  tree with XWayland enabled. No new analyzer diagnostics appeared. The focus
  warning concerns invariants across layer-surface lookup; runtime regression
  testing of layer-shell focus/locking remains worthwhile. Layout symbols in
  these defaults fit the existing 16-byte buffer.

An independent read-only review found no remaining blocking issues after the
startup stdin correction. No compositor installation or system configuration
changes were performed.
