#include "ModelMeshSkin.h"
#include "Texture.h"
#include "GLSLShader.h"
#include "ManagerOpenGLState.h"
#include "ManagerModelMultiplicity.h"
#include <sstream>
#include <iostream>
#include "AftrUtilities.h"
#include "ManagerEnvironmentConfiguration.h"
#include "ManagerLight.h"
#include "ManagerShader.h"
#include "Mat4.h"
#include "Camera.h"
#include "GLSLShaderDefaultGL32.h"

using namespace Aftr;

ModelMeshSkin::ModelMeshSkin()
{
   this->initMemberData();
   if( ManagerOpenGLState::isGLContextProfileVersion32orGreater() )
   {
      this->multiTextures.push_back( ManagerTexture::getDefaultWhite2x2Texture() ); //added this to appease the default 3.2+ shader. This may cause issues w/ MGLGUI objects that don't immediately pass in a texture
      //and assume the first texture manually added will be at index 0...
      //std::cout << AFTR_FILE_LINE_STR << " Automatically inserted white 2x2 tex into ModelMeshSkin::getMultiTextureSet()[0]...\n";
   }
}

ModelMeshSkin::ModelMeshSkin( ModelMeshSkin&& toMove )
{
   //this->initMemberData();
   (*this) = std::move( toMove );
}

ModelMeshSkin::ModelMeshSkin( const ModelMeshSkin& toCopy )
{
   this->initMemberData();
   (*this) = toCopy;
}

ModelMeshSkin::ModelMeshSkin( Texture* tex )
{
   this->initMemberData();
   this->multiTextures.push_back( tex );
}

ModelMeshSkin::ModelMeshSkin( const std::vector< Texture* >& textures )
{
   this->initMemberData();
   for( size_t i = 0; i < textures.size(); ++i )
      this->multiTextures.push_back( textures.at(i) );
}

ModelMeshSkin::ModelMeshSkin( GLSLShader* shader )
{
   this->initMemberData();
   this->shader = shader;

   if( ManagerOpenGLState::isGLContextProfileVersion32orGreater() )
   {
      this->multiTextures.push_back( ManagerTexture::getDefaultWhite2x2Texture() ); //added this to appease the default 3.2+ shader. This may cause issues w/ MGLGUI objects that don't immediately pass in a texture
      //and assume the first texture manually added will be at index 0...
      std::cout << AFTR_FILE_LINE_STR << " Automatically inserted white 2x2 tex into ModelMeshSkin::getMultiTextureSet()[0]...\n";
   }
}

ModelMeshSkin::ModelMeshSkin( Texture* tex, GLSLShader* shader )
{
   this->initMemberData();
   this->multiTextures.push_back( tex );
   this->shader = shader;
}

void ModelMeshSkin::initMemberData()
{
   this->shader = nullptr;
   if( ManagerOpenGLState::isGLContextProfileVersion32orGreater() )
      this->shader = static_cast<GLSLShader*>( ManagerShader::getDefaultShaderCopy() ); ///< Modern GL requires a shader. The default shader performs standard Ambient, Diffuse, Specular lighting computations
   this->shadingType = MESH_SHADING_TYPE::mstNOTINITIALIZED;
   this->glPrimType = GL_TRIANGLES;
   this->glLineWidthThickness = 1.0f;
   if( ManagerModelMultiplicity::getRenderMode() == RENDER_HW_MODE::rhmVBO_VERTEX_BUFFER_OBJECT )
   {
      this->renderType = MESH_RENDER_TYPE::mrtVBO;
   }
   else
   {
      std::cout << "WARNING: ModelMeshSkin, VBOs are not supported, skin renderType is forced to VA...\n";
      this->renderType = MESH_RENDER_TYPE::mrtVA;
   }
   this->useColorMaterial = true;
   this->ambient = aftrColor4f( 0.4f, 0.4f, 0.4f, 1.0f ); //alpha value of this ambient color determines transparency
   this->diffuse = aftrColor4f( 0.75f, 0.75f, 0.75f, 1.0f );
   this->specular = aftrColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
   this->specularCoefficient = 1000; //usually 1 - 200
   if( !ManagerOpenGLState::isGLContextProfileVersion32orGreater() ) this->specularCoefficient = 128.0f; //if Using GL 2.1, the max value is 128 or GL_INVALID_VALUE is produced
   this->color = aftrColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
   this->renderNormals = false;

   normalsScale = 1.0f;
   normalLineWidthThickness = 1.0f;
   normalsScaleModified = false;
   this->visible = true;
}

ModelMeshSkin::~ModelMeshSkin()
{
   for( size_t i = 0; i < this->multiTextures.size(); ++i )
   {
      delete this->multiTextures.at(i); this->multiTextures.at(i) = nullptr;
   }
   this->multiTextures.clear();

   if( this->shader != nullptr ) { delete this->shader; this->shader = nullptr; }
}

ModelMeshSkin& ModelMeshSkin::operator =( ModelMeshSkin&& moveSkin )
{
   if( this != &moveSkin )
   {
      //First, we will free any resources used by this instance before it is overwritten
      delete this->shader; this->shader = nullptr;

      //Remove textures that are owned by this instance
      for( size_t i = 0; i < this->multiTextures.size(); ++i )
         delete this->multiTextures.at( i );
      this->multiTextures.clear();

      //Second, we will move the internals from moveSkin to this instance
      this->multiTextures = std::move( moveSkin.multiTextures );
      this->shader = moveSkin.shader;

      //Now "repair" the moveSkin, so its destructor doesn't deallocate stolen resources
      moveSkin.getMultiTextureSet().clear();
      moveSkin.shader = nullptr;

      //Now perform this simple member-wise assignments
      this->ambient = moveSkin.ambient;
      this->diffuse = moveSkin.diffuse;
      this->specular = moveSkin.specular;
      this->specularCoefficient = moveSkin.specularCoefficient;
      this->color = moveSkin.color;
      this->useColorMaterial = moveSkin.useColorMaterial;
      this->shadingType = moveSkin.shadingType;
      this->glPrimType = moveSkin.glPrimType;
      this->glLineWidthThickness = moveSkin.glLineWidthThickness;
      this->renderType = moveSkin.renderType;
      this->renderNormals = moveSkin.renderNormals;
      this->normalsScale = moveSkin.normalsScale;
      this->normalLineWidthThickness = moveSkin.normalLineWidthThickness;
      this->normalsScaleModified = moveSkin.normalsScaleModified;
      this->visible = moveSkin.visible;
      this->name = moveSkin.name;
      this->patchVertices = moveSkin.patchVertices; // added for tessellation
   }
   return *this;
}

ModelMeshSkin& ModelMeshSkin::operator =( const ModelMeshSkin& skin )
{
   if( this != &skin )
   {
      //Remove textures that are owned by this instance
      for( size_t i = 0; i < this->multiTextures.size(); ++i )
         delete this->multiTextures.at(i);
      this->multiTextures.clear();

      //Create copy of the texture object so this instance owns
      //its own textures
      for( size_t i = 0; i < skin.multiTextures.size(); ++i )
      {
         Texture* tex = skin.multiTextures.at(i);
         Texture* cpyTex = tex->cloneMe(); //creates a clone, owned by the LHS
         this->multiTextures.push_back( cpyTex );
      }

      if( this->shader != nullptr ) { delete this->shader; this->shader = nullptr; }
      if( skin.shader != nullptr )
         this->shader = skin.getShader()->getCopyOfThisInstance(); //preserve polymorphic type of shader

      this->ambient = skin.ambient;
      this->diffuse = skin.diffuse;
      this->specular = skin.specular;
      this->specularCoefficient = skin.specularCoefficient;
      this->color = skin.color;
      this->useColorMaterial = skin.useColorMaterial;
      this->shadingType = skin.shadingType;
      this->glPrimType = skin.glPrimType;
      this->glLineWidthThickness = skin.glLineWidthThickness;
      this->renderType = skin.renderType;
      this->renderNormals = skin.renderNormals;
      this->normalsScale = skin.normalsScale;
      this->normalLineWidthThickness = skin.normalLineWidthThickness;
      this->normalsScaleModified = skin.normalsScaleModified;
      this->visible = skin.visible;
      this->name = skin.name;
      this->patchVertices = skin.patchVertices; // added for tessellation
   }
   return (*this);
}

std::vector< Texture* >& ModelMeshSkin::getMultiTextureSet()
{
   return this->multiTextures;
}

const std::vector< Texture* >& ModelMeshSkin::getMultiTextureSet() const
{
   return this->multiTextures;
}

GLSLShader* ModelMeshSkin::getShader() const
{
   return this->shader;
}

void ModelMeshSkin::setShader( GLSLShader* shader )
{
   if( this->shader != nullptr )
   {
      delete this->shader;
      this->shader = nullptr;
   }
   this->shader = shader;
}

void ModelMeshSkin::setMeshRenderType( MESH_RENDER_TYPE type )
{
   if( ManagerOpenGLState::getRenderMode() == RENDER_HW_MODE::rhmVA_VERTEX_ARRAY )
   {
      if( type == MESH_RENDER_TYPE::mrtVBO ||type == MESH_RENDER_TYPE::mrtVA_AND_VBO )
      {
         std::cout << "WARNING: ModelMeshSkin::setMeshRenderType( type )\n"
                   << "   type is MESH_RENDER_TYPE::mrtVBO (strictly VBO rendering only) or MESH_RENDER_TYPE::mrtVA_AND_VBO (supporting both). \n"
                   << "   However, the current graphical hardware on this machine\n"
                   << "   does not support any VBOs or corresponding methods.\n"
                   << "   MeshRenderType will be FORCED to MESH_RENDER_TYPE::mrtVA and will use\n"
                   << "   vertex arrays (VAs).\n\n"
                   << "   Using only a VBO and removing client side VA data will cause a crash since the\n"
                   << "   Graphics engine must fall back to using VAs when the hardware does not support\n"
                   << "   VBOs.\n";
         this->renderType = MESH_RENDER_TYPE::mrtVA;
      }
   }
   else
   {
      this->renderType = type;
   }
}

std::string ModelMeshSkin::toString() const
{
   std::stringstream ss;
   ss << "ModelMeshSkin Info...\n"
      << "   multi-texture set size: " << this->multiTextures.size() << "\n"
      << "   shading type: " << (unsigned int) this->shadingType << "\n"
      << "   shader: ";

   if( this->shader != nullptr )
      ss << this->shader->toString() << "\n";
   else
      ss << "nullptr\n";

   for( int i = 0; i < 4; ++i )
      ss << "   Ambient[" << i << "]: " << this->ambient[i] << "\n";
   for( int i = 0; i < 4; ++i )
      ss << "   Diffuse[" << i << "]: " << this->diffuse[i] << "\n";
   for( int i = 0; i < 4; ++i )
      ss << "   Specular[" << i << "]: " << this->specular[i] << "\n";
   ss << "   Specular Coefficient: " << this->specularCoefficient << "\n";
   for( int i = 0; i < 4; ++i )
      ss << "   Color[" << i << "]: " << (unsigned int) this->color[i] << "\n";
   ss << "   isUsingColorMaterial: " << (unsigned int) this->useColorMaterial << "\n";

   return ss.str();
}

void ModelMeshSkin::setColori( int r, int g, int b, int a )
{
   this->color[0] = static_cast< float >( r ) / 255.0f;
   this->color[1] = static_cast< float >( g ) / 255.0f;
   this->color[2] = static_cast< float >( b ) / 255.0f;
   this->color[3] = static_cast< float >( a ) / 255.0f;
}

void ModelMeshSkin::setColorf( float r, float g, float b, float a )
{
   this->color[0] = r;
   this->color[1] = g;
   this->color[2] = b;
   this->color[3] = a;
}

void ModelMeshSkin::setColor( const aftrColor4ub& c )
{
   this->color = aftrColor4f( c );
}

void Aftr::ModelMeshSkin::bind( const std::tuple< const Mat4&, const Mat4&, const Camera& >* const shaderParams ) const
{
   if( ! ManagerOpenGLState::isGLContextProfileVersion32orGreater() )   
   {
      // std::cout << "ModelMeshSkin::bind()" << std::endl;

      //if( this->shadingType == MESH_SHADING_TYPE::mstNOTINITIALIZED )
      //   std::cout << "WARNING: Binding ModelMeshSkin with MESH_SHADING_TYPE of MESH_SHADING_TYPE::mstNOTINITIALIZED...\n"
      //             << "Offending Skin: " << this->toString() << "\n";
      if( this->useColorMaterial )
      {
         glEnable( GL_COLOR_MATERIAL );
         glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
         glColor4fv( &this->color.r ); //this is "overwritten" if each vertex has color data
      }
      else
      {
         glDisable( GL_COLOR_MATERIAL );
         glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, &this->ambient.r );
         glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, &this->diffuse.r );
      }

      if( this->shadingType == MESH_SHADING_TYPE::mstNONE )
      {
         glDisable( GL_LIGHTING );
      }

      //Set specular properties
      glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &this->specular.r );
      if( this->specularCoefficient > 128 )
         glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 128 ); // AMD Driver says "glMaterialf parameter <params[0]> has an invalid 
      else                                                    //value '1000.000000': must be within [0.000000, 128.000000] ( GL_INVALID_VALUE )"
         glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, this->specularCoefficient );

   }

   if( this->glPrimType == GL_LINES || this->glPrimType == GL_LINE_STRIP ||
       this->glPrimType == GL_LINE_LOOP || this->glPrimType == GL_LINE_STRIP_ADJACENCY ||
       this->glPrimType == GL_LINES_ADJACENCY )
   {
      //if we are rendering either a GL_LINES | GL_LINE_STRIP | GL_LINE_LOOP, we 
      //need to set the line width appropriately.
      if( (ManagerOpenGLState::isGLContextProfileCore() && this->glLineWidthThickness > 1.0f)  )
         glLineWidth( this->glLineWidthThickness );         
   }

   // set GL_PATCH_VERTICES if using new OpenGL and primitive type is GL_PATCHES
   if (ManagerOpenGLState::isGLContextProfileVersion32orGreater() && this->glPrimType == GL_PATCHES) {
       glPatchParameteri(GL_PATCH_VERTICES, this->patchVertices);
   }

   printOpenGLErrors( 4301, nullptr, AFTR_FILE_LINE_STR );

   if( this->shader != nullptr )
   {
      if( shaderParams == nullptr )
         this->shader->bind();
      else
         this->shader->bind( std::get<0>( *shaderParams ), std::get<1>( *shaderParams ), std::get<2>( *shaderParams ), *this );

   }

   printOpenGLErrors( 4302, nullptr, AFTR_FILE_LINE_STR );

   //Setup texture parameters
   for( size_t i = 0; i < this->multiTextures.size(); ++i )
   {
      glActiveTexture( GL_TEXTURE0 + (unsigned int)i );
      this->multiTextures.at( i )->bind();
      glActiveTexture( GL_TEXTURE0 );
   }

   //std::cout << "Leaving ModelMeshSkin::bind()" << std::endl;
}

void ModelMeshSkin::unbind() const
{
   printOpenGLErrors( 4311, nullptr, AFTR_FILE_LINE_STR );
   for( size_t i = 0; i < this->multiTextures.size(); ++i )
   {
      glActiveTexture( GL_TEXTURE0 + (unsigned int) i );
      this->multiTextures.at(i)->unbind();
      glActiveTexture( GL_TEXTURE0 );
   }
   printOpenGLErrors( 4312, nullptr, AFTR_FILE_LINE_STR );

   if( this->shader != nullptr )
      this->shader->unbind();

   printOpenGLErrors( 4313, nullptr, AFTR_FILE_LINE_STR );

   // don't make this call if using newer version of OpenGL
   if (!ManagerOpenGLState::isGLContextProfileVersion32orGreater() && this->shadingType == MESH_SHADING_TYPE::mstNONE && ManagerLight::getNumLightsTotal() > 0)
      glEnable(GL_LIGHTING);
}


