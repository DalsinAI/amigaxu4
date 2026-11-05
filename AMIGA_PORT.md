xu4 (Ultima IV) for AmigaOS 3.x
================================

A port of xu4, the engine that plays Ultima IV: Quest of the Avatar
(https://github.com/xu4-engine/u4, tag pre-alleg, commit
62f3d9096409b5c9a5337d45cb1d3eaa9827276c: the last version on SDL 1.2
and libxml2, before upstream moved to Allegro, OpenGL and Boron), for
AmigaOS 3.2 on a 68040 or 68060 with an FPU and an RTG screen, on
GFOpenGPU's SDL 1.2 (sdl12-compat on SDL 2).

Status: builds, starts and stops cleanly in the lab; not yet played,
for want of the Ultima IV files (see "Testing" at the end).


What you need
-------------

- A 68040 or 68060 with an FPU (not an LC040 or EC040), AmigaOS 3.2, an
  RTG card (or an AmiCradle instance).
- GFOpenGPU with LIBS:GFOpenGPU/SDL2.module.
- About 12 MB of disk, and about 16 MB of free memory.
- **The PC version of Ultima IV.** It is not in this package (see below).


Ultima IV's own files
---------------------

xu4 plays the original game's maps, tiles, talk and pictures, so it needs
the PC (DOS) Ultima IV. Electronic Arts gives it away free, but does not
let others pass it on, so get it from where EA offers it:

1. Download Ultima IV: Quest of the Avatar from GOG.com (free; it needs a
   GOG account).
2. Copy the game's files (AVATAR.EXE, WORLD.MAP, the .TLK, .ULT, .CON,
   .DNG, .EGA and .PIC files and the rest) into the xu4 drawer, or into a
   drawer named ultima4 inside it. A zip of them named ultima4.zip in the
   xu4 drawer works too, as does u4upgrad.zip (the VGA upgrade) beside it.

Without them xu4 stops at once with a requester that says so.


How to play
-----------

Double-click xu4 in its drawer, or from a Shell:

    cd Work:Games/xu4
    xu4 [-f] [-s 1..3] [--filter point|2xBi|2xSaI|Scale2x] [-q] [-i] [-v]

-f opens a screen of its own rather than a window on the Workbench; -s
the scale (2, 640x400, is the default); -q no sound; -i skips the intro
and loads the saved game; -v prints what it does.

Ultima IV's keys are the original's: a letter per command (A)ttack,
(B)oard, (C)ast, (E)nter, (G)et, (T)alk, (Z)tats, ... the cursor keys walk,
Space passes a turn, Esc cancels. Q saves the game (Quit & Save, where
the game lets you save). Alt+Q goes back to the title menu, Alt+S turns
the sound effects on and off, Alt+V shows the version.

- Right Amiga+Q (or Right Amiga+X, or upstream's Alt+X) quits at once;
  so does the window's close gadget. Save with Q first.
- The title's menu: (J)ourney Onward loads the saved game, (I)nitiate a
  new game, (C)onfigure the settings, (A)bout.

The settings (xu4rc) and the saved game (party.sav, monsters.sav,
outmonst.sav) are kept in the drawer's User drawer; the installer leaves
User and ultima4 alone when it updates the drawer.


Building
--------

The os32-gcc16-open6 stove (GFOpenAmigaGCC 16.2.0-open6 with the
GFOpenGPU 0.8 kit), and the network the first time (for the three
libraries' source tarballs):

    make -f amiga/Makefile.amigaos -j4 dist STOVE=/path/to/os32-gcc16-open6

STOVE defaults to ~/AmiCradle/stoves/os32-gcc16-open6. The first build
runs amiga/deps/build-deps.sh, which fetches zlib 1.3.1, libpng 1.6.44 and
libxml2 2.9.14 into amiga/deps/cache (SHA-256 checked) and builds them
static into build/amigaos/deps. Output: build/amigaos/xu4; 'dist' makes
dist/xu4/ (xu4, conf/, graphics/, sound/, User/, ReadMe, ReadMe-xu4,
COPYING.txt, Licences.txt, with GlowIcons), dist/xu4.info and
dist/Install-xu4 with its icon. The icons are made from
amiga/icons/icons.json and the installer from amiga/installer.json with
AmiCradle's scripts/port_icons.py and port_installer.py.


What was changed
----------------

- amiga/Makefile.amigaos: upstream's source list (src/Makefile.common),
  with sound_sdl.cpp replaced; links the static zlib, libpng and libxml2
  and the stove's libSDL.a (SDL 1.2 on SDL 2) and libpthread.a (libstdc++
  asks for it).
- amiga/deps/build-deps.sh: the three libraries; libxml2 with only its
  tree, parser, XPath, XInclude and DTD validation (what xu4 uses).
- src/u4file.cpp: the files are looked for in PROGDIR: and
  PROGDIR:ultima4 (upstream's "%s/%s/%s" makes "/conf/x", the parent
  drawer on AmigaOS, and its ../ paths are left out).
- src/settings.cpp: the settings and saves in PROGDIR:User/.
- amiga/sound_amiga.cpp: the kit has no SDL_mixer 1.2. The sound effects
  (sound/*.ogg) are decoded the first time each plays, by stb_vorbis
  (amiga/stb_vorbis.c, v1.22), down to 22050 Hz mono, and mixed on one
  channel into SDL's audio as upstream's one effects channel. No music.
- src/error.cpp: errors in an Intuition requester as well as on the Shell
  (amiga/amiga_sys.c); src/u4.cpp says where Ultima IV's files go.
- amiga/amiga_sys.c: closes any empty "SDL" screen left in front of the
  Workbench at exit (from the Abbaye and Seven Kingdoms ports).
- amiga/stack.c: 512 KB of stack, and a $VER string.
- src/tile.h: includes stdint.h for uint8_t.

Big-endian: xu4 was played on PowerPC Macs. The saved game is written and
read a byte at a time (src/io.cpp); the pictures' masks follow
SDL_BYTEORDER (src/image_sdl.cpp); Ultima IV's own files are read as
bytes. No change was needed.

Frame pacing: xu4 is driven by SDL timers (four game cycles a second, 24
animation frames a second) and waits in SDL_WaitEvent, so it does not
spin.


Known limits
------------

- No music. Upstream plays MIDI and Impulse Tracker files through
  SDL_mixer, which the kit does not have; the music settings are kept.
- The Ultima IV files are not in the package (see above).
- The FM Towns pictures and the music files are not in the package.
- Scale2x (the default filter) is drawn by the 68k; on a slow machine
  --filter point is cheaper.


Licence
-------

xu4 is under the GNU GPL, version 2 or later (COPYING.txt); the port's
files too. What else is in the package, and why Ultima IV is not, is in
Licences.txt.


Testing
-------

Tested in an AmiCradle lab (a copy of Instance-32: AmigaOS 3.2, 68040,
RTG; the lab is muted, so no sound could be heard), 5 November 2026,
without Ultima IV's files, which the port does not have:

- With no Ultima IV files, xu4 stops at once with the requester saying
  where they go, and quits cleanly.
- With a placeholder AVATAR.EXE in ultima4/ (to get past that check),
  xu4 parses conf/config.xml and its XInclude files with the static
  libxml2, opens its 640x400 "Ultima IV" window on the Workbench (in
  video RAM), and stops with upstream's "Unable to load the charset data
  file" requester, as it must without the game; the window closes and no
  screen is left behind.
- Not yet seen: the title, play, saving and loading, sound, Right
  Amiga+Q in play. Those need the real game files in the drawer.
