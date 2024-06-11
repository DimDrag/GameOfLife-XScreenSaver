# Game of life module for XScreenSaver
## About
This is my XScreenSaver module based on John Conway's Game of Life.

![GoL_clock](./misc/GoL_clock.gif)

For now you can configure:
- Simulation speed
- Switch clock mode on/off

Every GoL project for XScreenSaver that I found didn't work on my system (to be honest, I didn't try hard), so I decided to write my own. The visual part is very simple, but this acid green color and the clock looks pretty nice to me.

## How to add this to xscreensaver?

1. Install XScreenSaver manually or use ```sudo apt install xscreensaver``` command
3. Сopy the binary file "life" to "/usr/libexec/xscreensaver" (if you want to compile it yourself, see the instructions below)
4. Copy xml file to "/usr/share/xscreensaver/config"
5. Add "life --root" line to $HOME/.xscreensaver file after "programs:" line

## Installing from scratch

1. Download xscreensaver from https://www.jwz.org/xscreensaver/download.html
2. Put life.c into "hacks" directory and life.xml into "hacks/config"
3. Replace Makefile.in from "hacks" directory by the same file from this repository
4. Install the program by following the README file instructions (run these commands from the xscreensaver main directory)
```shell
./configure --prefix=/usr
        make
        sudo make install
        make clean
```
6. Add "life --root" to $HOME/.xscreensaver file after "programs:" line

**How it should look after installation:**
![GoL_xscreensaver](misc/GoL_xscreensaver.gif)