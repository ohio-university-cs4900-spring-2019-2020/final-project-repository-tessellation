#pragma once

#include "AftrGlobals.h"
#include "AftrOpenGLIncludes.h"
#include "Mat4Fwd.h"
#include <vector>
#include <string>
#include <tuple>

namespace Aftr
{

class Texture;
class GLSLShader;
class ModelMesh;
class Camera;

/**
   This class encapsulates a "skin" that is applied on the surface of 
   a mesh when it is rendered. The skin is composed of 3 main parts:
   -# A set of textures that are applied in parallel (multi-texture)
   -# A GLSLShader to perform OpenGL Vertex and Fragment shader operations
   -# OpenGL Color/Material properties (ambient, diffuse, specular, color RGBA)
   
   When rendering a mesh, that mesh's corresponding skin is "bound", then
   the mesh's geometry is rendered, then the skin may be "unbound". At most,
   one skin can be bound at any time.

   If the OpenGL Context is 3.2+ Core, then the Default Shader has uniforms to store the
   skin's Ambient, Diffuse, and Specular data. The Shader is the fully responsible for performing
   the desired lighting. Depricated and options like enabling/disabling USE_COLOR_MATERIAL no 
   longer exist and are not called.

   When using the default 3.2+ shaders (such as GLSLShaderDefaultGL32):
   
   TRANSPARENCY is determined by:
   float alpha = min( ModelMeshSkin::ambient.a, ModelMeshSkin::multiTextures[usedTex].a ).
   That is, the alpha is set by either the ambients alpha channel or the used texture's alpha channel,
   which ever is smaller. If this will not work for an object, an alternative shader may be bound that
   performs the desired transparency.
   
   --------------------------------
   OLD SCHOOL Fixed function 2.1+ Compatibility mode:

   When NOT using default 3.2+ shaders and relying on old "fixed functionality":

   A brief note on OpenGL and Materials vs Colors and USE_COLOR_MATERIAL and 
   how this skin implementation works with them:

   If USE_COLOR_MATERIAL is enabled (and OpenGL Lighting is enabled),
   then the OpenGL color state is used to approximate the 
   material properties. Materials are always used if OpenGL Lighting
   is enabled; therefore, one must either use the material properties 
   (ambient, diffuse, specular), or use the colors (r,g,b,a) to approximate
   the material properties. If using colors to approximate materials, then
   each vertex's color data can be used. If each vertex contains no color
   data, then the single color (this->color[4]) is used to approximate the
   material across all verticies of this mesh.

   If USE_COLOR_MATERIAL is disabled and OpenGL Lighting is enabled, 
   then this->ambient[4], this->diffuse[4], and this->specular[4]
   are used as the materials; OpenGL color state has no effect.

   If USE_COLOR_MATERIAL is disabled and OpenGL Lighting is disabled, 
   each vertex's color data is used. If each vertex contains no color data,
   then this->color[4] is used as the color (materials are disabled and have
   no effect).

   If lighting is disabled, then OpenGL Material properties cannot be used; instead
   OpenGL Colors must be used. If each vertex has color data, then that color will
   be used. If each vertex contains no color data, then this->color[4] will be used
   across all meshes to set the color (Materials are disabled and have no effect).
*/
class ModelMeshSkin
{
public:
   ModelMeshSkin();
   ModelMeshSkin( ModelMeshSkin&& toMove ); //TODO: add when compiler supports it noexcept;
   ModelMeshSkin( const ModelMeshSkin& toCopy );
   ModelMeshSkin( Texture* tex );
   ModelMeshSkin( const std::vector< Texture* >& textures );
   ModelMeshSkin( GLSLShader* shader );
   ModelMeshSkin( Texture* tex, GLSLShader* shader );
   virtual ~ModelMeshSkin();

   ModelMeshSkin& operator =( ModelMeshSkin&& moveSkin ); //TODO: add when compiler supports it noexcept;
   ModelMeshSkin& operator =( const ModelMeshSkin& skin );

   /**
      This method binds the current skin. All geometry rendered while this
      skin is bound will utilize this skins' multiTexture set, shader, and
      material properties.

      When using the default 3.2+ shaders (such as GLSLShaderDefaultGL32, transparency is determined by:
      float alpha = min( ModelMeshSkin::ambient.a, ModelMeshSkin::multiTextures[usedTex].a ).
      That is, the alpha is set by either the ambient's alpha channel or the used texture's alpha channel,
      which ever is smaller.

      The optional tuple contains common/useful data for GLSLShader uniforms, params [0,2]:
                       const Mat4& modelMatrix, const Mat4& normalMatrix, const Camera& cam
      That will be passed into the shader's bind method. If these parameters are not supplied and a GL 3.2+ 
      context without any fixed functionality pipeline is being used, then the user is responsible for updating
      the shader values via their own mechanism.
   */
   virtual void bind( const std::tuple< const Mat4&, const Mat4&, const Camera& >* const shaderParams = nullptr ) const;

   /**
      This method unbinds the current skin. This consists of deactivating
      all but the 0th texture, unbinding the currently bound texture,
      and unbinding the current shader.
   */
   virtual void unbind() const;

   std::vector< Texture* >& getMultiTextureSet();
   const std::vector< Texture* >& getMultiTextureSet() const;
   //const std::vector< Texture* >* getMultiTextureSet() const { return &multiTextures; }
   std::vector< Texture* >* getMultiTextureSetPtr() { return &this->multiTextures; }
   GLSLShader* getShader() const;
   
   //Same as getShader(), but is templated so the user does not need to perform
   //a static_cast<T> on the return type. This is simply a convenience method.
   template< typename T, typename R = std::remove_pointer_t<T> > R* getShaderT()
   {
      assert( this->shader != nullptr );
      if constexpr( std::is_pointer_v<T> )
         return static_cast<R*>(this->shader);//Lets skin->getShaderT<GLSLShaderPointTesselatorBillboard*>() work as expected.
      return static_cast<R*>(this->shader);   //Lets wo->getShaderT<GLSLShaderPointTesselatorBillboard>() work as expected.
   }

   /// Deletes and replaces any existing shader already associated with this ModelMeshSkin.
   void setShader( GLSLShader* shader );

   MESH_SHADING_TYPE getMeshShadingType() const { return this->shadingType; } 
   void setMeshShadingType(MESH_SHADING_TYPE shadingType) { this->shadingType = shadingType; }
   GLenum getGLPrimType() const { return this->glPrimType; }
   void setGLPrimType(GLenum type) { this->glPrimType = type; }
   MESH_RENDER_TYPE getMeshRenderType() const { return this->renderType; }
   void setMeshRenderType( MESH_RENDER_TYPE type );
   
   std::string toString() const;

   const std::string& getName() { return this->name; }
   void setName( const std::string& name ) { this->name = name; }

   ///Transparency is determined by the min value of the alpha component of the ambient or the bound texture's alpha value.
   void setAmbient( const aftrColor4f& ambient )   { this->ambient = ambient; }
   void setDiffuse( const aftrColor4f& diffuse )   { this->diffuse = diffuse; }
   void setSpecular( const aftrColor4f& specular ) { this->specular = specular; }
   void setColor4f( const aftrColor4f& color )     { this->color = color; }
   void setColor4ub( const aftrColor4ub& color )   { this->color = aftrColor4f( (GLubyte)color.r, (GLubyte)color.g, (GLubyte)color.b, ( GLubyte )color.a ); }
   void setSpecularCoefficient( GLfloat exponent ){ this->specularCoefficient = exponent; }
   /// Each int must be in the range [0,255]
   void setColori( int r, int g, int b, int a );
   /// Each float must be in the range [0, 1]
   void setColorf( float r, float g, float b, float a );
   void setColor( const aftrColor4ub& c );
   /// Returns ambient colors [0,3]: r,g,b,a, respectively.
   const aftrColor4f& getAmbient() const { return ambient; }
   /// Returns diffuse colors [0,3]: r,g,b,a, respectively.
   const aftrColor4f& getDiffuse() const { return diffuse; }
   /// Returns specular [0,3]: r,g,b,a, respectively.
   const aftrColor4f& getSpecular() const { return specular; }
   /// Exponent on the specular highlighting equation. See documentation for this variable in this class.
   GLfloat getSpecularCoefficient() const { return this->specularCoefficient; }
   /// Returns colors [0,3]: r,g,b,a, respectively.
   const aftrColor4f& getColor() const { return this->color; }
   
   void isUsingColorMaterial( bool x ) { useColorMaterial = x; }
   bool isUsingColorMaterial() const { return useColorMaterial; }

   void isRenderingNormals( bool x ) { renderNormals = x; }
   bool isRenderingNormals() const { return renderNormals; }

   void setNormalsScale( float x ) { normalsScale = x; normalsScaleModified = true; }
   float getNormalsScale() { return normalsScale; }
   bool isNormalScaleModified() { return normalsScaleModified; }
   void isNormalScaleModified( bool x ) { normalsScaleModified = x; }

   bool isVisible() const { return visible; }
   void isVisible( bool visible ) { this->visible = visible; }

   void setGLLineWidth( float glLineWidthThickness ) { this->glLineWidthThickness = glLineWidthThickness; }
   float getGLLineWidth() { return this->glLineWidthThickness; }

   void setNormalLineWidth( float normalLineWidthThickness ) { this->normalLineWidthThickness = normalLineWidthThickness; }
   float getNormalLineWidth() { return this->normalLineWidthThickness; }

   // Introduce functions to access patchVertices, which describes the number of vertices
   // for a single tessellation patch.
   void setPatchVertices(GLuint verts) { this->patchVertices = verts; }
   GLuint getPatchVertices() const { return this->patchVertices; }

protected:

   void initMemberData();

   std::vector< Texture* > multiTextures; ///< Owned by this instance, delete upon destruction
   GLSLShader* shader = nullptr; ///< Owned by this instance, delete upon destruction
   
   std::string name = ""; //Optional name or description to help identify this skin/material information. Ex: "glass windshield".

   aftrColor4f ambient; ///< The ambient material property of this mesh. Used only if using default 3.2+ shader or useColorMaterial is false.
   aftrColor4f diffuse; ///< The diffuse material property of this mesh. Used only if using default 3.2+ shader or useColorMaterial is false.
   aftrColor4f specular; ///< The specular material property of this mesh. Used only if using default 3.2+ shader or useColorMaterial is false.
   
   /**
      Ranges from 0 to 1000+, this is the specular exponent used in shininess. The smaller the
      value, the larger the shiny spot (and less intense). The larger the value, the smaller
      the shiny spot (but more intense).
   */
   GLfloat specularCoefficient;
   
   /**
      r,g,b,a. Used in glColor(...) when rendered iff, this->useColorMaterial is true and the 
      corresponding Mesh has no color data in its vertex list.
      This is NOT used if the default GLSLShaderDefaultGL32 is bound.
   */
   aftrColor4f color;

   /**
      If true (and OpenGL Lighting is enabled),
      then the OpenGL color state is used to approximate the 
      material properties. Materials are always used if OpenGL Lighting
      is enabled; therefore, one must either use the material properties 
      (ambient, diffuse, specular), or use the colors (r,g,b,a) to approximate
      the material properties. If using colors to approximate materials, then
      each vertex's color data can be used. If each vertex contains no color
      data, then the single color (this->color[4]) is used to approximate the
      material across all verticies of this mesh.

      If this is false and OpenGL Lighting is enabled, then this->ambient[4],
      this->diffuse[4], and this->specular[4] are used as the materials.

      If this is false and OpenGL Lighting is disabled, each vertex's color data
      is used or this->color[4] is used as the color (materials are disabled).

      If lighting is disabled, then OpenGL Material properties cannot be used; instead
      OpenGL Colors must be used.
   */
   bool useColorMaterial;
   
   /**
      Determines if this mesh should be rendered using Smooth Shading (mstShared)
      (one normal per vertex) or Flat Shading (MESH_SHADING_TYPE::mstFLAT) (one normal per face).
      This value defaults to MESH_SHADING_TYPE::mstNOTINITIALIZED and must be set prior to actually
      using this skin (typically this is set by the parser reading in a model from
      file.
   */
   MESH_SHADING_TYPE shadingType;

   /**
      Determines the type of primitive this skin tries to render.  If the associated
      shading type-primitive type render data has not been loaded, it will be the first
      time this skin is rendered.

      This value defaults to GL_TRIANGLES.
      Valid values include GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, 
      GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_QUAD_STRIP, GL_QUADS, and GL_POLYGON 
   */
   GLenum glPrimType;

   /**
      Specifies the rasterized width of both aliased and antialiased lines. Using a line
      width other than 1 has different effects, depending on whether line antialiasing is
      enabled. To enable and disable line antialiasing, call glEnable and glDisable with 
      argument GL_LINE_SMOOTH. Line antialiasing is initially disabled.

      This is only used if glPrimType is set to GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP,
      GL_LINE_STRIP_ADJACENCY, or GL_LINES_ADJACENCY

      If line antialiasing is disabled, the actual width is determined by rounding the
      supplied width to the nearest integer. (If the rounding results in the value 0,
      it is as if the line width were 1.) If dx >= dy, i pixels are filled in each column
      that is rasterized, where i is the rounded value of width. Otherwise, i pixels are
      filled in each row that is rasterized.
   */
   float glLineWidthThickness;

   /**
      Specifies the number of vertices for a single tessellation patch. This must match that
      of the intended TCS/TES stages.
   */
   GLuint patchVertices = 3;

   /**
      The mesh hardware render type for this skin.
      If VA will stream to server every render.
      If VBO will live on server if room.
      If VA_AND_VBO will use VBO when available, but will keep copy of VA data on host.
   */
   MESH_RENDER_TYPE renderType;

   bool renderNormals;

   float normalsScale; ///< Defines length of normals. Default length is 1. This value scales the length.
   float normalLineWidthThickness; ///< Defines the glLineWidth() of the Normal vectors of the associated mesh
   bool normalsScaleModified; ///< After changing the normal scale, this flag is set to let the ModelMesh
                              ///< ModelMeshDataShared know that it needs to recompute its normal lengths
                              ///< so they can subsequently be rendered to the length of normal scale.
                              ///< After regeneration of normals, this flag is automatically set back
                              ///< to false.

   bool visible;
};

}

