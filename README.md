# 3D Alien maze

## Video:

[![Gameplay video by Révo](http://i.freegifmaker.me/1/6/1/7/5/4/1617541486915960.gif?1617541497)](https://www.youtube.com/watch?v=itz25823FkI)

## Known bugs

* Very rarely, the game will still produce unwinnable mazes;
* The build is failing to generate the standard SEGA ROM headers, meaning it may not run on real hardware.

## Tools/resources used

* [Sverx's devkitSMS][https://github.com/sverx/devkitSMS] for access to the Sega Master System hardware;
* [Sverx's PSGlib][https://github.com/sverx/PSGlib] for playing the sound effects;
* [Maxim Zhao's BMP2Tile][https://github.com/maxim-zhao/bmp2tile] for converting the graphics;
* [Furrtek's PSGTalk][https://github.com/furrtek/PSGTalk/tree/958fa824493bae87975684ab544e09bffd48d12b] for converting the sound effects; must be this version; the newer one does not work correctly;
* [Nmn's Monster for 3d fps based on sprites][https://opengameart.org/content/monster-for-3d-fps-based-on-sprites-harkubus] for the main monster;
* [qubodup's Heartbeat (Single sound)][https://opengameart.org/content/heartbeat-single-sound] for the heartbeat sound effect;
* [Michael Klier's Large Monster][https://opengameart.org/content/large-monster] for the death sound effect.
