#pragma once

#include "MGL.h"
#include "Vector.h"

namespace Aftr {
class MGLEarthQuad : public MGL {
public:
    MGLEarthQuad(WO* parentWO) = delete;
    MGLEarthQuad(WO* parentWO, const Vector& ul, const Vector& lr, unsigned int nTilesX, unsigned int nTilesY,
        float s, float tess, const std::string& elev, const std::string& imagery);
    virtual ~MGLEarthQuad();
    virtual void render(const Camera& cam);
    virtual void renderSelection(const Camera& cam, GLubyte red, GLubyte green, GLubyte blue);

    bool isUsingLines() const { return usingLines; }
    void useLines(bool b);

    float getScaleFactor() const { return this->scale; }
    void setScaleFactor(float s);
    float getTessellationFactor() const { return this->tessellationFactor; }
    void setTessellationFactor(float t);

protected:
    bool usingLines;
    float scale;
    float tessellationFactor;

    Texture* elevTex;
    Texture* imageryTex;

    void generateData(const Vector& upperLeft, const Vector& lowerRight, unsigned int numTilesX, unsigned int numTilesY);
    void loadElevationTexture(const std::string& dataset);
    void loadImageryTexture(const std::string& imagery);
};
}