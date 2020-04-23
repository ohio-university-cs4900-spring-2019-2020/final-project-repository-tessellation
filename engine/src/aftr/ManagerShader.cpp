#include "ManagerShader.h"
#include "AftrUtilities.h"
#include "GLSLAttribute.h"
#include "GLSLShader.h"
#include "GLSLShaderDefaultBoundingBoxLinesGL32.h"
#include "GLSLShaderDefaultGL32.h"
#include "GLSLShaderDefaultIndexedGeometryLinesGL32.h"
#include "GLSLShaderDefaultLoadingScreenGL32.h"
#include "GLSLShaderDefaultOrthoStencilGL32.h"
#include "GLSLShaderDefaultSelectionGL32.h"
#include "GLSLShaderDescriptor.h"
#include "GLSLShaderPointTesselatorBillboard.h"
#include "GLSLUniform.h"
#include "ManagerEnvironmentConfiguration.h"
#include "ManagerOpenGLState.h"
#include "Mat4.h"
#include <sstream>
#include <vector>
using namespace Aftr;

GLuint ManagerShader::currentlyBoundShaderHandle = 0;
GLSLShaderDefaultGL32* ManagerShader::DEFAULT_SHADER = nullptr;
GLSLShaderDefaultSelectionGL32* ManagerShader::DEFAULT_SELECTION_SHADER = nullptr;
GLSLShaderDefaultBoundingBoxLinesGL32* ManagerShader::DEFAULT_BOUNDING_BOX_LINES = nullptr;
GLSLShaderDefaultLoadingScreenGL32* ManagerShader::DEFAULT_LOADING_SCREEN = nullptr;
GLSLShaderDefaultIndexedGeometryLinesGL32* ManagerShader::DEFAULT_INDEXED_GEOMETRY_LINES = nullptr;
GLSLShaderDefaultOrthoStencilGL32* ManagerShader::DEFAULT_ORTHO_STENCIL_SHADER = nullptr;

std::vector<GLSLShader*> ManagerShader::defaultShadersLoadedByManagerAtInit;

std::set<GLSLShaderDataShared*, ShaderSetCompare> ManagerShader::shaders;

std::string ManagerShader::toString()
{
    std::stringstream ss;
    ss << "ManagerShader:\n";
    ss << ManagerShader::queryShaderSupport() << "\n";
    int i = 0;
    for (std::set<GLSLShaderDataShared*, ShaderSetCompare>::iterator it = ManagerShader::shaders.begin(); it != ManagerShader::shaders.end(); it++) {
        GLSLShaderDataShared* shader = (*it);
        ss << "   [" << i << "]: " << shader->toString() << "\n";
        ++i;
    }

    return ss.str();
}

void ManagerShader::init()
{
    ManagerShader::shutdown();

    if (ManagerOpenGLState::isGLContextProfileVersion32orGreater()) {
        ManagerShader::loadGL32DefaultShaders();
    } else {
        //std::string vert = ManagerEnvironmentConfiguration::getSMM() + "shaders/oneLightOverTexture.vert";
        //std::string frag = ManagerEnvironmentConfiguration::getSMM() + "shaders/oneLightOverTexture.frag";

        ////Load the default shader
        //DEFAULT_SHADER = ManagerShader::loadShader(vert, frag, "" );
        //ManagerShader::defaultShadersLoadedByManagerAtInit.push_back( DEFAULT_SHADER );

        if (DEFAULT_SHADER == nullptr && ManagerEnvironmentConfiguration::getVariableValue("createwindow") != "0") {
            std::cout << "WARNING: OpenGL Context Version is < 3.2 and Cannot load DEFAULT_SHADER...\n"
                      << "Modern Shaders require a GL Context of 3.2+, only GLSL Shaders using #version 1.20 or earlier are supported.\n"
                      << "OpenGL Version | GLSL Version\n"
                      << "    2.0        |    1.10\n"
                      << "    2.1        |    1.20\n"
                      << "    3.0        |    1.30\n"
                      << "    3.1        |    1.40\n"
                      << "    3.2        |    1.50\n"
                      << "    4.x+       |    4.x+ (After 3.2, the OpenGL Version MATCHES the GLSL Version).\n";

            //std::cin.get();
            //exit( -1 );
        }
    }
}

void ManagerShader::shutdown()
{
    for (auto it = std::begin(ManagerShader::defaultShadersLoadedByManagerAtInit); it != std::end(ManagerShader::defaultShadersLoadedByManagerAtInit); ++it)
        delete (*it);
    ManagerShader::defaultShadersLoadedByManagerAtInit.clear();

    std::set<GLSLShaderDataShared*, ShaderSetCompare>::iterator it = ManagerShader::shaders.begin();
    while (ManagerShader::shaders.size() > 0) {
        it = ManagerShader::shaders.begin();
        //std::cout << "Erasing GLSLShaderDataShared " << (*it)->toString() << "\n";
        delete (*it);
        ManagerShader::shaders.erase(it);
    }
}

GLSLShaderDefaultGL32* ManagerShader::getDefaultShaderCopy()
{
    return static_cast<GLSLShaderDefaultGL32*>(DEFAULT_SHADER->getCopyOfThisInstance());
}

GLSLShader* ManagerShader::loadComputeShader(const std::string& computeShader)
{
    GLSLShaderDataShared* shader = new GLSLShaderDataShared("", "", "", GL_TRIANGLES, GL_TRIANGLE_STRIP, 3,
        computeShader);

    std::set<GLSLShaderDataShared*, ShaderSetCompare>::iterator it = ManagerShader::shaders.find(shader);
    if (it == ManagerShader::shaders.end()) {
        if (ManagerShader::instantiateOpenGLShader(shader)) {
            ManagerShader::shaders.insert(shader); //store in set
            return new GLSLShader(shader);
        } else {
            delete shader;
            shader = nullptr;
            return nullptr;
        }
    } else {
        delete shader;
        shader = nullptr; //if the shader was already found, use the original and delete the new one
        return new GLSLShader((*it)); //return pointer to already loaded texture
    }
}

GLSLShaderDataShared* ManagerShader::loadShaderDataShared(const std::string& vertexShader, const std::string& fragmentShader,
    std::string geometryShader, GLenum geometryInputPrimitiveType,
    GLenum geometryOutputPrimitiveType, GLuint geometryMaxOutputVerts)
{
    if (glCreateShader == nullptr)
        return nullptr;

    if (geometryOutputPrimitiveType != GL_POINTS && geometryOutputPrimitiveType != GL_LINE_STRIP && geometryOutputPrimitiveType != GL_TRIANGLE_STRIP) {
        //GL_POINTS, GL_LINE_STRIP, or GL_TRIANGLE_STRIP
        std::cout << "ERROR: ManagerShader::loadShader(...):\n"
                  << "Geometry Shader Output Primitive type must be either: \n"
                  << "   'GL_POINTS', 'GL_LINE_STRIP', or 'GL_TRIANGLE_STRIP'...\n"
                  << "   Invalid output primitive type ' " << geometryOutputPrimitiveType << "'...\n"
                  << "Press ENTER to exit...\n";
        std::cin.get();
        exit(-1);
    }

    if (geometryInputPrimitiveType != GL_POINTS && geometryInputPrimitiveType != GL_LINES && geometryInputPrimitiveType != GL_LINES_ADJACENCY_EXT && geometryInputPrimitiveType != GL_TRIANGLES && geometryInputPrimitiveType != GL_TRIANGLES_ADJACENCY_EXT) {
        //GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_EXT, GL_TRIANGLES, GL_TRIANGLES_ADJACENCY_EXT
        std::cout << "ERROR: ManagerShader::loadShader(...):\n"
                  << "Geometry Shader Input Primitive type must be either: \n"
                  << "   'GL_POINTS', 'GL_LINES', 'GL_LINES_ADJACENCY_EXT',\n"
                  << "   'GL_TRIANGLES', or 'GL_TRIANGLES_ADJACENCY_EXT'...\n"
                  << "   Invalid input primitive type ' " << geometryInputPrimitiveType << "'...\n"
                  << "Press ENTER to exit...\n";
        std::cin.get();
        exit(-1);
    }

    GLSLShaderDataShared* shader = new GLSLShaderDataShared(vertexShader, fragmentShader, geometryShader,
        geometryInputPrimitiveType, geometryOutputPrimitiveType, geometryMaxOutputVerts, "");

    std::set<GLSLShaderDataShared*, ShaderSetCompare>::iterator it = ManagerShader::shaders.find(shader);
    if (it == ManagerShader::shaders.end()) {
        if (ManagerShader::instantiateOpenGLShader(shader)) {
            ManagerShader::shaders.insert(shader); //store in set
            return shader;
        } else {
            delete shader;
            shader = nullptr;
            return nullptr;
        }
    } else {
        delete shader; //if the shader was already found, use the original and delete the new one
        return *it; //return pointer to already loaded texture
    }
}

GLSLShaderDataShared* ManagerShader::loadShaderDataShared(const GLSLShaderDescriptor& desc)
{
    if (glCreateShader == nullptr)
        return nullptr;

    if (desc.geometryOutputPrimitiveType != GL_POINTS && desc.geometryOutputPrimitiveType != GL_LINE_STRIP && desc.geometryOutputPrimitiveType != GL_TRIANGLE_STRIP) {
        //GL_POINTS, GL_LINE_STRIP, or GL_TRIANGLE_STRIP
        std::cout << "ERROR: ManagerShader::loadShader(...):\n"
                  << "Geometry Shader Output Primitive type must be either: \n"
                  << "   'GL_POINTS', 'GL_LINE_STRIP', or 'GL_TRIANGLE_STRIP'...\n"
                  << "   Invalid output primitive type ' " << desc.geometryOutputPrimitiveType << "'...\n"
                  << "Press ENTER to exit...\n";
        std::cin.get();
        exit(-1);
    }

    if (desc.geometryInputPrimitiveType != GL_POINTS && desc.geometryInputPrimitiveType != GL_LINES && desc.geometryInputPrimitiveType != GL_LINES_ADJACENCY_EXT && desc.geometryInputPrimitiveType != GL_TRIANGLES && desc.geometryInputPrimitiveType != GL_TRIANGLES_ADJACENCY_EXT) {
        //GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_EXT, GL_TRIANGLES, GL_TRIANGLES_ADJACENCY_EXT
        std::cout << "ERROR: ManagerShader::loadShader(...):\n"
                  << "Geometry Shader Input Primitive type must be either: \n"
                  << "   'GL_POINTS', 'GL_LINES', 'GL_LINES_ADJACENCY_EXT',\n"
                  << "   'GL_TRIANGLES', or 'GL_TRIANGLES_ADJACENCY_EXT'...\n"
                  << "   Invalid input primitive type ' " << desc.geometryInputPrimitiveType << "'...\n"
                  << "Press ENTER to exit...\n";
        std::cin.get();
        exit(-1);
    }

    GLSLShaderDataShared* shader = new GLSLShaderDataShared(desc);

    std::set<GLSLShaderDataShared*, ShaderSetCompare>::iterator it = ManagerShader::shaders.find(shader);
    if (it == ManagerShader::shaders.end()) {
        if (ManagerShader::instantiateOpenGLShader(shader)) {
            ManagerShader::shaders.insert(shader); //store in set
            return shader;
        } else {
            delete shader;
            shader = nullptr;
            return nullptr;
        }
    } else {
        delete shader; //if the shader was already found, use the original and delete the new one
        return *it; //return pointer to already loaded texture
    }
}

GLSLShader* ManagerShader::loadShader(const std::string& vertexShader, const std::string& fragmentShader,
    std::string geometryShader, GLenum geometryInputPrimitiveType,
    GLenum geometryOutputPrimitiveType, GLuint geometryMaxOutputVerts)
{
    return GLSLShader::New(vertexShader, fragmentShader, geometryShader, geometryInputPrimitiveType, geometryOutputPrimitiveType, geometryMaxOutputVerts);
}

GLSLShader* ManagerShader::loadShader(const GLSLShaderDescriptor& desc)
{
    return GLSLShader::New(desc);
}

bool ManagerShader::instantiateOpenGLShader(Aftr::GLSLShaderDataShared* shader)
{
    return shader->instantiate();
}

void ManagerShader::bindShader(GLuint shaderHandle)
{
    if (currentlyBoundShaderHandle == shaderHandle)
        return; //the desired shader is already bound, no need to call glUseProgram again

    glUseProgram(shaderHandle);
    currentlyBoundShaderHandle = shaderHandle;
}

std::string ManagerShader::queryShaderSupport()
{
    std::stringstream ss;
    ss << "glCreateShader"
       << " " << glCreateShader << std::endl
       << "glShaderSource"
       << " " << glShaderSource << std::endl
       << "glCompileShader"
       << " " << glCompileShader << std::endl
       << "glGetShaderiv"
       << " " << glGetShaderiv << std::endl
       << "glGetShaderInfoLog"
       << " " << glGetShaderInfoLog << std::endl
       << "glCreateProgram"
       << " " << glCreateProgram << std::endl
       << "glAttachShader"
       << " " << glAttachShader << std::endl
       << "glGetProgramiv"
       << " " << glGetProgramiv << std::endl
       << "glProgramParameteri"
       << " " << glProgramParameteriEXT << std::endl
       << "glGetProgramInfoLog"
       << " " << glGetProgramInfoLog << std::endl
       << "glLinkProgram"
       << " " << glLinkProgram << std::endl;
    return ss.str();
}

GLSLShader* ManagerShader::loadShaderCrazyBump()
{
    std::vector<std::string> attributes;
    //attributes.push_back( "tangent" );
    GLSLShader* shader = ManagerShader::loadShader(
        ManagerEnvironmentConfiguration::getSMM() + "/shaders/crazybump_DIFFUSE_NRM_SPEC.vert",
        ManagerEnvironmentConfiguration::getSMM() + "/shaders/crazybump_DIFFUSE_NRM_SPEC.frag",
        //ManagerEnvironmentConfiguration::getSMM() + "/shaders/fixedFunctionality.geom" );
        "");

    GLSLUniform* diffuseTexture = new GLSLUniform("diffuseTexture", utSAMPLER2D, shader->getHandle());
    diffuseTexture->setValues((GLuint)0);
    GLSLUniform* normalTexture = new GLSLUniform("normalTexture", utSAMPLER2D, shader->getHandle());
    normalTexture->setValues((GLuint)1);
    GLSLUniform* specularTexture = new GLSLUniform("specularTexture", utSAMPLER2D, shader->getHandle());
    specularTexture->setValues((GLuint)2);

    shader->addUniform(diffuseTexture);
    shader->addUniform(normalTexture);
    shader->addUniform(specularTexture);

    shader->addAttribute(new GLSLAttribute("tangent", atVEC3, shader));

    return shader;
}

GLSLShader* ManagerShader::loadShaderCrazyBumpParallaxMapping()
{
    std::vector<std::string> attributes;
    GLSLShader* shader = ManagerShader::loadShader(
        ManagerEnvironmentConfiguration::getSMM() + "/shaders/crazybump_DIFFUSE_NRMPARALLAX_SPEC.vert",
        ManagerEnvironmentConfiguration::getSMM() + "/shaders/crazybump_DIFFUSE_NRMPARALLAX_SPEC.frag",
        "");

    GLSLUniform* diffuseTexture = new GLSLUniform("diffuseTexture", utSAMPLER2D, shader->getHandle());
    diffuseTexture->setValues((GLuint)0);
    GLSLUniform* normalTexture = new GLSLUniform("normalTexture", utSAMPLER2D, shader->getHandle());
    normalTexture->setValues((GLuint)1);
    GLSLUniform* specularTexture = new GLSLUniform("specularTexture", utSAMPLER2D, shader->getHandle());
    specularTexture->setValues((GLuint)2);

    shader->addUniform(diffuseTexture);
    shader->addUniform(normalTexture);
    shader->addUniform(specularTexture);

    shader->addAttribute(new GLSLAttribute("tangent", atVEC3, shader));

    return shader;
}

GLSLShader* ManagerShader::loadShaderPointTesselatorBillboard(Camera** cam)
{
    return GLSLShaderPointTesselatorBillboard::New(cam);
}

void ManagerShader::loadGL32DefaultShaders()
{
    ManagerShader::loadShaderDefaultGL32();
    ManagerShader::loadShaderDefaultSelectionGL32();
    ManagerShader::loadShaderLoadingScreenGL32();
    ManagerShader::loadShaderIndexedGeometryLinesGL32();
    ManagerShader::loadShaderDefaultBoundingBoxLinesGL32();
    ManagerShader::loadShaderDefaultOrthoStencilGL32();
}

void ManagerShader::loadShaderDefaultGL32()
{
    GLSLShaderDefaultGL32* shdr = GLSLShaderDefaultGL32::New();
    ManagerShader::DEFAULT_SHADER = shdr;
    ManagerShader::defaultShadersLoadedByManagerAtInit.push_back(shdr);
}

void ManagerShader::loadShaderDefaultSelectionGL32()
{
    GLSLShaderDefaultSelectionGL32* shdr = GLSLShaderDefaultSelectionGL32::New();
    ManagerShader::DEFAULT_SELECTION_SHADER = shdr;
    ManagerShader::defaultShadersLoadedByManagerAtInit.push_back(static_cast<GLSLShader*>(shdr));
}

void ManagerShader::loadShaderLoadingScreenGL32()
{
    GLSLShaderDefaultLoadingScreenGL32* shdr = GLSLShaderDefaultLoadingScreenGL32::New();
    ManagerShader::DEFAULT_LOADING_SCREEN = shdr;
    ManagerShader::defaultShadersLoadedByManagerAtInit.push_back(shdr);
}

void ManagerShader::loadShaderIndexedGeometryLinesGL32()
{
    GLSLShaderDefaultIndexedGeometryLinesGL32* shdr = GLSLShaderDefaultIndexedGeometryLinesGL32::New();
    ManagerShader::DEFAULT_INDEXED_GEOMETRY_LINES = shdr;
    ManagerShader::defaultShadersLoadedByManagerAtInit.push_back(shdr);
}

void ManagerShader::loadShaderDefaultBoundingBoxLinesGL32()
{
    GLSLShaderDefaultBoundingBoxLinesGL32* shdr = GLSLShaderDefaultBoundingBoxLinesGL32::New();
    ManagerShader::DEFAULT_BOUNDING_BOX_LINES = shdr;
    ManagerShader::defaultShadersLoadedByManagerAtInit.push_back(static_cast<GLSLShader*>(shdr));
}

void ManagerShader::loadShaderDefaultOrthoStencilGL32()
{
    GLSLShaderDefaultOrthoStencilGL32* shdr = GLSLShaderDefaultOrthoStencilGL32::New();
    ManagerShader::DEFAULT_ORTHO_STENCIL_SHADER = shdr;
    ManagerShader::defaultShadersLoadedByManagerAtInit.push_back(static_cast<GLSLShader*>(shdr));
}

bool ManagerShader::deletePreviouslyLoadedShaderFromCachedSet(GLSLShaderDataShared* shader)
{
    std::set<GLSLShaderDataShared*, ShaderSetCompare>::iterator it = ManagerShader::shaders.find(shader);
    if (it == ManagerShader::shaders.end()) {
        std::cout << AFTR_FILE_LINE_STR << ": Shader Data Shared not loaded in ManagerShader... " << shader->toString() << "\n";
        return false;
    } else {
        GLSLShaderDataShared* ptr = *it;
        ManagerShader::shaders.erase(it);
        delete ptr;
        ptr = nullptr;
        return true;
    }
}
