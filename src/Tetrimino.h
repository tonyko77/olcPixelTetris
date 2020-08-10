#ifndef TETRIMINO_H
#define TETRIMINO_H

#include <cstdint>


struct TetroConstData;


class Tetrimino
{
public:
    Tetrimino(int8_t typeIdx = 0);
    Tetrimino(const Tetrimino& other);

    Tetrimino& operator= (const Tetrimino& other);

    int32_t getX(int8_t tileIdx) const;
    int32_t getY(int8_t tileIdx) const;
    void resetPosition();

    void move(int32_t deltaX, int32_t deltaY);
    void rotateLeft(int8_t wallKickIdx);
    void rotateRight(int8_t wallKickIdx);

    int8_t getTypeIndex() const   { return m_nTypeIdx;  }
    char   getTypeChar() const    { return m_chType;    }


private:
    void copyFrom(const Tetrimino& other);

private:
    int8_t m_nTypeIdx;
    char   m_chType;
    const TetroConstData* m_pData;

    int32_t m_xOfs, m_yOfs;
    int32_t m_xTileOffset[4];
    int32_t m_yTileOffset[4];
    int8_t  m_nRotationPos;
};


#endif // TETRIMINO_H
