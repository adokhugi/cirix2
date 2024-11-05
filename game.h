#ifndef _GAME_H_
#define _GAME_H_

#include <array>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>

#include "NeHeGL.h"
#include "NeHe_Window.h"
#include "Bitmap.h"
#include "resource.h"

class Game
{
public:
	bool Initialize (GL_Window *window, Keys *keys, DWORD tickCount);
	void Deinitialize ();
	void Update (DWORD tickCount, DWORD lastTickCount);
	void Draw (DWORD tickCount);

private:
	void NewGame ();
	int SelectColor (int i);
	std::array<int,3> SelectColorIntArray (int i);
	void ClearMarked ();
	void MarkNeighborsSameColor (int i, int j);
	int CountMarked ();
	void RemoveClearedBlocks ();
	void RemoveBlocks (int x, int y);
	void UpdateScoreDisplay ();
	void MarkedBlocksShallBlink ();
	void EnterGameOver (DWORD tickCount);
	void GenerateNewTile ();
	void DrawBlock (int i, int j, int caption);
	void MarkBlocksForBlinking(DWORD tickCount);

	enum GameStates
	{
		ActualGame,
		GameOver,
		TitlePicture,
		Instructions
	} gameState;

	enum TileDirections
	{
		Down,
		Left,
		Right
	};

	struct TileItem
	{
		int caption;
		TileDirections dir;
	};

	GL_Window *g_window;
	Keys *g_keys;
 	bool spacePressed;
	bool escapePressed;
	bool keyPressed;
	int cursorPosition;
	static const int playfieldSize = 10;
	static const int totalPlayfieldSize = playfieldSize + 3;
	static const int tileSize = 30;
	int matrix [playfieldSize] [playfieldSize];
	int currentPlayer;
	int numPlayers;
	bool marked [playfieldSize] [playfieldSize];
	int count [playfieldSize] [playfieldSize];
	static const int maxNumPlayers = 5;
	int score [maxNumPlayers];
	bool blinking;
	float blinkingDuration;
	bool toBechecked [playfieldSize] [playfieldSize];
	TextObject text;
	Bitmap bitmap [maxNumPlayers];
	GLuint texture [maxNumPlayers];
	bool blinkingBlocks [playfieldSize] [playfieldSize];
	Bitmap bitmap_gameover;
	GLuint texture_gameover;
	Bitmap bitmap_gameover1;
	GLuint texture_gameover1;
	Bitmap bitmap_title;
	GLuint texture_title;
	Bitmap bitmap_instructions;
	GLuint texture_instructions;
	Bitmap bitmap_number [maxNumPlayers];
	GLuint texture_number [maxNumPlayers];
	TileItem tile[3];
	int tile_left;
	int tile_width;
	static const int tile_numItems = 3;
	static const int tile_numDirections = 3;
};

#endif