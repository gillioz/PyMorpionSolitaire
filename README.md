# PyMorpionSolitaire

A fast C++ engine for the pen-and-paper game [Morpion Solitaire](https://en.wikipedia.org/wiki/Morpion_solitaire), with Python bindings via pybind11.

## The game

Morpion Solitaire is a single-player game played on a grid. You start from a cross-shaped pattern of dots and take turns placing a new dot such that it completes a line of exactly *n* dots (including itself). The goal is to place as many dots as possible before no moves remain.

The package supports four variants of the game:

| Class     | Line length | Rule     | Description                                  |
|-----------|-------------|----------|----------------------------------------------|
| `Game5T`  | 5           | Touching | Lines may share endpoints (default)          |
| `Game5D`  | 5           | Disjoint | Lines must not share any points              |
| `Game4T`  | 4           | Touching | Shorter lines, touching variant              |
| `Game4D`  | 4           | Disjoint | Shorter lines, disjoint variant              |

## Installation

```bash
pip install morpion-solitaire
```

The package compiles from source on install, so you need a C++17 compiler (`g++` or `clang++`) and CMake ≥ 3.25. jsoncpp and pybind11 are fetched automatically during the build.

For visualisation (the `.print()` method) you also need matplotlib:

```bash
pip install "morpion-solitaire[viz]"
```

### Installing from source

```bash
git clone https://github.com/gillioz/PyMorpionSolitaire.git
cd PyMorpionSolitaire
pip install -e .
```

## Quick start

```python
from pymorpionsolitaire import Game5T

# Create a new game (standard cross-shaped starting position)
game = Game5T()

print(game.getScore())          # 0 — no moves played yet
print(game.getNumberOfMoves())  # 28 — possible moves at the start

# Play a random move, then play randomly to the end
game.playAtRandom()
game.playAtRandom()  # play n moves: game.playAtRandom(n)

game.playAtRandom()  # with no argument, plays until no moves remain
print(game.getScore())          # typically 50–70 for a random game
print(game.getNumberOfMoves())  # 0 — game is over
```

## Examples

Two Jupyter notebooks are provided as examples:
- [01_basic_usage](./examples/01_basic_usage.ipynb) shows how the module works.
- [02_statistics](./examples/02_statistics.ipynb) explores the outcome of Monte-Carlo simulations of the game, both using a naive approach and the more advanced [Nested Monte-Carlo search method of Tristan Cazenave](https://www.ijcai.org/Proceedings/09/Papers/083.pdf).

## Playing strategies

### Random play

```python
game = Game5T()
game.playAtRandom(1)         # play one random move
game.playAtRandom(10)        # play 10 random moves
game.playAtRandom()          # play to the end (no argument = until stuck)
```

### Nested Monte Carlo Search

[Nested Monte Carlo Search (NMCS)](https://en.wikipedia.org/wiki/Nested_Monte_Carlo_search) is a planning algorithm that generally reaches much higher scores than random play. Higher levels are stronger but slower.

```python
game = Game5T()
game.playNestedMC(1)         # level-1 NMCS (~seconds)
print(game.getScore())       # typically > 100

game.restart()
game.playFastNestedMC(2)     # faster level-2 variant (~minutes)
print(game.getScore())       # typically > 120
```

### Playing by move index

The engine tracks all currently legal moves. You can inspect how many there are and play a specific one by index:

```python
game = Game5T()
print(game.getNumberOfMoves())  # 28

game.playByIndex(0)             # play the first available move
game.playByIndex(3)             # play the fourth available move
```

## Undoing moves

```python
game = Game5T()
game.playAtRandom(10)
print(game.getScore())   # 10

game.undo()              # undo the last move
game.undo(3)             # undo the last 3 moves
print(game.getScore())   # 6

game.undoAll()           # undo everything (same as restart)
game.restart()           # equivalent to undoAll()
print(game.getScore())   # 0
```

You can also revert to a specific score:

```python
game.playAtRandom()
game.revertToScore(20)          # undo moves until score == 20
game.revertToRandomScore()      # revert to a uniformly random score along the history
```

## Deleting a candidate move

You can permanently remove a move from the set of legal options at the current position — useful for guided search:

```python
game = Game5T()
print(game.getNumberOfMoves())    # 28
game.deleteMoveByIndex(4)
print(game.getNumberOfMoves())    # 27
```

## Exploring and estimating depth

`exploreDepth(cutoff)` performs a systematic depth-first search up to `cutoff` nodes visited and returns the deepest score found:

```python
game = Game5T()
best = game.exploreDepth(10)    # visit up to 10 nodes
print(best)                     # deepest score found so far
```

`estimateDepth()` estimates the average depth reachable from the current position by sampling random playouts:

```python
game = Game5T()
print(game.estimateDepth())     # typical score from here, via random play
```

## Saving and loading games

Games are serialised to JSON:

```python
import json

game = Game5T()
game.playAtRandom()

# Export to a JSON string
data = game.exportJSON()

# Save to a file
game.save("my_game.json")

# Load from a file
restored = Game5T.load("my_game.json")
print(restored.getScore() == game.getScore())   # True
```

## Getting the board as a NumPy array

The board state is available as a 2D boolean array, and the set of candidate moves as a 3D array:

```python
import numpy as np
from pymorpionsolitaire import Game5T

game = Game5T()
game.playAtRandom(20)

board = game.image()              # shape: (IMAGESIZE, IMAGESIZE), dtype bool
moves = game.getMoveImages()      # shape: (n_moves, IMAGESIZE, IMAGESIZE), dtype bool

print(board.shape)
print(moves.shape)
```

These are particularly useful as inputs to neural networks or other machine-learning models.

## Visualisation

Requires matplotlib (`pip install "morpion-solitaire[viz]"`).

```python
game = Game5T()
game.playAtRandom(30)

game.print()               # display the current board
game.printMove(0)          # display the board with move 0 highlighted in blue
```

## Starting position variants

The default starting position is a cross (`'c'`). A pipe-shaped starting position is also available:

```python
from pymorpionsolitaire import Game5T

cross_game = Game5T('c')   # default
pipe_game  = Game5T('p')
empty_game = Game5T('e')   # empty board, no initial dots
```

## Performance

The engine is designed for high-throughput use in research and machine learning pipelines. On a modern laptop, random games can be played at tens of thousands of games per second:

```python
import time
from pymorpionsolitaire import Game5T

game = Game5T()
n = 10_000
t0 = time.perf_counter()
for _ in range(n):
    game.playAtRandom()
    game.restart()
elapsed = time.perf_counter() - t0
print(f"{n / elapsed:.0f} random games/second")
```

## Licence

MIT

## Development and running tests

The C++ tests require CMake and are built separately from the Python package. From the repo root:

```bash
cmake -B build -DBUILD_DEV_TOOLS=ON
cmake --build build
```

This fetches [Catch2](https://github.com/catchorg/Catch2) automatically and compiles `GameTests` and `GraphGameTests`. To run all tests:

```bash
ctest --test-dir build --output-on-failure
```

### Selective test runs
 
The tests are tagged, which allows running subsets. You can filter through `ctest` or by calling the executables directly.
 
Run only the core game tests:
```bash
./build/GameTests
```
 
Run only the graph game tests, excluding slow ones:
```bash
./build/GraphGameTests ~[slow]
```
 
Run the slow NMCS algorithm tests explicitly (may take several minutes):
```bash
./build/GraphGameTests [slow]
```
 
Run the performance benchmarks:
```bash
./build/GraphGameTests [benchmark]
```
 
List all available tests without running them:
```bash
./build/GameTests --list-tests
./build/GraphGameTests --list-tests
```
 