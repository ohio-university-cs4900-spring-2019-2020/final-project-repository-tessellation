#include "MGLEarthQuad.h"

#include "GLSLEarthShader.h"
#include "GLSLUniform.h"

#include "ManagerTexture.h"
#include "ManagerEnvironmentConfiguration.h"

#ifdef AFTR_CONFIG_USE_GDAL // this class won't work without GDAL

#include "gdal_priv.h"
#include "cpl_conv.h"

using namespace Aftr;

MGLEarthQuad::MGLEarthQuad(WO* parentWO, const Vector& ul, const Vector& lr, unsigned int nTilesX, unsigned int nTilesY,
    float s, float tess, const std::string& elev, const std::string& imagery)
    : MGL(parentWO)
{
    this->scale = s;
    this->tessellationFactor = tess;
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
    
    this->getModelDataShared()->getModelMeshes().at(0)->getSkin().getShader()->getUniforms()->at(1)->set(this->scale);
    this->getModelDataShared()->getModelMeshes().at(0)->getSkin().getShader()->getUniforms()->at(2)->set(this->tessellationFactor);
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

void MGLEarthQuad::generateData(const Vector& upperLeft, const Vector& lowerRight, unsigned int numTilesX, unsigned int numTilesY)
{
    // create mesh data generator
    std::unique_ptr<ModelMeshRenderDataGenerator> data = std::make_unique<ModelMeshRenderDataGenerator>();
    data->setIndexTopology(GL_PATCHES);

    for (unsigned int x = 0; x <= numTilesX; ++x) {
        float lat = upperLeft.x + (lowerRight.x - upperLeft.x) * static_cast<float>(x) / numTilesX;
        float latRad = lat * Aftr::DEGtoRAD;

        for (unsigned int y = 0; y <= numTilesY; ++y) {
            float lon = upperLeft.y + (lowerRight.y - upperLeft.y) * static_cast<float>(y) / numTilesY;
            float lonRad = lon * Aftr::DEGtoRAD;

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

    /*int step = 1;
    for (int x = 90; x >= -90; x -= step) {
        for (int y = -180; y <= 180; y += step) {
            Vector v(x * Aftr::DEGtoRAD, y * Aftr::DEGtoRAD, 0);
            data->getVerts()->push_back(v);

            if (x != 90 && y != -180) {
                unsigned int x_ind1 = -(x - 90) / step - 1;
                unsigned int y_ind1 = (y + 180) / step - 1;
                unsigned int x_ind2 = x_ind1 + 1;
                unsigned int y_ind2 = y_ind1 + 1;

                data->getIndicies()->push_back(x_ind1 * (360 / step + 1) + y_ind1);
                data->getIndicies()->push_back(x_ind2 * (360 / step + 1) + y_ind1);
                data->getIndicies()->push_back(x_ind2 * (360 / step + 1) + y_ind2);
                data->getIndicies()->push_back(x_ind1 * (360 / step + 1) + y_ind2);
            }
        }
    }*/

    // create triangle skin
    ModelMeshSkin skin1;
    skin1.setGLPrimType(GL_PATCHES);
    skin1.setMeshShadingType(MESH_SHADING_TYPE::mstNONE);
    skin1.setShader(GLSLEarthShader::New(false, scale, tessellationFactor));
    skin1.setPatchVertices(4);
    skin1.getMultiTextureSet().at(0) = new TextureSharesTexDataOwnsGLHandle(static_cast<TextureDataOwnsGLHandle*>(elevTex->getTextureData()));
    skin1.getMultiTextureSet().push_back(imageryTex);

    // create line skin
    ModelMeshSkin skin2;
    skin2.setGLPrimType(GL_PATCHES);
    skin2.setMeshShadingType(MESH_SHADING_TYPE::mstNONE);
    skin2.setShader(GLSLEarthShader::New(true, scale, tessellationFactor));
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
}

void MGLEarthQuad::loadImageryTexture(const std::string& imagery)
{
    imageryTex = ManagerTexture::loadTexture(imagery);
}

void MGLEarthQuad::loadElevationTexture(const std::string& dataset) {
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

    // create OpenGL texture from our tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    elevTex = ManagerTexture::loadDynamicTexture(GL_TEXTURE_2D, 0, nXSize, nYSize, GL_R16I, 0, GL_RED_INTEGER, GL_SHORT, pafScanline);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    CPLFree(pafScanline); // we no longer need the data, so free it
}

#endif // AFTR_CONFIG_USE_GDAL