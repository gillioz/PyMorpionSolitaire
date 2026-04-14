#include <catch2/catch_test_macros.hpp>
#include "Game.h"
#include "Coordinates.h"

// Plays five specific moves from the initial position, bringing the score to 5.
// Several tests below depend on this exact board state.
static void playFiveMoves(Game<4, false>& game)
{
    game.tryPlay({Coordinates(30, 36).toPoint(), Coordinates(34, 36).toPoint()});
    game.tryPlay({Coordinates(29, 27).toPoint(), Coordinates(33, 27).toPoint()});
    game.tryPlay({Coordinates(31, 27).toPoint(), Coordinates(27, 31).toPoint()});
    game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(30, 30).toPoint()});
    game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(34, 30).toPoint()});
}

TEST_CASE("Initial position has 28 possible moves", "[game]")
{
    Game<4, false> game;
    REQUIRE(game.findAllMoves().size() == 28);
}

TEST_CASE("Playing a valid move changes the move count", "[game]")
{
    Game<4, false> game;
    REQUIRE(game.tryPlay({Coordinates(30, 36).toPoint(), Coordinates(34, 36).toPoint()}));
    REQUIRE(game.findAllMoves().size() == 26);
}

TEST_CASE("Illegal moves are rejected", "[game]")
{
    Game<4, false> game;
    // Zero-length segment
    CHECK_FALSE(game.tryPlay({Coordinates(30, 30).toPoint(), Coordinates(30, 30).toPoint()}));
    // Segment with no available dot
    CHECK_FALSE(game.tryPlay({Coordinates(30, 30).toPoint(), Coordinates(30, 34).toPoint()}));
    // Diagonal segment that does not align with any initial dot
    CHECK_FALSE(game.tryPlay({Coordinates(30, 26).toPoint(), Coordinates(34, 30).toPoint()}));
}

TEST_CASE("Legal moves increase the score", "[game]")
{
    Game<4, false> game;
    playFiveMoves(game);
    REQUIRE(game.getScore() == 5);
}

TEST_CASE("Undo reverts the last move and allows replaying it", "[game]")
{
    Game<4, false> game;
    playFiveMoves(game);
    REQUIRE(game.getScore() == 5);

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

TEST_CASE("findNewMoves returns the moves unlocked by a dot placement", "[game]")
{
    Game<4, false> game;
    playFiveMoves(game);

    auto move1 = game.tryBuildMove({Coordinates(30, 32).toPoint(), Coordinates(30, 36).toPoint()});
    REQUIRE(move1.has_value());
    game.applyMove(move1.value());
    // This dot does not unlock any new move
    CHECK(game.findNewMoves(move1->dot).empty());

    auto move2 = game.tryBuildMove({Coordinates(33, 32).toPoint(), Coordinates(33, 36).toPoint()});
    REQUIRE(move2.has_value());
    game.applyMove(move2.value());
    // This dot unlocks exactly one new move
    CHECK(game.findNewMoves(move2->dot).size() == 1);
}

TEST_CASE("JSON round-trip preserves score and move count", "[game][serialisation]")
{
    Game<4, false> game;
    playFiveMoves(game);

    auto json   = game.exportJSON();
    auto copy   = Game<4, false>::importJSON(json);

    REQUIRE(copy.getScore()          == game.getScore());
    REQUIRE(copy.findAllMoves().size() == game.findAllMoves().size());
}

TEST_CASE("Restart resets the game to score zero", "[game]")
{
    Game<4, false> game;
    playFiveMoves(game);
    REQUIRE(game.getScore() == 5);

    game.restart();
    REQUIRE(game.getScore() == 0);
}

TEST_CASE("Empty game has no initial dots and no possible moves", "[game]")
{
    Game<4, false> emptyGame('e');
    REQUIRE(emptyGame.getScore() == 0);
    REQUIRE(emptyGame.findAllMoves().empty());
}
