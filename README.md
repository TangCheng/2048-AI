AI for the [2048 game](http://gabrielecirulli.github.io/2048/). This uses *expectimax optimization*, along with a highly-efficient bitboard representation to search. Heuristics used include bonuses for empty squares and bonuses for placing large values near edges and corners.  

## Building

### Unix/Linux/OS X

You should install build-essential and autoconf tools first.

Execute

    autoreconf
    mkdir build
    cd build
    ../configure
    make
    ./src/2048

Note that you don't do `make install`; this program is meant to be run from this directory.
