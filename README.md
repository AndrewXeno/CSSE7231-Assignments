# CSSE7231-Assignments
Course assignment code for CSSE7231 (Computer Systems Principles and Programming).

## Assignment 1

An ANSI-C program (called `boxes`) which allows the user to play a game.

Compile with command: `gcc -Wall -ansi -pedantic ass1.c -o boxes`

## Assignment 3

Two programs to automatically play a card game (“`clubs`”). 

- The first program (`clubber`) will listen on its stdin for information about the game and give its moves to stdout.The program will send information about the state of the game to stderr.
- The second program (`clubhub`) will start a number of processes (eg `clubber`) to be players and communicate with them via pipes. The hub will be responsible for running the game (sending information to players; processing their moves and determining the score).

Compile with command: `make`

## Assignment 4

An abstract simulation of a transportation network. It will require the use of pthreads, tcp networking and thread-safety.

- The simulation will consist of a number of “stations”..Each station may be connected to a number of other stations via network connections.Network messages representing “trains” will arrive via these network connections, pick up or deposit resouces and move on to the next station.

Compile with command: `make`