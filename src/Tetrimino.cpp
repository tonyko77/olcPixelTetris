#include "Tetrimino.h"
#include "TetrisConstants.h"


struct TetroConstData {
    int32_t m_nRotationTransformer;
    int32_t m_xStartPos, m_yStartPos;
    int32_t m_RotRightWallKick[32];
    int32_t m_RotLeftWallKick[32];
};


// Constants
namespace {
    const char* TYPE_CHARS = "OITSZJL";

    const char* TETRO_STR[] =
    {
        ".... .##. .##.",   // O (square)
        ".... #### ....",   // I (line)
        ".#.. ###. ....",   // T
        ".##. ##.. ....",   // S
        "##.. .##. ....",   // Z
        "#... ###. ....",   // J
        "..#. ###. ...."    // L
    };

    // static data for 'I' and 'O'
    const TetroConstData FOUR_DATA = {
        3, 3, -1,
        // right wall kick data
        {
            -2, 0,   +1, 0,   -2, +1,   +1, -2,
            -1, 0,   +2, 0,   -1, -2,   +2, +1,
            +2, 0,   -1, 0,   +2, -1,   -1, +2,
            +1, 0,   -2, 0,   +1, +2,   -2, -1
        },
        // left wall kick data
        {
            -1, 0,   +2, 0,   -1, -2,   +2, +1,
            +2, 0,   -1, 0,   +2, -1,   -1, +2,
            +1, 0,   -2, 0,   +1, +2,   -2, -1,
            -2, 0,   +1, 0,   -2, +1,   +1, -2
        }
    };

    // static data for J, L, S, T, Z
    const TetroConstData THREE_DATA = {
        2, 3, 0,
        // right wall kick data
        {
            -1, 0,   -1, -1,   0, +2,   -1, +2,
            +1, 0,   +1, +1,   0, -2,   +1, -2,
            +1, 0,   +1, -1,   0, +2,   +1, +2,
            -1, 0,   -1, +1,   0, -2,   -1, -2
        },
        // left wall kick data
        {
            +1, 0,   +1, -1,   0, +2,   +1, +2,
            +1, 0,   +1, +1,   0, -2,   +1, -2,
            -1, 0,   -1, -1,   0, +2,   -1, +2,
            -1, 0,   -1, +1,   0, -2,   -1, -2
        }
    };

    /*
 I Tetrimino Wall Kick Data 	Test 1 	Test 2 	Test 3 	Test 4 	Test 5
0>>1 	( 0, 0) 	(-2, 0) 	( 1, 0) 	(-2,-1) 	( 1, 2)
1>>2 	( 0, 0) 	(-1, 0) 	( 2, 0) 	(-1, 2) 	( 2,-1)
2>>3 	( 0, 0) 	( 2, 0) 	(-1, 0) 	( 2, 1) 	(-1,-2)
3>>0 	( 0, 0) 	( 1, 0) 	(-2, 0) 	( 1,-2) 	(-2, 1)

0>>3	( 0, 0)	(-1, 0)	( 2, 0)	(-1, 2)	( 2,-1)
1>>0	( 0, 0)	( 2, 0)	(-1, 0)	( 2, 1)	(-1,-2)
2>>1	( 0, 0)	( 1, 0)	(-2, 0)	( 1,-2)	(-2, 1)
3>>2	( 0, 0)	(-2, 0)	( 1, 0)	(-2,-1)	( 1, 2)
    */
}


Tetrimino::Tetrimino(int8_t typeIdx)
{
    // translate given type to char and type index
    m_nTypeIdx = typeIdx % CNT_TETRIMINOS;
    m_chType = TYPE_CHARS[m_nTypeIdx];

    // set correct data struct
    if (m_chType == 'O' || m_chType == 'I') {
        m_pData = &FOUR_DATA;
    }
    else {
        m_pData = &THREE_DATA;
    }

    m_xOfs = m_yOfs = 0;
    m_nRotationPos = 0;

    // compute the 4 tile coordinates
    const char* layout = TETRO_STR[m_nTypeIdx];
    int idx = 0, xy = 0;
    for (const char* ch = layout; (*ch) != 0; ch++)
    {
        switch (*ch)
        {
        case '#':
            m_xTileOffset[idx] = xy % 4;
            m_yTileOffset[idx] = xy / 4;
            idx++, xy++;
            break;
        case '.':
            xy++;
            break;
        }
        if (idx >= 4) break;
    }
}


Tetrimino::Tetrimino(const Tetrimino& other)
{
    copyFrom(other);
}


Tetrimino& Tetrimino::operator=(const Tetrimino& other)
{
    copyFrom(other);
    return *this;
}


int32_t Tetrimino::getX(int8_t tileIdx) const
{
    return m_pData->m_xStartPos + m_xOfs + m_xTileOffset[tileIdx & 0x03];
}

int32_t Tetrimino::getY(int8_t tileIdx) const
{
    return m_pData->m_yStartPos + m_yOfs + m_yTileOffset[tileIdx & 0x03];
}

void Tetrimino::resetPosition()
{
    while (m_nRotationPos != 0)
    {
        rotateRight(0);
    }
    m_xOfs = m_yOfs = 0;
}


void Tetrimino::move(int32_t deltaX, int32_t deltaY)
{
    m_xOfs += deltaX;
    m_yOfs += deltaY;
}


void Tetrimino::rotateRight(int8_t wallKickIdx)
{
    // no rotation or wall kick for 'O'
    if (m_chType == 'O')
        return;

    // apply wall kick
    if (wallKickIdx >= 1 && wallKickIdx <= 4) {
        int idx = (m_nRotationPos * 8) + ((wallKickIdx - 1) * 2);
        m_xOfs += m_pData->m_RotRightWallKick[idx + 0];
        m_yOfs += m_pData->m_RotRightWallKick[idx + 1];
    }

    for (int i = 0; i < 4; i++)
    {
        int32_t newX = m_pData->m_nRotationTransformer - m_yTileOffset[i];
        int32_t newY = m_xTileOffset[i];
        m_xTileOffset[i] = newX;
        m_yTileOffset[i] = newY;
    }
    m_nRotationPos = (m_nRotationPos + 1) & 0x03;
}


void Tetrimino::rotateLeft(int8_t wallKickIdx)
{
    // no rotation or wall kick for 'O'
    if (m_chType == 'O')
        return;

    // apply wall kick
    if (wallKickIdx >= 1 && wallKickIdx <= 4) {
        int idx = (m_nRotationPos * 8) + ((wallKickIdx - 1) * 2);
        m_xOfs += m_pData->m_RotLeftWallKick[idx + 0];
        m_yOfs += m_pData->m_RotLeftWallKick[idx + 1];
    }

    for (int i = 0; i < 4; i++)
    {
        int32_t newX = m_yTileOffset[i];
        int32_t newY = m_pData->m_nRotationTransformer - m_xTileOffset[i];
        m_xTileOffset[i] = newX;
        m_yTileOffset[i] = newY;
    }
    m_nRotationPos = (m_nRotationPos + 3) & 0x03;
}


void Tetrimino::copyFrom(const Tetrimino& other)
{
    this->m_nTypeIdx = other.m_nTypeIdx;
    this->m_chType = other.m_chType;
    this->m_pData = other.m_pData;

    this->m_xOfs = other.m_xOfs;
    this->m_yOfs = other.m_yOfs;
    this->m_nRotationPos = other.m_nRotationPos;

    for (int i = 0; i < 4; i++)
    {
        this->m_xTileOffset[i] = other.m_xTileOffset[i];
        this->m_yTileOffset[i] = other.m_yTileOffset[i];
    }
}
