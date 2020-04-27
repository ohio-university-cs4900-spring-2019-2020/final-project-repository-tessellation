#pragma once

#include "AftrOpenGLIncludes.h"
#include <string>
#include <iostream>
#include <map>
#include <vector>

namespace Aftr
{
   struct GLSLShaderDescriptor; // added for describing tessellation stages
   class GLSLShaderDataShared
   {
   public:

      GLSLShaderDataShared();
      GLSLShaderDataShared( const GLSLShaderDataShared& shader );
      GLSLShaderDataShared( const std::string& vertexShader, const std::string& fragmentShader,
                            const std::string& geometryShader, GLenum geometryInputPrimitiveType,
                            GLenum geometryOutputPrimitiveType, GLuint geometryMaxOutputVerts,
                            const std::string& computeShader );
      /**
        Construct shader data given a descriptor object.
      */
      GLSLShaderDataShared(const GLSLShaderDescriptor& desc);

      virtual ~GLSLShaderDataShared();

      GLSLShaderDataShared& operator =( const GLSLShaderDataShared& tex );

      /**
      \return true iff the file names of both Textures are identical; false, otherwise.
      This method does NOT compare/account for any difference in the OpenGL
      pointers to the texture memory on the graphics card.
      */
      bool operator ==( const GLSLShaderDataShared& tex ) const;

      /**
      \return true iff the file names of both Textures are NOT identical; false, otherwise.
      This method does NOT compare/account for any difference in the OpenGL
      pointers to the texture memory on the graphics card.
      */
      bool operator !=( const GLSLShaderDataShared& tex ) const;

      /**
      \return true iff the LHS (Left Hand Side) is lexagraphically less 
      than the RHS (Right Hand Side); false, otherwise.
      */
      bool operator <( const GLSLShaderDataShared& tex ) const;

      /**
      \return a string containg "FileName:'(fileName)', GLuint:'(glTextureName)'".
      */
      std::string toString() const;

      /// \returns the file name loaded in this texture or "" if not set.
      std::string getVertexShaderPath() const;
      std::string getFragmentShaderPath() const;
      std::string getGeometryShaderPath() const;
      std::string getTessellationControlShaderPath() const;
      std::string getTessellationEvalShaderPath() const;
      std::string getComputeShaderPath() const;
      void setVertexShaderPath( const std::string& path );
      void setFragmentShaderPath( const std::string& path );
      void setGeometryShaderPath( const std::string& path );
      void setTessellationControlShaderPath(const std::string& path);
      void setTessellationEvalShaderPath(const std::string& path);
      void setComputeShaderPath( const std::string& path );

      /// \returns the name of the OpenGL texture. That is, this is the value to use in glBindTexture.
      GLuint getShaderHandle() const;
      GLuint getFragmentShaderHandle() const { return fragmentShaderHandle; } 
      GLuint getVertexShaderHandle() const { return vertexShaderHandle; }
      GLuint getGeometryShaderHandle() const { return this->geometryShaderHandle; }
      GLuint getTessellationControlShaderHandle() const { return this->tessellationControlShaderHandle; }
      GLuint getTessellationEvalShaderHandle() const { return this->tessellationEvalShaderHandle; }
      GLuint getComputeShaderHandle() const { return this->computeShaderHandle; }

      void setShaderHandle( GLuint shaderHandle );

      /**
      Enables TextureDataOwnsGLHandle to be outputted to an ostream.
      */
      friend std::ostream& operator <<( std::ostream& xout, const GLSLShaderDataShared& shdrData );

      bool instantiate();

      ////std::map< std::string, GLint > attributeLocations;

   protected:

      std::string vertexShaderPath; ///< name of texture read from disk
      std::string fragmentShaderPath;
      std::string geometryShaderPath;
      std::string tessellationControlShaderPath;
      std::string tessellationEvalShaderPath;
      std::string computeShaderPath;

      GLuint vertexShaderHandle;
      GLuint fragmentShaderHandle;
      GLuint geometryShaderHandle;
      GLenum geomShdrInputPrimType; ///< GL_TRIANGLES, GL_POINTS, etc
      GLenum geomShdrOutputPrimType;
      GLuint geomShdrOutputMaxVerts;
      GLuint tessellationControlShaderHandle;
      GLuint tessellationEvalShaderHandle;
      GLuint tessShdrMaxPatchVerts;
      GLuint computeShaderHandle;
      GLuint shaderHandle; ///< Actual GLuint handle to server-side texture on graphics card, this is used w/ glBindTexture

      bool loadVertexShader( const std::string& vertexShaderPath );
      bool loadFragmentShader( const std::string& fragmentShaderPath );
      bool loadGeometryShader( const std::string& geometryShaderpath );
      bool loadTessellationControlShader(const std::string& tessControlShaderPath);
      bool loadTessellationEvalShader(const std::string& tessEvalShaderPath);
      bool loadComputeShader( const std::string& computeShaderPath );
      bool loadShader( GLenum shaderType, const std::string& shaderPath, GLuint& handle );
      bool createProgramShader();
      bool linkProgramShader();

   };
}

