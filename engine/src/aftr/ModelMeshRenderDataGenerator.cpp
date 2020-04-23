#include "ModelMeshRenderDataGenerator.h"
#include "ModelMeshDataShared.h"
#include "ManagerSDLTime.h"
#include <map>
#include <iostream>
#include <set>
#include <sstream>
#include "AftrUtilities.h"
using namespace Aftr;

ModelMeshRenderDataGenerator::ModelMeshRenderDataGenerator()
{
   useTangents = false;
   this->numColorChannels = GL_RGB;
   this->indexTopology = GL_TRIANGLES;
}

ModelMeshRenderDataGenerator::~ModelMeshRenderDataGenerator()
{
   //for( size_t i = 0; i < this->verts.size(); ++i )
   //{
   //   delete this->verts.at(i); this->verts.at(i) = NULL;
   //}
   //this->verts.clear();
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generate( MESH_SHADING_TYPE shadingType, GLenum glPrimType )
{
   if( shadingType == MESH_SHADING_TYPE::mstFLAT )
      return generateFlat( glPrimType );
   else if( shadingType == MESH_SHADING_TYPE::mstSMOOTH )
      return generateSmooth( glPrimType );
   else if( shadingType == MESH_SHADING_TYPE::mstNONE )
      return generateNoNormals( glPrimType );
   else
   {
      std::cout << "Attempting to generate render data that is not of type MESH_SHADING_TYPE::mstSMOOTH, MESH_SHADING_TYPE::mstFLAT, or MESH_SHADING_TYPE::mstNONE." << std::endl;
      std::cout << "This is not acceptable behavior." << std::endl;
      std::cout << "Press ENTER to exit...\n";
      int x = 0;
      int z = 5 / x; ++z;
      std::cin.get();
      exit(-1);
   }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateFlat( GLenum glPrimType )
{
   switch( glPrimType )
   {
   case GL_TRIANGLES:
      return generateFlatTriangles();
   default:
      std::cout << "All primitive types other than GL_TRIANGLES currently unimplemented for generateFlat" << std::endl;
      int f = 0;
      int g = 1 / f; ++g;
      std::cin.get();
      exit(1);
   }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateFlatTriangles( )
{
   std::cout << "Generating Flat Triangles" << std::endl;

   std::pair<GLvoid*,GLvoid*> temp; 
   std::pair<GLvoid*, GLvoid* > normalTemp;

   //make copy of originals
   std::vector< Vector > vertsCopy = verts;
   std::vector< unsigned int > indicesCopy = indicies;
   std::vector< std::vector< aftrTexture4f > > texCopy;
   for(size_t i = 0; i < texCoords.size(); i++)
   {
      texCopy.push_back( texCoords[i].first );
   }

   //replicate the new vertices
   std::set< unsigned int > s;
   std::vector< Vector > replicatedVerts;
   for(size_t i = 0; i < this->indicies.size(); i++)
   {
      if( s.find( this->indicies[i] ) != s.end() )//already used, make a new one
      {
         Vector replicatedVertex = Vector( verts[indicies[i]] );
         verts.push_back( replicatedVertex );
         replicatedVerts.push_back( replicatedVertex );

         if(colors.size() > 0)
            colors.push_back( colors[indicies[i]] );
         for(size_t j = 0; j < texCoords.size(); j++)
         {
            if( texCoords[j].first.size() > 0 )            
               texCoords[j].first.push_back( texCoords[j].first[indicies[i]] );            
         }
         indicies[i] = (unsigned int) (verts.size() - 1);
      }
      else
         s.insert( this->indicies[i] );
   }

   //generate tangents
   if(this->useTangents && texCoords.size() > 0)
   {
      std::cout << "Generating tangents flat." << std::endl;
      //std::cin.get();
      this->attributes.push_back( GLSLAttributeArray( "tangent", atVEC3 ) );
      this->attributes.rbegin()->getElements()->resize( indicies.size() );
      for(size_t i = 0; i < this->attributes.rbegin()->getElements()->size(); i++)
         this->attributes.rbegin()->getElements()->at(i) = new GLfloat[3];
      for(size_t i = 0; i < indicies.size(); i += 3)
      {
         Vector vec1 = verts[indicies[i]];
         Vector vec2 = verts[indicies[i+1]];
         Vector vec3 = verts[indicies[i+2]];

         Vector t1, t2, t3;
         if(this->texCoords.size() > 0)
         {
            t1 = Vector(this->texCoords[0].first[indicies[i]].u, this->texCoords[0].first[indicies[i]].v, 0);
            t2 = Vector(this->texCoords[0].first[indicies[i+1]].u, this->texCoords[0].first[indicies[i+1]].v, 0);
            t3 = Vector(this->texCoords[0].first[indicies[i+2]].u, this->texCoords[0].first[indicies[i+2]].v, 0);
         }

         //Vector v1 = vec1 - vec2;
         //Vector v2 = vec1 - vec3;

         Vector v1 = vec2 - vec1;
         Vector v2 = vec3 - vec1;

         //Vector st1 = t1 - t2;
         //Vector st2 = t1 - t3;

         Vector st1 = t2 - t1;
         Vector st2 = t1 - t3;

         v1.normalize();
         v2.normalize();
         st1.normalize();
         st2.normalize();
         Vector tangent = calculateTangentVector( v1, v2, st1, st2 );

         this->attributes.rbegin()->setElement(indicies[i], tangent );
         this->attributes.rbegin()->setElement(indicies[i+1], tangent );
         this->attributes.rbegin()->setElement(indicies[i+2], tangent );

         std::cout << tangent.toString() << std::endl;
      }
   }
   generateOffsets( MESH_SHADING_TYPE::mstFLAT );

   //allocate buffer
   temp.first = new GLubyte[stride * this->indicies.size()];

   //calculate flat normals
   for(size_t i = 0; i < indicies.size(); i += 3)
   {
      Vector v1 = verts[indicies[i]];
      Vector v2 = verts[indicies[i+1]];
      Vector v3 = verts[indicies[i+2]];

      Vector v = (v1-v2).crossProduct(v1-v3);
      v.normalize();

      GLfloat* ptr = (GLfloat*) ((char*) temp.first + this->normalsOffset + stride * indicies[i]);
      ptr[0] = v.x;
      ptr[1] = v.y;
      ptr[2] = v.z;

      ptr = (GLfloat*) ((char*) temp.first + this->normalsOffset + stride * indicies[i+1]);
      ptr[0] = v.x;
      ptr[1] = v.y;
      ptr[2] = v.z;

      ptr = (GLfloat*) ((char*) temp.first + this->normalsOffset + stride * indicies[i+2]);
      ptr[0] = v.x;
      ptr[1] = v.y;
      ptr[2] = v.z;
   }

   GLenum idxMemType = GL_OUT_OF_MEMORY;
   GLenum normalIdxMemType = GL_OUT_OF_MEMORY;
   populateVertices( temp.first );
   populateColors( temp.first );
   populateTextures( temp.first );
   populateAttributes( temp.first );
   populateIndicesFlat( idxMemType, &temp.second );

   size_t vertsSize = verts.size();
   size_t indicesSize = indicies.size();

   // now restore originals
   verts = vertsCopy;
   indicies = indicesCopy;
   for(size_t i = 0; i < texCopy.size(); i++)
   {
      texCoords[i].first = texCopy[i];
   }

   bool isUsingColorsArray = false;
   if( this->colors.size() > 0 )
      isUsingColorsArray = true;

   std::vector< GLSLAttributeType > attributesType;
   for( size_t i = 0; i < this->attributes.size(); ++i )
      attributesType.push_back( this->attributes.at(i).getType() );

   std::map< std::string, GLuint > attNameToIndex;
   for( size_t i = 0; i < this->attributes.size(); ++i )
      attNameToIndex[ attributes[i].getName() ] = (unsigned int)i;

   //normal render stuff
   populateIndicesNormal( normalIdxMemType, &normalTemp.second, vertsSize);
   processNormalData( temp.first, &normalTemp.first, vertsSize );

   ModelMeshRenderData renderData( temp.second, temp.first, (GLsizei)indicesSize, (GLsizei)vertsSize,
      idxMemType, this->stride, isUsingColorsArray, this->numColorChannels, this->vertsOffset,
      this->colorsOffset, this->normalsOffset, this->texCoordsOffset, 
      this->attributesOffset, attributesType, attNameToIndex, normalTemp.second, normalTemp.first, (GLsizei) vertsSize * 2, (GLsizei) vertsSize * 2, normalIdxMemType, GL_LINES );

   return renderData;
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateSmooth( GLenum glPrimType )
{
   switch( glPrimType )
   {
   case GL_TRIANGLES:
      return generateSmoothTriangles();
   default:
      std::cout << "All primitive types other than GL_TRIANGLES currently unimplemented for 'smooth normals'." << std::endl;
      std::cout << "Recieved: " << glPrimType << std::endl;
      std::cin.get();
      exit(1);
   }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateSmoothTriangles()
{
   //std::cout << "Generating Smooth Triangles." << std::endl;

   std::pair< GLvoid*, GLvoid* > temp;// Vertex Data, Indices
   std::pair< GLvoid*, GLvoid* > normalTemp;//Vertex Data, Indices

   if(useTangents)
      this->attributes.push_back( GLSLAttributeArray( "tangent", atVEC3 ) );

   generateOffsets( MESH_SHADING_TYPE::mstSMOOTH );

   //allocate buffer
   temp.first = new GLubyte[stride * this->verts.size()];

   //find normal of every face, put them in a list for each vertex
   std::map< unsigned int, std::vector< Vector > > m;//map of index to vector of normals for all faces containing that index
   std::map< unsigned int, std::vector< Vector > > t;

   for(size_t i = 0; i < this->indicies.size(); i += 3)
   {
      //find normal of face
      Vector vec1 = verts[indicies[i]];
      Vector vec2 = verts[indicies[i+1]];
      Vector vec3 = verts[indicies[i+2]];

      Vector v1 = vec2 - vec1;
      Vector v2 = vec3 - vec1;

      v1.normalize();
      v2.normalize();

      Vector normal = Vector(v1.crossProduct(v2));
      normal.normalize();

      Vector tangent;
      if(useTangents)
      {
         Vector t1, t2, t3;
         if(this->texCoords.size() > 0)
         {
            t1 = Vector(this->texCoords[0].first[indicies[i]].u, this->texCoords[0].first[indicies[i]].v, 0);
            t2 = Vector(this->texCoords[0].first[indicies[i+1]].u, this->texCoords[0].first[indicies[i+1]].v, 0);
            t3 = Vector(this->texCoords[0].first[indicies[i+2]].u, this->texCoords[0].first[indicies[i+2]].v, 0);
         }


         Vector st1 = t1 - t2;
         Vector st2 = t1 - t3;

         st1.normalize();
         st2.normalize();

         //tangent = this->calcTangentVector( vec1, vec2, vec3, t1, t2, t3, normal ); //SLN: we've had more luck w/ SQPA tangents
         //std::cout << "Tangent is " << tangent.toString() << "\n";
         tangent = calculateTangentVectorSQPA( vec1, vec2, vec3, t1, t2, t3 );
         //std::cout << "SQPA Tangent is " << tangent.toString() << "\n";
      }

      //insert normal into list for all 3 indices
      for(int j = 0; j < 3; j++)
      {
         m[this->indicies[i+j]].push_back(normal);
         if(useTangents)
            t[this->indicies[i+j]].push_back(tangent);
      }
   }

   std::map< unsigned int, std::vector< unsigned int > > mm;

   //m - map of index to vector of normals for all faces containing that index

   //maps each distinct vertex to all the indices that represent it
   std::map< Vector, std::vector< unsigned int > > v2is;
   for( std::map< unsigned int, std::vector < Vector > >::iterator itr = m.begin(); itr != m.end(); itr++ )
   {
      /*
      bool found = false;
      for( auto itrV = v2is.begin(); itrV != v2is.end(); itrV++)
      {
         //std::cout << this->verts[itr->first]->toString() << " " << itrV->first.toString() << std::endl;
         if( this->verts[ itr->first ]->isEqual( itrV->first, .001 ) )
         {
            //std::cout << "Merge!" << std::endl;
            itrV->second.push_back( itr->first );
               found = true;
               break;
         }
      }
      if(!found)
      {
         std::vector< unsigned int > temp;
         temp.push_back( itr->first );
         v2is.insert( std::make_pair( *this->verts[ itr->first ], temp ) );
      }
      */
        v2is[ this->verts[ itr->first ] ].push_back( itr->first );
   }

   //maps indices to all indices that represent the same distinct vertex
   for( std::map< Vector, std::vector < unsigned int > >::iterator itr = v2is.begin(); itr != v2is.end(); itr++ )
      for(size_t i = 0; i < itr->second.size(); i++)
         for(size_t j = 0; j < itr->second.size(); j++)
            mm[ itr->second[i] ].push_back( itr->second[j] );

   //*************
   //peak memory usage
   //*************
   //std::cout << "Peak memory usage!" << std::endl;

   v2is.clear();

   //average normals for each index
   for(std::map< unsigned int, std::vector< unsigned int > >::iterator itr = mm.begin(); itr != mm.end(); itr++)
   {
      Vector v(0,0,0);
      for(size_t i = 0; i < itr->second.size(); i++)
         for(size_t j = 0; j < m[itr->second[i]].size(); j++)
            v += m[itr->second[i]][j];
      size_t counter = 0;
      for(size_t i = 0; i < itr->second.size(); i++)
         counter += m[itr->second[i]].size();
      v /= (float)counter;
      v.normalize();

      //insert normals into normal list
      GLfloat* ptr = (GLfloat*) ((char*) temp.first + this->normalsOffset + stride * itr->first);
      ptr[0] = v.x;
      ptr[1] = v.y;
      ptr[2] = v.z;

      //itr->second.clear(); //if this is cleared, the tangent generation will be broken
   }

   m.clear();

   if(useTangents)
   {
      std::cout << "Generating tangents smooth" << std::endl;
      //std::cin.get();
      std::vector < Vector > temp;

      for(std::map< unsigned int, std::vector< unsigned int > >::iterator itr = mm.begin(); itr != mm.end(); itr++)
      {
         Vector v(0,0,0);
         size_t counter = 0;
         for(size_t i = 0; i < itr->second.size(); i++)
            for(size_t j = 0; j < t[itr->second[i]].size(); j++)
               v += t[itr->second[i]][j];
         for(size_t i = 0; i < itr->second.size(); i++)
            counter += t[itr->second[i]].size();
         v /= (float)counter;
         v.normalize();

         if(temp.size() < itr->first + 1)
            temp.resize(itr->first + 1);

         temp[itr->first].x = v.x;
         temp[itr->first].y = v.y;
         temp[itr->first].z = v.z;
      }
      //std::cout << "Temp size" << temp.size() << std::endl;
      for(size_t i = 0; i < temp.size(); i++)
      {
         this->attributes.rbegin()->getElements()->push_back( new GLfloat[3] );
         ((GLfloat*) *this->attributes.rbegin()->getElements()->rbegin())[0] = temp[i].x;
         ((GLfloat*) *this->attributes.rbegin()->getElements()->rbegin())[1] = temp[i].y;
         ((GLfloat*) *this->attributes.rbegin()->getElements()->rbegin())[2] = temp[i].z;
      }
      //std::cout << this->attributes.rbegin()->getElements()->size() << std::endl;
   }

   mm.clear();

   GLenum idxMemType = GL_OUT_OF_MEMORY;
   GLenum NormalidxMemType = GL_OUT_OF_MEMORY;
   populateVertices( temp.first );
   populateColors( temp.first ); 
   populateTextures( temp.first );
   populateAttributes( temp.first );
   populateIndicesSmooth( idxMemType, &temp.second );

   bool isUsingColorsArray = false;
   if( this->colors.size() > 0 )
      isUsingColorsArray = true;

   std::vector< GLSLAttributeType > attributesType;
   for( size_t i = 0; i < this->attributes.size(); ++i )
      attributesType.push_back( this->attributes.at(i).getType() );

   std::map< std::string, GLuint > attNameToIndex;
   for( size_t i = 0; i < this->attributes.size(); ++i )
      attNameToIndex[ attributes[i].getName() ] = (GLuint)i;

   //normal render data
   processNormalData( temp.first, &normalTemp.first, this->verts.size() );
   populateIndicesNormal( NormalidxMemType, &normalTemp.second, this->verts.size() );

  // std::cout << this->toString() << std::endl;

  // std::cout << printBytes( (char*) temp.first, 100 ) << std::endl;
  // std::cin.get();

   ModelMeshRenderData renderData( temp.second, temp.first, (GLsizei)this->indicies.size(), (GLsizei)this->verts.size(),
      idxMemType, this->stride, isUsingColorsArray, this->numColorChannels, this->vertsOffset,
      this->colorsOffset, this->normalsOffset, this->texCoordsOffset, 
      this->attributesOffset, attributesType, attNameToIndex, normalTemp.second, normalTemp.first, (GLsizei) this->verts.size() * 2, (GLsizei) this->verts.size() * 2, NormalidxMemType, GL_LINES );

   return renderData;
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormals( GLenum glPrimType )
{
   switch( glPrimType )
   {
   case GL_POINTS:
      return generateNoNormalsPoints();
   case GL_TRIANGLES:
      return generateNoNormalsTriangles();
   case GL_LINE_STRIP:
      return generateNoNormalsLineStrip();
   case GL_LINES:
      return generateNoNormalsLines();
   case GL_LINE_LOOP:
      return generateNoNormalsLineLoop();
   case GL_PATCHES:
      return generateNoNormalsPatches();
   default:
      std::cout << "Only generation of No Normals for { Points, Triangles, Patches } are implemented." << std::endl;
      int f = 0;
      int a = 3 / f; ++a;
      std::cin.get();
      exit(1);
   }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsLineLoop()
{
   switch( this->indexTopology )
   {
   case GL_LINE_LOOP:
      return generateNoNormalsLineLoopFromLineLoop();
   case GL_TRIANGLES:
      return generateNoNormalsLineLoopFromTriangles();
   default:
      std::cout << "Only generation of No Normals Points from { LineLoop } are implemented." << std::endl;
      std::cin.get();
      exit(1);
   }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsPoints()
{
   switch( this->indexTopology )
   {
   case GL_POINTS:
      return generateNoNormalsPointsFromPoints();
   case GL_TRIANGLES:
      return generateNoNormalsPointsFromTriangles();
   default:
      std::cout << "Only generation of No Normals Points from { Points, Triangles } are implemented." << std::endl;
      std::cin.get();
      exit(1);
   }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsLines()
{
   switch( this->indexTopology )
   {
   case GL_LINES:
      return generateNoNormalsLinesFromLines();
   default:
      std::cout << "Only generation of No Normals Lines from { Lines } are implemented." << std::endl;
      std::cin.get();
      exit(1);
   }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsLineStrip()
{
   switch( this->indexTopology )
   {
   case GL_LINE_STRIP:
      return generateNoNormalsLineStripFromLineStrip();
   default:
      std::cout << "Only generation of No Normals Line Strip from { Line Strip } are implemented." << std::endl;
      std::cin.get();
      exit(1);
   }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsTriangles()
{
   generateOffsets( MESH_SHADING_TYPE::mstNONE );
   return generateNoTransformRequired();
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsPointsFromPoints()
{
   generateOffsets( MESH_SHADING_TYPE::mstNONE );
   return generateNoTransformRequired();  
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsLineLoopFromLineLoop()
{
   generateOffsets( MESH_SHADING_TYPE::mstNONE );
   return generateNoTransformRequired();  
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsLineLoopFromTriangles()
{
   generateOffsets( MESH_SHADING_TYPE::mstNONE );
   return generateNoTransformRequired();
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsPointsFromTriangles()
{
   std::pair<GLvoid*,GLvoid*> temp;
   std::vector< unsigned int > indexCopy = this->indicies;
   indicies.clear();
   for(size_t i = 0; i < verts.size(); i++)
      indicies.push_back( (unsigned int) i);

   generateOffsets( MESH_SHADING_TYPE::mstNONE );

   //allocate buffer
   temp.first = new GLubyte[stride * this->verts.size()];

   GLenum idxMemType = GL_OUT_OF_MEMORY;
   populateVertices( temp.first );
   populateColors( temp.first ); 
   populateTextures( temp.first );
   populateAttributes( temp.first );
   populateIndicesSmooth( idxMemType, &temp.second );

   bool isUsingColorsArray = false;
   if( this->colors.size() > 0 )
      isUsingColorsArray = true;

   std::vector< GLSLAttributeType > attributesType;
   for( size_t i = 0; i < this->attributes.size(); ++i )
      attributesType.push_back( this->attributes.at(i).getType() );

   std::map< std::string, GLuint > attNameToIndex;
   for( size_t i = 0; i < this->attributes.size(); ++i )
      attNameToIndex[ attributes[i].getName() ] = (GLuint) i;

   this->indicies = indexCopy;

   ModelMeshRenderData renderData( temp.second, temp.first, (GLsizei)this->verts.size(), (GLsizei)this->verts.size(),//same size verts and indices for points
      idxMemType, this->stride, isUsingColorsArray, this->numColorChannels, this->vertsOffset,
      this->colorsOffset, this->normalsOffset, this->texCoordsOffset, 
      this->attributesOffset, attributesType, attNameToIndex, NULL, NULL, 0, 0, GL_UNSIGNED_BYTE, GL_LINES  );

   return renderData;

}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsLineStripFromLineStrip()
{
   generateOffsets( MESH_SHADING_TYPE::mstNONE );
   return generateNoTransformRequired();
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsLinesFromLines()
{
   //std::cout << "lines from lines" << std::endl;
   generateOffsets( MESH_SHADING_TYPE::mstNONE );
   return generateNoTransformRequired(); 
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsPatches()
{
    switch (this->indexTopology) {
    case GL_PATCHES:
        return generateNoNormalsPatchesFromPatches();
    default:
        std::cout << "Only generation of No Normals Patches from { Patches } are implemented." << std::endl;
        std::cin.get();
        exit(1);
    }
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoNormalsPatchesFromPatches()
{
    generateOffsets(MESH_SHADING_TYPE::mstNONE);
    return generateNoTransformRequired();
}

ModelMeshRenderData ModelMeshRenderDataGenerator::generateNoTransformRequired()
{
   //std::cout << "Indices: " << this->indicies.size() << std::endl;
   //std::cout << "vertices: " << this->verts.size() << std::endl;
   //std::cout << "colors: " << this->colors.size() << std::endl;

   std::pair<GLvoid*,GLvoid*> temp;

   //allocate buffer
   temp.first = new GLubyte[stride * this->verts.size()];

   GLenum idxMemType = GL_OUT_OF_MEMORY;
   populateVertices( temp.first );
   populateColors( temp.first ); 
   populateTextures( temp.first );
   populateAttributes( temp.first );
   populateIndicesSmooth( idxMemType, &temp.second );

   bool isUsingColorsArray = false;
   if( this->colors.size() > 0 )
      isUsingColorsArray = true;

   std::vector< GLSLAttributeType > attributesType;
   for( size_t i = 0; i < this->attributes.size(); ++i )
      attributesType.push_back( this->attributes.at(i).getType() );

   std::map< std::string, GLuint > attNameToIndex;
   for( size_t i = 0; i < this->attributes.size(); ++i )
      attNameToIndex[ attributes[i].getName() ] = (unsigned int) i;

   ModelMeshRenderData renderData( temp.second, temp.first, (GLsizei)this->indicies.size(), (GLsizei)this->verts.size(),
      idxMemType, this->stride, isUsingColorsArray, this->numColorChannels, this->vertsOffset,
      this->colorsOffset, this->normalsOffset, this->texCoordsOffset, 
      this->attributesOffset, attributesType, attNameToIndex, NULL, NULL, 0, 0, GL_UNSIGNED_BYTE, GL_LINES  );

   return renderData;
}

GLsizei ModelMeshRenderDataGenerator::getOffsetFromTextureType( GLenum textureType )
{
   if(textureType == GL_TEXTURE_1D)
      return sizeof(GLfloat);
   if(textureType == GL_TEXTURE_2D)
      return sizeof(GLfloat) * 2;
   if(textureType == GL_TEXTURE_3D)
      return sizeof(GLfloat) * 3;
   //if(textureType == GL_TEXTURE_4D)
   // return sizeof(GLfloat) * 4;
   std::cout << "Attempting to get offset for unsupported texture dimensionality" << std::endl;
   return 0;
}

GLsizei ModelMeshRenderDataGenerator::getOffsetFromAttributeType( GLSLAttributeType type )
{
   switch( type )
   {
   case atFLOAT:
      return sizeof( GLfloat );
   case atBOOL:
      return sizeof( bool );
   case atINT:
      return sizeof( GLint );
   case atVEC2:
      return sizeof( GLfloat ) * 2;
   case atVEC3:
      return sizeof( GLfloat ) * 3;
   case atVEC4:
      return sizeof( GLfloat ) * 4;
   case atBVEC2:
      return sizeof( bool ) * 2;
   case atBVEC3:
      return sizeof( bool ) * 3;
   case atBVEC4:
      return sizeof( bool ) * 4;
   case atIVEC2:
      return sizeof( GLint ) * 2;
   case atIVEC3:
      return sizeof( GLint ) * 3;
   case atIVEC4:
      return sizeof( GLint ) * 4;
   case atMAT2:
      return sizeof( GLfloat ) * 4;
   case atMAT3:
      return sizeof( GLfloat ) * 9;
   case atMAT4:
      return sizeof( GLfloat ) * 16;
      //atSAMPLER1D,
      //atSAMPLER2D,
      //atSAMPLER3D,
      //atSAMPLERCUBE,
      //atSAMPLER1DSHADOW,
      //atSAMPLER2DSHADOW,
   default:
      std::cout << "No size avaialable for type " << type << std::endl;
      std::cin.get();
      return 0;
   }
}

void ModelMeshRenderDataGenerator::generateOffsets( MESH_SHADING_TYPE shadingType )
{
   //verts
   //normals - if any
   //textures - if any
   //attributes - if any
   //colors - if any
   //stride

   //generate offsets
   this->vertsOffset = 0;
   this->normalsOffset = 0;
   this->colorsOffset = 0;
   this->texCoordsOffset.clear();
   this->attributesOffset.clear();
   unsigned int nextAvailableByte = 0;

   nextAvailableByte += sizeof( GLfloat ) * 3; //X,Y,Z Verts

   if( shadingType != MESH_SHADING_TYPE::mstNONE )
   {
      this->normalsOffset = nextAvailableByte;
      nextAvailableByte += sizeof( GLfloat ) * 3;
   }

   for(size_t i = 0; i < this->texCoords.size(); i++)
   {
      this->texCoordsOffset.push_back( nextAvailableByte );
      nextAvailableByte += getOffsetFromTextureType(this->texCoords[i].second);
   }

   for(size_t i = 0; i < this->attributes.size(); i++)
   {
      this->attributesOffset.push_back( nextAvailableByte );
      nextAvailableByte += getOffsetFromAttributeType( this->attributes[i].getType() );
   }

   if( this->colors.size() > 0 )
   {
      this->colorsOffset = nextAvailableByte;
      if( this->numColorChannels == GL_RGB )
         nextAvailableByte += 3 * sizeof( GLubyte );
      else if( this->numColorChannels == GL_RGBA )
         nextAvailableByte += 4 * sizeof( GLubyte );
      else
      {
         std::cout << "Error: improper color channel selection" << std::endl;
         std::cout << "Bad things will happen." << std::endl;
         std::cin.get();
      }
   }

   this->stride = nextAvailableByte;  
}

void ModelMeshRenderDataGenerator::populateVertices( GLvoid* x )
{

   for(size_t i = 0; i < this->verts.size(); i++)
   {
      //populate vertices
      GLfloat* ptr = (GLfloat*) ((char*) x + this->vertsOffset + stride * i);
      ptr[0] = verts[i].x;
      ptr[1] = verts[i].y;
      ptr[2] = verts[i].z;
   }

}

void ModelMeshRenderDataGenerator::populateColors( GLvoid* x )
{
   //populate colors
   for(size_t i = 0; i < this->colors.size(); i++)
   {
      GLubyte* ptr = (GLubyte*) ((char*) x + this->colorsOffset + stride * i);
      ptr[0] = this->colors[i].r;
      ptr[1] = this->colors[i].g;
      ptr[2] = this->colors[i].b;
      if(this->numColorChannels == GL_RGBA)
         ptr[3] = this->colors[i].a; 
   }
}

void ModelMeshRenderDataGenerator::populateTextures( GLvoid* x )
{
   //generate texture coords
   for(size_t i = 0; i < this->texCoords.size(); i++)
   {
      for(size_t j = 0; j < this->texCoords[i].first.size(); j++)
      {
         GLfloat* ptr = (GLfloat*) ((char*) x + this->texCoordsOffset[i] + stride * j);
         ptr[0] = this->texCoords[i].first[j].u;
         if(this->texCoords[i].second == GL_TEXTURE_2D
            || this->texCoords[i].second == GL_TEXTURE_3D
            //|| this->texCoords[i].second == GL_TEXTURE_4D
            )
            ptr[1] = this->texCoords[i].first[j].v;
         if(this->texCoords[i].second == GL_TEXTURE_3D
            //|| this->texCoords[i].second == GL_TEXTURE_4D
            )
            ptr[2] = this->texCoords[i].first[j].c;
         //if(this->texCoords[i].second == GL_TEXTURE_4D)
         //ptr[3] = this->texCoords[i].first[j].d;
         
         //std::cout << "TexCoord2D[" << j << " (" << ptr[0] << ", " << ptr[1] << ")\n";
      }
   }
}

void ModelMeshRenderDataGenerator::populateAttributes( GLvoid* x )
{
   //std::cout << "Populate attributes: " << std::endl;
   //std::cout << this->attributesOffset.size() << std::endl;
   for(size_t i = 0; i < this->attributesOffset.size(); i++)
   {
      //std::cout << this->attributes[i].getElements()->size() << std::endl;
      for(size_t j = 0; j < this->attributes[i].getElements()->size(); j++)
      {
         if(this->attributes[i].getType() == atVEC3)
         {
            //std::cout << "Assigning attributes" << std::endl;
            GLfloat* ptr = (GLfloat*) ((char*) x + this->attributesOffset[i] + stride * j);
            ptr[0] = ((GLfloat*) this->attributes[i].getElements()->at(j))[0];
            ptr[1] = ((GLfloat*) this->attributes[i].getElements()->at(j))[1];
            ptr[2] = ((GLfloat*) this->attributes[i].getElements()->at(j))[2];
         }
         else
         {
            std::cout << "Warning: Everything except atVEC3 is unimplemented!" << std::endl;
            std::cin.get();
         }
      }
   }
}

void ModelMeshRenderDataGenerator::populateIndicesSmooth( GLenum& idxMemType, GLvoid** x )//<-- check if this is actually any different from populateIndicesFlat
{
   //populate indices
   if(this->indicies.size() < 0xFF)
   {
      idxMemType = GL_UNSIGNED_BYTE;
      *x = new GLubyte[this->indicies.size()];
   }
   else if(this->indicies.size() < 0xFFFF)
   {
      idxMemType = GL_UNSIGNED_SHORT;
      *x = new GLshort[this->indicies.size()];
   }
   else if(this->indicies.size() < 0xFFFFFFFF)
   {
      idxMemType = GL_UNSIGNED_INT;
      *x = new GLuint[this->indicies.size()];
   }
   else
   {
      std::cout << "Too many indices in mesh to fit in GL_UNSIGNED_INT, this behavior is unsupported." << std::endl;
      idxMemType = GL_OUT_OF_MEMORY;
   }

   for(size_t i = 0; i < this->indicies.size(); i++)
   {
      if( this->indicies.size() < 0xFF )
         ( (GLubyte*)*x )[i] = static_cast<GLubyte>( indicies[i] );
      else if(this->indicies.size() < 0xFFFF)
         ( (GLushort*)*x )[i] = static_cast<GLushort>( indicies[i] );
      else if(this->indicies.size() < 0xFFFFFFFF)
         ( (GLuint*)*x )[i] = static_cast<GLuint>( indicies[i] );
   }
}

void ModelMeshRenderDataGenerator::populateIndicesFlat( GLenum& idxMemType, GLvoid** x )//<-- check if this is actually any different from populateIndicesFlat
{
   //populate indices
   if(this->indicies.size() < 0xFF)
   {
      idxMemType = GL_UNSIGNED_BYTE;
      *x = new GLubyte[this->indicies.size()];
   }
   else if(this->indicies.size() < 0xFFFF)
   {
      idxMemType = GL_UNSIGNED_SHORT;
      *x = new GLshort[this->indicies.size()];
   }
   else if(this->indicies.size() < 0xFFFFFFFF)
   {
      idxMemType = GL_UNSIGNED_INT;
      *x = new GLuint[this->indicies.size()];
   }
   else
   {
      std::cout << "Too many indices in mesh to fit in GL_UNSIGNED_INT, this behavior is unsupported." << std::endl;
      idxMemType = GL_OUT_OF_MEMORY;
   }

   for(size_t i = 0; i < this->indicies.size(); i++)
   {
      if(this->indicies.size() < 0xFF)
         ((GLubyte*) *x)[i] = (GLubyte) indicies[i];
      else if(this->indicies.size() < 0xFFFF)
         ((GLushort*) *x)[i] = (GLushort) indicies[i];
      else if(this->indicies.size() < 0xFFFFFFFF)
         ((GLuint*) *x)[i] = (GLuint) indicies[i];
   }
}

void ModelMeshRenderDataGenerator::populateIndicesNormal( GLenum& idxMemType, GLvoid** x, size_t vertsSize )
{
   //populate indices
   if(this->indicies.size() < 0xFF)
   {
      idxMemType = GL_UNSIGNED_BYTE;
      *x = new GLubyte[vertsSize * 2];
   }
   else if(this->indicies.size() < 0xFFFF)
   {
      idxMemType = GL_UNSIGNED_SHORT;
      *x = new GLshort[vertsSize * 2];
   }
   else if(this->indicies.size() < 0xFFFFFFFF)
   {
      idxMemType = GL_UNSIGNED_INT;
      *x = new GLuint[vertsSize * 2];
   }
   else
   {
      std::cout << "Too many indices in mesh to fit in GL_UNSIGNED_INT, this behavior is unsupported." << std::endl;
      idxMemType = GL_OUT_OF_MEMORY;
   }

   for(size_t i = 0; i < vertsSize * 2; i++)
   {
      if(this->indicies.size() < 0xFF)
         ((GLubyte*) *x)[i] = (GLubyte) i;
      else if(this->indicies.size() < 0xFFFF)
         ((GLushort*) *x)[i] = (GLushort) i;
      else if(this->indicies.size() < 0xFFFFFFFF)
         ((GLuint*) *x)[i] = (GLuint) i;
   }
}

void ModelMeshRenderDataGenerator::processNormalData( GLvoid* x, GLvoid** y, size_t vertsSize )
{
   //each normal has two endpoints (this is why there are two verticies for each normal)
   *y = new GLubyte[sizeof(GLfloat) * 3 * 2 * vertsSize];//2 normal vertices for every face
   for(size_t i = 0; i < vertsSize; i++)
   {
      GLfloat* ptr = (GLfloat*) ((char*) x + this->vertsOffset + stride * i);
      ((GLfloat*) *y)[i*6+0] = ptr[0];
      ((GLfloat*) *y)[i*6+1] = ptr[1];
      ((GLfloat*) *y)[i*6+2] = ptr[2];

      ptr = (GLfloat*) ((char*) x + this->normalsOffset + stride * i);
      ((GLfloat*) *y)[i*6+3] = ((GLfloat*) *y)[i*6+0] + ptr[0];
      ((GLfloat*) *y)[i*6+4] = ((GLfloat*) *y)[i*6+1] + ptr[1];
      ((GLfloat*) *y)[i*6+5] = ((GLfloat*) *y)[i*6+2] + ptr[2];
   }
}

Vector ModelMeshRenderDataGenerator::calculateTangentVector( const Vector& v1, const Vector& v2, const Vector& st1, const Vector& st2 )
{
   float coef = 1 / (st1.x * st2.y + st2.x * st1.y );
   Vector tangent( coef * ((v1.x * st2.y) + (v2.x * -st1.y)),
      coef * ((v1.y * st2.y) + (v2.y * -st1.y)),
      coef * ((v1.z * st2.y) + (v2.z * -st1.y)));
   tangent.normalize();
   return tangent;
}

/// Generate tangents assuming the square patch assumption
Vector ModelMeshRenderDataGenerator::calculateTangentVectorSQPA( const Vector& p0, const Vector& p1, const Vector& p2, const Vector& t0, const Vector& t1, const Vector& t2 )
{
   //generate T (Tangent Vector)
   Vector T, Ttop;
   float Tbot = 0;
   Ttop = (p2 - p0 ) * (t1.y - t0.y) - (p1 - p0 ) * (t2.y - t0.y);
   Tbot = (t2.x - t0.x) * (t1.y-t0.y) - (t1.x - t0.x) * (t2.y - t0.y );
   T = Ttop / Tbot;


   Vector B, Btop;
   float Bbot = 0;
   Btop = (p2 - p0 ) * (t1.x - t0.x) - (p1 - p0 ) * (t2.x - t0.x);
   Bbot = (t1.x - t0.x) * (t2.y - t0.y ) - (t2.x - t0.x) * (t1.y-t0.y);
   B = Btop / Bbot;

   Vector N = T.crossProduct( B );
   N.normalize();

   //B = N.crossProduct( T );
   B.normalize();
   T.normalize();

   //std::cout << "Used SQPA tangent generation...\n";
   //return Vector(0,1,0).normalizeMe();
   return T;

}

//void ModelMeshRenderDataGenerator::calcTangentVector(const float pos1[3], const float pos2[3],
//                                                      const float pos3[3], const float texCoord1[2],
//                                                      const float texCoord2[2], const float texCoord3[2],
//                                                      const float normal[3], float tangent[4]) const
Vector ModelMeshRenderDataGenerator::calcTangentVector( const Vector& p0, const Vector& p1, const Vector& p2, const Vector& t0, const Vector& t1, const Vector& t2, const Vector& normal )
{
   // Given the 3 vertices (position and texture coordinates) of a triangle
   // calculate and return the triangle's tangent vector.

   // Create 2 vectors in object space.
   //
   // edge1 is the vector from vertex positions pos1 to pos2.
   // edge2 is the vector from vertex positions pos1 to pos3.

   //Vector3 edge1(pos2[0] - pos1[0], pos2[1] - pos1[1], pos2[2] - pos1[2]);
   //Vector3 edge2(pos3[0] - pos1[0], pos3[1] - pos1[1], pos3[2] - pos1[2]);

   Vector e1 = p1 - p0;
   Vector e2 = p2 - p0;

   //edge1.normalize();
   //edge2.normalize();

   e1.normalize();
   e2.normalize();

   // Create 2 vectors in tangent (texture) space that point in the same
   // direction as edge1 and edge2 (in object space).
   //
   // texEdge1 is the vector from texture coordinates texCoord1 to texCoord2.
   // texEdge2 is the vector from texture coordinates texCoord1 to texCoord3.
   //Vector2 texEdge1(texCoord2[0] - texCoord1[0], texCoord2[1] - texCoord1[1]);
   //Vector2 texEdge2(texCoord3[0] - texCoord1[0], texCoord3[1] - texCoord1[1]);

   Vector te1 = t1 - t0;
   Vector te2 = t2 - t0;

   //texEdge1.normalize();
   //texEdge2.normalize();

   te1.normalize();
   te2.normalize();

   // These 2 sets of vectors form the following system of equations:
   //
   //  edge1 = (texEdge1.x * tangent) + (texEdge1.y * bitangent)
   //  edge2 = (texEdge2.x * tangent) + (texEdge2.y * bitangent)
   //
   // Using matrix notation this system looks like:
   //
   //  [ edge1 ]     [ texEdge1.x  texEdge1.y ]  [ tangent   ]
   //  [       ]  =  [                        ]  [           ]
   //  [ edge2 ]     [ texEdge2.x  texEdge2.y ]  [ bitangent ]
   //
   // The solution is:
   //
   //  [ tangent   ]        1     [ texEdge2.y  -texEdge1.y ]  [ edge1 ]
   //  [           ]  =  -------  [                         ]  [       ]
   //  [ bitangent ]      det A   [-texEdge2.x   texEdge1.x ]  [ edge2 ]
   //
   //  where:
   //        [ texEdge1.x  texEdge1.y ]
   //    A = [                        ]
   //        [ texEdge2.x  texEdge2.y ]
   //
   //    det A = (texEdge1.x * texEdge2.y) - (texEdge1.y * texEdge2.x)
   //
   // From this solution the tangent space basis vectors are:
   //
   //    tangent = (1 / det A) * ( texEdge2.y * edge1 - texEdge1.y * edge2)
   //  bitangent = (1 / det A) * (-texEdge2.x * edge1 + texEdge1.x * edge2)
   //     normal = cross(tangent, bitangent)

   //Vector3 t;
   //Vector3 b;
   //Vector3 n(normal[0], normal[1], normal[2]);

   Vector t,b,n;
   n = Vector(1,0,0);

   //float det = (texEdge1.x * texEdge2.y) - (texEdge1.y * texEdge2.x);
   float det = ( te1.x * te2.y ) - ( te1.y * te2.x );

   //if (Math::closeEnough(det, 0.0f))
   //{
   //   t.set(1.0f, 0.0f, 0.0f);
   //   b.set(0.0f, 1.0f, 0.0f);
   //}
   //else
   //{
   //   det = 1.0f / det;

   //   t.x = (texEdge2.y * edge1.x - texEdge1.y * edge2.x) * det;
   //   t.y = (texEdge2.y * edge1.y - texEdge1.y * edge2.y) * det;
   //   t.z = (texEdge2.y * edge1.z - texEdge1.y * edge2.z) * det;

   //   b.x = (-texEdge2.x * edge1.x + texEdge1.x * edge2.x) * det;
   //   b.y = (-texEdge2.x * edge1.y + texEdge1.x * edge2.y) * det;
   //   b.z = (-texEdge2.x * edge1.z + texEdge1.x * edge2.z) * det;

   //   t.normalize();
   //   b.normalize();
   //}

   if( fabs( det ) < 0.001 )
   {
      t = Vector(1,0,0);
      b = Vector(0,1,0);
   }
   else
   {
      det = 1.0f / det;

      t.x = (te2.y * e1.x - te1.y * e2.x) * det;
      t.y = (te2.y * e1.y - te1.y * e2.y) * det;
      t.z = (te2.y * e1.z - te1.y * e2.z) * det;

      b.x = (-te2.x * e1.x + te1.x * e2.x) * det;
      b.y = (-te2.x * e1.y + te1.x * e2.y) * det;
      b.z = (-te2.x * e1.z + te1.x * e2.z) * det;

      t.normalize();
      b.normalize();
   }

   // Calculate the handedness of the local tangent space.
   // The bitangent vector is the cross product between the triangle face
   // normal vector and the calculated tangent vector. The resulting bitangent
   // vector should be the same as the bitangent vector calculated from the
   // set of linear equations above. If they point in different directions
   // then we need to invert the cross product calculated bitangent vector. We
   // store this scalar multiplier in the tangent vector's 'w' component so
   // that the correct bitangent vector can be generated in the normal mapping
   // shader's vertex shader.

   //Vector3 bitangent = Vector3::cross(n, t);
   //float handedness = (Vector3::dot(bitangent, b) < 0.0f) ? -1.0f : 1.0f;
   Vector bitangent = n.crossProduct( t );
   if ( bitangent.dotProduct( b ) < 0.0f )
   {
      //swap t's handedness
      //This code isn't verified / tested.
   }   

   return t;
}

std::string Aftr::ModelMeshRenderDataGenerator::toString()
{
   std::stringstream ss;
   ss << "Stride: " << stride << std::endl
      << "Vert Offset: " << this->vertsOffset << std::endl
      << "Color Offset: " << this->colorsOffset << std::endl
      << "Color Channels: " << this->numColorChannels << std::endl
      << "Normals Offset: " << this->normalsOffset << std::endl
      << "Texture Offsets: " << this->texCoordsOffset.size() << std::endl;
   for(size_t i = 0; i < this->texCoordsOffset.size(); i++)
   {
      ss << this->texCoordsOffset[i] << std::endl;
   }
   ss << "Attribute Offsets: " << this->attributesOffset.size() << std::endl;
   for(size_t i = 0; i < this->attributesOffset.size(); i++)
   {
      ss << this->attributesOffset[i] << std::endl;
   }

   for( std::map< unsigned int, std::pair< unsigned int, unsigned int > >::iterator it = this->vertIdxToOrigVertIdx.begin(); 
        it != this->vertIdxToOrigVertIdx.end(); it++ )
   {
      ss << "vertIdxToOrigVertIdx[" << it->first << "] = (" << it->second.first << ", " << it->second.second << ") -> "
         << this->verts[it->first].toString() << "\n";
   }
   return ss.str();
}