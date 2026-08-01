# Full Duel Engine and UI Integration

## Summary

Import the complete embedded duel implementation from `shandalar-decomp` commit
`4068e1b`, preserving original source and behavior while simplifying obsolete
host boundaries.

Use a duel-scoped `setjmp`/`longjmp` to replace `ExitThread`. This matches its
immediate deep-stack termination and follows Shandalar's existing save-loading
and session-restart convention.

Remove Facemaker's unreachable palette-inspector popup while retaining all
palette behavior used by normal rendering.

Coin-flip animation is low priority. The initial integration will provide a
safe MCI-window stub so coin dialogs remain functional; actual Statwin-backed
WebP/WAV playback is deferred.

## Implementation Order

1. Establish the shared UI foundation:
   - Extract an ImGui-independent SDL compositor from the Deck bridge.
   - Move Deck-only drag and interaction behavior behind a Deck-owned layer.
   - Convert Facemaker to the compositor and remove its ImGui dependency and
     unreachable palette inspector.
2. Import the original Duel code and replace the runtime stubs.
3. Wire synchronous Duel execution and its duel-scoped non-local exit.
4. Fill out the Win32 compatibility behavior, dialogs, menus, and controls
   reached by Duel.
5. Complete lifecycle, outcome, and repeated-duel testing.
6. Add coin-flip playback later; an inert compatibility window is sufficient
   during the initial Duel integration.

The first foundation step is implemented. Facemaker now renders directly
through the shared SDL compositor, while the generic ImGui bridge and
Deck-specific interaction code are independently consumable by the future
Duel host.

## Implementation Changes

### Original source and stub replacement

- Copy the missing `magic` sources into the SDL repository, excluding the
  standalone Magic entry point and frontend-only resources.
- Record the decomp baseline and synchronized source list.
- Preserve imported filenames, structures, functions, and message-driven
  behavior. Merge existing SDL fixes in shared files.
- Compile all real card handlers and remove `dummycards.c`.
- Move valid host glue out of `magic_runtime_stubs.c`, replace every stub with
  its original implementation, and delete the stub file.
- Audit shared symbols so each global and function has exactly one owner.
- Remove the early `return 1` from `RunDuelEngine` and retain its portable
  direct invocation.

### Synchronous duel execution and non-local exit

- Replace `CreateThread(start_duel_thread, ...)` with a normal synchronous
  duel-runner call.
- Remove the duel thread handle, priority changes, exit-code polling, waits,
  and handle cleanup.
- Do not implement fake thread APIs for Duel.
- Add a duel-local jump context containing:
  - A `jmp_buf` valid only while the synchronous runner is active.
  - A separate result field because zero is a valid result.
  - An active-context guard for diagnostics.
- Establish the jump point immediately around `play_duel`.
- Change `exit_duel_thread(result)` to store the result and jump to the runner.
- Replace the direct `ExitThread` in target cancellation with the same helper.
- Post the original `0x401` completion message exactly once after normal return
  or non-local exit.
- Do not depend on modified automatic variables after `longjmp`.
- Remove thread-status checks from modal selection loops.
- Pump SDL events, rendering, timers, and audio from empty nested message loops
  so native and Asyncify/Web remain responsive.
- Keep the adventure save/world jump buffers independent from the duel jump
  context.

### SDL compositor, Facemaker, and ImGui

- Extract HDC upload, window-tree traversal, z-order composition, coordinate
  conversion, hit-testing, and mouse forwarding into an ImGui-independent SDL
  compositor.
- Route events through ImGui only when an overlay control, modal, or popup owns
  the interaction.
- Move deck-specific behavior out of the generic compositor.
- Convert Facemaker to the SDL compositor and remove its ImGui dependency.
- Remove Facemaker's `WM_COMMAND 0xffff` palette-inspector branch, popup state,
  `ShowPaletteClass` procedure, registration, and creation helpers.
- Retain normal palette creation, realization, animation, and indexed-color
  rendering.
- Leave adventure Shandalar on its current direct framebuffer path.
- Use the compositor for original custom Duel windows and ImGui only for
  standard controls, dialogs, popup menus, and up-down widgets.

### Duel resources and compatibility

- Add target-local compatibility headers so imported Win32 includes remain
  close to the decomp source.
- Complete required behavior for capture, focus, non-client and menu messages,
  window/class values, scrollbars, up-down controls, accelerators, key state,
  cursors, and z-order.
- Replace the singleton dialog factory with a resource-ID registry.
- Implement every embedded-duel dialog reached from `magic.rc`, including known
  IDs `220`, `222-225`, `227`, `233-235`, `239-240`, `243-244`, `246`, and
  `249`.
- Add recursive popup submenus, checked/enabled state changes, dismissal, and
  keyboard/mouse selection.
- Implement accelerator table `111`, focus traversal, default/cancel behavior,
  double-clicking, and wheel input.
- Add a 1024x768 duel lifecycle that restores adventure graphics, title, input,
  timers, and audio afterward.
- Enforce the original 32-bit assumptions: native remains x86 and Web remains
  32-bit.

### Legacy I/O and assets

- Route imported Magic file operations through `platform/legacy_io.h`.
- Rely on the existing Shandalar asset bundle and automatic blocking fetches.
- Do not add a separate duel asset manifest.
- Retain targeted smoke tests for legacy paths and unsupported access patterns.

### Deferred coin-flip playback

For the initial Duel integration:

- Implement `MCIWndCreateA` as an inert 320x240 compatibility child window,
  matching the original coin AVI dimensions.
- Accept palette, play, stop, and close messages without errors.
- Allow the original coin dialog timers and input-dismissal paths to operate
  normally.
- Do not block Duel integration on animation decoding or synchronized sidecar
  audio.

For the later media pass:

- Map the legacy AVI basenames to the existing bundled files in `Statwin_SDL`.
- Reuse Statwin's WebP decoder, frame timing, renderer, and WAV support through
  a thin non-blocking MCI adapter.
- Replace the inert child contents without changing the original duel dialog
  code.

## Test Plan

- Build native x86 and Emscripten Shandalar without unresolved symbols,
  duplicate globals, dummy cards, runtime stubs, or Duel thread emulation.
- Test normal completion and every non-local result: win, loss, draw, library
  exhaustion, alternate-loss cards, target cancellation, and window close.
- Verify each jump posts one completion message, skips deep-stack continuation,
  and leaves the next duel clean.
- Test returning from Duel followed by adventure save loading to ensure jump
  contexts do not interfere.
- Verify Facemaker builds without ImGui or palette-inspector symbols and
  matches its existing main-window smoke snapshot.
- Test compositor window hierarchy, z-order, hit-testing, capture, scaling, and
  input routing.
- Test dialogs, accelerators, recursive menus, up-down controls, and modal
  interaction.
- Test legacy I/O using bundled and first-access-downloaded files.
- For initial acceptance, verify the stubbed coin dialog opens at the expected
  size, does not hang, and dismisses through mouse, keyboard, and timeout paths.
- Defer animated coin playback and audio synchronization tests until the later
  media pass.
- Add a complete Duel smoke harness and manually cover native/Web encounters,
  targeting, phases, combat, stack resolution, AI, rewards, outcomes, and
  repeated duels.

## Assumptions

- Full playable single-player integration is required.
- Source fidelity takes precedence over modernization.
- `setjmp`/`longjmp` is the intended replacement for deep `ExitThread`
  termination.
- Facemaker's palette inspector is intentionally removed as unreachable debug
  functionality.
- Coin animation is explicitly non-blocking and low priority.
- Native x64 and multiplayer networking remain out of scope.
- Existing legacy I/O and asset-bundle behavior remains authoritative.
