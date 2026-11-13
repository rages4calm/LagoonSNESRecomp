# Recording and streaming Lagoon

On Windows, select **Display → Window mode → Borderless fullscreen**, Apply,
and restart Lagoon. This is the option previously called Desktop fullscreen.
Select **Graphics backend → Auto (D3D11)** or **D3D11**. Enhanced original pixels
uses a D3D11 GPU shader on Windows; OpenGL remains an explicit alternative.

In OBS Studio:

1. Launch the actual game, not just Companion.
2. Add a **Game Capture** source.
3. Set **Mode → Capture specific window**.
4. Choose **[LagoonSNESRecomp.exe]: Lagoon**. Do not select the Companion launcher.
5. Check moving gameplay in the preview before recording a long take.

Prefer explicit-window selection over **Capture any fullscreen application**,
particularly if other games are open. Local November 12, 2026 D3D11 tests
recorded moving 1920×1080 borderless gameplay while repeatedly switching focus
between Lagoon and OBS32.2.2. Both specific-window Game Capture and modern
Windows Graphics Capture continued updating. Decoded recordings had no black
frames after hook acquisition and no low-change run longer than0.067seconds
in the checked focus intervals. This checks one PC, not every GPU/driver.

If a source still freezes, deactivate/reactivate it after the game opens and
reselect Lagoon. As an alternative, try **Window Capture** with the modern
**Windows Graphics Capture / Windows 10 (1903 and up)** method. Legacy BitBlt
is unsuitable for many GPU-rendered applications. Keep OBS and your graphics
driver current. Minimized windows may stop updating.

Game Capture supports Direct3D11 and OpenGL. The Windows default uses a
windowed DXGI flip swapchain rather than an OpenGL fullscreen presentation path.
See the [OBS Game Capture guide](https://obsproject.com/kb/game-capture-source)
and [Window Capture guide](https://obsproject.com/kb/window-capture-sources).

You should not need to expose your entire desktop to record Lagoon. For a bug
report, include the game release, fullscreen mode, OBS version, capture mode and
an OBS log from the failing session. Do not include your ROM.

**World zoom out** is separate from window scale and OBS resolution. Enable
widescreen and choose1x,1.25x or1.5x in Companion's Display page, then Apply
and restart. Larger extents show more world in both directions; menus and HUD
retain readable source-pixel sizes. Original cutscenes and unqualified visual
states retain their authored composition.

Pixel proportions is independent: **Square pixels + widescreen +1.5x zoom out**
works together. Original CRT proportions remains selectable for television-style
proportions. Neither choice changes the OBS capture source or recording resolution.
