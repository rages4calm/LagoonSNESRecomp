# Optional X68000 soundtrack research

No released Lagoon MSU-1 patch/audio pack was verified on October 15, 2026.
The [Zeldix Lagoon topic](https://www.zeldix.net/t2387-lagoon) is a request from
October 27, 2021, specifically asking for X68000 music, with no replies.
Lagoon is absent from the current [MSU-1 master list](https://www.zeldix.net/t2684-alphabetical-list-every-snes-msu1-hack).
A cover image labeled MSU1 in a third-party emulator bundle is insufficient
evidence of a playable patch or mapped soundtrack pack.

Some actual listener comparisons favor X68000 while also praising SNES music:
[NeoGAF, post 153](https://www.neogaf.com/threads/what-are-some-sucky-games-with-good-music.1434190/page-4),
[FPGA discussion](https://www.reddit.com/r/fpgagaming/comments/vivxhn/),
[JRPG discussion](https://www.reddit.com/r/JRPG/comments/1deh6ai/first_impressions_of_lagoon_snes/),
and [Singing Mountain's listening episode](https://singingmountainpod.com/episodes/lagoon-sharp-x68000).
This is a small selection of opinions, not a community preference survey.

The owner supplied `Lagoon_(Sharp_X68000).zip`, a VGM package containing 40 VGZ
tracks, a playlist and notes. The notes credit Hideki Suzuki, package creator
GTheGuardian and optimizer Valley Bell. The files record YM2151 and OKIM6258
chip commands with native loop metadata. They are not MP3 files, and YouTube
ripping is unnecessary. See the [package information page](https://vgmrips.net/packs/pack/lagoon-sharp-x68000).

The optional port mode must preserve stock SNES audio by default and retain
sound effects when replacing BGM. Track mapping and music-only suppression
need source/runtime proof. Conversion tools and imported audio remain private;
the public source contains no soundtrack files. An offline VGM renderer must
support both of the original sound chips and preserve sample-accurate looping.
