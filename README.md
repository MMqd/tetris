# Tetris

<div style="text-align:center">
   <img src="https://github.com/MMqd/tetris/blob/main/screenshot.png?raw=true" width="50%" alt="center image" />
</div>

A simple linux terminal Tetris game, with a look inspired by bastet.

This was my first terminal program on linux. I found many basic terminal tasks difficult while working on it, and made this basic game/demo to demonstrate how to interact with the terminal on linux. It also serves as a reference on how to interact with the terminal without **ncurses**.

It contains examples of how to:
* overwrite the screen without flicker
* receive input
* interpret arrow keys
* handle window resizing and termination
* get the user's path
* pass flags to the program

**Note:** that the screen is overwritten and not cleared, to clear a character instead of ovewriting the character needs to be overwritten with a space.
