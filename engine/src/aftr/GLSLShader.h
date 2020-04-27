/**
   \class GLSLShader
   \author Chad Mourning and Scott Nykl
   \brief A wrapper for OpenGL Shaders
*/

#pragma once

#include "AftrConfig.h"
#ifdef AFTR_CONFIG_USE_OGL_GLEW
#include "AftrOpenGLIncludes.h"
#include "Mat4Fwd.h"

#include <vector>
#include <string>

namespace Aftr
{
struct GLSLShaderDescriptor; // added for describing tessellation stages
class GLSLUniform;
class GLSLAttribute;
class GLSLShaderDataShared;
class Camera;
class ModelMeshSkin;

/**
   \brief A class that represents a Vertex Shader and Fragment Shader that can 
   be bound across one or more verticies.

   Each GLSLShader object has two parts... Shared data and per instance data. The shared
   data is stored in memory exactly once and is managed by the ManagerShader; this 
   is the fundamental data that does not ever change regardless of how it is 
   drawn. This includes things like the server side OpenGL handle, the filename 
   corresponding to that shader program, etc. This is owned 
   by the ManagerShader and should not be directly manipulated by the user without using 
   the ManagerShader's functions. This data is stored inside the GLSLShaderDataShared Object.
   There is a many-to-one mapping where many GLSLShader Objects map to a single 
   GLSLShaderDataShared. A GLSLShader Object has an internal pointer to a GLSLShaderDataShared Object.

   The per instance data is unique to this GLSLShader Object and is owned by this GLSLShader Object.
   This includes such things as this GLSLShader Object's GLSLUniforms and GLSLAttributes.
*/
class GLSLShader
{
public:

   static GLSLShader* New( const std::string& vertexShader, const std::string& fragmentShader, std::string geometryShader = "",
                           GLenum geometryInputPrimitiveType = GL_TRIANGLES, GLenum geometryOutputPrimitiveType = GL_TRIANGLE_STRIP,
                           GLuint geometryMaxOutputVerts = 3 );

   /**
      Construct a GLSLShader given a descriptor object. The stages described in the descriptor will
      be compiled and linked into the shader program.
   */
   static GLSLShader* New(const GLSLShaderDescriptor& desc);

   GLSLShader() = delete; // No default constructor, creation requires a valid GLSLShaderDataShared*
   
   /**
      The constructor for a GLSLShader, takes in the vertex shader and fragment shaders as
      parameters and creates a program shader from them and assigns the GLuint as the member
      variable of this shader.
      \param vertexShader - the vertex shader being used in the program
      \param fragmentShader - the fragment shader being used in the program
   */
   GLSLShader( GLSLShaderDataShared* sharedData );

   /**
      The destructor for the program, cleans up any memory allocation and the program shader
   */
   virtual ~GLSLShader();

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

   virtual void initUniformBlockInfo(); ///< If this shader uses the UBO, it queries for this info once at creation

   /**
      Set of add uniform functions, which adds the uniform variable and how it updates to the world.
      Each one represents the type of uniform variable that can change
   */
   void addUniform( GLSLUniform* uniform );
   void addAttribute( GLSLAttribute* attributeVar );

   /**
      Assumes the shader code corresponding to this GLSLShader has a Uniform Block specified.
      This method is used to activate a given Uniform Block so it is actually populated within
      the shader.

      uniformBlockName corresponds to the name of the Uniform Block specified in the shader code.
      For example, if the shader code is:
      "layout (binding = 0, shared) uniform CameraTransforms {... } T;"
      Then the uniformBlockName passed in to this method will be "CameraTransforms".

      uniformBlockBinding is the uniform buffer binding point at the index specified by the
      binding layout qualifier in the program shader:
      layout (binding = 0, shared) uniform CameraTransforms {... } T;
      This value is global to all programs that use this Uniform Block as well as to the
      Uniform Buffer Object which provides storage for the Uniform Block.

      Once these two parameters are set, they do not need to be reupdated unless the program
      is linked again.
   */
   bool useUniformBlock( const std::string uniformBlockName, GLint uniformBlockBinding );

   const std::vector< GLSLUniform* >* getUniforms() const { return &uniforms; }
   const std::vector< GLSLAttribute* >* getAttributes() const { return &attributes; }

   /**
      Binds this shader to the current shader 
   */
   virtual void bind();
   virtual void bind( const Mat4& modelMatrix, const Mat4& normalMatrix, const Camera& cam, const ModelMeshSkin& skin );

   /**
      Unbinds this shader and uses the default fixed functionality of the pipeline
   */
   virtual void unbind();

   GLuint getHandle() const;

   GLSLShaderDataShared* getSharedData() const { return dataShared; }

   std::string toString() const;
   std::string toStringAttributes() const;
   std::string toStringUniforms() const;
   std::string toStringUniformBlocks() const;

   //Given an OpenGL enum of a uniform or attribute type, a string representation of that is returned.
   static std::string getTypeString( GLenum type );

protected:
   GLSLShader( const GLSLShader& toCopy );
   GLSLShader& operator =( const GLSLShader& shader );

   GLSLShaderDataShared* dataShared;
   std::vector< GLSLUniform* > uniforms;
   std::vector< GLSLAttribute* > attributes;

   ///1) Uniform Block Name (string), 2) Shader-specified UniformBlockIndex (GLuint), and 3) Global UniformBlockBinding point
   ///Param 2 is determined at shader link time by GL.
   ///Param 3 is a chosen global constant used by all shaders that want to access the Uniform Block named by Param 1.
   ///Only successfully found and bound Uniform Blocks are contained in this list.
   std::vector< std::tuple< std::string, GLuint, GLuint > > activeUniformBlocks;

};

} //namespace Aftr

#endif //#ifdef AFTR_CONFIG_USE_OGL_GLEW
