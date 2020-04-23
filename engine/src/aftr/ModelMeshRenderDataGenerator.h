#pragma once

#include "AftrOpenGLIncludes.h"
#include "AftrGlobals.h"
#include "Vector.h"
#include "GLSLAttributeArray.h"
#include "ModelMeshRenderData.h"
#include <vector>
#include <cstdlib>
#include <map>


namespace Aftr
{
   class ModelMeshDataShared;

/**
   This class is the internal mechanism used by the ModelMeshDataShared object to
   generate the proper Vertex Arrays used by OpenGL so that the mesh can be rendered
   via VBOs or VAs. The associted strides and offsets for the verticies, indicies,
   colors, and textures are publicly accessible for use by the owning 
   ModelMeshDataShared object.
*/
class ModelMeshRenderDataGenerator
{
public:
   ModelMeshRenderDataGenerator();

   virtual ~ModelMeshRenderDataGenerator();

   std::string toString();

   /**
      Public strides and offsets to provide fast access when dispatching the VBOs/VAs.
      The user should not modify these; however, getters are not implemented because these
      need to be accessed as quickly as possible. They are also listed first so a compiler
      may optimize the cache layout for this class to put these members first.
   */
   GLsizei stride = 0;
   unsigned int vertsOffset = 0;
   GLenum numColorChannels = GL_RGBA; ///< GL_RGB or GL_RGBA
   unsigned int colorsOffset = 0;
   unsigned int normalsOffset = 0;
   std::vector< unsigned int > texCoordsOffset;
   std::vector< unsigned int > attributesOffset;

   /**
      Determines the primitive shape the indicies define for the corresponding set of verticies. Valid values 
      include GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, 
      GL_TRIANGLES, GL_QUAD_STRIP, GL_QUADS, and GL_POLYGON.
   */
   void setIndexTopology( GLenum indexTopology ) { this->indexTopology = indexTopology; }
   GLenum getIndexTopology() { return this->indexTopology; }

   bool useTangents = false;

   std::vector< Vector >* getVerts() { return &this->verts; }
   std::vector< unsigned int >* getIndicies() { return &this->indicies; }
   std::vector< aftrColor4ub >* getColors() { return &this->colors; }
   std::vector< std::pair< std::vector< aftrTexture4f >, GLenum > >* getTexCoords() { return &this->texCoords; }
   std::vector< GLSLAttributeArray >* getAttributes() { return &this->attributes; }

   std::map< unsigned int, std::pair< unsigned int, unsigned int > >* getVertIdxToOrigVertIdxMap() { return &this->vertIdxToOrigVertIdx; }


   /**
      Generates an optimized buffer for this mesh. The first GLvoid* is a pointer to the interleaved
      vertex data. The second GLvoid* is a pointer to the index data. The member variables of this
      instance (namely, strides (size of each vertex entry), vertsOffsets, numColorChannels, colorsOffset, normalsOffset, 
      texCoordsOffset, and attributesOffset) indicate the necessary offset to generate a VBO or render
      using a VA.
   */
   virtual ModelMeshRenderData generate( MESH_SHADING_TYPE shadingType, GLenum glPrimType );
   virtual ModelMeshRenderData generateFlat( GLenum glPrimType );
   virtual ModelMeshRenderData generateFlatTriangles();
   virtual ModelMeshRenderData generateSmooth( GLenum glPrimType );
   virtual ModelMeshRenderData generateSmoothTriangles();
   virtual ModelMeshRenderData generateNoNormals( GLenum glPrimType );
   virtual ModelMeshRenderData generateNoNormalsTriangles();
   virtual ModelMeshRenderData generateNoNormalsPoints();
   virtual ModelMeshRenderData generateNoNormalsPointsFromPoints();
   virtual ModelMeshRenderData generateNoNormalsPointsFromTriangles();
   virtual ModelMeshRenderData generateNoNormalsLineStrip();
   virtual ModelMeshRenderData generateNoNormalsLineStripFromLineStrip();
   virtual ModelMeshRenderData generateNoNormalsLines();
   virtual ModelMeshRenderData generateNoNormalsLinesFromLines();
   virtual ModelMeshRenderData generateNoNormalsLineLoop();
   virtual ModelMeshRenderData generateNoNormalsLineLoopFromLineLoop();
   virtual ModelMeshRenderData generateNoNormalsLineLoopFromTriangles();

   // New methods to support generating tessellation patches (which have no normals).
   virtual ModelMeshRenderData generateNoNormalsPatches();
   virtual ModelMeshRenderData generateNoNormalsPatchesFromPatches();


   //generates the render data when no conversion is necessary lines->lines, triangles->triangles, etc...
   virtual ModelMeshRenderData generateNoTransformRequired();

protected:

   std::vector< Vector > verts; ///< All verticies contained in this mesh
   std::vector< unsigned int > indicies; ///< Indicies into the verts

   /**
      This stores a mapping from a current vertex index to the original (meshID, vertex index)
      that was read from file/standard input within the ModelParser::parse method. The size of
      this map will always equal the size of ModelMeshRenderDataGenerator::verts.size().

      This map is useful when one generates per vertex attributes off line that are associated
      with vertices in a particular file. For example, if I need to generate per vertex attributes 
      for a given 3ds file, I will have a vertex attribute set with a one-to-one mapping with the
      vertices in the 3ds file. Unfortunately, when that 3ds file is read in an parsed, the indices
      are changed by Aftr for efficiency reasons. Therefore, when the per vertex attributes are
      loaded from file the Aftr Mesh/Vertex Index pairs are different from those of the original
      3ds file; one can no longer map the original 3ds mesh/vertIdx to the Aftr mesh/VertIdx.

      This map resolves the issue:
      [OrigFileMeshID, OrigFilVertIdx] = vertIdxToOrigVertIdx[y];
      Where y is the yth vert within ModelMeshRenderDataGenerator::verts.

      This is not associated with the splitting / Aftrbimap reordering that may occur
      due to splitting within the ModelDataShared. This maps original file data to the data
      within this mesh.
   */
   std::map< unsigned int, std::pair< unsigned int, unsigned int > > vertIdxToOrigVertIdx;

   virtual void generateOffsets( MESH_SHADING_TYPE shadingType ); ///< This generates the offsets for both smooth and flat shading (they are the same).
   virtual void populateVertices( GLvoid* x ); ///< This populates the buffer with the values stored in verts
   virtual void populateColors( GLvoid* x );
   virtual void populateTextures( GLvoid* x );
   virtual void populateAttributes( GLvoid* x );

   virtual void populateIndicesSmooth( GLenum& idxMemType, GLvoid** x );
   virtual void populateIndicesFlat( GLenum& idxMemType, GLvoid** x );//<-- may be the same as smooth, check this

   //normal rendering stuff
   virtual void processNormalData( GLvoid* x, GLvoid** y, size_t vertsSize );
   virtual void populateIndicesNormal( GLenum& idxMemType, GLvoid** x, size_t vertsSize );

   std::vector< aftrColor4ub > colors; ///< If size > 0, then this data uses glColor Data
   /**
      This is the color used by glColor(...). This only takes effect if this->colors.size() is 0 and the 
      corresponding skin's isUsingColorMaterial() returns true.
   */
   aftrColor4ub meshColor;

   /**
      Collection of multi-textured coordinates and their cooresponding GLenum defining the
      dimensionality of that texture set.
      GLenum is set to GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, or GL_TEXTURE_4D; default is 2D.
   */
   std::vector< std::pair< std::vector< aftrTexture4f >, GLenum > > texCoords;
   std::vector< GLSLAttributeArray > attributes;

   GLsizei getOffsetFromTextureType( GLenum textureType );
   GLsizei getOffsetFromAttributeType( GLSLAttributeType type );

   virtual Vector calculateTangentVector( const Vector& v1, const Vector& v2, const Vector& st1, const Vector& st2 );
   
   ///Calculates tangent space using the square patch assumption
   virtual Vector calculateTangentVectorSQPA( const Vector& p0, const Vector& p1, const Vector& p2, const Vector& t0, const Vector& t1, const Vector& t2 );

   virtual Vector calcTangentVector( const Vector& p0, const Vector& p1, const Vector& p2, const Vector& t0, const Vector& t1, const Vector& t2, const Vector& normal );

   GLenum indexTopology; ///< Valid values include GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_QUAD_STRIP, GL_QUADS, and GL_POLYGON 
};

} //namespace Aftr

