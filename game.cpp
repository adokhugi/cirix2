#include "game.h"

#include <array>
#include <string>

bool Game::Initialize (GL_Window* window, Keys* keys, DWORD tickCount)	
{
	int i;

	g_window			= window;
	g_keys				= keys;

	glEnable (GL_POINT_SMOOTH);
	glHint (GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable (GL_LINE_SMOOTH);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);

	glClearColor (1, 1, 1, 1);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluOrtho2D (0.0, 640.0, 0.0, 480.0);

	int color [3] = {255, 255, 255}; 

	for (int i = 0; i < maxNumPlayers; i++)
	{
		if (!bitmap [i].New (64, 16)) return false;
		text.SetText (("Player " + std::to_string(i + 1) + ": 0").c_str());
		bitmap [i].Clear (color);
		bitmap [i].SetRenderTextFont ("Arial", 14);
		bitmap [i].SetRenderTextColor (SelectColor(i + 1));
		bitmap [i].RenderText (&text, Align_Left);
		bitmap [i].GenerateTexture (texture [i]);

		if (!bitmap_number [i].New (64, 64)) return false;
		text.SetText ((std::to_string(i + 1)).c_str());
		bitmap_number [i].Clear (SelectColorIntArray(i + 1).data());
		bitmap_number [i].SetRenderTextFont ("Arial", 64);
		bitmap_number [i].SetRenderTextColor (0x01);
		bitmap_number [i].RenderText (&text, Align_Center);
		bitmap_number [i].GenerateTexture (texture_number [i]);
	}
	if (!bitmap_gameover.LoadFileFromMemory (IDR_JPG1, "JPG", FALSE, NULL)) return false;
	if (!bitmap_gameover1.New (256, 16)) return false;
	if (!bitmap_title.LoadFileFromMemory (IDR_JPG2, "JPG", FALSE, NULL)) return false;
/*
	if (!bitmap_instructions.LoadFileFromMemory (IDR_JPG3, "JPG", FALSE, NULL)) return false;
*/

	bitmap_gameover1.Clear (color);
	bitmap_gameover1.SetRenderTextFont ("Arial", 16);
	bitmap_gameover1.SetRenderTextColor (0x0000ff);
	bitmap_gameover1.GenerateTexture (texture_gameover1);

	bitmap_gameover.GenerateTexture (texture_gameover);
	bitmap_title.GenerateTexture (texture_title);
/*
	bitmap_instructions.GenerateTexture (texture_instructions);
*/

	srand (tickCount);
	gameState = TitlePicture;
	spacePressed = false;
	escapePressed = false;
	keyPressed = false;
	
	return true;
}

void Game::NewGame ()
{
	int i, j;

	for (i = 0; i < playfieldSize; i++)
		for (j = 0; j < playfieldSize; j++)
		{
			matrix [i] [j] = 0;
			count [i] [j] = 0;
			blinkingBlocks [i] [j] = false;
		}

	for (i = 0; i < maxNumPlayers; i++)
		score [i] = 0;
	UpdateScoreDisplay ();

	GenerateNewTile();
	
	currentPlayer = 1;
	numPlayers = maxNumPlayers; 
	blinking = false;
	gameState = ActualGame;
}

void Game::Deinitialize ()
{
}

void Game::Update (DWORD tickCount, DWORD lastTickCount)
{
	int i, j;

	if (escapePressed && !g_keys->keyDown [VK_ESCAPE])
	{
		if (gameState == TitlePicture)
			exit (1);
		else if (gameState == ActualGame)
		{
			escapePressed = false;
			EnterGameOver (tickCount);
		}
		else
		{
			escapePressed = false;
			gameState = TitlePicture;
		}
	}
	else if (!escapePressed && g_keys->keyDown [VK_ESCAPE])
		escapePressed = true;

	if (spacePressed && !g_keys->keyDown [VK_SPACE])
	{
		spacePressed = false;
		switch (gameState)
		{
		case GameOver:
			if (tickCount > blinkingDuration)
				gameState = TitlePicture;
			break;

		case TitlePicture:
			NewGame ();
			break;
		}
	}
	else if (!spacePressed && g_keys->keyDown [VK_SPACE])
		spacePressed = true;

	switch (gameState)
	{
	case ActualGame:
		if (blinking)
		{
			if (blinkingDuration < tickCount)
			{
				RemoveClearedBlocks ();
				MarkBlocksForBlinking(tickCount);
			}
			break;
		}

		if (g_keys->keyDown [VK_LEFT] && !keyPressed)
		{
			keyPressed = true;
			cursorPosition--;
			if (cursorPosition < tile_left)
				cursorPosition = 9 - tile_width + 1;
		}
		else if (g_keys->keyDown [VK_RIGHT] && !keyPressed)
		{
			keyPressed = true;
			cursorPosition++;
			if (cursorPosition > 9 - tile_width + 1)
				cursorPosition = tile_left;
		}
		else if (g_keys->keyDown [VK_SPACE] && !keyPressed)
		{
			keyPressed = true;
			int cur_x = cursorPosition;
			for (int tile_counter = 0; tile_counter < tile_numItems - 1; tile_counter++)
			{
				if (tile[tile_counter].dir == Left) cur_x--;
				else if (tile[tile_counter].dir == Right) cur_x++;
			}
			for (int tile_counter = tile_numItems - 1; tile_counter >= 0; tile_counter--)
			{
				if (matrix [0] [cur_x]) EnterGameOver (tickCount);
				for (i = 0; i < playfieldSize && !matrix [i] [cur_x]; i++);
				i--;
				if (i >= 0) matrix [i] [cur_x] = tile[tile_counter].caption;

				if (tile_counter > 0)
				{
					// reverse
					if (tile[tile_counter - 1].dir == Left) cur_x++;
					else if (tile[tile_counter - 1].dir == Right) cur_x--;
				}
			}

			// check if any blocks have been cleared
			MarkBlocksForBlinking(tickCount);

			GenerateNewTile();
		}
		else if (keyPressed && !g_keys->keyDown [VK_LEFT] && !g_keys->keyDown [VK_RIGHT] && !g_keys->keyDown [VK_SPACE])
			keyPressed = false;
		break;
	}
}

void Game::Draw (DWORD tickCount)
{
	int i, j;

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (gameState)
	{
/*
	case Instructions:
		glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, texture_instructions);
		glColor3f (1.0f, 1.0f, 1.0f);
		glBegin (GL_QUADS);
			glTexCoord2f (0.0f, 0.0f); glVertex2f ((640 - 512) / 2, (480 - 256) / 2);
			glTexCoord2f (1.0f, 0.0f); glVertex2f ((640 - 512) / 2 + 512, (480 - 256) / 2);
			glTexCoord2f (1.0f, 1.0f); glVertex2f ((640 - 512) / 2 + 512, (480 - 256) / 2 + 256);
			glTexCoord2f (0.0f, 1.0f); glVertex2f ((640 - 512) / 2, (480 - 256) / 2 + 256);
		glEnd ();
		glDisable (GL_TEXTURE_2D);
		break;
*/

	case TitlePicture:
		glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, texture_title);
		glColor3f (1.0f, 1.0f, 1.0f);
		glBegin (GL_QUADS);
			glTexCoord2f (0.0f, 0.0f); glVertex2f ((640 - 256) / 2, (480 - 128) / 2);
			glTexCoord2f (1.0f, 0.0f); glVertex2f ((640 - 256) / 2 + 256, (480 - 128) / 2);
			glTexCoord2f (1.0f, 1.0f); glVertex2f ((640 - 256) / 2 + 256, (480 - 128) / 2 + 128);
			glTexCoord2f (0.0f, 1.0f); glVertex2f ((640 - 256) / 2, (480 - 128) / 2 + 128);
		glEnd ();
		glDisable (GL_TEXTURE_2D);
		break;

	case GameOver:
	case ActualGame:
		// draw grid
		glColor3f (0, 0, 0);
		glBegin (GL_LINES);
			for (i = 0; i <= tileSize * playfieldSize; i += tileSize)
			{
				glVertex2f (i + (640 - tileSize * playfieldSize) / 2, (480 - tileSize * playfieldSize) / 2);
				glVertex2f (i + (640 - tileSize * playfieldSize) / 2, tileSize * playfieldSize + (480 - tileSize * playfieldSize) / 2);
				glVertex2f ((640 - tileSize * playfieldSize) / 2, i + (480 - tileSize * playfieldSize) / 2);
				glVertex2f (tileSize * playfieldSize + (640 - tileSize * playfieldSize) / 2, i + (480 - tileSize * playfieldSize) / 2);
			}
		glEnd ();
		// draw cursor
		int cur_x = cursorPosition;
		int cur_y = -tile_numItems;
		for (int i = 0; i < tile_numItems; i++)
		{
		 	DrawBlock (cur_y, cur_x, tile[i].caption);
			if (tile[i].dir == Left) cur_x--;
			else if (tile[i].dir == Right) cur_x++;
			else if (tile[i].dir == Down) cur_y++;
		}
/*
		// draw rest of grid where items cannot be placed
		glColor3f (1, 0, 0);
		glBegin (GL_LINES);
			for (i = tileSize * playfieldSize; i <= tileSize * totalPlayfieldSize; i += tileSize)
			{
				glVertex2f ((640 - tileSize * playfieldSize) / 2, i + (480 - tileSize * playfieldSize) / 2);
				glVertex2f (tileSize * playfieldSize + (640 - tileSize * playfieldSize) / 2, i + (480 - tileSize * playfieldSize) / 2);
			}
		glEnd ();
*/
		// draw blocks
		for (i = 0; i < playfieldSize; i++)
			for (j = 0; j < playfieldSize; j++)
				if (matrix [i] [j])
				{
					if (!blinking || !blinkingBlocks [i] [j] || ((int) (tickCount - blinkingDuration + 1000) / 200) % 2)
					{
						DrawBlock (i, j, matrix[i][j]);
					}
				}
		// draw score
		for (i = 0; i <= 0; i++)
		{
			glEnable (GL_TEXTURE_2D);
			glBindTexture (GL_TEXTURE_2D, texture [i]);
			glColor3f (1.0f, 1.0f, 1.0f);
			glBegin (GL_QUADS);
				glTexCoord2f (0.0f, 0.0f); glVertex2f ((640 - tileSize * playfieldSize) / 2 + 64 * i, 0);
				glTexCoord2f (1.0f, 0.0f); glVertex2f ((640 - tileSize * playfieldSize) / 2 + 64 * i + 64, 0);
				glTexCoord2f (1.0f, 1.0f); glVertex2f ((640 - tileSize * playfieldSize) / 2 + 64 * i + 64, 16);
				glTexCoord2f (0.0f, 1.0f); glVertex2f ((640 - tileSize * playfieldSize) / 2 + 64 * i, 16);
			glEnd ();
			glDisable (GL_TEXTURE_2D);
		}
		if (gameState == GameOver)
		{
			glEnable (GL_TEXTURE_2D);
			glBindTexture (GL_TEXTURE_2D, texture_gameover);
			glColor3f (1.0f, 1.0f, 1.0f);
			glBegin (GL_QUADS);
				glTexCoord2f (0.0f, 0.0f); glVertex2f ((640 - 256) / 2, (480 - 64) / 2);
				glTexCoord2f (1.0f, 0.0f); glVertex2f (256 + (640 - 256) / 2, (480 - 64) / 2);
				glTexCoord2f (1.0f, 1.0f); glVertex2f (256 + (640 - 256) / 2, 64 + (480 - 64) / 2);
				glTexCoord2f (0.0f, 1.0f); glVertex2f ((640 - 256) / 2, 64 + (480 - 64) / 2);
			glEnd ();
			glDisable (GL_TEXTURE_2D);
			glEnable (GL_TEXTURE_2D);
			glBindTexture (GL_TEXTURE_2D, texture_gameover1);
			glColor3f (1.0f, 1.0f, 1.0f);
			glBegin (GL_QUADS);
				glTexCoord2f (0.0f, 0.0f); glVertex2f ((640 - 256) / 2, (480 - 64) / 2 - 16);
				glTexCoord2f (1.0f, 0.0f); glVertex2f (256 + (640 - 256) / 2, (480 - 64) / 2 - 16);
				glTexCoord2f (1.0f, 1.0f); glVertex2f (256 + (640 - 256) / 2, (480 - 64) / 2);
				glTexCoord2f (0.0f, 1.0f); glVertex2f ((640 - 256) / 2, (480 - 64) / 2);
			glEnd ();
			glDisable (GL_TEXTURE_2D);
		}
		break;
	}
}

int Game::SelectColor (int i)
{
	switch (i)
	{
	case 1:
		glColor3f (1, 0, 0);
		return 0xff0000;

	case 2:
		glColor3f (0, .8, 0);
		return 0x00aa00;

	case 3:
		glColor3f (0, 0, .6);
		return 0x000088;

	case 4:
		glColor3f (1, .8, 0);
		return 0xffaa00;

	case 5:
		glColor3f (0, .8, .6);
		return 0x00aa88;
	}
}

std::array<int,3> Game::SelectColorIntArray (int i)
{
	switch (i)
	{
	case 1:
		return {0xff, 0x00, 0x00};

	case 2:
		return {0x00, 0xaa, 0x00};

	case 3:
		return {0x00, 0x00, 0x88};

	case 4:
		return {0xff, 0xaa, 0x00};

	case 5:
		return {0x00, 0xaa, 0x88};
	}
}

void Game::ClearMarked ()
{
	int i, j;

	for (i = 0; i < playfieldSize; i++)
		for (j = 0; j < playfieldSize; j++)
			marked [i] [j] = false;
}

void Game::MarkNeighborsSameColor (int i, int j)
{
	if (matrix [i] [j])
	{
		marked [i] [j] = true;
		if (i && matrix [i] [j] == matrix [i - 1] [j] && !marked [i - 1] [j])
			MarkNeighborsSameColor (i - 1, j);
		if (i < playfieldSize - 1 && matrix [i] [j] == matrix [i + 1] [j] && !marked [i + 1] [j])
			MarkNeighborsSameColor (i + 1, j);
		if (j && matrix [i] [j] == matrix [i] [j - 1] && !marked [i] [j - 1])
			MarkNeighborsSameColor (i, j - 1);
		if (j < playfieldSize - 1 && matrix [i] [j] == matrix [i] [j + 1] && !marked [i] [j + 1])
			MarkNeighborsSameColor (i, j + 1);
	}
}

int Game::CountMarked ()
{
	int i, j, n = 0;

	for (i = 0; i < playfieldSize; i++)
	{
		for (j = 0; j < playfieldSize; j++)
		{
			if (marked [i] [j])
			{
				n++;
			}
		}
	}

	for (i = 0; i < playfieldSize; i++)
	{
		for (j = 0; j < playfieldSize; j++)
		{
			if (marked [i] [j])
			{
				count [i] [j] = n;
			}
		}
	}

	return n;
}

void Game::RemoveClearedBlocks ()
{
	int i, j;

	for (j = 0; j < playfieldSize; j++)
	{
		for (i = 0; i < playfieldSize; i++)
		{
			if (blinkingBlocks [i] [j])
			{
				RemoveBlocks (i, j);
			}
		}
	}
/*
	for (i = 0; i < playfieldSize; i++)
	{
		for (j = 0; j < playfieldSize; j++)
		{
			ClearMarked ();
			MarkNeighborsSameColor (i, j);
			CountMarked ();
		}
	}
*/
}

void Game::RemoveBlocks (int y, int x)
{
	int i;

	for (i = y; i; i--)
	{
		matrix [i] [x] = matrix [i - 1] [x];
		blinkingBlocks [i] [x] = blinkingBlocks [i - 1] [x];
	}
	matrix [0] [x] = 0;
	blinkingBlocks [0] [x] = false;
}

void Game::UpdateScoreDisplay ()
{
	int i;
	char display [20], number [20], temp [2];

	strcpy (display, "Score: ");
	strcat (display, itoa (score [0], number, 10));
	int color [3] = {255, 255, 255}; 
	bitmap [0].Clear (color);
	text.SetText (display);
	bitmap [0].RenderText (&text, Align_Left);
	bitmap [0].UpdateTexture (texture [0]);
}

void Game::MarkedBlocksShallBlink ()
{
	for (int i = 0; i < playfieldSize; i++)
	{
		for (int j = 0; j < playfieldSize; j++)
		{
			if (marked [i] [j]) blinkingBlocks [i] [j] = true;
		}
	}
}

void Game::EnterGameOver (DWORD tickCount)
{
	char display [100], number [2];
	int max, maxIdx, i;

	strcpy(display, "Congratulations!");
	int color [3] = {255, 255, 255}; 
	bitmap_gameover1.Clear (color);
	text.SetText (display);
	bitmap_gameover1.RenderText (&text, Align_Center);
	bitmap_gameover1.UpdateTexture (texture_gameover1);
	blinkingDuration = tickCount + 10000;
	gameState = GameOver;
}

void Game::GenerateNewTile()
{
	tile_left = 0;
	tile_width = 1;
	TileDirections prev_dir = Down;
	
	for (int i = 0; i < tile_numItems; i++)
	{
		tile[i].caption = rand() % maxNumPlayers + 1;
		tile[i].dir = (TileDirections)(rand() % (tile_numDirections - 1) + 1);
		if (prev_dir == Left && tile[i].dir == Right) tile[i].dir = Down;
		else if (prev_dir == Right && tile[i].dir == Left) tile[i].dir = Down;
		if (i < tile_numItems - 1)
		{
			if (tile[i].dir == Left) tile_left++;
			else if (tile[i].dir == Right) tile_width++;
			prev_dir = tile[i].dir;
		}
	}

	cursorPosition = tile_left;
}

void Game::DrawBlock(int i, int j, int caption)
{
	SelectColor (caption);
	glBegin (GL_QUADS);
		glVertex2f (j * tileSize + (640 - tileSize * playfieldSize) / 2 + 1, tileSize + (playfieldSize - 1 - i) * tileSize + (480 - tileSize * playfieldSize) / 2 - 1);
		glVertex2f (tileSize + j * tileSize + (640 - tileSize * playfieldSize) / 2 - 1, tileSize + (playfieldSize - 1 - i) * tileSize + (480 - tileSize * playfieldSize) / 2 - 1);
		glVertex2f (tileSize + j * tileSize + (640 - tileSize * playfieldSize) / 2 - 1, (playfieldSize - 1 - i) * tileSize + (480 - tileSize * playfieldSize) / 2 + 1);
		glVertex2f (j * tileSize + (640 - tileSize * playfieldSize) / 2 + 1, (playfieldSize - 1 - i) * tileSize + (480 - tileSize * playfieldSize) / 2 + 1);
	glEnd ();
	glEnable (GL_TEXTURE_2D);
	glBindTexture (GL_TEXTURE_2D, texture_number[caption - 1]);
	glColor3f (1.0f, 1.0f, 1.0f);
	glBegin (GL_QUADS);
		glTexCoord2f (0.0f, 0.0f);
		glVertex2f (j * tileSize + (640 - tileSize * playfieldSize) / 2 + 1, (playfieldSize - 1 - i) * tileSize + (480 - tileSize * playfieldSize) / 2 + 1);
		glTexCoord2f (1.0f, 0.0f);
		glVertex2f (tileSize + j * tileSize + (640 - tileSize * playfieldSize) / 2 - 1, (playfieldSize - 1 - i) * tileSize + (480 - tileSize * playfieldSize) / 2 + 1);
		glTexCoord2f (1.0f, 1.0f);
		glVertex2f (tileSize + j * tileSize + (640 - tileSize * playfieldSize) / 2 - 1, tileSize + (playfieldSize - 1 - i) * tileSize + (480 - tileSize * playfieldSize) / 2 - 1);
		glTexCoord2f (0.0f, 1.0f);
		glVertex2f (j * tileSize + (640 - tileSize * playfieldSize) / 2 + 1, tileSize + (playfieldSize - 1 - i) * tileSize + (480 - tileSize * playfieldSize) / 2 - 1);
	glEnd ();
	glDisable (GL_TEXTURE_2D);
}

void Game::MarkBlocksForBlinking(DWORD tickCount)
{
	blinking = false;
	for (int i = 0; i < playfieldSize; i++)
	{
		for (int j = 0; j < playfieldSize; j++)
		{
			ClearMarked ();
			MarkNeighborsSameColor (i, j);
			int c = CountMarked();
			if (c > 0 && c >= matrix [i] [j])
			{
				score [0]++;
				UpdateScoreDisplay ();
				MarkedBlocksShallBlink ();
				blinkingDuration = tickCount + 1000;
				blinking = true;
			}
		}
	}
}