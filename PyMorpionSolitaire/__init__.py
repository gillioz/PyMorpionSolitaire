# PyMorpionSolitaire/__init__.py
#
# This package wraps the C++ extension CppMorpionSolitaire and re-exports its
# four game classes under a clean top-level namespace.
#
# Usage:
#   from PyMorpionSolitaire import Game5T
#   g = Game5T()       # 5-in-a-row, touching variant
#   g.playAtRandom()
#   print(g.getScore())
 
from .CppMorpionSolitaire import Game5T, Game5D, Game4T, Game4D
 
__all__ = ["Game5T", "Game5D", "Game4T", "Game4D"]