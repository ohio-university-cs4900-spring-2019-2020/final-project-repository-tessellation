#pragma once

#include "GLSLShaderDataShared.h"
#include <string>
#include <set>

namespace Aftr
{
   class GLSLShader;
   struct GLSLShaderDescriptor; // added to support tessellation stages
   class Camera;
   class GLSLShaderDefaultSelectionGL32;
   class GLSLShaderDefaultGL32;
   class GLSLShaderDefaultBoundingBoxLinesGL32;
   class GLSLShaderDefaultLoadingScreenGL32;
   class GLSLShaderDefaultIndexedGeometryLinesGL32;
   class GLSLShaderDefaultOrthoStencilGL32;

   struct ShaderSetCompare
   {
      bool operator() ( const GLSLShaderDataShared* lhs, const GLSLShaderDataShared* rhs ) const
      { return ( (*lhs) < (*rhs) ); }
   };


class ManagerShader
{
public:
   static size_t getNumberOfShaders() { return shaders.size(); }
   static std::string toString();
   static GLSLShaderDefaultGL32* getDefaultShader() { return DEFAULT_SHADER; }
   static GLSLShaderDefaultGL32* getDefaultShaderCopy();
   static GLSLShaderDefaultLoadingScreenGL32* getDefaultLoadingScreenShader() { return DEFAULT_LOADING_SCREEN; }
   static GLSLShaderDefaultIndexedGeometryLinesGL32* getDefaultIndexedGeometryLinesShader() { return DEFAULT_INDEXED_GEOMETRY_LINES; }
   static GLSLShaderDefaultSelectionGL32* getDefaultSelectionShader() { return DEFAULT_SELECTION_SHADER; }
   static GLSLShaderDefaultBoundingBoxLinesGL32* getDefaultBoundingBoxLinesShader() { return DEFAULT_BOUNDING_BOX_LINES; }
   static GLSLShaderDefaultOrthoStencilGL32* getDefaultOrthoStencilShader() { return DEFAULT_ORTHO_STENCIL_SHADER; }

   static void init();

   /**
   Resets the shader handler to it's original state.
   Deletes all of the glshaderPtrs allocated and stored in the shaderDataShared Handler
   */
   static void shutdown();

   /**
   \return A pointer to the shader loaded in memory. The user owns this pointer and is responsible for deleting it.
   The user does not own the returned sharder->getSharedData(), the user should NOT delete shared->getSharedData.
   */
   static GLSLShader* loadShader( const std::string& vertexShader, const std::string& fragmentShader, std::string geometryShader = "",
                                  GLenum geometryInputPrimitiveType = GL_TRIANGLES, GLenum geometryOutputPrimitiveType = GL_TRIANGLE_STRIP, 
                                  GLuint geometryMaxOutputVerts = 3 );//need to add tessellation control and tessellation evaluation shader stages

   /**
   Loads a shader from the given shader descriptor.
   \return A pointer to the shader loaded in memory. The user owns this pointer and is responsible for deleting it.
   The user does not own the returned sharder->getSharedData(), the user should NOT delete shared->getSharedData.
   */
   static GLSLShader* loadShader(const GLSLShaderDescriptor& desc);

   /**
      This method will attempt to compile and link the in passed shader programs. If successful, this unique shader program will be inserted into
      the multiplicity manager for the GLSLShaderDataShared (this way, only one instance of this unique program is ever in memory. If the same
      set of files is called with this method again, the previously complied/linked GLSLShaderDataShared will be returned. This returned
      GLSLShdaderDataShared is OWNED BY THIS ManagerShader and will be destroyed by this manager; the user should NOT delete this object.

      The intent of this method is to let a user get a pointer to a GLSLShaderDataShared instance and then pass this as a constructor parameter
      to a GLSLShader* program. The user then owns that GLSLShader instance and is responsible for deleting it.
   */
   static GLSLShaderDataShared* loadShaderDataShared( const std::string& vertexShader, const std::string& fragmentShader, std::string geometryShader = "",
                                                      GLenum geometryInputPrimitiveType = GL_TRIANGLES, GLenum geometryOutputPrimitiveType = GL_TRIANGLE_STRIP,
                                                      GLuint geometryMaxOutputVerts = 3 );//need to add tessellation control and tessellation evaluation shader stages

   /**
      This method will attempt to compile and link the shader programs described by the given shader descriptor. If successful, this unique shader
      program will be inserted into the multiplicity manager for the GLSLShaderDataShared (this way, only one instance of this unique program
      is ever in memory. If the same set of files is called with this method again, the previously complied/linked GLSLShaderDataShared will be
      returned. This returned GLSLShdaderDataShared is OWNED BY THIS ManagerShader and will be destroyed by this manager; the user should NOT
      delete this object.

      The intent of this method is to let a user get a pointer to a GLSLShaderDataShared instance and then pass this as a constructor parameter
      to a GLSLShader* program. The user then owns that GLSLShader instance and is responsible for deleting it.
   */
   static GLSLShaderDataShared* loadShaderDataShared(const GLSLShaderDescriptor& desc);

   static GLSLShader* loadComputeShader( const std::string& computeShader );

   static GLSLShader* loadShaderCrazyBump();
   static GLSLShader* loadShaderCrazyBumpParallaxMapping();
   static GLSLShader* loadShaderPointTesselatorBillboard( Camera** cam );
   static GLSLShader* loadShaderPointTesselatorBillboardDecimator( Camera** cam );

   static void bindShader( GLuint shaderHandle );

   static GLuint getCurrentlyBoundShader() { return currentlyBoundShaderHandle; }

   static std::string queryShaderSupport();

   /// Expected shader attribute locations for position, normal, tex coord, and color.
   ///layout( location = 0 ) in vec3 VertexPosition;
   ///layout( location = 1 ) in vec3 VertexNormal;
   ///layout( location = 2 ) in vec2 VertexTexCoord;
   ///layout( location = 3 ) in vec4 VertexColor;
   ///layout( location = 4 ) in vec4 VertexTangent;
   static unsigned int getDefaultAttributeLocationVertexPosition() { return 0; }
   static unsigned int getDefaultAttributeLocationVertexNormal()   { return 1; }
   static unsigned int getDefaultAttributeLocationVertexTexCoord() { return 2; }
   static unsigned int getDefaultAttributeLocationVertexColor()    { return 3; }
   static unsigned int getDefaultAttributeLocationVertexTangent()  { return 4; }

   /// This is the uniform buffer binding point at the index specified by the
   /// binding layout qualifier in the program shader:
   /// layout (binding = 0, shared) uniform CameraTransforms {... } T;
   static GLuint getUniformBlockBindingCameraTransforms() { return 0; }
   static GLuint getUniformBlockBindingLightInfo() { return 1; }

   static bool deletePreviouslyLoadedShaderFromCachedSet( GLSLShaderDataShared* shader );

protected:

   static void loadGL32DefaultShaders(); ///< Loads shaders expecting GL 3.2 or greater to exist in the current context
   static void loadShaderDefaultGL32();
   static void loadShaderDefaultSelectionGL32();
   static void loadShaderLoadingScreenGL32(); ///< Loading screen which requires OpenGL 3.2 or greater
   static void loadShaderIndexedGeometryLinesGL32();
   static void loadShaderDefaultBoundingBoxLinesGL32();
   static void loadShaderDefaultOrthoStencilGL32(); ///< Used by MGLGUI objects that touch the stencil buffer to enforce size

   /**
      Takes the current state of the shader (vert, frag, geo programs, etc. and attempts to compile and link them. Returns
      true if successfully compiled and linked; false, otherwise. This internally, simply calls shader->instantiate().
   */
   static bool instantiateOpenGLShader( GLSLShaderDataShared* shader = NULL );

   /**
      Set containing all shaders that have been loaded via 
      Managershader::loadshader( const std::string& fileName ). These shaders
      are owned and managed by this Manager. The user should not directly delete
      any shaderDataShared*. All shaders are loaded exactly once and may be referenced by
      many different Models and/or ModelData objects.
   */
   static std::set< GLSLShaderDataShared*, ShaderSetCompare > shaders;

   static GLuint currentlyBoundShaderHandle;

   /** 
      Default shader used by the engine. This default is ALWAYS returned in the
      event of Managershader::loadshader(...) failing to find/load the specific
      file. The engine loads the default shader at start up; if the default shader
      cannot be loaded, the engine will give an error and not load.
   */
   static GLSLShaderDefaultGL32* DEFAULT_SHADER;
   static GLSLShaderDefaultSelectionGL32* DEFAULT_SELECTION_SHADER;
   static GLSLShaderDefaultBoundingBoxLinesGL32* DEFAULT_BOUNDING_BOX_LINES;
   static GLSLShaderDefaultOrthoStencilGL32* DEFAULT_ORTHO_STENCIL_SHADER;

   /**
      These default shaders require at least OpenGL 4.3 Context
   */
   static GLSLShaderDefaultLoadingScreenGL32* DEFAULT_LOADING_SCREEN;
   static GLSLShaderDefaultIndexedGeometryLinesGL32* DEFAULT_INDEXED_GEOMETRY_LINES;
   
   static std::vector< GLSLShader* > defaultShadersLoadedByManagerAtInit;

};
}

