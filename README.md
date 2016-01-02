AI for the [2048 game](http://gabrielecirulli.github.io/2048/). This uses *expectimax optimization*, along with a highly-efficient bitboard representation to search. Heuristics used include bonuses for empty squares and bonuses for placing large values near edges and corners.  

## Building

### Unix/Linux/OS X

Execute

    ./configure
    make

Note that you don't do `make install`; this program is meant to be run from this directory.
