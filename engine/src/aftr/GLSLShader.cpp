#include "GLSLShader.h"
#include "AftrConfig.h"

#ifdef AFTR_CONFIG_USE_OGL_GLEW

#include <iostream>
#include <sstream>

#include "Camera.h"
#include "GLSLAttribute.h"
#include "GLSLShaderDescriptor.h"
#include "GLSLShaderDataShared.h"
#include "GLSLUniform.h"
#include "ManagerOpenGLState.h"
#include "ManagerSDLTime.h"
#include "ManagerShader.h"
#include "Mat4.h"
#include "ModelMeshSkin.h"

using namespace Aftr;

GLSLShader* GLSLShader::New(const std::string& vertexShader, const std::string& fragmentShader,
    std::string geometryShader, GLenum geometryInputPrimitiveType,
    GLenum geometryOutputPrimitiveType, GLuint geometryMaxOutputVerts)
{
    GLSLShaderDataShared* dataShared = ManagerShader::loadShaderDataShared(vertexShader, fragmentShader, geometryShader,
        geometryInputPrimitiveType, geometryOutputPrimitiveType,
        geometryMaxOutputVerts);
    if (dataShared == nullptr)
        return nullptr;

    GLSLShader* shader = new GLSLShader(dataShared);
    return shader;
}

GLSLShader* GLSLShader::New(const GLSLShaderDescriptor& desc)
{
    GLSLShaderDataShared* dataShared = ManagerShader::loadShaderDataShared(desc);
    if (dataShared == nullptr)
        return nullptr;

    GLSLShader* shader = new GLSLShader(dataShared);
    return shader;
}

//GLSLShader::GLSLShader()
//{
//   this->dataShared = nullptr;
//   if( ManagerOpenGLState::isGLContextProfileVersion32orGreater() )
//      this->initUniformBlockInfo();
//}

GLSLShader::GLSLShader(GLSLShaderDataShared* sharedData)
{
    this->dataShared = sharedData;
    if (ManagerOpenGLState::isGLContextProfileVersion32orGreater())
        this->initUniformBlockInfo();
}

GLSLShader::GLSLShader(const GLSLShader& toCopy)
{
    (*this) = toCopy;
}

GLSLShader::~GLSLShader()
{
    for (size_t i = 0; i < uniforms.size(); i++)
        delete uniforms[i];
    for (size_t i = 0; i < attributes.size(); i++)
        delete attributes[i];
    this->attributes.clear();
    this->uniforms.clear();
    this->activeUniformBlocks.clear();

    this->dataShared = nullptr; //not owned, do not delete
}

void GLSLShader::initUniformBlockInfo()
{
    //See if the this shader is using the engine default "CameraTransforms" UniformBlock
    if (!this->useUniformBlock("CameraTransforms", ManagerShader::getUniformBlockBindingCameraTransforms()))
        std::cout << "Shader (GLuint Handle" << this->getHandle() << ") does NOT contain and is NOT using \"CameraTransforms\" Uniform Block...\n";
    if (!this->useUniformBlock("LightInfo", ManagerShader::getUniformBlockBindingLightInfo()))
        std::cout << "Shader (GLuint Handle" << this->getHandle() << ") does NOT contain and is NOT using \"LightInfo\" Uniform Block...\n";
}

GLSLShader& GLSLShader::operator=(const GLSLShader& shader)
{
    if (this != &shader) {
        for (size_t i = 0; i < uniforms.size(); i++)
            delete uniforms[i];
        for (size_t i = 0; i < attributes.size(); i++)
            delete attributes[i];
        this->uniforms.clear();
        this->attributes.clear();
        this->activeUniformBlocks.clear();

        this->dataShared = shader.dataShared;

        for (size_t i = 0; i < shader.getUniforms()->size(); i++)
            this->uniforms.push_back(shader.getUniforms()->at(i)->getCopyOfThisInstance());

        for (size_t i = 0; i < shader.getAttributes()->size(); i++) {
            GLSLAttribute* copy = shader.getAttributes()->at(i)->getCopyOfThisInstance();
            copy->setParent(this);
            this->attributes.push_back(copy);
        }

        for (size_t i = 0; i < shader.activeUniformBlocks.size(); ++i) {
            auto& val = shader.activeUniformBlocks[i];
            this->activeUniformBlocks.push_back(val);
        }
    }
    return *this;
}

GLSLShader* GLSLShader::getCopyOfThisInstance()
{
    GLSLShader* copy = new GLSLShader(*this);
    return copy;
}

void GLSLShader::addAttribute(GLSLAttribute* attributeVar)
{
    attributes.push_back(attributeVar);
}

void GLSLShader::addUniform(GLSLUniform* uniform)
{
    uniforms.push_back(uniform);
}

bool GLSLShader::useUniformBlock(const std::string uniformBlockName, GLint uniformBlockBinding)
{
    GLuint uniformBlockIndex = glGetUniformBlockIndex(this->getHandle(), uniformBlockName.c_str());
    if (static_cast<GLint>(uniformBlockIndex) > -1) {
        glUniformBlockBinding(this->getHandle(), uniformBlockIndex, uniformBlockBinding);
        this->activeUniformBlocks.push_back(std::make_tuple(uniformBlockName, uniformBlockIndex, uniformBlockBinding));
        return true;
    }
    return false;
}

void GLSLShader::bind()
{
    ManagerShader::bindShader(this->getHandle());
}

void GLSLShader::bind(const Mat4& modelMatrix, const Mat4& normalMatrix, const Camera& cam, const ModelMeshSkin& skin)
{
    GLSLShader::bind();

    //Subclasses may update any necessary uniforms, attributes, uniform blocks, Shader Storage Buffer Objects, etc
    //std::cout << "WARNING: " << AFTR_FILE_LINE_STR << ": GLSLShader::bind() does not specify any uniforms to update!\n"
    //          << "   This method should be defined in a subclassed instance of the GLSLShader.\n"
    //          << "   Not setting any uniforms in this bind() call...\n";
}

void GLSLShader::unbind()
{
    ManagerShader::bindShader(0);
}

GLuint GLSLShader::getHandle() const
{
    return dataShared->getShaderHandle();
}

std::string GLSLShader::toString() const
{
    std::stringstream ss;
    ss << this->dataShared->toString();
    ss << this->toStringAttributes();
    ss << this->toStringUniforms();
    ss << this->toStringUniformBlocks();
    return ss.str();
}

std::string GLSLShader::toStringAttributes() const
{
    std::stringstream ss;

    ss << "\nActive Attributes:\n";
    { //this may require an OpenGL 4.3 context or higher
        GLint numAttribs = 0;
        glGetProgramInterfaceiv(this->getHandle(), GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numAttribs);

        ss << "GPU Side Active Attributes: " << numAttribs << " active attributes within this GPU GLSLShader...\n";
        ss << "Location, Name, Type\n";

        GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };
        for (GLint i = 0; i < numAttribs; ++i) {
            GLint results[3];
            glGetProgramResourceiv(this->getHandle(), GL_PROGRAM_INPUT, (GLuint)i, 3, properties, 3, NULL, results);

            GLint nameBufSize = results[0] + 1; //NULL terminated space
            char* name = new char[nameBufSize];
            glGetProgramResourceName(this->getHandle(), GL_PROGRAM_INPUT, i, nameBufSize, NULL, name);
            ss << results[2] << ", \"" << name << "\", " << GLSLShader::getTypeString(results[1]) << "\n";
            delete[] name;
        }
    }

    { //CPU-side Attributes
        ss << "CPU Side Attribute Information: " << this->attributes.size() << " CPU-side attributes within this GLSLShader...\n";
        ss << "Location, Name, Type\n";
        for (size_t i = 0; i < this->attributes.size(); ++i)
            ss << this->attributes.at(i)->getAttributeLocationInShader() << ", \"" << this->attributes.at(i)->getName() << "\", " << this->attributes.at(i)->getAttributeType() << "\n";
    }
    return ss.str();
}

std::string GLSLShader::toStringUniforms() const
{
    std::stringstream ss;

    ss << "\n\nActive Uniforms:\n";
    { //this may require an OpenGL 4.3 context or higher
        GLint numUniforms = 0;
        glGetProgramInterfaceiv(this->getHandle(), GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms);

        ss << "GPU Side Active Uniforms: " << numUniforms << " active Uniforms within this GPU GLSLShader...\n";
        ss << "Location, Name, Type\n";

        GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX };
        for (GLint i = 0; i < numUniforms; ++i) {
            GLint results[4];
            glGetProgramResourceiv(this->getHandle(), GL_UNIFORM, (GLuint)i, 4, properties, 4, NULL, results);

            if (results[3] != -1) {
                ss << "Found Uniform Block, skipping details of all uniforms in block...\n";
                continue;
            }

            GLint nameBufSize = results[0] + 1; //NULL terminated space
            char* name = new char[nameBufSize];
            glGetProgramResourceName(this->getHandle(), GL_UNIFORM, i, nameBufSize, NULL, name);
            ss << results[2] << ", \"" << name << "\", " << GLSLShader::getTypeString(results[1]) << "\n";
            delete[] name;
        }
    }

    { //CPU-side Uniforms
        ss << "CPU Side Active Uniforms Information: " << this->uniforms.size() << " CPU-side uniforms within this GLSLShader...\n";
        ss << "Location, Name, Type\n";
        for (size_t i = 0; i < this->uniforms.size(); ++i)
            ss << this->uniforms.at(i)->getUniformHandle() << ", \"" << this->uniforms.at(i)->getName() << "\", " << this->uniforms.at(i)->getUniformType() << "\n";
    }

    return ss.str();
}

std::string GLSLShader::toStringUniformBlocks() const
{
    std::stringstream ss;

    ss << "\n\nActive Uniform Blocks:\n";

    { //GPU-side Uniform Blocks
        ss << "GPU Side Uniform Blocks Information is not implemented yet...\n";
        ss << "Location, Name, Type\n";
        GLint numUBlocks = 0;
        glGetProgramInterfaceiv(this->getHandle(), GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numUBlocks);
        const GLenum blockProperties[1] = { GL_NUM_ACTIVE_VARIABLES };
        const GLenum activeUnifProp[1] = { GL_ACTIVE_VARIABLES };
        const GLenum unifProperties[3] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };

        for (int blockIx = 0; blockIx < numUBlocks; ++blockIx) {
            GLint numActiveUnifs = 0;
            glGetProgramResourceiv(this->getHandle(), GL_UNIFORM_BLOCK, blockIx, 1, blockProperties, 1, NULL, &numActiveUnifs);

            if (!numActiveUnifs)
                continue;

            std::vector<GLint> blockUnifs(numActiveUnifs);
            glGetProgramResourceiv(this->getHandle(), GL_UNIFORM_BLOCK, blockIx, 1, activeUnifProp, numActiveUnifs, NULL, &blockUnifs[0]);

            for (int i = 0; i < numActiveUnifs; ++i) {
                GLint results[3];
                glGetProgramResourceiv(this->getHandle(), GL_UNIFORM, blockUnifs[i], 3, unifProperties, 3, NULL, results);

                //Get the name. Must use a std::vector rather than a std::string for C++03 standards issues.
                //C++11 would let you use a std::string directly.
                GLint nameBufSize = results[0] + 1; // NULL terminated space
                char* name = new char[nameBufSize];
                glGetProgramResourceName(this->getHandle(), GL_UNIFORM, blockUnifs[i], nameBufSize, NULL, name);
                ss << results[2] << ", \"" << name << "\", " << GLSLShader::getTypeString(results[1]) << "\n";
            }
        }
    }

    { //CPU-side Uniform Blocks
        ss << "CPU Side Uniform Blocks Information: " << this->uniforms.size() << " CPU-side uniforms within this GLSLShader...\n";
        ss << "Uniform Block Name, UniformBlockIndex, UniformBlockBinding (Global binding point)\n";
        for (size_t i = 0; i < this->activeUniformBlocks.size(); ++i) {
            auto& t = this->activeUniformBlocks[i];
            ss << std::get<0>(t) << ", " << std::get<1>(t) << ", " << std::get<2>(t) << "\n";
        }
    }

    return ss.str();
}

std::string GLSLShader::getTypeString(GLenum type)
{
    // There are many more types than are covered here, but
    // these are the most common in these examples.
    switch (type) {
    case GL_FLOAT:
        return "GL_FLOAT";
    case GL_FLOAT_VEC2:
        return "GL_FLOAT_VEC2";
    case GL_FLOAT_VEC3:
        return "GL_FLOAT_VEC3";
    case GL_FLOAT_VEC4:
        return "GL_FLOAT_VEC4";
    case GL_DOUBLE:
        return "GL_DOUBLE";
    case GL_INT:
        return "GL_INT";
    case GL_UNSIGNED_INT:
        return "GL_UNSIGNED_INT";
    case GL_BOOL:
        return "GL_BOOL";
    case GL_FLOAT_MAT2:
        return "GL_FLOAT_MAT2";
    case GL_FLOAT_MAT3:
        return "GL_FLOAT_MAT3";
    case GL_FLOAT_MAT4:
        return "GL_FLOAT_MAT4";
    case GL_SAMPLER_2D:
        return "GL_SAMPLER_2D";
    default:
        return "?";
    }
}

#endif //AFTR_CONFIG_USE_OGL_GLEW
