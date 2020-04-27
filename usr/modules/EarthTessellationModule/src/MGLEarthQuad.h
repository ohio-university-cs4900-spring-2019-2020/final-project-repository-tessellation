#pragma once

#include "MGL.h"
#include "Vector.h"

namespace Aftr {
/**
   This class provides a model capable of rendering tessellated Earth quads.
*/
class MGLEarthQuad : public MGL {
public:
    MGLEarthQuad(WO* parentWO) = delete;

    /**
        Constructor for creating an Earth quad model.
        parentWO - WO that will use the model
        ul - The upper-left WGS84 coordinate of the Earth quad.
        lr - The lower-right WGS84 coordinate of the Earth quad.
        nTilesX - The number of tiles to subdivide the quad into on the x axis (lattitude).
        nTilesY - The number of tiles to subdivide the quad into on the y axis (longitude).
        s - The scale factor for the Earth.
        tess - The tessellation factor used for each tile.
        maxTess - The tessellation factor max value for the LOD.
        elev - The path to the elevation dataset file used for displacement of the Earth's surface.
        imagery - The path to the imagery file of the Earth's surface used for texturing.
    */
    MGLEarthQuad(WO* parentWO, const Vector& ul, const Vector& lr, unsigned int nTilesX, unsigned int nTilesY,
        float s, float tess, float maxTess, const std::string& elev, const std::string& imagery);
    virtual ~MGLEarthQuad();
    virtual void render(const Camera& cam);
    virtual void renderSelection(const Camera& cam, GLubyte red, GLubyte green, GLubyte blue);

    // Returns whether rendering with lines or not (with triangles instead).
    bool isUsingLines() const { return usingLines; }

    // Sets whether to render with lines or not (otherwise will render with triangles).
    void useLines(bool b);

    // Returns the Earth scale factor.
    float getScaleFactor() const { return this->scale; }

    // Sets the Earth scale factor.
    void setScaleFactor(float s);
    
    // Returns the tessellation factor.
    float getTessellationFactor() const { return this->tessellationFactor; }

    // Sets the tessellation factor.
    void setTessellationFactor(float t);

    // Returns the maximum tessellation factor.
    float getMaxTessellationFactor() const { return this->maxTessellationFactor; }

    // Sets the maximum tessellation factor.
    void setMaxTessellationFactor(float t);

protected:
    bool usingLines;
    float scale;
    float tessellationFactor;
    float maxTessellationFactor;

    Texture* elevTex;
    Texture* imageryTex;

    // Generates the tile vertex data for rendering.
    void generateData(const Vector& upperLeft, const Vector& lowerRight, unsigned int numTilesX, unsigned int numTilesY);

    // Loads and prepares the elevation texture.
    void loadElevationTexture(const std::string& dataset);

    // Loads and prepares the imagery texture.
    void loadImageryTexture(const std::string& imagery);
};
}