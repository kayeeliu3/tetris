#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#ifndef UNICODE
#define UNICODE
#define UNICODE_WAS_UNDEFINED
#endif

#include <stdio.h>
#include <Windows.h>
using namespace std::this_thread;
using std::cout;
using std::endl;

#ifdef UNICODE_WAS_UNDEFINED
#undef UNICODE
#endif

#define _GLIBCXX_USE_NANOSLEEP

std::wstring blocks[7]; // 7 kinds of blocks
int screenWidth = 120;
int screenHeight = 30;
int boardWidth = 15;
int boardHeight = 18;
unsigned char *pBoard = nullptr; // allocated dynamically

// rotation
// pX: X pos of block
// pY: Y pos of block
// pR: rotation
int rotate(int pX, int pY, int pR)
{
    switch (pR % 4) // 4 possible rotations
    {
    case 0:
        return pY * 4 + pX; // 0 degrees
    case 1:
        return 12 + pY - (pX * 4); // 90 degrees
    case 2:
        return 15 - (pY * 4) - pX; // 180 degrees
    case 3:
        return 3 - pY + (pX * 4); // 270 degrees
    }
    return 0;
}

// check for if piece fits in a space (collision)
// block: type of block
// rotation: possible rotation
// pX and pY: top left cell of block
bool doesPieceFit(int block, int rotation, int pX, int pY)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            // get index into piece
            // pi refers to piece index i.e. kind of block
            int pi = rotate(i, j, rotation);

            // get index into field
            // translate 4x4 block into board array (y * width + x)
            int fi = (pY + j) * boardWidth + (pX + i);

            // check within board limits
            if (pX + i >= 0 && pX + i < boardWidth)
            {
                if (pY + j >= 0 && pY + j < boardHeight)
                {
                    // if block piece's index is X AND board array index is NOT 0
                    if (blocks[block][pi] == L'X' && pBoard[fi] != 0)
                        return false; // collision
                }
            }
        }
    }

    return true; // fits
}

int main()
{
    // create all blocks (4 by 4)
    blocks[0].append(L"..X.");
    blocks[0].append(L"..X.");
    blocks[0].append(L"..X.");
    blocks[0].append(L"..X.");

    blocks[1].append(L"..X.");
    blocks[1].append(L".XX.");
    blocks[1].append(L".X..");
    blocks[1].append(L"....");

    blocks[2].append(L".X..");
    blocks[2].append(L".XX.");
    blocks[2].append(L"..X.");
    blocks[2].append(L"....");

    blocks[3].append(L"....");
    blocks[3].append(L".XX.");
    blocks[3].append(L".XX.");
    blocks[3].append(L"....");

    blocks[4].append(L"..X.");
    blocks[4].append(L".XX.");
    blocks[4].append(L"..X.");
    blocks[4].append(L"....");

    blocks[5].append(L"....");
    blocks[5].append(L".XX.");
    blocks[5].append(L"..X.");
    blocks[5].append(L"..X.");

    blocks[6].append(L"....");
    blocks[6].append(L".XX.");
    blocks[6].append(L".X..");
    blocks[6].append(L".X..");

    // init
    pBoard = new unsigned char[boardWidth * boardHeight];
    for (int x = 0; x < boardWidth; x++)
    {
        for (int y = 0; y < boardHeight; y++)
        {
            // setting board; if x is 0 or x is width of board
            // or y is height of board, then set 9 (border) else 0
            pBoard[y * boardWidth + x] = (x == 0 || x == boardWidth - 1 || y == boardHeight - 1) ? 9 : 0;
        }
    }

    // setting up screen
    // create char array and fill it with blank
    wchar_t *screen = new wchar_t[screenWidth * screenHeight];
    for (int i = 0; i < screenWidth * screenHeight; i++)
        screen[i] = L' ';
    HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(console); // set it as active screen buffer
    DWORD dwBytesWritten = 0;

    // logic variables
    int currentPiece = 0;          // current block kind
    int currentRotation = 0;       // rotation
    int currentX = boardWidth / 2; // middle in board
    int currentY = 0;

    bool keys[4];            // input keys
    bool rotateHold = false; // stop wild rotations

    int speed = 20;       // for speeding up as game progresses
    int speedCounter = 0; // count ticks until speed increment
    bool forceDown = false;
    int pieceCount = 0; // count number of pieces
    int score = 0;

    std::vector<int> tempLines; // to store lines to delete

    // GAME LOOP
    bool gameOver = false;
    while (!gameOver)
    {
        // timing
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // one game tick
        speedCounter++;                                             // increase speed
        forceDown = (speedCounter == speed);                        // force piece down

        // input
        for (int k = 0; k < 4; k++)
        {
            // loop through each key state,
            // and get if key is pressed or not
            //     --->                                            R    L  D  Z
            keys[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;
        }

        // logic and collision
        // if LEFT/RIGHT/DOWN key and piece fits,
        // then update relevant X or Y position by 1
        currentX -= (keys[1] && doesPieceFit(currentPiece, currentRotation, currentX - 1, currentY)) ? 1 : 0;
        currentX += (keys[0] && doesPieceFit(currentPiece, currentRotation, currentX + 1, currentY)) ? 1 : 0;
        currentY += (keys[2] && doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) ? 1 : 0;

        // rotation increment if piece fits
        // rotateHold stops wild spinning every 50ms
        if (keys[3])
        {
            currentRotation += (!rotateHold && doesPieceFit(currentPiece, currentRotation + 1, currentX, currentY)) ? 1 : 0;
            rotateHold = true;
        }
        else
            rotateHold = false;

        // force a piece down until collision/lose
        if (forceDown)
        {
            if (doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1))
                currentY++; // down!
            else
            {
                // lock piece in board as collide
                for (int pX = 0; pX < 4; pX++)
                {
                    for (int pY = 0; pY < 4; pY++)
                    {
                        // if block array index is X
                        // update board index accordingly as filled
                        if (blocks[currentPiece][rotate(pX, pY, currentRotation)] == L'X')
                            pBoard[(currentY + pY) * boardWidth + (currentX + pX)] = currentPiece + 1;
                    }
                }

                // increase speed of game as more pieces
                pieceCount++;
                if (pieceCount % 30 == 0)
                    if (speed >= 10)
                        speed--;

                // check any full hori lines
                // only need to check the four lines that blocks can occupy
                // this takes four rows of block translating into board
                for (int pY = 0; pY < 4; pY++)
                {
                    if (currentY + pY < boardHeight - 1)
                    {
                        bool line = true;

                        for (int pX = 1; pX < boardWidth - 1; pX++)
                        {
                            // while board space filled, line is true
                            line &= (pBoard[(currentY + pY) * boardWidth + pX]) != 0;
                        }

                        if (line)
                        {
                            // remove line, set to =
                            for (int pX = 1; pX < boardWidth - 1; pX++)
                                pBoard[(currentY + pY) * boardWidth + pX] = 8;

                            tempLines.push_back(currentY + pY); // add to vector
                        }
                    }
                }

                // update score
                // more lines cut, higher score points
                score += 25;
                if (!tempLines.empty())
                    score += (1 << tempLines.size()) * 100;

                // choose next piece
                currentX = boardWidth / 2;
                currentY = 0;
                currentRotation = 0;
                currentPiece = rand() % 7; // choose pseudo random piece

                // if piece not fit, lose!
                gameOver = !doesPieceFit(currentPiece, currentRotation, currentX, currentY);
            }

            speedCounter = 0;
        }

        // output display

        // draw field
        for (int x = 0; x < boardWidth; x++)
        {
            for (int y = 0; y < boardHeight; y++)
                // offset by 2 to prevent top left screen
                // set output of character in string array at rotation 0
                screen[(y + 2) * screenWidth + (x + 2)] = L" XXXXXXX=#"[pBoard[y * boardWidth + x]];
        }

        // draw current block piece
        for (int pX = 0; pX < 4; pX++)
        {
            for (int pY = 0; pY < 4; pY++)
            {
                // if block index of correct rotation is X
                if (blocks[currentPiece][rotate(pX, pY, currentRotation)] == L'X')
                {
                    // draw to screen with offset of current position + 2 (as board is offset by 2 already)
                    // follows regular (y * width + x)
                    // current piece ADD 65 (ascii no. for A), thus giving letters
                    screen[(currentY + pY + 2) * screenWidth + (currentX + pX + 2)] = currentPiece + 65;
                }
            }
        }

        // draw score and instructions
        swprintf_s(&screen[20 * screenWidth + boardWidth - 13], 16, L"SCORE:%8d", score);
        swprintf_s(&screen[21 * screenWidth + boardWidth - 12], 16, L"ARROWS: MOVE");
        swprintf_s(&screen[22 * screenWidth + boardWidth - 11], 16, L"Z: ROTATE");

        // if line complete, pause then remove
        if (!tempLines.empty())
        {
            // display
            WriteConsoleOutputCharacter(console, screen, screenWidth * screenHeight, {0, 0}, &dwBytesWritten);
            std::this_thread::sleep_for(std::chrono::milliseconds(400));

            // iterate through each collumn and pull
            // all above pieces down
            for (auto &v : tempLines)
            {
                for (int pX = 1; pX < boardWidth - 1; pX++)
                {
                    // set above blocks down
                    for (int pY = v; pY > 0; pY--)
                        pBoard[pY * boardWidth + pX] = pBoard[(pY - 1) * boardWidth + pX];

                    // clear above space
                    pBoard[pX] = 0;
                }

                tempLines.clear();
            }
        }

        // display
        WriteConsoleOutputCharacter(console, screen, screenWidth * screenHeight, {0, 0}, &dwBytesWritten);
    }

    // game over!!
    CloseHandle(console);
    std::cout << "Game Over! Score: " << score << std::endl;
    system("pause");

    return 0;
}