# Recording and streaming Lagoon

On Windows, select **Display → Window mode → Borderless fullscreen**, Apply,
and restart Lagoon. This is the option previously called Desktop fullscreen.

In OBS Studio:

1. Launch the actual game, not just Companion.
2. Add a **Game Capture** source.
3. Set **Mode → Capture specific window**.
4. Choose **[LagoonSNESRecomp.exe]: Lagoon**. Do not select the Companion launcher.
5. Check moving gameplay in the preview before recording a long take.

Prefer explicit-window selection over **Capture any fullscreen application**,
particularly if other games are open. Local November 12, 2026 tests recorded
moving 1920x1080 borderless gameplay through explicit OpenGL Game Capture.
Automatic fullscreen selection also worked after the hook settled. Initial
hook/reacquisition tests produced transient black or repeated frames; they do
not establish the cause of the previously reported persistent freeze. No
renderer or driver-wide capture fix is claimed by this launcher update.

If a source still freezes, deactivate/reactivate it after the game opens and
reselect Lagoon. As an alternative, try **Window Capture** with the modern
**Windows Graphics Capture / Windows 10 (1903 and up)** method. Legacy BitBlt
is unsuitable for many GPU-rendered applications. Keep OBS and your graphics
driver current. Minimized windows may stop updating.

Game Capture directly supports OpenGL, which Lagoon's Enhanced presentation uses.
See the [OBS Game Capture guide](https://obsproject.com/kb/game-capture-source)
and [Window Capture guide](https://obsproject.com/kb/window-capture-sources).

You should not need to expose your entire desktop to record Lagoon. For a bug
report, include the game release, fullscreen mode, OBS version, capture mode and
an OBS log from the failing session. Do not include your ROM.
