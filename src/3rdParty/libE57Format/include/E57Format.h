/*
 * E57Format.h - public header of E57 API for reading/writing .e57 files.
 *
 * Original work Copyright 2009 - 2010 Kevin Ackley (kackley@gwi.net)
 * Modified work Copyright 2018 - 2020 Andy Maloney <asmaloney@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

//! @file  E57Format.h header file for the E57 API

#include <cfloat>
#include <memory>
#include <vector>

#include "E57Exception.h"

namespace e57
{
   using std::int16_t;
   using std::int32_t;
   using std::int64_t;
   using std::int8_t;
   using std::uint16_t;
   using std::uint32_t;
   using std::uint64_t;
   using std::uint8_t;

   // Shorthand for unicode string
   //! @brief UTF-8 encodeded Unicode string
   using ustring = std::string;

   //! @brief Identifiers for types of E57 elements
   enum NodeType
   {
      E57_STRUCTURE = 1,         //!< StructureNode class
      E57_VECTOR = 2,            //!< VectorNode class
      E57_COMPRESSED_VECTOR = 3, //!< CompressedVectorNode class
      E57_INTEGER = 4,           //!< IntegerNode class
      E57_SCALED_INTEGER = 5,    //!< ScaledIntegerNode class
      E57_FLOAT = 6,             //!< FloatNode class
      E57_STRING = 7,            //!< StringNode class
      E57_BLOB = 8               //!< BlobNode class
   };

   //! @brief The IEEE floating point number precisions supported
   enum FloatPrecision
   {
      E57_SINGLE = 1, //!< 32 bit IEEE floating point number format
      E57_DOUBLE = 2  //!< 64 bit IEEE floating point number format
   };

   //! @brief Identifies the representations of memory elements API can transfer
   //! data to/from
   enum MemoryRepresentation
   {
      E57_INT8 = 1,    //!< 8 bit signed integer
      E57_UINT8 = 2,   //!< 8 bit unsigned integer
      E57_INT16 = 3,   //!< 16 bit signed integer
      E57_UINT16 = 4,  //!< 16 bit unsigned integer
      E57_INT32 = 5,   //!< 32 bit signed integer
      E57_UINT32 = 6,  //!< 32 bit unsigned integer
      E57_INT64 = 7,   //!< 64 bit signed integer
      E57_BOOL = 8,    //!< C++ boolean type
      E57_REAL32 = 9,  //!< C++ float type
      E57_REAL64 = 10, //!< C++ double type
      E57_USTRING = 11 //!< Unicode UTF-8 std::string
   };

   //! @brief Specifies the percentage of checksums which are verified when reading
   //! an ImageFile (0-100%).
   using ReadChecksumPolicy = int;

   //! Do not verify the checksums. (fast)
   constexpr ReadChecksumPolicy CHECKSUM_POLICY_NONE = 0;
   //! Only verify 25% of the checksums. The last block is always verified.
   constexpr ReadChecksumPolicy CHECKSUM_POLICY_SPARSE = 25;
   //! Only verify 50% of the checksums. The last block is always verified.
   constexpr ReadChecksumPolicy CHECKSUM_POLICY_HALF = 50;
   //! Verify all checksums. This is the default. (slow)
   constexpr ReadChecksumPolicy CHECKSUM_POLICY_ALL = 100;

   //! @brief The URI of ASTM E57 v1.0 standard XML namespace
   //! Note that even though this URI does not point to a valid document, the standard (section 8.4.2.3)
   //! says that this is the required namespace.
   constexpr char E57_V1_0_URI[] = "http://www.astm.org/COMMIT/E57/2010-e57-v1.0";

   //! @cond documentNonPublic   The following aren't documented
   // Minimum and maximum values for integers
   constexpr int8_t E57_INT8_MIN = -128;
   constexpr int8_t E57_INT8_MAX = 127;
   constexpr int16_t E57_INT16_MIN = -32768;
   constexpr int16_t E57_INT16_MAX = 32767;
   constexpr int32_t E57_INT32_MIN = -2147483647 - 1;
   constexpr int32_t E57_INT32_MAX = 2147483647;
   constexpr int64_t E57_INT64_MIN = -9223372036854775807LL - 1;
   constexpr int64_t E57_INT64_MAX = 9223372036854775807LL;
   constexpr uint8_t E57_UINT8_MIN = 0U;
   constexpr uint8_t E57_UINT8_MAX = 0xffU; /* 255U */
   constexpr uint16_t E57_UINT16_MIN = 0U;
   constexpr uint16_t E57_UINT16_MAX = 0xffffU; /* 65535U */
   constexpr uint32_t E57_UINT32_MIN = 0U;
   constexpr uint32_t E57_UINT32_MAX = 0xffffffffU; /* 4294967295U */
   constexpr uint64_t E57_UINT64_MIN = 0ULL;
   constexpr uint64_t E57_UINT64_MAX = 0xffffffffffffffffULL; /* 18446744073709551615ULL */

   constexpr float E57_FLOAT_MIN = -FLT_MAX;
   constexpr float E57_FLOAT_MAX = FLT_MAX;
   constexpr double E57_DOUBLE_MIN = -DBL_MAX;
   constexpr double E57_DOUBLE_MAX = DBL_MAX;
//! @endcond

//! @cond documentNonPublic   The following isn't part of the API, and isn't
//! documented.
// Internal implementation files should include E57FormatImpl.h first which
// defines symbol E57_INTERNAL_IMPLEMENTATION_ENABLE. Normal API users should
// not define this symbol. Basically the internal version allows access to the
// pointer to the implementation (impl_)
#ifdef E57_INTERNAL_IMPLEMENTATION_ENABLE
#define E57_OBJECT_IMPLEMENTATION( T )                                                                                 \
public:                                                                                                                \
   std::shared_ptr<T##Impl> impl() const                                                                               \
   {                                                                                                                   \
      return ( impl_ );                                                                                                \
   }                                                                                                                   \
                                                                                                                       \
protected:                                                                                                             \
   std::shared_ptr<T##Impl> impl_;
#else
#define E57_OBJECT_IMPLEMENTATION( T )                                                                                 \
protected:                                                                                                             \
   std::shared_ptr<T##Impl> impl_;
#endif
   //! @endcond

   class BlobNode;
   class BlobNodeImpl;
   class CompressedVectorNode;
   class CompressedVectorNodeImpl;
   class CompressedVectorReader;
   class CompressedVectorReaderImpl;
   class CompressedVectorWriter;
   class CompressedVectorWriterImpl;
   class FloatNode;
   class FloatNodeImpl;
   class ImageFile;
   class ImageFileImpl;
   class IntegerNode;
   class IntegerNodeImpl;
   class Node;
   class NodeImpl;
   class ScaledIntegerNode;
   class ScaledIntegerNodeImpl;
   class SourceDestBuffer;
   class SourceDestBufferImpl;
   class StringNode;
   class StringNodeImpl;
   class StructureNode;
   class StructureNodeImpl;
   class VectorNode;
   class VectorNodeImpl;

   class E57_DLL Node
   {
   public:
      Node() = delete;

      NodeType type() const;
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doDowncast = true );
      bool operator==( Node n2 ) const;
      bool operator!=( Node n2 ) const;

//! \cond documentNonPublic   The following isn't part of the API, and isn't
//! documented.
#ifdef E57_INTERNAL_IMPLEMENTATION_ENABLE
      explicit Node( std::shared_ptr<NodeImpl> ); // internal use only
#endif

   private:
      friend class NodeImpl;

      E57_OBJECT_IMPLEMENTATION( Node ) // Internal implementation details, not
                                        // part of API, must be last in object
      //! \endcond
   };

   class E57_DLL StructureNode
   {
   public:
      StructureNode() = delete;
      StructureNode( ImageFile destImageFile );

      int64_t childCount() const;
      bool isDefined( const ustring &pathName ) const;
      Node get( int64_t index ) const;
      Node get( const ustring &pathName ) const;
      void set( const ustring &pathName, const Node &n );

      // Up/Down cast conversion
      operator Node() const;
      explicit StructureNode( const Node &n );

      // Common generic Node functions
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doUpcast = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      friend class ImageFile;

      StructureNode( std::shared_ptr<StructureNodeImpl> ni );   // internal use only
      StructureNode( std::weak_ptr<ImageFileImpl> fileParent ); // internal use only

      E57_OBJECT_IMPLEMENTATION( StructureNode ) // Internal implementation details, not part of API, must
                                                 // be last in object
      //! \endcond
   };

   class E57_DLL VectorNode
   {
   public:
      VectorNode() = delete;
      explicit VectorNode( ImageFile destImageFile, bool allowHeteroChildren = false );

      bool allowHeteroChildren() const;

      int64_t childCount() const;
      bool isDefined( const ustring &pathName ) const;
      Node get( int64_t index ) const;
      Node get( const ustring &pathName ) const;
      void append( const Node &n );

      // Up/Down cast conversion
      operator Node() const;
      explicit VectorNode( const Node &n );

      // Common generic Node functions
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doUpcast = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      friend class CompressedVectorNode;

      VectorNode( std::shared_ptr<VectorNodeImpl> ni ); // internal use only

      E57_OBJECT_IMPLEMENTATION( VectorNode ) // Internal implementation details, not part of API, must be
                                              // last in object
      //! \endcond
   };

   class E57_DLL SourceDestBuffer
   {
   public:
      SourceDestBuffer() = delete;
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, int8_t *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( int8_t ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, uint8_t *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( uint8_t ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, int16_t *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( int16_t ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, uint16_t *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( uint16_t ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, int32_t *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( int32_t ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, uint32_t *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( uint32_t ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, int64_t *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( int64_t ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, bool *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( bool ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, float *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( float ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, double *b, const size_t capacity,
                        bool doConversion = false, bool doScaling = false, size_t stride = sizeof( double ) );
      SourceDestBuffer( ImageFile destImageFile, const ustring &pathName, std::vector<ustring> *b );

      ustring pathName() const;
      enum MemoryRepresentation memoryRepresentation() const;
      size_t capacity() const;
      bool doConversion() const;
      bool doScaling() const;
      size_t stride() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true ) const;

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      E57_OBJECT_IMPLEMENTATION( SourceDestBuffer ) // Internal implementation details, not part of
                                                    // API, must be last in object
      //! \endcond
   };

   class E57_DLL CompressedVectorReader
   {
   public:
      CompressedVectorReader() = delete;

      unsigned read();
      unsigned read( std::vector<SourceDestBuffer> &dbufs );
      void seek( int64_t recordNumber ); // !!! not implemented yet
      void close();
      bool isOpen();
      CompressedVectorNode compressedVectorNode() const;

      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      friend class CompressedVectorNode;

      CompressedVectorReader( std::shared_ptr<CompressedVectorReaderImpl> ni );

      E57_OBJECT_IMPLEMENTATION( CompressedVectorReader ) // Internal implementation details, not
                                                          // part of API, must be last in object
      //! \endcond
   };

   class E57_DLL CompressedVectorWriter
   {
   public:
      CompressedVectorWriter() = delete;

      void write( const size_t recordCount );
      void write( std::vector<SourceDestBuffer> &sbufs, const size_t recordCount );
      void close();
      bool isOpen();
      CompressedVectorNode compressedVectorNode() const;

      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      friend class CompressedVectorNode;

      CompressedVectorWriter( std::shared_ptr<CompressedVectorWriterImpl> ni );

      E57_OBJECT_IMPLEMENTATION( CompressedVectorWriter ) // Internal implementation details, not
                                                          // part of API, must be last in object
      //! \endcond
   };

   class E57_DLL CompressedVectorNode
   {
   public:
      CompressedVectorNode() = delete;
      explicit CompressedVectorNode( ImageFile destImageFile, const Node &prototype, const VectorNode &codecs );

      int64_t childCount() const;
      Node prototype() const;
      VectorNode codecs() const;

      // Iterators
      CompressedVectorWriter writer( std::vector<SourceDestBuffer> &sbufs );
      CompressedVectorReader reader( const std::vector<SourceDestBuffer> &dbufs );

      // Up/Down cast conversion
      operator Node() const;
      explicit CompressedVectorNode( const Node &n );

      // Common generic Node functions
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doUpcast = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      friend class CompressedVectorReader;
      friend class CompressedVectorWriter;
      friend class E57XmlParser;

      CompressedVectorNode( std::shared_ptr<CompressedVectorNodeImpl> ni ); // internal use only

      E57_OBJECT_IMPLEMENTATION( CompressedVectorNode ) // Internal implementation details, not part
                                                        // of API, must be last in object
      //! \endcond
   };

   class E57_DLL IntegerNode
   {
   public:
      IntegerNode() = delete;
      explicit IntegerNode( ImageFile destImageFile, int64_t value = 0, int64_t minimum = E57_INT64_MIN,
                            int64_t maximum = E57_INT64_MAX );

      int64_t value() const;
      int64_t minimum() const;
      int64_t maximum() const;

      // Up/Down cast conversion
      operator Node() const;
      explicit IntegerNode( const Node &n );

      // Common generic Node functions
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doUpcast = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      IntegerNode( std::shared_ptr<IntegerNodeImpl> ni ); // internal use only

      E57_OBJECT_IMPLEMENTATION( IntegerNode ) // Internal implementation details, not part of API, must be
                                               // last in object
      //! \endcond
   };

   class E57_DLL ScaledIntegerNode
   {
   public:
      ScaledIntegerNode() = delete;
      explicit ScaledIntegerNode( ImageFile destImageFile, int64_t rawValue, int64_t minimum, int64_t maximum,
                                  double scale = 1.0, double offset = 0.0 );
      explicit ScaledIntegerNode( ImageFile destImageFile, int rawValue, int64_t minimum, int64_t maximum,
                                  double scale = 1.0, double offset = 0.0 );
      explicit ScaledIntegerNode( ImageFile destImageFile, int rawValue, int minimum, int maximum, double scale = 1.0,
                                  double offset = 0.0 );
      explicit ScaledIntegerNode( ImageFile destImageFile, double scaledValue, double scaledMinimum,
                                  double scaledMaximum, double scale = 1.0, double offset = 0.0 );

      int64_t rawValue() const;
      double scaledValue() const;
      int64_t minimum() const;
      double scaledMinimum() const;
      int64_t maximum() const;
      double scaledMaximum() const;
      double scale() const;
      double offset() const;

      // Up/Down cast conversion
      operator Node() const;
      explicit ScaledIntegerNode( const Node &n );

      // Common generic Node functions
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doUpcast = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      ScaledIntegerNode( std::shared_ptr<ScaledIntegerNodeImpl> ni ); // internal use only

      E57_OBJECT_IMPLEMENTATION( ScaledIntegerNode ) // Internal implementation details, not part of
                                                     // API, must be last in object
      //! \endcond
   };

   class E57_DLL FloatNode
   {
   public:
      FloatNode() = delete;
      explicit FloatNode( ImageFile destImageFile, double value = 0.0, FloatPrecision precision = E57_DOUBLE,
                          double minimum = E57_DOUBLE_MIN, double maximum = E57_DOUBLE_MAX );

      double value() const;
      FloatPrecision precision() const;
      double minimum() const;
      double maximum() const;

      // Up/Down cast conversion
      operator Node() const;
      explicit FloatNode( const Node &n );

      // Common generic Node functions
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doUpcast = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      FloatNode( std::shared_ptr<FloatNodeImpl> ni ); // internal use only

      E57_OBJECT_IMPLEMENTATION( FloatNode ) // Internal implementation details, not part of API, must be
                                             // last in object
      //! \endcond
   };

   class E57_DLL StringNode
   {
   public:
      StringNode() = delete;
      explicit StringNode( ImageFile destImageFile, const ustring &value = "" );

      ustring value() const;

      // Up/Down cast conversion
      operator Node() const;
      explicit StringNode( const Node &n );

      // Common generic Node functions
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doUpcast = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      friend class StringNodeImpl;
      StringNode( std::shared_ptr<StringNodeImpl> ni ); // internal use only

      E57_OBJECT_IMPLEMENTATION( StringNode ) // Internal implementation details, not part of API, must be
                                              // last in object
      //! \endcond
   };

   class E57_DLL BlobNode
   {
   public:
      BlobNode() = delete;
      explicit BlobNode( ImageFile destImageFile, int64_t byteCount );

      int64_t byteCount() const;
      void read( uint8_t *buf, int64_t start, size_t count );
      void write( uint8_t *buf, int64_t start, size_t count );

      // Up/Down cast conversion
      operator Node() const;
      explicit BlobNode( const Node &n );

      // Common generic Node functions
      bool isRoot() const;
      Node parent() const;
      ustring pathName() const;
      ustring elementName() const;
      ImageFile destImageFile() const;
      bool isAttached() const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true, bool doUpcast = true );

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      friend class E57XmlParser;

      BlobNode( std::shared_ptr<BlobNodeImpl> ni ); // internal use only

      // Internal use only, create blob already in a file
      BlobNode( ImageFile destImageFile, int64_t fileOffset, int64_t length );

      E57_OBJECT_IMPLEMENTATION( BlobNode ) // Internal implementation details, not
                                            // part of API, must be last in object
      //! \endcond
   };

   class E57_DLL ImageFile
   {
   public:
      ImageFile() = delete;
      ImageFile( const ustring &fname, const ustring &mode, ReadChecksumPolicy checksumPolicy = CHECKSUM_POLICY_ALL );
      ImageFile( const char *input, const uint64_t size, ReadChecksumPolicy checksumPolicy = CHECKSUM_POLICY_ALL );

      StructureNode root() const;
      void close();
      void cancel();
      bool isOpen() const;
      bool isWritable() const;
      ustring fileName() const;
      int writerCount() const;
      int readerCount() const;

      // Manipulate registered extensions in the file
      void extensionsAdd( const ustring &prefix, const ustring &uri );
      bool extensionsLookupPrefix( const ustring &prefix, ustring &uri ) const;
      bool extensionsLookupUri( const ustring &uri, ustring &prefix ) const;
      size_t extensionsCount() const;
      ustring extensionsPrefix( const size_t index ) const;
      ustring extensionsUri( const size_t index ) const;

      // Field name functions:
      bool isElementNameExtended( const ustring &elementName ) const;
      void elementNameParse( const ustring &elementName, ustring &prefix, ustring &localPart ) const;

      // Diagnostic functions:
      void dump( int indent = 0, std::ostream &os = std::cout ) const;
      void checkInvariant( bool doRecurse = true ) const;
      bool operator==( ImageFile imf2 ) const;
      bool operator!=( ImageFile imf2 ) const;

      //! \cond documentNonPublic   The following isn't part of the API, and isn't
      //! documented.
   private:
      ImageFile( double ); // Give a second dummy constructor, better error msg
                           // for: ImageFile(0)

      friend class Node;
      friend class StructureNode;
      friend class VectorNode;
      friend class CompressedVectorNode;
      friend class IntegerNode;
      friend class ScaledIntegerNode;
      friend class FloatNode;
      friend class StringNode;
      friend class BlobNode;

      ImageFile( std::shared_ptr<ImageFileImpl> imfi ); // internal use only

      E57_OBJECT_IMPLEMENTATION( ImageFile ) // Internal implementation details, not part of API, must be
                                             // last in object
      //! \endcond
   };
}
