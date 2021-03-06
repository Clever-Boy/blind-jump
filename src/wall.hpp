#pragma once

#include <stdint.h>

class wall {
private:
    float xPosInitial, yPosInitial, xPos, yPos;

public:
    float getPosX() const;
    float getPosY() const;

    inline int8_t getWidth() const { return 32; }

    inline int8_t getHeight() const { return 26; }

    wall();
    void setPosition(float, float);
    void setYinit(float);
    void setXinit(float);
    float getXinit() const;
    float getYinit() const;
};
