#include "GLSLEarthShader.h"
#include "Camera.h"
#include "GLSLAttribute.h"
#include "GLSLShaderDescriptor.h"
#include "GLSLUniform.h"
#include "GLView.h"
#include "ManagerEnvironmentConfiguration.h"
#include "ManagerShader.h"
#include "Model.h"
using namespace Aftr;

GLSLEarthShader* GLSLEarthShader::New(bool useLines, float scale, float tess, float maxTess)
{
    // produce strings for the shader programs
    std::string vert = ManagerEnvironmentConfiguration::getLMM() + "shaders/earth.vert";
    std::string frag = ManagerEnvironmentConfiguration::getLMM() + "shaders/earth.frag";
    std::string geom = ManagerEnvironmentConfiguration::getLMM() + (useLines ? "shaders/earth.geom" : "shaders/earth_tri.geom");
    std::string tessCon = ManagerEnvironmentConfiguration::getLMM() + "shaders/earth.tesc";
    std::string tessEval = ManagerEnvironmentConfiguration::getLMM() + "shaders/earth.tese";

    // compose a shader descriptor
    GLSLShaderDescriptor desc;
    desc.vertexShader = vert;
    desc.fragmentShader = frag;
    desc.geometryShader = geom;
    desc.tessellationControlShader = tessCon;
    desc.tessellationEvalShader = tessEval;
    desc.geometryInputPrimitiveType = GL_TRIANGLES;
    desc.geometryOutputPrimitiveType = GL_TRIANGLE_STRIP;
    desc.geometryMaxOutputVerts = 3;
    desc.tessellationMaxVerticesPerPatch = 4;

    // create the shader data
    GLSLShaderDataShared* shdrData = ManagerShader::loadShaderDataShared(desc);
    if (shdrData == nullptr)
        return nullptr;

    // create the GLSLEarthShader object
    GLSLEarthShader* shdr = new GLSLEarthShader(shdrData);
    shdr->scaleFactor = scale;
    shdr->tessellationFactor = tess;
    shdr->maxTessellationFactor = maxTess;

    return shdr;
}

GLSLEarthShader* GLSLEarthShader::New(GLSLShaderDataShared* shdrData)
{
    GLSLEarthShader* shdr = new GLSLEarthShader(shdrData);
    return shdr;
}

GLSLEarthShader::GLSLEarthShader(GLSLShaderDataShared* dataShared)
    : GLSLShader(dataShared)
{
    this->addUniform(new GLSLUniform("MVPMat", utMAT4, this->getHandle()));
    this->addUniform(new GLSLUniform("scale", utFLOAT, this->getHandle()));
    this->addUniform(new GLSLUniform("tessellationFactor", utFLOAT, this->getHandle()));
    this->addUniform(new GLSLUniform("maxTessellationFactor", utFLOAT, this->getHandle()));
    this->addUniform(new GLSLUniform("elevationTexture", utSAMPLER2D, this->getHandle()));
    this->addUniform(new GLSLUniform("imageryTexture", utSAMPLER2D, this->getHandle()));

    this->addAttribute(new GLSLAttribute("VertexPosition", atVEC3, this));

    this->scaleFactor = 0.0f;
    this->tessellationFactor = 0.0f;
    this->maxTessellationFactor = 64.0f;
}

GLSLEarthShader::GLSLEarthShader(const GLSLEarthShader& toCopy)
    : GLSLShader(toCopy.dataShared)
{
    *this = toCopy;
}

GLSLEarthShader::~GLSLEarthShader()
{
    // Parent destructor deletes all uniforms and attributes
}

GLSLEarthShader& Aftr::GLSLEarthShader::operator=(const GLSLEarthShader& shader)
{
    if (this != &shader) {
        // copy all of parent info in base shader, then copy local members in this subclass instance
        GLSLShader::operator=(shader);

        // Now copy local members from this subclassed instance
        this->scaleFactor = shader.scaleFactor;
        this->tessellationFactor = shader.tessellationFactor;
        this->maxTessellationFactor = shader.maxTessellationFactor;
    }
    return *this;
}

GLSLShader* GLSLEarthShader::getCopyOfThisInstance()
{
    GLSLEarthShader* copy = new GLSLEarthShader(*this);
    return copy;
}

void GLSLEarthShader::bind(const Mat4& modelMatrix, const Mat4& normalMatrix, const Camera& cam, const ModelMeshSkin& skin)
{
    GLSLShader::bind(); // Must Bind this shader program handle to GL before updating any uniforms

    const std::vector<GLSLUniform*>* uniforms = this->getUniforms();

    // calculate and bind MVP matrix
    Mat4 MVPMat = cam.getCameraProjectionMatrix() * cam.getCameraViewMatrix() * modelMatrix;
    uniforms->at(0)->setValues(MVPMat.getPtr());

    // bind factors
    this->getUniforms()->at(1)->set(scaleFactor);
    this->getUniforms()->at(2)->set(tessellationFactor);
    this->getUniforms()->at(3)->set(maxTessellationFactor);

    // bind texture unit locations
    this->getUniforms()->at(4)->set(0);
    this->getUniforms()->at(5)->set(1);
}

void GLSLEarthShader::setMVPMatrix(const Mat4& mvpMatrix)
{
    this->getUniforms()->at(0)->setValues(mvpMatrix.getPtr());
}

void GLSLEarthShader::setScaleFactor(float s)
{
    scaleFactor = s;
    this->getUniforms()->at(1)->set(scaleFactor);
}

void GLSLEarthShader::setTessellationFactor(float t)
{
    tessellationFactor = t;
    this->getUniforms()->at(2)->set(tessellationFactor);
}

void GLSLEarthShader::setMaxTessellationFactor(float m)
{
    maxTessellationFactor = m;
    this->getUniforms()->at(3)->set(maxTessellationFactor);
}