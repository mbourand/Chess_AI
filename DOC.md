## Magic number generation algorithm
https://www.chessprogramming.org/index.php?title=Looking_for_Magics

## Blockers board (= relevant masks in the code)
https://stackoverflow.com/questions/30680559/how-to-find-magic-bitboards


## Move binary format
```
3 bits
Old piece before promotion

4 bits
Old castling rights

8 bits
Old en passant square

6 bits
Captured piece pos

1 bit
Captured piece color

3 bits
Captured piece type

6 bits
From

6 bits
To
```

## Legal move generation
https://peterellisjones.com/posts/generating-legal-chess-moves-efficiently/

## Perft test positions
https://www.chessprogramming.net/perfect-perft/
