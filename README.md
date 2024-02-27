# cwolfmap

[![Build Status](https://github.com/cxong/cwolfmap/workflows/Build/badge.svg)](https://github.com/cxong/cwolfmap/actions)

Wolf 3D map and data reader in C

This project implements a C library that reads Wolf 3D maps (GameMaps format): http://www.shikadi.net/moddingwiki/GameMaps_Format
Plus other data like music and sound effects. Example programs are available for extracting/playing music/sounds, printing maps to console.

Use this library as a basis for your Wolf3D-engine games and mods that use original game data.

## Features

- Read map data, levels, planes
- Extract and play adlib music (WLF/IMF files)
- Play adlib sound effects
- Extract and play digital sound (RAW/WAV)

### Supported Games

- Blake Stone: Aliens of Gold (shareware, registered)
- Spear of Destiny
- Spear of Destiny mission packs
- Super 3D Noah's Ark
- Wolfenstein 3D (shareware, registered)

![screenshot](https://github.com/cxong/cwolfmap/blob/master/screenshot.png)
