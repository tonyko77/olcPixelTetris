#include "TetrisEngine.h"

using namespace std;
using namespace olc;



/////////////////////////////////////////////
// Constants
/////////////////////////////////////////////
namespace
{
    // Level times (drop delays) - https://tetris.fandom.com/wiki/Tetris_Worlds#Gravity
    const float LEVEL_DROP_DELAY[MAX_LEVEL] = {
        1.000f, 0.793f, 0.618f, 0.473f, 0.355f,
        0.262f, 0.190f, 0.135f, 0.094f, 0.065f,
        0.043f, 0.028f, 0.018f, 0.012f, 0.008f,
        0.006f, 0.004f, 0.003f, 0.002f, 0.001f
    };

    // Score multipliers for full lines
    const int32_t FULL_LINE_SCORES[4] = { 100, 300, 500, 800 };

    // Score multiplier for T-Spin
    const int32_t T_SPIN_SCORE = 400;

    // Score multiplier for FULL CLEAR
    const int32_t FULL_CLEAR_SCORE = 1600;

    // constants for cell contents
    const int8_t EMPTY_CELL = -1;
    const int8_t OUTSIDE_CELL = 99;

    // constants for Animations
    enum {
        ANIM_LINES_MASK = 0x07,
        ANIM_TSPIN      = 0x08,
        ANIM_FULL_CLEAR = 0x10,
    };

    // constant timeouts
    const float LOCK_DELAY = 0.5f;
    const float MOVING_LOCK_DELAY = 2.0f;
    const float FULL_LINES_ANIMATION_DELAY = 0.3f;

    // alpha values for fading piece before lock
    const uint8_t LOCK_ALPHA_MAX = 255;
    const uint8_t LOCK_ALPHA_MIN = 20;

    const int32_t Y_LAST_LINE = TABLE_HEIGHT_TILES - 1;
}



/////////////////////////////////////////////
//  DEBUG Logging
/////////////////////////////////////////////
namespace
{
#if 0
    const int32_t NO_NUMBER = 0x7FFF7FFF;
    const int32_t LOG_X = 270;
    const int32_t LOG_Y = 170;
    const int32_t CNT_LOG_LINES = 14;

    bool displayDebugLog = false;
    uint32_t lineCounter = 0;
    string logLines[CNT_LOG_LINES];

    void debuglogReset() {
        for (int i = 0; i < CNT_LOG_LINES; i++) {
            logLines[i] = "";
        }
    }

    void debuglogAppend(const char* strToLog, int32_t val = NO_NUMBER)
    {
        int lastIdx = CNT_LOG_LINES - 1;
        for (int i = 0; i < lastIdx; i++) {
            logLines[i] = logLines[i + 1];
        }
        lineCounter++;

        string strval = (val == NO_NUMBER) ? "" : to_string(val);
        logLines[lastIdx] = to_string(lineCounter) + string("> ") + string(strToLog) + strval;
    }

    void debuglogDraw(PixelGameEngine* pPGE)
    {
        if (pPGE->GetKey(Key::TAB).bPressed) {
            displayDebugLog = (!displayDebugLog);
        }
        if (!displayDebugLog) return;

        pPGE->FillRect(LOG_X, LOG_Y, SCREEN_WIDTH_PIXELS - LOG_X, SCREEN_HEIGHT_PIXELS - LOG_Y, BLACK);
        pPGE->DrawRect(LOG_X, LOG_Y, SCREEN_WIDTH_PIXELS - LOG_X, SCREEN_HEIGHT_PIXELS - LOG_Y, CYAN);
        int y = LOG_Y + 2;
        for (uint32_t i = 0; i < CNT_LOG_LINES; i++) {
            const Pixel& color = ((i % 2) == (lineCounter % 2)) ? WHITE : YELLOW;
            pPGE->DrawString(LOG_X + 2, y, logLines[i], color);
            y += 9;
        }
    }

#else
    void debuglogReset() {}
    void debuglogAppend(const char* strToLog, int32_t val = 0) {}
    void debuglogDraw(PixelGameEngine* pPGE) {}

#endif
}



/////////////////////////////////////////////
//  TETRIS GAME ENGINE
/////////////////////////////////////////////

TetrisEngine::TetrisEngine(PixelGameEngine* pPGE, Sprite* pTilesSprite, const TetrisSettings& settings)
: m_pPGE(pPGE)
, m_pTilesSprite(pTilesSprite)
, m_Settings(settings)
, m_bGameOver(false)
, m_Board{}
, m_RandomBag{}
, m_PerformedTSpin(false)
{
    srand ((unsigned int) time(NULL));
    // reset board
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        m_Board[i] = EMPTY_CELL;
    }
    // reset scores
    m_nScore = 0;
    m_nLevel = m_Settings.nStartLevel;
    m_nLines = 0;
    // reset time
    m_fFallDuration = LEVEL_DROP_DELAY[m_nLevel - 1];
    m_fCurrentTime = m_fMovingLockTime = 0.0f;
    // generate random pieces (current + nexts)
    m_nRandomBagIndex = 999; // to trigger a reset of the random bag
    for (int i = 0; i < CNT_NEXT_PIECES; i++) {
        RandomNextPiece();
    }
    m_bSpawnNextPiece = true;
    // reset HOLD
    m_bIsPieceHeld = false;
    m_bAllowedToHold = true;
    // reset key autorepeat
    m_fAutoRepeatCountdown = 0.0f;
    m_fAnimationTimer = 0.0f;
    m_AnimationFlags = 0;

    debuglogReset();
    debuglogAppend("START lev=", m_nLevel);
}


// Game loop for RUNNING GAME
void TetrisEngine::UpdateGame(float fElapsedTime)
{
    if (m_bGameOver) return;

    // check if we're currently animating dropped lines
    if (m_LinesBeingDropped.size() > 0)
    {
        DrawDroppedLines(fElapsedTime);
        return;
    }

    // Spawn next piece
    if (m_bSpawnNextPiece)
    {
        m_bSpawnNextPiece = false;
        RandomNextPiece();
        m_fCurrentTime = 0.0f;
        debuglogAppend("New Piece ", m_CurrentPiece.getTypeIndex());
        // check for GAME OVER
        if (CurrentPieceCollides()) {
            // try one row above
            m_CurrentPiece.move(0, -1);
            debuglogAppend("SHIFT UP 1");
            // try one more row above (but NOT for 'I')
            if (m_CurrentPiece.getTypeChar() != 'I' && CurrentPieceCollides()) {
                m_CurrentPiece.move(0, -1);
                debuglogAppend("SHIFT UP 2");
            }
        }
        if (CurrentPieceCollides())
        {
            m_bGameOver = true;
            return;
        }
    }

    bool bPieceWasMoved = false, bMustLock = false;

    // Read Keys WITHOUT auto-repeat: ROTATE, HARD-DROP, HOLD, PAUSE
    if (m_pPGE->GetKey(m_Settings.keyRotLeft).bPressed) {
        bPieceWasMoved = PerformRotateLeft();
    }
    else if (m_pPGE->GetKey(m_Settings.keyRotRight).bPressed) {
        bPieceWasMoved = PerformRotateRight();
    }
    else if (m_pPGE->GetKey(m_Settings.keyHardDrop).bPressed) {
        bPieceWasMoved = bMustLock = true;
        do {
            m_CurrentPiece.move(0, 1);
            m_nScore += 2;
        } while (!CurrentPieceCollides());
        m_CurrentPiece.move(0, -1);
        m_nScore -= 2;
    }
    else if (m_pPGE->GetKey(m_Settings.keyHold).bPressed) {
        if (m_bAllowedToHold)
        {
            bPieceWasMoved = true;
            Tetrimino backupCurrentPiece = m_CurrentPiece;
            // change current piece
            if (m_bIsPieceHeld) {
                m_CurrentPiece = m_HeldPiece;
            } else {
                RandomNextPiece();
            }
            // put former piece into HOLD
            m_HeldPiece = backupCurrentPiece;
            m_HeldPiece.resetPosition();
            m_bIsPieceHeld = true;
            m_bAllowedToHold = false;
        }
    }

    // Read Keys WITH auto-repeat: LEFT, RIGHT, SOFT DROP
    else if (CheckKeyWithAutoRepeat(m_Settings.keyMoveLeft, fElapsedTime)) {
        bPieceWasMoved = PerformMove(-1, 0);
    }
    else if (CheckKeyWithAutoRepeat(m_Settings.keyMoveRight, fElapsedTime)) {
        bPieceWasMoved = PerformMove(+1, 0);
    }
    else if (CheckKeyWithAutoRepeat(m_Settings.keySoftDrop, fElapsedTime)) {
        if (PerformMove(0, +1)) {
            m_nScore += 1, m_fCurrentTime = 0.0f, bPieceWasMoved = true;
        }
    }

    // Update time
    m_fCurrentTime += fElapsedTime;
    m_fMovingLockTime += fElapsedTime;

    // Check if piece should lock or do a timed drop (if not already HARD-DROPped)
    bool bCouldLock = false;
    if (!bMustLock)
    {
        // Check if piece could lock (it is 'resting' on something)
        Tetrimino droppedByOne = m_CurrentPiece;
        droppedByOne.move(0, 1);
        bCouldLock = DoesPieceCollide(droppedByOne);
        if (bCouldLock)
        {
            // if it just moved => reset lock time
            if (bPieceWasMoved)
                m_fCurrentTime = 0.0f;
            // check if it must lock
            bMustLock = (m_fCurrentTime >= LOCK_DELAY) || (m_fMovingLockTime >= MOVING_LOCK_DELAY);
        }
        else
        {
            // reset moving lock time if no longer resting on something
            m_fMovingLockTime = 0.0f;
            // if piece cannot lock => check for timed drop
            if (m_fCurrentTime >= m_fFallDuration)
            {
                m_fCurrentTime -= m_fFallDuration;
                m_CurrentPiece.move(0, 1);
                m_PerformedTSpin = false;
            }
        }
    }

    // Perform lock if needed
    if (bMustLock) {
        LockCurrentPiece();
        m_bSpawnNextPiece = true;
    }
    else {
        // Calculate fade level if piece could lock
        int32_t fadeLevel = FADE_OUT_STEPS;
        if (bCouldLock) {
            float f = 1.0f - fmax(m_fCurrentTime / LOCK_DELAY, m_fMovingLockTime / MOVING_LOCK_DELAY);
            f = fmax(0.0f, fmin(1.0f, f));
            fadeLevel = 1 + (int32_t)(f * FADE_OUT_STEPS);
        }
        // Update screen
        DrawGameScreen(fElapsedTime, fadeLevel);
    }
}


// Lock current piece + Spawn next piece
//   - The I and O spawn in the middle columns
//   - The rest spawn in the left-middle columns
//   - The tetriminoes spawn horizontally with J, L and T spawning flat-side first.
//   - (!) Spawn above playfield, row 21 for I, and 21/22 for all other tetriminoes.
//   - (!) Immediately drop one space if no existing Block is in its path
//   - (!) The player tops out when a piece is spawned overlapping at least one block (block out),
//            or a piece locks completely above the visible portion of the playfield (lock out).
void TetrisEngine::LockCurrentPiece()
{
    debuglogAppend("Lock Piece ", m_CurrentPiece.getTypeIndex());

    m_AnimationFlags = 0;
    m_fAnimationTimer = 0.0f;
    m_bAllowedToHold = true;

    // Lock the current piece on the board
    bool bLockOut = true;
    for (int i = 0; i < 4; i++) {
        int32_t tileX = m_CurrentPiece.getX(i);
        int32_t tileY = m_CurrentPiece.getY(i);
        SetBoardTile(tileX, tileY, m_CurrentPiece.getTypeIndex());
        // GAME OVER check: LOCK OUT
        // (the piece locks completely above the visible portion of the playfield).
        if (tileY >= 0) bLockOut = false;
    }
    // if lock out => game over
    if (bLockOut) {
        m_bGameOver = true;
        return;
    }

    // check for T-Spin
    if (m_PerformedTSpin) {
        m_nScore += m_nLevel * T_SPIN_SCORE;
        m_AnimationFlags |= ANIM_TSPIN;
        debuglogAppend("T-SPIN");
    }
    m_PerformedTSpin = false;

    // check for FULL LINES
    m_LinesBeingDropped.clear();
    for (int32_t y = 0; y < TABLE_HEIGHT_TILES; y++)
    {
        bool isFullLine = true;
        for (int32_t x = 0; x < TABLE_WIDTH_TILES; x++)
        {
            if (GetBoardTile(x, y) < 0)
            {
                isFullLine = false;
                break;
            }
        }
        if (isFullLine) m_LinesBeingDropped.push_back(y);
    }
}


void TetrisEngine::RandomNextPiece()
{
    // reset random bag if necessary
    if (m_nRandomBagIndex >= CNT_TETRIMINOS)
    {
        debuglogAppend("NEW RandBag");
        m_nRandomBagIndex = 0;
        for (int8_t i = 0; i < CNT_TETRIMINOS; i++) {
            m_RandomBag[i] = i;
        }
        // shuffle bag
        for (int32_t i = 0; i < 1000; i++)
        {
            // get 2 random indices
            int32_t x, y;
            x = rand() % CNT_TETRIMINOS;
            do {
                y = rand() % CNT_TETRIMINOS;
            } while (y == x);
            // swap indices;
            int8_t tmp = m_RandomBag[x];
            m_RandomBag[x] = m_RandomBag[y];
            m_RandomBag[y] = tmp;
        }
    }

    // pick next random number from bag
    int32_t nextIdx = m_RandomBag[m_nRandomBagIndex++];

    // shift pieces into current
    m_CurrentPiece = m_NextPieces[0];
    int32_t lastIdx = CNT_NEXT_PIECES - 1;
    for (int32_t i = 0; i < lastIdx; i++) {
        m_NextPieces[i] = m_NextPieces[i + 1];
    }
    m_NextPieces[lastIdx] = Tetrimino(nextIdx);

    // reset T-Spin flag
    m_PerformedTSpin = false;
}


bool TetrisEngine::PerformMove(int32_t deltaX, int32_t deltaY)
{
    // try to move
    m_CurrentPiece.move(deltaX, deltaY);
    // if there is collision => move back
    if (CurrentPieceCollides()) {
        m_CurrentPiece.move(-deltaX, -deltaY);
        return false;
    }
    // moved successfully
    m_PerformedTSpin = false;
    return true;
}


bool TetrisEngine::PerformRotateLeft()
{
    // backup piece
    Tetrimino backup = m_CurrentPiece;
    // try all 5 wall kicks
    for (int wk = 0; wk <= 4; wk++) {
        m_CurrentPiece.rotateLeft(wk);
        if (!CurrentPieceCollides()) {
            if (wk > 0) debuglogAppend("RotLEFT wk=", wk);
            CheckForTSpinAfterRotate();
            return true;
        }
        // failed => restore backup
        m_CurrentPiece = backup;
    }
    // all wall kicks have failed
    debuglogAppend("RotLeft FAIL");
    return false;
}


bool TetrisEngine::PerformRotateRight()
{
    // backup piece
    Tetrimino backup = m_CurrentPiece;
    // try all 5 wall kicks
    for (int wk = 0; wk <= 4; wk++) {
        m_CurrentPiece.rotateRight(wk);
        if (!CurrentPieceCollides()) {
            if (wk > 0) debuglogAppend("RotRGHT wk=", wk);
            CheckForTSpinAfterRotate();
            return true;
        }
        // failed => restore backup
        m_CurrentPiece = backup;
    }
    // all wall kicks have failed
    debuglogAppend("RotRGHT FAIL");
    return false;
}


bool TetrisEngine::CheckKeyWithAutoRepeat(Key key, float fElapsedTime)
{
    // if key was pressed just now => init auto-repeat
    if (m_pPGE->GetKey(key).bPressed)
    {
        m_fAutoRepeatCountdown = (float) m_Settings.nDelayAutoRepeatMs / 1000.f;
        return true;
    }
    // check if key held
    if (m_pPGE->GetKey(key).bHeld)
    {
        // still held => auto-repeat mechanism
        m_fAutoRepeatCountdown -= fElapsedTime;
        if (m_fAutoRepeatCountdown <= 0.0)
        {
            m_fAutoRepeatCountdown += (float)m_Settings.nSpeedAutoRepeatMs / 1000.f;
            return true;
        }
    }
    // key neither pressed, nor held (auto-repeating)
    return false;
}


bool TetrisEngine::DoesPieceCollide(const Tetrimino& tetro)
{
    for (int i = 0; i < 4; i++)
    {
        int32_t tileX = tetro.getX(i);
        int32_t tileY = tetro.getY(i);
        // left/right edge collision check
        if (tileX < 0 || tileX >= TABLE_WIDTH_TILES)
            return true;
        // bottom edge collision check
        if (tileY >= TABLE_HEIGHT_TILES)
            return true;
        // board contents collision check
        if (GetBoardTile(tileX, tileY) >= 0)
            return true;
    }
    // if we got here, we found no collision
    return false;
}


void TetrisEngine::DrawGameScreen(float fElapsedTime, int32_t fadeLevel)
{
    DrawBoardContents();

    // Draw ghost piece
    if (m_Settings.bShowGhost)
    {
        Tetrimino ghostPiece = m_CurrentPiece;
        // drop ghost piece
        while (!DoesPieceCollide(ghostPiece))
            ghostPiece.move(0, 1);
        ghostPiece.move(0, -1);
        // draw only if the ghost Y position is different from the current piece
        if (ghostPiece.getY(0) != m_CurrentPiece.getY(0))
        {
            DrawTetriminoOnBoard(ghostPiece, 0);
        }
    }

    // Draw current piece
    DrawTetriminoOnBoard(m_CurrentPiece, fadeLevel);

    // Draw Score, Level, Lines
    m_pPGE->DrawString(28, 202, to_string(m_nScore), WHITE);
    m_pPGE->DrawString(28, 232, to_string(m_nLevel), WHITE);
    m_pPGE->DrawString(28, 262, to_string(m_nLines), WHITE);

    // Draw HOLD Tetrimino
    if (m_bIsPieceHeld)
        DrawTetriminoAnywhere(m_HeldPiece, 2, 58);

    // Draw NEXT Tetriminos
    for (int i = 0; i < CNT_NEXT_PIECES; i++) {
        DrawTetriminoAnywhere(m_NextPieces[i], 285, (2 * TILE_PIXELS + 8) * i + 52);
    }

    // TODO Draw special animations
    if (m_AnimationFlags > 0) {
        // build string
        string msg;
        msg.reserve(256);
        int lines = (m_AnimationFlags & ANIM_LINES_MASK);
        if (lines > 0) {
            msg.append(to_string(lines));
            msg.append(" Lines");
        }
        if ((m_AnimationFlags & ANIM_TSPIN) != 0) {
            msg.append(" Tspin");
        }
        if ((m_AnimationFlags & ANIM_FULL_CLEAR) != 0) {
            msg.append(" FullCLr");
        }
        // draw string
        int k = (int)(m_fAnimationTimer * 3) & 0x01;
        m_pPGE->FillRect(130, 282, 140, 12, VERY_DARK_GREY);
        m_pPGE->DrawString(134, 284, msg, (k == 0) ? CYAN : GREEN);
        // update time
        m_fAnimationTimer += fElapsedTime;
        if (m_fAnimationTimer > 5.0f) {
            m_AnimationFlags = 0;
            m_fAnimationTimer = 0.0f;
        }
    }

    // Draw LOG
    debuglogDraw(m_pPGE);
}


void TetrisEngine::DrawDroppedLines(float fElapsedTime)
{
    static bool drawStarted = false;

    // update time
    if (!drawStarted) {
        m_fCurrentTime = fElapsedTime;
        drawStarted = true;
    }
    else {
        m_fCurrentTime += fElapsedTime;
    }

    // animate fading lines
    if (m_fCurrentTime <= FULL_LINES_ANIMATION_DELAY) {
        int32_t fadeLevel = (int32_t)
            ((FULL_LINES_ANIMATION_DELAY - m_fCurrentTime) * FADE_OUT_STEPS / FULL_LINES_ANIMATION_DELAY);
        DrawBoardContents(fadeLevel);
        return;
    }

    // lines animation is FINISHED
    drawStarted = false;
    m_fCurrentTime = 0.0f;

    // remove full lines
    int32_t cntLines = (int32_t) m_LinesBeingDropped.size();
    m_AnimationFlags |= (cntLines & ANIM_LINES_MASK);
    debuglogAppend("LINES: ", cntLines);
    // update score according to lines
    m_nLines += cntLines;
    m_nScore += m_nLevel * FULL_LINE_SCORES[cntLines - 1];
    // update level & speed (level increases every 10 lines)
    m_nLevel = (m_nLines / 10) + m_Settings.nStartLevel;
    if (m_nLevel > MAX_LEVEL)
        m_fFallDuration = LEVEL_DROP_DELAY[MAX_LEVEL - 1];
    else
        m_fFallDuration = LEVEL_DROP_DELAY[m_nLevel - 1];
    // remove full lines
    for (auto line : m_LinesBeingDropped)
    {
        for (int32_t x = 0; x < TABLE_WIDTH_TILES; x++)
        {
            for (int32_t y = line; y > -EXTRA_HEIGHT_TILES; y--)
            {
                SetBoardTile(x, y, GetBoardTile(x, y - 1));
            }
        }
    }
    m_LinesBeingDropped.clear();

    // check for FULL CLEAR
    bool bFullClear = true;
    for (int32_t x = 0; x < TABLE_WIDTH_TILES; x++)
    {
        if (GetBoardTile(x, Y_LAST_LINE) >= 0) {
            bFullClear = false;
            break;
        }
    }
    if (bFullClear) {
        m_nScore += m_nLevel * FULL_CLEAR_SCORE;
        m_AnimationFlags |= ANIM_FULL_CLEAR;
        debuglogAppend("FULL CLEAR");
    }
}


void TetrisEngine::DrawBoardContents(int32_t fadeLevel)
{
    // Draw board contents
    for (int32_t y = 0; y < TABLE_HEIGHT_TILES; y++)
    {
        // check for fade level
        int32_t lineFadeLevel = FADE_OUT_STEPS;
        if (fadeLevel != FADE_OUT_STEPS)
        {
            bool fadeLine =
                find(m_LinesBeingDropped.begin(), m_LinesBeingDropped.end(), y) != m_LinesBeingDropped.end();
            lineFadeLevel = fadeLine ? fadeLevel : FADE_OUT_STEPS;
        }
        for (int32_t x = 0; x < TABLE_WIDTH_TILES; x++)
        {
            int32_t screenX = TABLE_START_X + x * TILE_PIXELS;
            int32_t screenY = TABLE_START_Y + y * TILE_PIXELS;
            int8_t colIdx = GetBoardTile(x, y);
            DrawTetroTile(screenX, screenY, colIdx, lineFadeLevel);
        }
    }
}


// Draw a Tetrimino tile
void TetrisEngine::DrawTetroTile(int screenX, int screenY, int8_t colorIndex, int32_t fadeLevel)
{
    int32_t ox = 0, oy = 0;
    if (colorIndex < 0)
    {
        // draw empty grid cell
        oy = (m_Settings.bShowGrid ? 1 : 0);
    }
    else
    {
        // draw colorful tile
        ox = 1 + (colorIndex % CNT_TETRIMINOS);
        oy = max(fadeLevel, 0);
        // if fade too large => empty cell instead
        if (oy > FADE_OUT_STEPS) {
            ox = 0, oy = (m_Settings.bShowGrid ? 1 : 0);
        }
    }

    m_pPGE->DrawPartialSprite(screenX, screenY, m_pTilesSprite,
                               ox * TILE_PIXELS, oy * TILE_PIXELS,
                               TILE_PIXELS, TILE_PIXELS, 1);
}


void TetrisEngine::DrawTetriminoOnBoard(const Tetrimino& tetro, int32_t fadeLevel)
{
    for (int i = 0; i < 4; i++) {
        // draw each tile ONLY if it fits on the board
        int tileX = tetro.getX(i);
        int tileY = tetro.getY(i);
        if (tileX >= 0 && tileX < TABLE_WIDTH_TILES && tileY >= 0 && tileY < TABLE_HEIGHT_TILES) {
            int32_t startX = TABLE_START_X + tileX * TILE_PIXELS;
            int32_t startY = TABLE_START_Y + tileY * TILE_PIXELS;
            DrawTetroTile(startX, startY, tetro.getTypeIndex(), fadeLevel);
        }
    }
}


void TetrisEngine::DrawTetriminoAnywhere(const Tetrimino& tetro, int screenX, int screenY)
{
    // adjust vertical offset for I tetro in NEXT and HOLD (so that it looks good)
    int32_t offsY = (tetro.getTypeChar() == 'I') ? (TILE_PIXELS / 2) : 0;
    for (int i = 0; i < 4; i++) {
        int32_t startX = screenX + tetro.getX(i) * TILE_PIXELS;
        int32_t startY = screenY + tetro.getY(i) * TILE_PIXELS + offsY;
        DrawTetroTile(startX, startY, tetro.getTypeIndex());
    }
}


int8_t TetrisEngine::GetBoardTile(int32_t tileX, int32_t tileY)
{
    if (tileX < 0 || tileX >= TABLE_WIDTH_TILES
        || tileY < (-EXTRA_HEIGHT_TILES) || tileY >= TABLE_HEIGHT_TILES) {
        return OUTSIDE_CELL;
    }
    int idx = (tileY + EXTRA_HEIGHT_TILES) * TABLE_WIDTH_TILES + tileX;
    return m_Board[idx];
}


void TetrisEngine::SetBoardTile(int32_t tileX, int32_t tileY, int8_t colorIndex)
{
    if (tileX >= 0 && tileX < TABLE_WIDTH_TILES
        && tileY >= (-EXTRA_HEIGHT_TILES) && tileY < TABLE_HEIGHT_TILES) {
        int idx = (tileY + EXTRA_HEIGHT_TILES) * TABLE_WIDTH_TILES + tileX;
        m_Board[idx] = colorIndex;
    }
}


void TetrisEngine::CheckForTSpinAfterRotate()
{
    int32_t cntOccupiedCorners = 0;
    if (m_CurrentPiece.getTypeChar() == 'T') {
        // piece was rotated => check if 3 of 4 corners are full
        int32_t xCenter = m_CurrentPiece.getX(2);
        int32_t yCenter = m_CurrentPiece.getY(2);
        for (int32_t i = 0; i <= 3; i++) {
            int32_t xDelta = ((i & 0x01) == 0) ? -1 : +1;
            int32_t yDelta = ((i & 0x02) == 0) ? -1 : +1;
            if (GetBoardTile(xCenter + xDelta, yCenter + yDelta) >= 0) {
                cntOccupiedCorners++;
            }
        }
    }
    m_PerformedTSpin = (cntOccupiedCorners >= 3);
}
