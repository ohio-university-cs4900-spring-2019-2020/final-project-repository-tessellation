#pragma once

#include <string>

#include "AftrOpenGLIncludes.h"

#ifdef AFTR_CONFIG_USE_OGL_GLEW

namespace Aftr {
    /**
        This class describes the stages and properties required to compile a shader
        program. An empty shader source path indicates that the stage is not used.
    */
    struct GLSLShaderDescriptor {
        // paths to shader source files for the stages
        std::string vertexShader = "";
        std::string fragmentShader = "";
        std::string geometryShader = "";
        std::string tessellationControlShader = "";
        std::string tessellationEvalShader = "";
        std::string computeShader = "";
        // properties specific to geometry shaders
        GLenum geometryInputPrimitiveType = GL_TRIANGLES;
        GLenum geometryOutputPrimitiveType = GL_TRIANGLE_STRIP;
        GLuint geometryMaxOutputVerts = 3;
        // properties specific to tessellation shaders
        GLuint tessellationMaxVerticesPerPatch = 3;
    };
}

#endif // AFTR_CONFIG_USE_OGL_GLEW