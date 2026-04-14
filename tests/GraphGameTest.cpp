#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "GraphGame.h"
#include "Coordinates.h"

// Plays five specific moves from the initial position, bringing the score to 5.
// Several tests below depend on this exact board state.
static void playFiveMoves(GraphGame<4, false>& game)
{
    game.tryPlay({Coordinates(30, 36).toPoint(), Coordinates(34, 36).toPoint()});
    game.tryPlay({Coordinates(29, 27).toPoint(), Coordinates(33, 27).toPoint()});
    game.tryPlay({Coordinates(31, 27).toPoint(), Coordinates(27, 31).toPoint()});
    game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(30, 30).toPoint()});
    game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(34, 30).toPoint()});
}

// ---------------------------------------------------------------------------
// Basic mechanics
// ---------------------------------------------------------------------------

TEST_CASE("Initial position has 28 possible moves", "[graphgame]")
{
    GraphGame<4, false> game;
    REQUIRE(game.getNumberOfMoves() == 28);
}

TEST_CASE("Playing a valid move changes the move count", "[graphgame]")
{
    GraphGame<4, false> game;
    REQUIRE(game.tryPlay({Coordinates(30, 36).toPoint(), Coordinates(34, 36).toPoint()}));
    REQUIRE(game.getNumberOfMoves() == 26);
}

TEST_CASE("Illegal moves are rejected", "[graphgame]")
{
    GraphGame<4, false> game;
    // Zero-length segment
    CHECK_FALSE(game.tryPlay({Coordinates(30, 30).toPoint(), Coordinates(30, 30).toPoint()}));
    // Segment with no available dot
    CHECK_FALSE(game.tryPlay({Coordinates(30, 30).toPoint(), Coordinates(30, 34).toPoint()}));
    // Diagonal segment that does not align with any initial dot
    CHECK_FALSE(game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(34, 30).toPoint()}));
}

TEST_CASE("Legal moves increase the score", "[graphgame]")
{
    GraphGame<4, false> game;
    playFiveMoves(game);
    REQUIRE(game.getScore() == 5);
}

TEST_CASE("Undo reverts the last move and allows replaying it", "[graphgame]")
{
    GraphGame<4, false> game;
    playFiveMoves(game);

    // The last move cannot be played again while it is on the board
    CHECK_FALSE(game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(34, 30).toPoint()}));

    game.undo();
    REQUIRE(game.getScore() == 4);

    // After undoing, the same move is legal again
    REQUIRE(game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(34, 30).toPoint()}));
    REQUIRE(game.getScore() == 5);

    // And illegal once more after replaying
    CHECK_FALSE(game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(34, 30).toPoint()}));
}

TEST_CASE("deleteBranch removes a candidate move", "[graphgame]")
{
    GraphGame<4, false> game;
    int before = game.getNumberOfMoves();
    REQUIRE(before > 5);

    game.deleteBranch(4);
    REQUIRE(game.getNumberOfMoves() == before - 1);
}

TEST_CASE("Restart resets score and restores all initial moves", "[graphgame]")
{
    GraphGame<4, false> game;
    playFiveMoves(game);
    REQUIRE(game.getScore() == 5);

    game.restart();
    REQUIRE(game.getScore() == 0);
    REQUIRE(game.getNumberOfMoves() == 28);
}

TEST_CASE("Empty game has no initial dots and no possible moves", "[graphgame]")
{
    GraphGame<4, false> emptyGame('e');
    REQUIRE(emptyGame.getScore() == 0);
    REQUIRE(emptyGame.getNumberOfMoves() == 0);
}

// ---------------------------------------------------------------------------
// Random play
// ---------------------------------------------------------------------------

TEST_CASE("playAtRandom(n) plays exactly n moves", "[graphgame]")
{
    GraphGame<4, false> game;
    game.playAtRandom(3);
    REQUIRE(game.getScore() == 3);
    REQUIRE(game.getNumberOfMoves() > 0);
}

TEST_CASE("playAtRandom() plays a complete game", "[graphgame]")
{
    GraphGame<4, false> game;
    game.playAtRandom();
    CHECK(game.getScore() >= 20);   // a reasonable lower bound for any random game
    REQUIRE(game.getNumberOfMoves() == 0);
}

TEST_CASE("undoAll reverts a complete game to the initial position", "[graphgame]")
{
    GraphGame<4, false> game;
    game.playAtRandom();
    REQUIRE(game.getScore() > 0);

    game.undoAll();
    REQUIRE(game.getScore() == 0);
    REQUIRE(game.getNumberOfMoves() == 28);
}

// ---------------------------------------------------------------------------
// Serialisation
// ---------------------------------------------------------------------------

TEST_CASE("JSON round-trip preserves score and move count", "[graphgame][serialisation]")
{
    GraphGame<4, false> game;
    game.playAtRandom(3);

    auto json = game.exportJSON();
    auto copy = GraphGame<4, false>::importJSON(json);

    REQUIRE(copy.getScore()          == game.getScore());
    REQUIRE(copy.getNumberOfMoves()  == game.getNumberOfMoves());
}

// ---------------------------------------------------------------------------
// Search algorithms
// ---------------------------------------------------------------------------

TEST_CASE("exploreDepth returns the deepest score within the node cutoff", "[graphgame]")
{
    GraphGame<4, false> game;
    // Play one move so the exploration has a non-trivial starting point
    game.tryPlay({Coordinates(30, 36).toPoint(), Coordinates(34, 36).toPoint()});
    REQUIRE(game.getScore() == 1);

    CHECK(game.exploreDepth(1)  == 1);
    CHECK(game.exploreDepth(10) == 10);
    // exploreDepth must not permanently alter the game state
    REQUIRE(game.getScore()          == 1);
    REQUIRE(game.getNumberOfMoves()  == 26);
}

TEST_CASE("estimateDepth returns a plausible score without changing game state", "[graphgame]")
{
    GraphGame<4, false> game;
    int estimate = game.estimateDepth();

    CHECK(estimate >= 20);
    // estimateDepth must not permanently alter the game state
    REQUIRE(game.getScore()         == 0);
    REQUIRE(game.getNumberOfMoves() == 28);
}

TEST_CASE("estimateDepth near the end of a game returns at least the remaining moves", "[graphgame]")
{
    GraphGame<4, false> game;
    game.playAtRandom();
    int finalScore = game.getScore();

    game.revertToScore(finalScore - 5);
    CHECK(game.estimateDepth() >= 5);
}

// These tests run the NMCS algorithms to completion and may take several minutes
// at level 2. They are excluded from the default test run and must be
// requested explicitly: ./GraphGameTests "[slow]"
TEST_CASE("playNestedMC level 1 reaches a score of at least 20", "[graphgame][slow]")
{
    GraphGame<4, false> game;
    game.playNestedMC(1);
    REQUIRE(game.getScore() >= 20);
}

TEST_CASE("playFastNestedMC level 2 reaches a score of at least 20", "[graphgame][slow]")
{
    GraphGame<4, false> game;
    game.playFastNestedMC(2);
    REQUIRE(game.getScore() >= 20);
}

// ---------------------------------------------------------------------------
// Performance benchmarks
// Excluded from the default run; enable with: ./GraphGameTests "[benchmark]"
// ---------------------------------------------------------------------------

TEST_CASE("Performance benchmarks", "[graphgame][.benchmark]")
{
    GraphGame<4, false> game;

    BENCHMARK("random game from start to finish")
    {
        game.playAtRandom();
        game.undoAll();
        return game.getScore();
    };

    BENCHMARK("estimateDepth from a mid-game position")
    {
        game.playAtRandom();
        game.revertToRandomScore();
        int depth = game.estimateDepth();
        game.undoAll();
        return depth;
    };
}
