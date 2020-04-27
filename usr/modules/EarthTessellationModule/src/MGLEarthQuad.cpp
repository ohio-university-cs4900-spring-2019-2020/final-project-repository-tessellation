#include "MGLEarthQuad.h"

#include "GLSLEarthShader.h"
#include "GLSLUniform.h"

#include "ManagerEnvironmentConfiguration.h"
#include "ManagerTexture.h"
#include "Texture.h"

#ifdef AFTR_CONFIG_USE_GDAL // this class won't work without GDAL

// Note: GDAL internally has warnings in their library headers, so I'm doing this to suppress them
#pragma warning(push, 0)
#include "cpl_conv.h"
#include "gdal_priv.h"
#pragma warning(pop)

using namespace Aftr;

MGLEarthQuad::MGLEarthQuad(WO* parentWO, const Vector& ul, const Vector& lr, unsigned int nTilesX, unsigned int nTilesY,
    float s, float tess, float maxTess, const std::string& elev, const std::string& imagery)
    : MGL(parentWO)
{
    this->scale = s;
    this->tessellationFactor = tess;
    this->maxTessellationFactor = maxTess;
    this->usingLines = false;
    this->elevTex = nullptr;
    this->imageryTex = nullptr;

    // ensure number of tiles is nonzero
    assert(nTilesX > 0);
    assert(nTilesY > 0);

    // generate data
    loadElevationTexture(elev);
    loadImageryTexture(imagery);
    generateData(ul, lr, nTilesX, nTilesY);
}

MGLEarthQuad::~MGLEarthQuad()
{
    delete this->modelData->getModelMeshes().at(0)->getMeshDataShared();

    this->modelData->destroyCompositeLists();
    delete this->modelData;
    this->modelData = nullptr;

    // destroy elevation texture
    if (elevTex != nullptr) {
        delete elevTex;
        elevTex = nullptr;
    }

    // note: we don't delete imageryTex because ManagerTexture handles that
}

void MGLEarthQuad::render(const Camera& cam)
{
    Model::render(cam);
}

void MGLEarthQuad::renderSelection(const Camera& cam, GLubyte red, GLubyte green, GLubyte blue)
{
    Model::renderSelection(cam, red, green, blue);
}

void MGLEarthQuad::useLines(bool b)
{
    this->usingLines = b;

    // use skin 0 for triangles and skin 1 for lines
    this->getModelDataShared()->getModelMeshes().at(0)->useSkinAtIndex(this->usingLines ? 1 : 0);
}

void MGLEarthQuad::setScaleFactor(float s)
{
    this->scale = s;

    // get triangle and line skins
    ModelMesh* mesh = this->getModelDataShared()->getModelMeshes().at(0);
    ModelMeshSkin& skin1 = mesh->getSkins().at(0);
    ModelMeshSkin& skin2 = mesh->getSkins().at(1);

    // set both skins' scale factors
    skin1.getShaderT<GLSLEarthShader>()->setScaleFactor(this->scale);
    skin2.getShaderT<GLSLEarthShader>()->setScaleFactor(this->scale);
}

void MGLEarthQuad::setTessellationFactor(float t)
{
    this->tessellationFactor = t;

    // get triangle and line skins
    ModelMesh* mesh = this->getModelDataShared()->getModelMeshes().at(0);
    ModelMeshSkin& skin1 = mesh->getSkins().at(0);
    ModelMeshSkin& skin2 = mesh->getSkins().at(1);

    // set both skins' tessellation factors
    skin1.getShaderT<GLSLEarthShader>()->setTessellationFactor(this->tessellationFactor);
    skin2.getShaderT<GLSLEarthShader>()->setTessellationFactor(this->tessellationFactor);
}

void MGLEarthQuad::setMaxTessellationFactor(float t)
{
    this->maxTessellationFactor = t;

    // get triangle and line skins
    ModelMesh* mesh = this->getModelDataShared()->getModelMeshes().at(0);
    ModelMeshSkin& skin1 = mesh->getSkins().at(0);
    ModelMeshSkin& skin2 = mesh->getSkins().at(1);

    // set both skins' max tessellation factors
    skin1.getShaderT<GLSLEarthShader>()->setMaxTessellationFactor(this->maxTessellationFactor);
    skin2.getShaderT<GLSLEarthShader>()->setMaxTessellationFactor(this->maxTessellationFactor);
}

void MGLEarthQuad::generateData(const Vector& upperLeft, const Vector& lowerRight, unsigned int numTilesX, unsigned int numTilesY)
{
    // create mesh data generator
    std::unique_ptr<ModelMeshRenderDataGenerator> data = std::make_unique<ModelMeshRenderDataGenerator>();
    data->setIndexTopology(GL_PATCHES);

    // generate patch vertices
    for (unsigned int x = 0; x <= numTilesX; ++x) {
        // calculate the lattitude at this subdivison level
        float lat = upperLeft.x + (lowerRight.x - upperLeft.x) * static_cast<float>(x) / numTilesX;
        float latRad = lat * Aftr::DEGtoRAD;

        for (unsigned int y = 0; y <= numTilesY; ++y) {
            // calculate the longitude at this subdivision level
            float lon = upperLeft.y + (lowerRight.y - upperLeft.y) * static_cast<float>(y) / numTilesY;
            float lonRad = lon * Aftr::DEGtoRAD;

            // combine lat and lon into WGS84 coordinate
            data->getVerts()->push_back(Vector(latRad, lonRad, 0.0f));
        }
    }

    unsigned int width = numTilesY + 1;

    // generate indices
    for (unsigned int x = 0; x < numTilesX; ++x) {
        for (unsigned int y = 0; y < numTilesY; ++y) {
            // convert 2d array indices to 1d array indices
            unsigned int ul = y + x * width;
            unsigned int ll = y + (x + 1) * width;
            unsigned int lr = (y + 1) + (x + 1) * width;
            unsigned int ur = (y + 1) + x * width;

            data->getIndicies()->push_back(ul);
            data->getIndicies()->push_back(ll);
            data->getIndicies()->push_back(lr);
            data->getIndicies()->push_back(ur);
        }
    }

    // create triangle skin
    ModelMeshSkin skin1;
    skin1.setGLPrimType(GL_PATCHES);
    skin1.setMeshShadingType(MESH_SHADING_TYPE::mstNONE);
    skin1.setShader(GLSLEarthShader::New(false, scale, tessellationFactor, maxTessellationFactor));
    skin1.setPatchVertices(4);
    skin1.getMultiTextureSet().at(0) = new TextureSharesTexDataOwnsGLHandle(static_cast<TextureDataOwnsGLHandle*>(elevTex->getTextureData()));
    skin1.getMultiTextureSet().push_back(imageryTex);

    // create line skin
    ModelMeshSkin skin2;
    skin2.setGLPrimType(GL_PATCHES);
    skin2.setMeshShadingType(MESH_SHADING_TYPE::mstNONE);
    skin2.setShader(GLSLEarthShader::New(true, scale, tessellationFactor, maxTessellationFactor));
    skin2.setPatchVertices(4);
    skin2.getMultiTextureSet().at(0) = new TextureSharesTexDataOwnsGLHandle(static_cast<TextureDataOwnsGLHandle*>(elevTex->getTextureData()));
    skin2.getMultiTextureSet().push_back(imageryTex->cloneMe());

    // create mesh data with skin1 and our data generator
    ModelMeshDataShared* dataShared = new ModelMeshDataShared(std::move(data));
    ModelMesh mesh(skin1, dataShared);
    mesh.setParentModel(this);
    this->modelData = new ModelDataShared(std::vector<ModelMesh*>(1, &mesh));

    // add skin2 to the mesh data
    this->getModelDataShared()->getModelMeshes().at(0)->addSkin(skin2);

    // Note that mesh is deallocated when this function returns, but that's okay because
    // the constructor of ModelDataShared actually makes a copy of it.
}

void MGLEarthQuad::loadImageryTexture(const std::string& imagery)
{
    imageryTex = ManagerTexture::loadTexture(imagery);
}

void MGLEarthQuad::loadElevationTexture(const std::string& dataset)
{
    GDALAllRegister(); // initialize GDAL

    // load dataset
    GDALDataset* poDataset;
    poDataset = static_cast<GDALDataset*>(GDALOpen(dataset.c_str(), GA_ReadOnly));

    if (poDataset == nullptr) {
        std::cout << "Error: unable to load dataset" << std::endl;
        exit(-1);
    } else if (poDataset->GetRasterCount() == 0) {
        std::cout << "Error: No raster bands in dataset" << std::endl;
        exit(-1);
    }

    // get raster band
    GDALRasterBand* poBand;
    poBand = poDataset->GetRasterBand(1);

    // get band dimensions
    int nXSize = poBand->GetXSize();
    int nYSize = poBand->GetYSize();

    // read all of band's data into pafScanline
    GLshort* pafScanline = static_cast<GLshort*>(CPLMalloc(sizeof(GLshort) * nXSize * nYSize));
    poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, pafScanline, nXSize, nYSize, GDT_Int16, 0, 0);

    // close the dataset since we're now done
    GDALClose(poDataset);

    // Now manually create OpenGL texture using 4.2 features and generate mipmaps manually
    // (Because apparently OpenGL doesn't support mipmaps for integer textures, at least not on my
    //  hardware.)

    // calculate number of mipmap levels to generate (including base level)
    unsigned int numLevels = 1 + static_cast<unsigned int>(std::log2(std::max(nXSize, nYSize)));

    // generate texture
    GLuint texID;
    glGenTextures(1, &texID);

    // bind texture
    glBindTexture(GL_TEXTURE_2D, texID);

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // allocate space for all texture levels (OpenGL 4.2+ only)
    glTexStorage2D(GL_TEXTURE_2D, numLevels, GL_R16I, nXSize, nYSize);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nXSize, nYSize, GL_RED_INTEGER, GL_SHORT, pafScanline); // upload base level

    // generate mipmap levels
    unsigned int dWidth = nXSize;
    unsigned int dHeight = nYSize;
    unsigned int level = 0;
    GLshort* source = pafScanline;
    while (dWidth > 1 || dHeight > 1) {
        unsigned int sWidth = dWidth; // keep source width

        dWidth = std::max(dWidth / 2, 1u);
        dHeight = std::max(dHeight / 2, 1u);

        // allocate space for dest data
        GLshort* dest = static_cast<GLshort*>(CPLMalloc(sizeof(GLshort) * dWidth * dHeight));

        // downscale the source data and put into dest
        for (unsigned int j = 0; j < dHeight; ++j) {
            for (unsigned int i = 0; i < dWidth; ++i) {
                // take the average of the 4 pixels in the source image that make up this one pixel in
                // the dest image. (Do summation as integer to avoid short overflow).
                int sum = source[j * 2 * sWidth + i * 2];
                sum += source[(j * 2 + 1) * sWidth + i * 2];
                sum += source[j * 2 * sWidth + i * 2 + 1];
                sum += source[(j * 2 + 1) * sWidth + i * 2 + 1];

                dest[j * dWidth + i] = static_cast<GLshort>(sum / 4);
            }
        }

        level++; // increment mipmap level

        // send to OpenGL
        glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, dWidth, dHeight, GL_RED_INTEGER, GL_SHORT, dest);

        CPLFree(source); // we no longer need the source data, free it
        source = dest; // calculate next level from this dest
    }

    CPLFree(source); // free the final level

    // generate CPU side texture data
    TextureDataOwnsGLHandle* tex = new TextureDataOwnsGLHandle("DynamicTexture");
    tex->isMipmapped(true);
    tex->setTextureDimensionality(GL_TEXTURE_2D);
    tex->setGLInternalFormat(GL_R16I);
    tex->setGLRawTexelFormat(GL_RED_INTEGER);
    tex->setGLRawTexelType(GL_SHORT);
    tex->setTextureDimensions(nXSize, nYSize);
    tex->setGLTex(texID);

    elevTex = new TextureOwnsTexDataOwnsGLHandle(tex);
}

#endif // AFTR_CONFIG_USE_GDAL