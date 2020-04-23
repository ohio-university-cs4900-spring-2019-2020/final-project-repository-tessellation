#pragma once

#include "GLSLShader.h"
#include "Mat4Fwd.h"
#include "VectorFwd.h"

namespace Aftr {
class Model;

class GLSLEarthShader : public GLSLShader {
public:
    static GLSLEarthShader* New(bool useLines, float scale, float tess);
    static GLSLEarthShader* New(GLSLShaderDataShared* shdrData);
    virtual ~GLSLEarthShader();
    virtual void bind(const Mat4& modelMatrix, const Mat4& normalMatrix, const Camera& cam, const ModelMeshSkin& skin);

    GLSLEarthShader& operator=(const GLSLEarthShader& shader);

    virtual void setMVPMatrix(const Mat4& mvpMatrix);

    void setScaleFactor(float s);
    void setTessellationFactor(float t);

    /**
      Returns a copy of this instance. This is identical to invoking the copy constructor with
      the addition that this preserves the polymorphic type. That is, if this was a subclass
      of GLSLShader with additional members and methods, this will internally create the
      shader instance as that subclass, thereby preserving polymorphic behavior, members, and
      methods.

      This is in contrast with a copy constructor in the case where one performs:
      GLSLShader* myCopy = new GLSLShader( shaderToBeCopied );
      This will always create a GLSLShader* instance, not a subclass corresponding to the exact
      type of shaderToBeCopied.
    */
    virtual GLSLShader* getCopyOfThisInstance();

protected:
    float scaleFactor;
    float tessellationFactor;

    GLSLEarthShader(GLSLShaderDataShared* dataShared);
    GLSLEarthShader(const GLSLEarthShader&);
};
}