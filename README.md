# CHIP-8

A CHIP-8 and S-CHIP emulator written in C99. The core has no I/O and
is [freestanding](https://port70.net/~nsz/c/c99/n1256.html#4p6).

**Supported platforms**: "modern" CHIP-8, CHIP-48, S-CHIP 1.0 and
S-CHIP 1.1.

## Compilation and usage

To compile the SDL implementation, make sure that SDL2 is installed,
and then run `make`.

You can launch the program with the following command:

```
./chip8 <scale-factor> <emulator-frequency> <ROM>
```

For instance:

```
./chip8 10 1200 ./ROMs/games/ALIEN
```

Common emulator frequencies: `540`, `840` and `1200` (for S-CHIP
games)

## References

-   [Guide to making a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator)
-   [Cowgod's Chip-8 Technical Reference v1.0](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
-   [SUPER-CHIP v1.1](http://devernay.free.fr/hacks/chip8/schip.txt)
-   [CHIP-8 database](https://github.com/chip-8/chip-8-database)
-   [Mastering SuperChip](https://johnearnest.github.io/Octo/docs/SuperChip.html)
-   [HP48-Superchip](https://github.com/Chromatophore/HP48-Superchip)
-   [CHIP-8 test suite](https://github.com/Timendus/chip8-test-suite)
-   [Chip-8 Program Pack](https://github.com/kripod/chip8-roms)
-   [Octo](https://github.com/JohnEarnest/Octo)
-   [r/EmuDev](https://www.reddit.com/r/EmuDev)

## License

This source code is licensed under the GNU General Public License
v3.0.
