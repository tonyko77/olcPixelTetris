#include "olcPixelGameEngine.h"
#include "TetrisEngine.h"
#include "TetrisConstants.h"

#include <cstdint>
#include <string>
#include <cmath>

using namespace std;
using namespace olc;


/*
STILL TO DO:
    * nice animation effects:
        - on T-Spin
        - on Full Clear
    * Improve scoring mechanism (esp. Back2Back stuff)
        -> https://tetris.fandom.com/wiki/Scoring#Guideline_scoring_system
    * Help screen(s)

    * T-spin Simple/Double/Triple (what are THOSE??)
    + OTHERS from Tetris Guideline:
        https://tetris.fandom.com/wiki/Tetris_Guideline
        https://tetris.com/play-tetris

    * FIX TODOs
    * Add comments to methods

DONE, not sure if ok:
    * key auto-repeat (not fast enough on higher levels)
        => DECREASE times based on level ?!?
    * lock delay = 0.5 sec (if checked after move => locks too soon ?!)

(OPTIONAL) Sounds:
    * add (CREATE ??) SOUND LIB
    * Kobeiniki music
    * sound effects for:
        - on rotation
        - on movement
        - landing on surface
        - touching a wall
        - locking
        - line clear
        - game over.

(VERY OPTIONAL) Graphics Improvements:
    * Background screen PNG
    * Nicer menu screens: main menu, pause, options, game over
    * Custom Font

DONE !!!
    * NO LOCK if continuously moving
        => add a 5 sec overall delay
    * FULL CLEAR bonus
        - value = 1600 ???
        - TO TEST: hack RandomBag to generate only squares
    * Tetrimino start locations
        - The I and O spawn in the middle columns
        - The rest spawn in the left-middle columns
        - The tetriminoes spawn horizontally with J, L and T spawning flat-side first.
        - (!!) Spawn above playfield, row 21 for I, and 21/22 for all other tetriminoes.
        - (!!) Immediately drop one space if no existing Block is in its path
        - (!!) The player tops out when a piece is spawned overlapping at least one block (block out),
               or a piece locks completely above the visible portion of the playfield (lock out).
    * Improve scoring mechanism (esp. Back2Back stuff)
        -> https://tetris.fandom.com/wiki/Scoring#Guideline_scoring_system
    * volume option for sound effects (default = 50%)
    * animation effects:
        - while locking
        - on line(s) removed
*/



/////////////////////////////////////////////
// Constants
/////////////////////////////////////////////
namespace {
    // Some personalized pixel colors
    const Pixel
        MOST_DARK_GREY    (16, 16, 16),
        DARKER_BLUE       (0, 0, 96),
        ORANGE            (255, 172, 0),
        DARK_ORANGE       (128, 86, 0),
        VERY_DARK_ORANGE  (64, 43, 0);

    // Tile colors
    // O=YELLOW, I=CYAN, T=MAGENTA, S=GREEN, Z=RED, J=BLUE, L=ORANGE
    const Pixel TETRO_COLOR_LIGHT[CNT_TETRIMINOS] = {
        YELLOW, CYAN, MAGENTA, GREEN, RED, BLUE, ORANGE
    };
    const Pixel TETRO_COLOR_DARK[CNT_TETRIMINOS] = {
        DARK_YELLOW, DARK_CYAN, DARK_MAGENTA, DARK_GREEN, DARK_RED, DARK_BLUE, DARK_ORANGE
    };
    const Pixel TETRO_COLOR_VERYDARK[CNT_TETRIMINOS] = {
        VERY_DARK_YELLOW, VERY_DARK_CYAN, VERY_DARK_MAGENTA, VERY_DARK_GREEN, VERY_DARK_RED, DARKER_BLUE, VERY_DARK_ORANGE
    };

    // Game State Values
    enum class GameState {
        GAME_MAIN_MENU,
        GAME_PAUSE_MENU,
        GAME_OPTIONS_MENU,
        GAME_SET_KEYS,
        GAME_OVER_MENU,
        GAME_RESUME_COUNTDOWN,
        GAME_HELP_SCREEN,
        GAME_RUNNING
    };

    // Key names
    const map<Key, string> KEY_LABELS = {
        { NONE, "NONE" },
        {A, "A"}, {B, "B"}, {C, "C"}, {D, "D"}, {E, "E"}, {F, "F"}, {G, "G"}, {H, "H"},
        {I, "I"}, {J, "J"}, {K, "K"}, {L, "L"}, {M, "M"}, {N, "N"}, {O, "O"}, {P, "P"},
        {Q, "Q"}, {R, "R"}, {S, "S"}, {T, "T"}, {U, "U"}, {V, "V"}, {W, "W"}, {X, "X"},
        {Y, "Y"}, {Z, "Z"},
        {K0, "0"}, {K1, "1"}, {K2, "2"}, {K3, "3"}, {K4, "4"},
        {K5, "5"}, {K6, "6"}, {K7, "7"}, {K8, "8"}, {K9, "9"},
        {F1, "F1"}, {F2, "F2"}, {F3, "F3"}, {F4, "F4"}, {F5, "F5"}, {F6, "F6"},
        {F7, "F7"}, {F8, "F8"}, {F9, "F9"}, {F10, "F10"}, {F11, "F11"}, {F12, "F12"},
        {UP, "Up"}, {DOWN, "Down"}, {LEFT, "Left"}, {RIGHT, "Right"},
        {SPACE, "Space"}, {TAB, "Tab"}, {SHIFT, "Shift"}, {CTRL, "Ctrl"},
        {INS, "Ins"}, {DEL, "Del"}, {HOME, "Home"}, {END, "End"},
        {PGUP, "PgUp"}, {PGDN, "PgDn"}, {BACK, "Bksp"}, {ESCAPE, "Esc"},
        {RETURN, "Return"}, {ENTER, "Enter"}, {PAUSE, "Pause"}, {SCROLL, "ScrLk"},
        {NP0, "NP 0"}, {NP1, "NP 1"}, {NP2, "NP 2"}, {NP3, "NP 3"},
        {NP4, "NP 4"}, {NP5, "NP 5"}, {NP6, "NP 6"}, {NP7, "NP 7"},
        {NP8, "NP 8"}, {NP9, "NP 9"}, {NP_MUL, "NP *"}, {NP_DIV, "NP /"},
        {NP_ADD, "NP +"}, {NP_SUB, "NP -"}, {NP_DECIMAL, "NP ."}
    };

    // Maximum high score entries
    const int32_t MAX_HIGH_SCORES = 6;

    // Auto-repeat adjustment values
    const int32_t AUTO_REPEAT_MIN = 10;
    const int32_t AUTO_REPEAT_MAX = 500;

    // Constants for redefine keys
    enum {
        KEY_MOVE_LEFT, KEY_MOVE_RIGHT,
        KEY_ROT_LEFT, KEY_ROT_RIGHT,
        KEY_SOFT_DROP,
        KEY_HARD_DROP,
        KEY_HOLD,
        NUM_KEYS
    };
    const char* KEY_NAMES[NUM_KEYS] = {
        "Move Left", "Move Right",
        "Rotate Left", "Rotate Right",
        "Soft Drop", "Hard Drop", "Hold"
    };

    // Delay between the 3-2-1 steps in the resume screen
    const float RESUME_STEP_DELAY = 0.6f;

} // Constants namespace



/////////////////////////////////////////////
//  TETRIS GAME ENGINE
/////////////////////////////////////////////

struct HighScore
{
    int32_t score;
    bool isUserScore;

    HighScore()
        : score(0), isUserScore(false)
    {}
};


class TetrisGame : public olc::PixelGameEngine
{
private:
    // The Game
    TetrisEngine* m_pTetris;

    // Settings
    TetrisSettings m_Settings;

    // Game State flags
    GameState m_nGameState;

    // Game Over scores
    int32_t m_nGameOverScore;
    int32_t m_nGameOverLevel;
    int32_t m_nGameOverLines;

    // Sprites
    Sprite* m_pBackgroundSprite;
    Sprite* m_pTilesSprite;

    // High Scores
    HighScore m_HighScores[MAX_HIGH_SCORES];

public:
    TetrisGame()
        : m_pTetris(nullptr)
        , m_Settings()
        , m_nGameState(GameState::GAME_MAIN_MENU)
        , m_nGameOverScore(0)
        , m_nGameOverLevel(0)
        , m_nGameOverLines(0)
        , m_pBackgroundSprite(nullptr)
        , m_pTilesSprite(nullptr)
        , m_HighScores {}
    {}

    bool OnUserCreate() override
    {
        // prepare background Sprite
        m_pBackgroundSprite = new Sprite(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS);
        SetDrawTarget(m_pBackgroundSprite);
        FillRect(0, 0, SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS, VERY_DARK_BLUE);
        // draw board
        DrawFrame(TABLE_START_X, TABLE_START_Y, TABLE_WIDTH_PIXELS, TABLE_HEIGHT_PIXELS);
        FillRect(TABLE_START_X, TABLE_START_Y, TABLE_WIDTH_PIXELS, TABLE_HEIGHT_PIXELS, BLACK);
        // draw HOLD area
        DrawFrame(20, 20, 80, 80);
        FillRect(20, 20, 80, 80, VERY_DARK_GREY);
        DrawString(45, 28, "HOLD", YELLOW);
        FillRect(25, 45, 70, 50, BLACK);
        // draw SCORE, LEVEL, LINES area
        DrawFrame(20, 180, 80, 100);
        FillRect(20, 180, 80, 100, VERY_DARK_GREY);
        DrawString(32, 190, "SCORE", YELLOW);
        DrawString(32, 220, "LEVEL", YELLOW);
        DrawString(32, 250, "LINES", YELLOW);
        FillRect(25, 200, 70, 12, BLACK);
        FillRect(25, 230, 70, 12, BLACK);
        FillRect(25, 260, 70, 12, BLACK);
        // draw NEXT area
        DrawFrame(300, 20, 80, 130);
        FillRect(300, 20, 80, 130, VERY_DARK_GREY);
        DrawString(325, 28, "NEXT", YELLOW);
        FillRect(305, 45, 70, 100, BLACK);
        // draw KEYS area
        DrawFrame(290, 180, 100, 110);
        FillRect(290, 180, 100, 110, BLACK);
        DrawString(294, 184, " Left:", WHITE);
        DrawString(294, 197, "Right:", WHITE);
        DrawString(294, 210, "Rot.L:", WHITE);
        DrawString(294, 223, "Rot.R:", WHITE);
        DrawString(294, 236, " Soft:", WHITE);
        DrawString(294, 249, " Hard:", WHITE);
        DrawString(294, 262, " Hold:", WHITE);
        DrawString(294, 275, "Pause:", WHITE);
        DrawString(344, 275, "<Esc>", GREEN);

        // prepare TILES Sprite
        m_pTilesSprite = new Sprite(TILE_PIXELS * 8, TILE_PIXELS * (1 + FADE_OUT_STEPS));
        SetDrawTarget(m_pTilesSprite);
        // draw empty cells
        FillRect(0, 0, TILE_PIXELS, 2 * TILE_PIXELS, BLACK);
        DrawLine(0, TILE_PIXELS, TILE_PIXELS, TILE_PIXELS, MOST_DARK_GREY);
        DrawLine(0, TILE_PIXELS, 0, 2 * TILE_PIXELS, MOST_DARK_GREY);
        // draw color tiles
        auto FadeColor = [&] (uint8_t& color, int32_t k) {
            int32_t faded = (int32_t)color * (k + 4) / (FADE_OUT_STEPS + 4);
            color = (uint8_t)(faded & 0xFF);
        };
        const int a = 2, b = 6;
        for (int32_t c = 0; c < CNT_TETRIMINOS; c++) {
            int32_t x = (c + 1) * TILE_PIXELS;
            // draw ghost tile
            const Pixel& pixGhost = TETRO_COLOR_VERYDARK[c];
            DrawRect(x, 0, TILE_PIXELS, TILE_PIXELS, pixGhost);
            // draw normal/faded tiles
            for (int32_t k = 0; k < FADE_OUT_STEPS; k++) {
                int32_t y = (k + 1) * TILE_PIXELS;
                Pixel pixFill = TETRO_COLOR_LIGHT[c];
                Pixel pixOutl = TETRO_COLOR_DARK[c];
                DrawRect(x, y, TILE_PIXELS, TILE_PIXELS, pixOutl);
                FadeColor(pixFill.r, k);
                FadeColor(pixFill.g, k);
                FadeColor(pixFill.b, k);
                FadeColor(pixOutl.r, k);
                FadeColor(pixOutl.g, k);
                FadeColor(pixOutl.b, k);
                FillRect(x+1, y+1, TILE_PIXELS-2, TILE_PIXELS-2, pixFill);
                DrawLine(x + a, y + a, x + a, y + b, pixOutl);
                DrawLine(x + a, y + a, x + b, y + a, pixOutl);
            }
        }
        SetDrawTarget(nullptr);

        // init high scores
        for (int32_t i = 0; i < MAX_HIGH_SCORES; i++) {
            m_HighScores[i].score = 1000 * (MAX_HIGH_SCORES - i);
            m_HighScores[i].isUserScore = false;
        }
        // init state
        m_pTetris = nullptr;
        m_nGameState = GameState::GAME_MAIN_MENU;
        m_nGameOverScore = m_nGameOverLevel = m_nGameOverLines = 0;
        ResetOptions();

        return true;
    }


    bool OnUserDestroy() override
    {
        delete m_pTilesSprite;
        delete m_pBackgroundSprite;
        return true;
    }


    bool OnUserUpdate(float fElapsedTime) override
    {
        // sleep a bit, so we don't hog the CPU/GPU
        //std::this_thread::sleep_for(std::chrono::microseconds(10));

        // Draw background
        DrawSprite(0, 0, m_pBackgroundSprite, 1);
        // Draw keys
        DrawString(344, 184, KEY_LABELS.at(m_Settings.keyMoveLeft), GREEN);
        DrawString(344, 197, KEY_LABELS.at(m_Settings.keyMoveRight), GREEN);
        DrawString(344, 210, KEY_LABELS.at(m_Settings.keyRotLeft), GREEN);
        DrawString(344, 223, KEY_LABELS.at(m_Settings.keyRotRight), GREEN);
        DrawString(344, 236, KEY_LABELS.at(m_Settings.keySoftDrop), GREEN);
        DrawString(344, 249, KEY_LABELS.at(m_Settings.keyHardDrop), GREEN);
        DrawString(344, 262, KEY_LABELS.at(m_Settings.keyHold), GREEN);

        // perform update based on current state
        bool bRetValue = true;
        switch (m_nGameState)
        {
        case GameState::GAME_MAIN_MENU:
            bRetValue = UpdateMainMenu();
            break;
        case GameState::GAME_PAUSE_MENU:
            UpdatePauseMenu();
            break;
        case GameState::GAME_OPTIONS_MENU:
            UpdateOptionsMenu();
            break;
        case GameState::GAME_SET_KEYS:
            UpdateSetKeys();
            break;
        case GameState::GAME_OVER_MENU:
            UpdateGameOver();
            break;
        case GameState::GAME_RESUME_COUNTDOWN:
            UpdateResumeCountdown(fElapsedTime);
            break;
        case GameState::GAME_HELP_SCREEN:
            UpdateHelpScreen();
            break;
        case GameState::GAME_RUNNING:
            UpdateGameIsRunning(fElapsedTime);
            break;
        default:
            // OOPS unknown state
            m_nGameState = GameState::GAME_MAIN_MENU;
        }
        return bRetValue;
    }


private:
    void DrawMenuEntries(int x, int y, std::initializer_list<const char*> strings, int32_t ySpacing = 14)
    {
        char letter[2] = {0, 0};
        for (auto str : strings) {
            letter[0] = *str;
            DrawString(x - 1, y - 1, letter, DARK_GREEN);
            DrawString(x    , y    , letter, GREEN);
            DrawString(x + 8, y    , str + 1, WHITE);
            y += ySpacing;
        }
    }

    // Game loop for MAIN MENU
    bool UpdateMainMenu()
    {
        static bool bExitConfirmation = false;

        // clean up any left-over game
        if (m_pTetris != nullptr) {
            delete m_pTetris;
            m_pTetris = nullptr;
        }

        // Draw main menu
        DrawString(TABLE_START_X +  9, TABLE_START_Y + 10, "T", DARK_RED, 2);
        DrawString(TABLE_START_X + 10, TABLE_START_Y + 11, "T", RED, 2);
        DrawString(TABLE_START_X + 26, TABLE_START_Y + 10, "E", DARK_ORANGE, 2);
        DrawString(TABLE_START_X + 27, TABLE_START_Y + 11, "E", ORANGE, 2);
        DrawString(TABLE_START_X + 43, TABLE_START_Y + 10, "T", DARK_YELLOW, 2);
        DrawString(TABLE_START_X + 44, TABLE_START_Y + 11, "T", YELLOW, 2);
        DrawString(TABLE_START_X + 60, TABLE_START_Y + 10, "R", DARK_GREEN, 2);
        DrawString(TABLE_START_X + 61, TABLE_START_Y + 11, "R", GREEN, 2);
        DrawString(TABLE_START_X + 77, TABLE_START_Y + 10, "I", DARK_CYAN, 2);
        DrawString(TABLE_START_X + 78, TABLE_START_Y + 11, "I", CYAN, 2);
        DrawString(TABLE_START_X + 94, TABLE_START_Y + 10, "S", DARK_MAGENTA, 2);
        DrawString(TABLE_START_X + 95, TABLE_START_Y + 11, "S", MAGENTA, 2);
        // Draw menu entries
        DrawMenuEntries(TABLE_START_X + 11, TABLE_START_Y + 51,
                        {"Start game", "Level:", "Options", "How to play", "Exit game"});
        DrawString(TABLE_START_X + 64, TABLE_START_Y + 65, to_string(m_Settings.nStartLevel), YELLOW);
        // Draw high scores
        DisplayHighScores();

        // Exit confirmation ?
        if (bExitConfirmation)
        {
            ShowConfirmationDialog("Exit Game");
            if (GetKey(Key::Y).bPressed) {
                return false;
            }
            if (GetKey(Key::N).bPressed) {
                bExitConfirmation = false;
            }
            return true;
        }

        // Check keys
        if (GetKey(Key::S).bPressed) {
            // start game
            m_nGameState = GameState::GAME_RESUME_COUNTDOWN;
        }
        else if (GetKey(Key::L).bPressed) {
            // change level
            m_Settings.nStartLevel = (m_Settings.nStartLevel / 5 + 1) * 5;
            if (m_Settings.nStartLevel > MAX_LEVEL)
                m_Settings.nStartLevel = 1;
        }
        else if (GetKey(Key::O).bPressed) {
            // Options
            m_nGameState = GameState::GAME_OPTIONS_MENU;
        }
        else if (GetKey(Key::H).bPressed) {
            // Help / How to play
            m_nGameState = GameState::GAME_HELP_SCREEN;
        }
        else if (GetKey(Key::E).bPressed || GetKey(Key::ESCAPE).bPressed) {
            // Exit
            bExitConfirmation = true;
        }
        return true;
    }


    void DisplayHighScores()
    {
        DrawString(TABLE_START_X + 16, TABLE_START_Y + 140, "HIGH SCORES", DARK_CYAN);
        DrawString(TABLE_START_X + 17, TABLE_START_Y + 141, "HIGH SCORES", CYAN);
        for (int32_t i = 0; i < MAX_HIGH_SCORES; i++) {
            int32_t x = TABLE_START_X + 20;
            int32_t y = TABLE_START_Y + 156 + 12*i;
            const Pixel& bgColor = ((i & 0x01) == 0) ? MOST_DARK_GREY : VERY_DARK_GREY;
            FillRect(x, y, TABLE_WIDTH_PIXELS - 40, 12, bgColor);
            string score = to_string(m_HighScores[i].score);
            int32_t scoreWidth = 8 * (int32_t)score.size();
            DrawString(x + TABLE_WIDTH_PIXELS - 42 - scoreWidth, y + 2,
                       score, (m_HighScores[i].isUserScore ? GREEN : WHITE));
        }
    }

    void DrawStringCenter(int32_t y, const string& str, const Pixel& color, int32_t scale = 1)
    {
        int32_t x = TABLE_START_X + (TABLE_WIDTH_PIXELS / 2) - 4 * scale * (int32_t)str.size();
        DrawString(x, y, str, color, scale);
    }

    void DrawStringRight(int32_t x, int32_t y, const string& str, const Pixel& color, int32_t scale = 1)
    {
        x -= 8 * scale * (int32_t)str.size();
        DrawString(x, y, str, color, scale);
    }

    // Game loop for PAUSE MENU
    void UpdatePauseMenu()
    {
        static bool bQuitConfirmation = false;

        // draw PAUSE menu
        DrawString(TABLE_START_X + 10, TABLE_START_Y + 5, "PAUSED", DARK_YELLOW, 2);
        DrawString(TABLE_START_X + 11, TABLE_START_Y + 6, "PAUSED", YELLOW, 2);
        DrawMenuEntries(TABLE_START_X + 11, TABLE_START_Y + 50,
                        {"Resume", "Options", "How to play", "Quit game"}, 20);

        // Quit Game confirmation ?
        if (bQuitConfirmation)
        {
            ShowConfirmationDialog("Quit Game");
            if (GetKey(Key::Y).bPressed) {
                m_nGameState = GameState::GAME_MAIN_MENU;
                bQuitConfirmation = false;
            }
            if (GetKey(Key::N).bPressed) {
                bQuitConfirmation = false;
            }
            return;
        }

        // check keys
        if (GetKey(Key::R).bPressed)
        {
            m_nGameState = GameState::GAME_RESUME_COUNTDOWN;
        }
        else if (GetKey(Key::O).bPressed)
        {
            // Options (during game)
            m_nGameState = GameState::GAME_OPTIONS_MENU;
        }
        else if (GetKey(Key::H).bPressed)
        {
            // Help / How to play
            m_nGameState = GameState::GAME_HELP_SCREEN;
        }
        else if (GetKey(Key::Q).bPressed)
        {
            // Quit Game
            bQuitConfirmation = true;
        }
    }


    // Game loop for OPTIONS MENU
    void UpdateOptionsMenu()
    {
        static bool bResetConfirmation = false;

        auto DecreaseValue = [&](int32_t& value, int32_t minVal)
        {
            value -= 10;
            if (value < minVal) value = minVal;
        };

        auto IncreaseValue = [&](int32_t& value, int32_t maxVal)
        {
            value += 10;
            if (value > maxVal) value = maxVal;
        };

        // Draw OPTIONS menu
        DrawString(TABLE_START_X + 4, TABLE_START_Y + 7, "OPTIONS", DARK_ORANGE, 2);
        DrawString(TABLE_START_X + 5, TABLE_START_Y + 8, "OPTIONS", ORANGE, 2);
        DrawMenuEntries(TABLE_START_X + 4, TABLE_START_Y + 40,
                        {"Keys ...", "Table grid:", "Drop ghost:"});
        DrawMenuEntries(TABLE_START_X + 8, TABLE_START_Y + 212,
                        {"Reset options", "Finished"});
        // Key auto-repeat
        DrawString(TABLE_START_X + 2, TABLE_START_Y + 100, "Key auto-repeat", CYAN);
        DrawString(TABLE_START_X + 2, TABLE_START_Y + 110, "(milliseconds)", CYAN);
        DrawString(TABLE_START_X +  2, TABLE_START_Y + 124, "(Q/W)", GREEN);
        DrawString(TABLE_START_X + 44, TABLE_START_Y + 124, "Delay:", WHITE);
        DrawString(TABLE_START_X +  2, TABLE_START_Y + 136, "(A/S)", GREEN);
        DrawString(TABLE_START_X + 44, TABLE_START_Y + 136, "Speed:", WHITE);
        // Sound volumes
        DrawString(TABLE_START_X + 2, TABLE_START_Y + 160, "Audio volume %", CYAN);
        DrawString(TABLE_START_X +  2, TABLE_START_Y + 174, "(Z/X)", GREEN);
        DrawString(TABLE_START_X + 44, TABLE_START_Y + 174, "Music:", WHITE);
        DrawString(TABLE_START_X +  2, TABLE_START_Y + 186, "(C/V)", GREEN);
        DrawString(TABLE_START_X + 44, TABLE_START_Y + 186, "   FX:", WHITE);

        // Draw option values
        DrawString(TABLE_START_X + 94, TABLE_START_Y + 54, (m_Settings.bShowGrid ? "ON" : "off"), YELLOW);
        DrawString(TABLE_START_X + 94, TABLE_START_Y + 68, (m_Settings.bShowGhost ? "ON" : "off"), YELLOW);
        DrawString(TABLE_START_X + 94, TABLE_START_Y + 124, to_string(m_Settings.nDelayAutoRepeatMs), YELLOW);
        DrawString(TABLE_START_X + 94, TABLE_START_Y + 136, to_string(m_Settings.nSpeedAutoRepeatMs), YELLOW);
        DrawString(TABLE_START_X + 94, TABLE_START_Y + 174, to_string(m_Settings.nMusicVolume), YELLOW);
        DrawString(TABLE_START_X + 94, TABLE_START_Y + 186, to_string(m_Settings.nFxVolume), YELLOW);

        // Reset Options confirmation ?
        if (bResetConfirmation)
        {
            ShowConfirmationDialog("Reset Options");
            if (GetKey(Key::Y).bPressed) {
                ResetOptions();
                bResetConfirmation = false;
            }
            if (GetKey(Key::N).bPressed) {
                bResetConfirmation = false;
            }
            return;
        }

        // Check keys
        if (GetKey(Key::K).bPressed) {
            // redefine keys
            m_nGameState = GameState::GAME_SET_KEYS;
        }
        else if (GetKey(Key::T).bPressed) {
            // table grid ON/OFF
            m_Settings.bShowGrid = !m_Settings.bShowGrid;
        }
        else if (GetKey(Key::D).bPressed) {
            // drop hint ON/OFF
            m_Settings.bShowGhost = !m_Settings.bShowGhost;
        }

        else if (GetKey(Key::Q).bPressed) {
            // delay -= 10
            DecreaseValue(m_Settings.nDelayAutoRepeatMs, AUTO_REPEAT_MIN);
        }
        else if (GetKey(Key::W).bPressed) {
            // delay += 10
            IncreaseValue(m_Settings.nDelayAutoRepeatMs, AUTO_REPEAT_MAX);
        }
        else if (GetKey(Key::A).bPressed) {
            // speed -= 10
            DecreaseValue(m_Settings.nSpeedAutoRepeatMs, AUTO_REPEAT_MIN);
        }
        else if (GetKey(Key::S).bPressed) {
            // speed += 10
            IncreaseValue(m_Settings.nSpeedAutoRepeatMs, AUTO_REPEAT_MAX);
        }

        else if (GetKey(Key::Z).bPressed) {
            // music volume -= 10 %
            DecreaseValue(m_Settings.nMusicVolume, 0);
        }
        else if (GetKey(Key::X).bPressed) {
            // music volume += 10 %
            IncreaseValue(m_Settings.nMusicVolume, 100);
        }
        else if (GetKey(Key::C).bPressed) {
            // FX volume -= 10 %
            DecreaseValue(m_Settings.nFxVolume, 0);
        }
        else if (GetKey(Key::V).bPressed) {
            // FX volume += 10 %
            IncreaseValue(m_Settings.nFxVolume, 100);
        }

        else if (GetKey(Key::R).bPressed) {
            // reset options
            bResetConfirmation = true;
        }
        else if (GetKey(Key::F).bPressed || GetKey(Key::ESCAPE).bPressed) {
            // exit menu
            m_nGameState = (m_pTetris == nullptr) ? GameState::GAME_MAIN_MENU : GameState::GAME_RESUME_COUNTDOWN;
        }
    }


    void UpdateSetKeys()
    {
        static int32_t currentIndex = 0;
        static Key definedKeys[NUM_KEYS];

        // Upon <Esc> => return
        if (GetKey(Key::ESCAPE).bPressed) {
            // exit menu
            currentIndex = 0;
            m_nGameState = GameState::GAME_OPTIONS_MENU;
            return;
        }
        // safety net for buffer overrun
        if (currentIndex < 0 || currentIndex >= NUM_KEYS) {
            currentIndex = 0;
        }

        // Draw "Define Keys" screen
        DrawString(TABLE_START_X + 8, TABLE_START_Y + 7, "DEFINE", DARK_MAGENTA, 2);
        DrawString(TABLE_START_X + 9, TABLE_START_Y + 8, "DEFINE", MAGENTA, 2);
        DrawString(TABLE_START_X + 32, TABLE_START_Y + 27, "KEYS", DARK_MAGENTA, 2);
        DrawString(TABLE_START_X + 33, TABLE_START_Y + 28, "KEYS", MAGENTA, 2);
        for (int32_t i = 0; i <= currentIndex; i++) {
            DrawStringCenter(TABLE_START_Y + 60 + 24*i, KEY_NAMES[i], GREY);
            string keyValue = (i == currentIndex) ? "..." : KEY_LABELS.at(definedKeys[i]);
            DrawStringCenter(TABLE_START_Y + 70 + 24*i, keyValue, CYAN);
        }

        // get pressed key
        Key pressedKey = Key::NONE;
        for (int idx = 1; idx <= (int)(Key::NP_DECIMAL); idx++) {
            Key key = (Key)idx;
            if (GetKey(key).bPressed) {
                pressedKey = key;
                break;
            }
        }
        // store pressed key
        if (pressedKey != Key::NONE) {
            // avoid key repeats
            bool isRepeated = false;
            for (int i = 0; i < currentIndex; i++) {
                isRepeated |= (pressedKey == definedKeys[i]);
            }
            if (!isRepeated) {
                definedKeys[currentIndex] = pressedKey;
                currentIndex++;
            }
        }

        // check if done
        if (currentIndex >= NUM_KEYS) {
            // store keys
            m_Settings.keyMoveLeft = definedKeys[0];
            m_Settings.keyMoveRight = definedKeys[1];
            m_Settings.keyRotLeft = definedKeys[2];
            m_Settings.keyRotRight = definedKeys[3];
            m_Settings.keySoftDrop = definedKeys[4];
            m_Settings.keyHardDrop = definedKeys[5];
            m_Settings.keyHold = definedKeys[6];
            // exit menu
            currentIndex = 0;
            m_nGameState = GameState::GAME_OPTIONS_MENU;
        }
    }


    void UpdateHelpScreen()
    {
        DrawString(TABLE_START_X, TABLE_START_Y, "[TODO] HELP SCREEN");
        if (GetKey(Key::ESCAPE).bPressed) {
            if (m_pTetris == nullptr) {
                m_nGameState = GameState::GAME_MAIN_MENU;
            }
            else {
                m_nGameState = GameState::GAME_RESUME_COUNTDOWN;
            }
        }
    }


    // Game loop for 3-2-1 resume countdown
    void UpdateResumeCountdown(float fElapsedTime)
    {
        static float internalTimer = -1.0f;
        static char str[2] = {0, 0};

        // update time
        if (internalTimer < 0.0f) {
            internalTimer = fElapsedTime;
        }
        else {
            internalTimer += fElapsedTime;
        }

        // compute 3-2-1
        int8_t seconds = 3 - (int8_t) floor(internalTimer / RESUME_STEP_DELAY);
        if (seconds > 0) {
            // display timer
            str[0] = '0' + seconds;
            DrawString(TABLE_START_X + 32, TABLE_START_Y + 60, str, WHITE, 8);
        }
        else {
            // countdown finished
            internalTimer = -1.0f;
            m_nGameState = GameState::GAME_RUNNING;
        }
    }


    // Game loop for RUNNING GAME
    void UpdateGameIsRunning(float fElapsedTime)
    {
        bool isGameOver = false;

        // init game if not yet initialized
        if (m_pTetris == nullptr) {
            m_pTetris = new TetrisEngine(this, m_pTilesSprite, m_Settings);
        }

        // check for pause key OR lost focus
        if (GetKey(Key::ESCAPE).bPressed || !IsFocused()) {
            m_nGameState = GameState::GAME_PAUSE_MENU;
        }
        else {
            // run game loop
            m_pTetris->UpdateGame(fElapsedTime);
            isGameOver = m_pTetris->IsGameOver();
        }

        // handle game over
        if (isGameOver) {
            m_nGameOverScore = m_pTetris->GetScore();
            m_nGameOverLevel = m_pTetris->GetLevel();
            m_nGameOverLines = m_pTetris->GetLines();
            m_nGameState = GameState::GAME_OVER_MENU;
            // clean up game
            delete m_pTetris;
            m_pTetris = nullptr;
            // update high scores
            int32_t highScoreIdx = -1;
            for (int32_t i = 0; i < MAX_HIGH_SCORES; i++) {
                if (m_nGameOverScore >= m_HighScores[i].score) {
                    highScoreIdx = i;
                    break;
                }
            }
            if (highScoreIdx >= 0) {
                for (int32_t i = MAX_HIGH_SCORES-1; i > highScoreIdx; i--) {
                    m_HighScores[i] = m_HighScores[i - 1];
                }
                m_HighScores[highScoreIdx].isUserScore = true;
                m_HighScores[highScoreIdx].score = m_nGameOverScore;
            }
        }
    }


    // Game loop for GAME OVER
    void UpdateGameOver()
    {
        // Draw GAME OVER screen
        DrawString(TABLE_START_X + 28, 36, "GAME", DARK_RED, 2);
        DrawString(TABLE_START_X + 29, 37, "GAME", RED, 2);
        DrawString(TABLE_START_X + 28, 56, "OVER", DARK_RED, 2);
        DrawString(TABLE_START_X + 29, 57, "OVER", RED, 2);
        string score = to_string(m_nGameOverScore);
        string level = to_string(m_nGameOverLevel);
        string lines = to_string(m_nGameOverLines);
        DrawString(TABLE_START_X + TABLE_WIDTH_PIXELS/2 - 8 * (int32_t)score.size(), 80, score, CYAN, 2);
        DrawString(TABLE_START_X + 30, 104, "Level:", WHITE);
        DrawString(TABLE_START_X + 80, 104, level, GREEN);
        DrawString(TABLE_START_X + 30, 118, "Lines:", WHITE);
        DrawString(TABLE_START_X + 80, 118, lines, GREEN);
        // Draw menu entries
        FillRect(TABLE_START_X + 5, 132, TABLE_WIDTH_PIXELS - 10, 30, VERY_DARK_BLUE);
        DrawMenuEntries(TABLE_START_X + 11, 137, {"Restart game", "Main menu"}, 12);
        // Draw high scores
        DisplayHighScores();

        // check menu keys
        if (GetKey(Key::R).bPressed) {
            m_nGameState = GameState::GAME_RESUME_COUNTDOWN;
        }
        else if (GetKey(Key::M).bPressed) {
            m_nGameState = GameState::GAME_MAIN_MENU;
        }
    }


    // Draw a nice frame around the board and other places
    void DrawFrame(int32_t x, int32_t y, int32_t w, int32_t h)
    {
        DrawRect(x - 1, y - 1, w + 2, h + 2, MOST_DARK_GREY);
        DrawRect(x - 2, y - 2, w + 4, h + 4, VERY_DARK_GREY);
        DrawRect(x - 3, y - 3, w + 6, h + 6, DARK_GREY);
        DrawRect(x - 4, y - 4, w + 8, h + 8, GREY);
        DrawRect(x - 5, y - 5, w + 10, h + 10, WHITE);
    }


    void ResetOptions()
    {
        m_Settings.keyMoveLeft = Key::LEFT;
        m_Settings.keyMoveRight = Key::RIGHT;
        m_Settings.keyRotLeft = Key::Z;
        m_Settings.keyRotRight = Key::UP;
        m_Settings.keySoftDrop = Key::DOWN;
        m_Settings.keyHardDrop = Key::SPACE;
        m_Settings.keyHold = Key::C;
        m_Settings.bShowGrid = true;
        m_Settings.bShowGhost = true;
        m_Settings.nDelayAutoRepeatMs = 170;
        m_Settings.nSpeedAutoRepeatMs = 50;
        m_Settings.nStartLevel = 1;
        m_Settings.nMusicVolume = 50;
        m_Settings.nFxVolume = 50;
    }


    void ShowConfirmationDialog(const string& title)
    {
        DrawFrame(110, 100, 180, 50);
        FillRect(110, 100, 180, 50, VERY_DARK_RED);
        DrawStringCenter(110, title, GREEN);
        DrawStringCenter(130, "Are you sure? (Y/N)", WHITE);
    }

}; // class TetrisGame



/////////////////////////////////////////////

int main()
{
    TetrisGame game;
    game.sAppName = "Toni's Simple Tetris";

    bool gameOK = game.Construct(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS,
                                 SCREEN_PIXEL_SIZE, SCREEN_PIXEL_SIZE);
    if (gameOK) {
        game.Start();
    }
    else {
        cout << "FAILED to start game !!!" << endl;
    }

    return 0;
}
