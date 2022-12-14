Attack on Voxelburg
===================

![GBA build status](https://github.com/MutantStargoat/voxelburg/actions/workflows/build_gba.yml/badge.svg)

A Gameboy Advance game for the gbajam22.

![screenshot](http://nuclear.mutantstargoat.com/sw/games/voxelburg/img/gameshot1-2x.png)

Fly over a 3D terrain, find and destroy all enemy defense pods, before they
manage to shoot you down. Your final score depends on the difficulty level, time
it took to take out all the enemy pods, and damage you sustained.

You can download pre-compiled release versions of this game, which include the
Gameboy Advance ROM image, from the game page on itch.io:
https://nuclear.itch.io/attack-on-voxelburg
or the github repo releases page.

Controls
--------
  - D-Pad: turn left/right/up/down.
  - A: fly forward.
  - B: shoot.
  - L/R triggers: strafe left/right.
  - Start: exit to main menu.

License
-------
Copyright (C) 2022 John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software. Feel free to use, modify, and/or redistribute it
under the terms of the GNU General Public License v3, or at your option any
later version published by the Free Software Foundation. See COPYING for
details.

Build
-----
The data files are kept separately on a subversion repository. Grab them with:

    svn co svn://mutantstargoat.com/datadirs/voxelburg data

Alternatively grab a release tarball if available.

To build the game,
[setup the devkitarm environment](https://devkitpro.org/wiki/Getting_Started),
and type `make`. Only the basic cross-toolchain, libc and C runtime are required
from devkitarm. No other libraries are used.

The tools which are part of the build process might have more requirements:

  - **pngdump**: libpng and zlib.
