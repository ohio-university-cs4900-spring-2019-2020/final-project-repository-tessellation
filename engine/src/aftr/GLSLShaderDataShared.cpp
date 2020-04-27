#include "GLSLShaderDataShared.h"
#include "AftrUtilities.h"
#include "GLSLShaderDescriptor.h"
#include "ManagerOpenGLState.h"
#include "ManagerShader.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
using namespace Aftr;

Aftr::GLSLShaderDataShared::GLSLShaderDataShared()
{
    this->vertexShaderPath = "";
    this->fragmentShaderPath = "";
    this->geometryShaderPath = "";
    this->tessellationControlShaderPath = "";
    this->tessellationEvalShaderPath = "";
    this->computeShaderPath = "";

    this->vertexShaderHandle = 0;
    this->fragmentShaderHandle = 0;
    this->geometryShaderHandle = 0;
    this->tessellationControlShaderHandle = 0;
    this->tessellationEvalShaderHandle = 0;
    this->computeShaderHandle = 0;

    this->shaderHandle = 0;
}

Aftr::GLSLShaderDataShared::GLSLShaderDataShared(const GLSLShaderDataShared& shaderData)
{
    (*this) = shaderData;
}

Aftr::GLSLShaderDataShared::GLSLShaderDataShared(const std::string& vertexShader, const std::string& fragmentShader,
    const std::string& geometryShader, GLenum geometryInputPrimitiveType,
    GLenum geometryOutputPrimitiveType, GLuint geometryMaxOutputVerts,
    const std::string& computeShader)
{
    this->vertexShaderPath = vertexShader;
    this->fragmentShaderPath = fragmentShader;
    this->geometryShaderPath = geometryShader;
    this->tessellationControlShaderPath = "";
    this->tessellationEvalShaderPath = "";
    this->computeShaderPath = computeShader;

    //A shader can ONLY have a valid computeShader by itself. If a computeShader will be loaded,
    //the vertex, fragment, geometry, etc shaders must be empty ("") or a linker error will occur.

    this->vertexShaderHandle = 0;
    this->fragmentShaderHandle = 0;
    this->geometryShaderHandle = 0;
    this->tessellationControlShaderHandle = 0;
    this->tessellationEvalShaderHandle = 0;
    this->computeShaderHandle = 0;

    this->shaderHandle = 0;

    this->geomShdrInputPrimType = geometryInputPrimitiveType;
    this->geomShdrOutputPrimType = geometryOutputPrimitiveType;
    this->geomShdrOutputMaxVerts = geometryMaxOutputVerts;
    this->tessShdrMaxPatchVerts = 3;
}

GLSLShaderDataShared::GLSLShaderDataShared(const GLSLShaderDescriptor& desc)
{
    this->vertexShaderPath = desc.vertexShader;
    this->fragmentShaderPath = desc.fragmentShader;
    this->geometryShaderPath = desc.geometryShader;
    this->tessellationControlShaderPath = desc.tessellationControlShader;
    this->tessellationEvalShaderPath = desc.tessellationEvalShader;
    this->computeShaderPath = desc.computeShader;

    //A shader can ONLY have a valid computeShader by itself. If a computeShader will be loaded,
    //the vertex, fragment, geometry, etc shaders must be empty ("") or a linker error will occur.

    this->vertexShaderHandle = 0;
    this->fragmentShaderHandle = 0;
    this->geometryShaderHandle = 0;
    this->tessellationControlShaderHandle = 0;
    this->tessellationEvalShaderHandle = 0;
    this->computeShaderHandle = 0;

    this->shaderHandle = 0;

    this->geomShdrInputPrimType = desc.geometryInputPrimitiveType;
    this->geomShdrOutputPrimType = desc.geometryOutputPrimitiveType;
    this->geomShdrOutputMaxVerts = desc.geometryMaxOutputVerts;
    this->tessShdrMaxPatchVerts = desc.tessellationMaxVerticesPerPatch;
}

GLSLShaderDataShared::~GLSLShaderDataShared()
{
    //std::cout << "~GLSLShaderDataShared()" << std::endl;
    //std::cin.get();
    if (glDeleteShader != NULL) {
        glDeleteShader(this->vertexShaderHandle);
        this->vertexShaderHandle = 0;
        glDeleteShader(this->geometryShaderHandle);
        this->geometryShaderHandle = 0;
        glDeleteShader(this->tessellationControlShaderHandle);
        this->tessellationControlShaderHandle = 0;
        glDeleteShader(this->tessellationEvalShaderHandle);
        this->tessellationEvalShaderHandle = 0;
        glDeleteShader(this->fragmentShaderHandle);
        this->fragmentShaderHandle = 0;
        glDeleteShader(this->computeShaderHandle);
        this->computeShaderHandle = 0;
    }

    if (glDeleteProgram != NULL)
        glDeleteProgram(this->shaderHandle);
}

GLSLShaderDataShared& Aftr::GLSLShaderDataShared::operator=(const GLSLShaderDataShared& shaderData)
{
    if (this != &shaderData) {
        this->vertexShaderPath = shaderData.vertexShaderPath;
        this->fragmentShaderPath = shaderData.fragmentShaderPath;
        this->geometryShaderPath = shaderData.geometryShaderPath;
        this->tessellationControlShaderPath = shaderData.tessellationControlShaderPath;
        this->tessellationEvalShaderPath = shaderData.tessellationEvalShaderPath;
        this->computeShaderPath = shaderData.computeShaderPath;

        this->vertexShaderHandle = shaderData.vertexShaderHandle;
        this->fragmentShaderHandle = shaderData.fragmentShaderHandle;
        this->geometryShaderHandle = shaderData.geometryShaderHandle;
        this->tessellationControlShaderHandle = shaderData.tessellationControlShaderHandle;
        this->tessellationEvalShaderHandle = shaderData.tessellationEvalShaderHandle;
        this->computeShaderHandle = shaderData.computeShaderHandle;

        this->geomShdrInputPrimType = shaderData.geomShdrInputPrimType;
        this->geomShdrOutputPrimType = shaderData.geomShdrOutputPrimType;
        this->geomShdrOutputMaxVerts = shaderData.geomShdrOutputMaxVerts;
        this->tessShdrMaxPatchVerts = shaderData.tessShdrMaxPatchVerts;

        this->shaderHandle = shaderData.shaderHandle;
    }
    return (*this);
}

bool GLSLShaderDataShared::operator==(const GLSLShaderDataShared& shader) const
{
    if (this == &shader)
        return true;
    else if (this->vertexShaderPath == shader.vertexShaderPath && this->fragmentShaderPath == shader.fragmentShaderPath && this->geometryShaderPath == shader.geometryShaderPath && this->tessellationControlShaderPath == shader.tessellationControlShaderPath && this->tessellationEvalShaderPath == shader.tessellationEvalShaderPath && this->geomShdrInputPrimType == shader.geomShdrInputPrimType && this->geomShdrOutputPrimType == shader.geomShdrOutputPrimType && this->geomShdrOutputMaxVerts == shader.geomShdrOutputMaxVerts && this->tessShdrMaxPatchVerts == shader.tessShdrMaxPatchVerts && this->computeShaderPath == shader.computeShaderPath)
        return true;

    return false;
}

bool GLSLShaderDataShared::operator!=(const GLSLShaderDataShared& shader) const
{
    return (!((*this) == shader));
}

bool Aftr::GLSLShaderDataShared::operator<(const GLSLShaderDataShared& shader) const
{
    if (this == &shader)
        return false;
    else {
        //this will still not work properly if the same vert, frag, geo shader is used w/ diff
        //geom shader params (ie, geoInputPrim, geoOutputPrim, geoMaxOutputVerts) and
        //tess shader params (ie, tessMaxPatchSize)
        std::string myPath = this->vertexShaderPath + this->fragmentShaderPath + this->geometryShaderPath + this->tessellationControlShaderPath + this->tessellationEvalShaderPath + this->computeShaderPath;
        std::string otherPath = shader.vertexShaderPath + shader.fragmentShaderPath + shader.geometryShaderPath + this->tessellationControlShaderPath + this->tessellationEvalShaderPath + shader.computeShaderPath;
        return (myPath < otherPath);
    }
}

std::string GLSLShaderDataShared::toString() const
{
    std::stringstream ss;
    ss << "GLSLShaderDataShared Info...\n"
       << "   VertexShaderPath:'" << this->vertexShaderPath << "'...\n"
       << "   FragmentShaderPath: '" << this->fragmentShaderPath << "'...\n"
       << "   GeometryShaderPath: '" << this->geometryShaderPath << "'...\n"
       << "   GeometryShaderInputPrimType: '" << this->geomShdrInputPrimType << "'...\n"
       << "   GeometryShaderOutputPrimType: '" << this->geomShdrOutputPrimType << "'...\n"
       << "   GeometryShaderOutputMaxVerts: '" << this->geomShdrOutputMaxVerts << "'...\n"
       << "   TessellationControlShaderPath: '" << this->tessellationControlShaderPath << "'...\n"
       << "   TessellationEvalShaderPath: '" << this->tessellationEvalShaderPath << "'...\n"
       << "   TessellationMaxPatchVerts: '" << this->tessShdrMaxPatchVerts << "'...\n"
       << "   ComputeShaderPath: '" << this->computeShaderPath << "'...\n"
       << "   GLuint   :'" << this->getShaderHandle() << "'\n";
    return ss.str();
}

std::string GLSLShaderDataShared::getVertexShaderPath() const
{
    return this->vertexShaderPath;
}

std::string GLSLShaderDataShared::getFragmentShaderPath() const
{
    return this->fragmentShaderPath;
}

std::string GLSLShaderDataShared::getGeometryShaderPath() const
{
    return this->geometryShaderPath;
}

std::string GLSLShaderDataShared::getTessellationControlShaderPath() const
{
    return this->tessellationControlShaderPath;
}

std::string GLSLShaderDataShared::getTessellationEvalShaderPath() const
{
    return this->tessellationEvalShaderPath;
}

std::string GLSLShaderDataShared::getComputeShaderPath() const
{
    return this->computeShaderPath;
}

void GLSLShaderDataShared::setShaderHandle(GLuint shaderHandle)
{
    this->shaderHandle = shaderHandle;
}

GLuint GLSLShaderDataShared::getShaderHandle() const
{
    return this->shaderHandle;
}

void GLSLShaderDataShared::setVertexShaderPath(const std::string& vertexShaderPath)
{
    this->vertexShaderPath = vertexShaderPath;
}

void GLSLShaderDataShared::setFragmentShaderPath(const std::string& fragmentShaderPath)
{
    this->fragmentShaderPath = fragmentShaderPath;
}

void GLSLShaderDataShared::setGeometryShaderPath(const std::string& geometryShaderpath)
{
    this->geometryShaderPath = geometryShaderPath;
}

void GLSLShaderDataShared::setTessellationControlShaderPath(const std::string& tessControlShaderPath)
{
    this->tessellationControlShaderPath = tessControlShaderPath;
}

void GLSLShaderDataShared::setTessellationEvalShaderPath(const std::string& tessEvalShaderPath)
{
    this->tessellationEvalShaderPath = tessEvalShaderPath;
}

void GLSLShaderDataShared::setComputeShaderPath(const std::string& computeShaderpath)
{
    this->computeShaderPath = computeShaderpath;
}

std::ostream& Aftr::operator<<(std::ostream& out, const GLSLShaderDataShared& shdrData)
{
    out << shdrData.toString();
    return out;
}

bool Aftr::GLSLShaderDataShared::instantiate()
{
    if (!((glCreateShader != NULL) && (glShaderSource != NULL) && (glCompileShader != NULL) && (glGetShaderiv != NULL) && //GL 3.0+ changes
            (glGetShaderInfoLog != NULL) && //GL 3.0+ changes
            (glCreateProgram != NULL) && (glAttachShader != NULL) && (glGetProgramiv != NULL) &&
            //(glProgramParameteriEXT != NULL ) && //check is in geometry shader linker so 2.1 can load non geo shaders
            (glGetIntegerv != NULL) &&
            (glGetProgramInfoLog != NULL) && (glLinkProgram != NULL))) {

        std::cout << "WARNING: Your graphics card does not support necessary functions to\n"
                  << "   instantiate an OpenGL Shader program...\n"
                  << "   NOT creating GLSLShader...\n";
        //std::cin.get();
        return false;
    }

    printOpenGLErrors(201214, NULL, AFTR_FILE_LINE_STR);

    bool hasLoadedSuccessfully = true;

    if (this->vertexShaderPath != "")
        hasLoadedSuccessfully &= this->loadVertexShader(this->vertexShaderPath);

    if (this->geometryShaderPath != "")
        hasLoadedSuccessfully &= this->loadGeometryShader(this->geometryShaderPath);

    if (this->tessellationControlShaderPath != "")
        hasLoadedSuccessfully &= this->loadTessellationControlShader(this->tessellationControlShaderPath);

    if (this->tessellationEvalShaderPath != "")
        hasLoadedSuccessfully &= this->loadTessellationEvalShader(this->tessellationEvalShaderPath);

    if (this->fragmentShaderPath != "")
        hasLoadedSuccessfully &= this->loadFragmentShader(this->fragmentShaderPath);

    if (this->computeShaderPath != "")
        hasLoadedSuccessfully &= this->loadComputeShader(this->computeShaderPath);

    hasLoadedSuccessfully &= this->createProgramShader();
    hasLoadedSuccessfully &= this->linkProgramShader();

    // make sure hardware can support our desired number of patch vertices
    if (this->tessellationControlShaderPath != "" || this->tessellationEvalShaderPath != "") {
        GLint data;
        glGetIntegerv(GL_MAX_PATCH_VERTICES, &data);
        if (this->tessShdrMaxPatchVerts > static_cast<GLuint>(data)) {
            std::cout << "Warning: Maximum tessellation shader patch vertices is " << data << ", but " << this->tessShdrMaxPatchVerts << " were requested...\n";
        }
    }

    return hasLoadedSuccessfully;

    //if( this->geometryShaderPath == "" )
    //   return loadVertexShader(vertexShaderPath) &&
    //   loadFragmentShader(fragmentShaderPath) &&
    //   createProgramShader() &&
    //   linkProgramShader();

    //return loadVertexShader(vertexShaderPath) &&
    //   loadFragmentShader(fragmentShaderPath) &&
    //   loadGeometryShader(geometryShaderPath) &&
    //   createProgramShader() &&
    //   linkProgramShader();
}

bool GLSLShaderDataShared::loadShader(GLenum shaderType, const std::string& shaderPath, GLuint& handle)
{
    std::ifstream fin;
    fin.open(shaderPath.c_str());

    if (fin.fail()) {
        std::cout << "ERROR: The shader " << shaderPath << " failed to open." << std::endl;
        std::cin.get(); //error get
    } else {
        std::string shader;
        std::string data;
        std::getline(fin, shader);
        while (!fin.eof()) {
            shader += data;
            shader += '\n';
            getline(fin, data);
        }
        shader += data;

        //info for debugging if there are any issues with the shaders
        GLint logLength = 0;
        GLint status = 0;

        //creates the storage space for the shader
        handle = glCreateShader(shaderType);

        //loads the shader information from the c-string
        const char* str = shader.c_str();
        std::cout << "Compiling Shader: '" << shaderPath << "'\n";
        glShaderSource(handle, 1, (const GLchar**)&str, NULL);

        //compiles the shader
        glCompileShader(handle);

        //prints off any error messages
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLchar* log = (GLchar*)malloc(logLength);
            glGetShaderInfoLog(handle, logLength, NULL, log);
            printf("Vertex Shader compile log:\n%s\n", log);
            free(log);
        }
        printOpenGLErrors(2012141, NULL, AFTR_FILE_LINE_STR);
        //checks the status of the compilation if there was an issue reports error
        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
        if (status == 0) {
            printf("Failed to compile shader %d\n", handle);
            std::cout << "Press ENTER to continue without this shader...\n";
            std::cin.get();
            return false;
        }
        return true;
    }
    return false;
}

bool GLSLShaderDataShared::loadVertexShader(const std::string& vertexShaderPath)
{
    return loadShader(GL_VERTEX_SHADER, vertexShaderPath, this->vertexShaderHandle);
}

bool GLSLShaderDataShared::loadFragmentShader(const std::string& fragmentShaderPath)
{
    return loadShader(GL_FRAGMENT_SHADER, fragmentShaderPath, this->fragmentShaderHandle);
}

bool GLSLShaderDataShared::loadGeometryShader(const std::string& geometryShaderPath)
{
    return loadShader(GL_GEOMETRY_SHADER_EXT, geometryShaderPath, this->geometryShaderHandle);
}

bool GLSLShaderDataShared::loadTessellationControlShader(const std::string& tessControlShaderPath)
{
    return loadShader(GL_TESS_CONTROL_SHADER, tessControlShaderPath, this->tessellationControlShaderHandle);
}

bool GLSLShaderDataShared::loadTessellationEvalShader(const std::string& tessEvalShaderPath)
{
    return loadShader(GL_TESS_EVALUATION_SHADER, tessEvalShaderPath, this->tessellationEvalShaderHandle);
}

bool GLSLShaderDataShared::loadComputeShader(const std::string& computeShaderPath)
{
    printOpenGLErrors(0, NULL, AFTR_FILE_LINE_STR);
    return this->loadShader(GL_COMPUTE_SHADER, computeShaderPath, this->computeShaderHandle);
}

bool Aftr::GLSLShaderDataShared::createProgramShader()
{
    shaderHandle = glCreateProgram();

    if (this->vertexShaderHandle != 0)
        glAttachShader(this->shaderHandle, this->vertexShaderHandle);

    if (this->geometryShaderHandle != 0)
        glAttachShader(this->shaderHandle, this->geometryShaderHandle);

    if (this->tessellationControlShaderHandle != 0)
        glAttachShader(this->shaderHandle, this->tessellationControlShaderHandle);

    if (this->tessellationEvalShaderHandle != 0)
        glAttachShader(this->shaderHandle, this->tessellationEvalShaderHandle);

    if (this->fragmentShaderHandle != 0)
        glAttachShader(this->shaderHandle, this->fragmentShaderHandle);

    if (this->computeShaderHandle != 0)
        glAttachShader(this->shaderHandle, this->computeShaderHandle);

    printOpenGLErrors(123456, NULL, AFTR_FILE_LINE_STR);
    return true;
}

bool GLSLShaderDataShared::linkProgramShader()
{
    GLint logLength = 0;
    GLint status = 0;

    if (this->geometryShaderHandle != 0) {
        if (glProgramParameteriEXT == NULL) {
            std::cout << "glProgramParameteriEXT is NULL, cannot create a geometry shader on this hardware. Geometry Shader\n"
                      << "will not be linked...";
            return false;
        }

        if (ManagerOpenGLState::isGLContextProfileCompatibility()) {
            //Set Geometry shader parameters
            glProgramParameteri(this->shaderHandle, GL_GEOMETRY_INPUT_TYPE_EXT, this->geomShdrInputPrimType);
            glProgramParameteri(this->shaderHandle, GL_GEOMETRY_OUTPUT_TYPE_EXT, this->geomShdrOutputPrimType);
            glProgramParameteri(this->shaderHandle, GL_GEOMETRY_VERTICES_OUT_EXT, this->geomShdrOutputMaxVerts);
        } else //still reading up on it, but it looks like these are handled in the layout code inside the shader now
        {
            //handled in shader
        }
    }

    glLinkProgram(shaderHandle);
    printOpenGLErrors(98765, NULL, AFTR_FILE_LINE_STR);

    std::cout << "Linking!" << std::endl;

    glGetProgramiv(shaderHandle, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar* log = (char*)malloc(logLength);
        glGetProgramInfoLog(this->shaderHandle, logLength, NULL, log);
        printf("Program link log:\n%s\n", log);
        free(log);
    }

    glGetProgramiv(this->shaderHandle, GL_LINK_STATUS, &status);
    if (status == 0) {
        std::cout << "Failed to link program " << shaderHandle << ". Status returned is " << status << std::endl;
        std::cout << "Hit ENTER to continue normally...Fix this prior to a release...\n";
        std::cin.get();
        return false;
    } else {
        std::cout << "Link successful for program " << shaderHandle << std::endl;
    }

    return true;
}
