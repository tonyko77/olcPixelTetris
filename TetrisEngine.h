#ifndef TETRISENGINE_H
#define TETRISENGINE_H

#include "olcPixelGameEngine.h"
#include "TetrisConstants.h"
#include "Tetrimino.h"

#include <cstdint>


//=======================
// Game Settings
//=======================
struct TetrisSettings
{
    int32_t nStartLevel;
    olc::Key keyMoveLeft;
    olc::Key keyMoveRight;
    olc::Key keyRotLeft;
    olc::Key keyRotRight;
    olc::Key keySoftDrop;
    olc::Key keyHardDrop;
    olc::Key keyHold;
    bool bShowGrid;
    bool bShowGhost;

    // Key auto-repeat timings (in milliseconds)
    int32_t nDelayAutoRepeatMs;
    int32_t nSpeedAutoRepeatMs;

    // Sound volumes (as percentage, 0-100)
    int32_t nMusicVolume;
    int32_t nFxVolume;
};



//=======================
//  Tetris Game Engine
//=======================
class TetrisEngine
{
public:
    TetrisEngine(olc::PixelGameEngine* pPGE, olc::Sprite* pTilesSprite, const TetrisSettings& m_Settings);

    void UpdateGame(float fElapsedTime);

    bool IsGameOver() const     { return m_bGameOver; }
    int32_t GetScore() const    { return m_nScore;    }
    int32_t GetLevel() const    { return m_nLevel;    }
    int32_t GetLines() const    { return m_nLines;    }


private:
    void RandomNextPiece();
    bool PerformMove(int32_t deltaX, int32_t deltaY);
    bool PerformRotateLeft();
    bool PerformRotateRight();
    bool CheckKeyWithAutoRepeat(olc::Key key, float fElapsedTime);
    void LockCurrentPiece();
    bool DoesPieceCollide(const Tetrimino& tetro);
    bool CurrentPieceCollides()     { return DoesPieceCollide(m_CurrentPiece); }

    void DrawGameScreen(float fElapsedTime, int32_t fadeLevel);
    void DrawDroppedLines(float fElapsedTime);
    void DrawBoardContents(int32_t fadeLevel = FADE_OUT_STEPS);

    void DrawTetroTile(int screenX, int screenY, int8_t colorIndex, int32_t fadeLevel = FADE_OUT_STEPS);
    void DrawTetriminoOnBoard(const Tetrimino& tetro, int32_t fadeLevel = FADE_OUT_STEPS);
    void DrawTetriminoAnywhere(const Tetrimino& tetro, int screenX, int screenY);

    int8_t GetBoardTile(int32_t tileX, int32_t tileY);
    void SetBoardTile(int32_t tileX, int32_t tileY, int8_t colorIndex);

    void CheckForTSpinAfterRotate();


private:
    olc::PixelGameEngine* m_pPGE;
    olc::Sprite* m_pTilesSprite;
    const TetrisSettings& m_Settings;

    bool m_bGameOver;
    bool m_bSpawnNextPiece;

    // The board
    int8_t m_Board[BOARD_SIZE];

    // Current Scores
    int32_t m_nScore;
    int32_t m_nLevel;
    int32_t m_nLines;

    // Timing (fall speed)
    float m_fFallDuration;
    float m_fCurrentTime;
    float m_fMovingLockTime;

    // Current + next Tetriminos
    Tetrimino m_CurrentPiece;
    Tetrimino m_NextPieces[CNT_NEXT_PIECES];

    // HOLD data
    Tetrimino m_HeldPiece;
    bool m_bIsPieceHeld;       // set to TRUE if HOLD area holds something
    bool m_bAllowedToHold;     // set to FALSE if HOLD was used during current piece

    // Random Bag - https://tetris.fandom.com/wiki/Random_Generator
    uint8_t m_RandomBag[CNT_TETRIMINOS];
    int32_t m_nRandomBagIndex;

    // Auto-repeat support for LEFT, RIGHT and SOFT-DROP
    float m_fAutoRepeatCountdown;

    // Helper variable for T-Spin checking
    bool m_PerformedTSpin;

    // Messages for: lines removed, T-Spin, Next Level
    int32_t m_AnimationFlags;
    float m_fAnimationTimer;

    // List of lines that are in being dropped (for animation)
    std::vector<int32_t> m_LinesBeingDropped;
};


#endif // TETRISENGINE_H
