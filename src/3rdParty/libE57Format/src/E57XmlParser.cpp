/*
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

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

#include <xercesc/util/BinInputStream.hpp>
#include <xercesc/util/TransService.hpp>

#include "BlobNodeImpl.h"
#include "CheckedFile.h"
#include "CompressedVectorNodeImpl.h"
#include "E57XmlParser.h"
#include "FloatNodeImpl.h"
#include "ImageFileImpl.h"
#include "IntegerNodeImpl.h"
#include "ScaledIntegerNodeImpl.h"
#include "StringNodeImpl.h"
#include "VectorNodeImpl.h"

#if __GNUC__ >= 11
#include <limits>
#endif

using namespace e57;
using namespace XERCES_CPP_NAMESPACE;

// define convenient constants for the attribute names
static const XMLCh att_minimum[] = {
   chLatin_m, chLatin_i, chLatin_n, chLatin_i, chLatin_m, chLatin_u, chLatin_m, chNull
};
static const XMLCh att_maximum[] = {
   chLatin_m, chLatin_a, chLatin_x, chLatin_i, chLatin_m, chLatin_u, chLatin_m, chNull
};
static const XMLCh att_scale[] = { chLatin_s, chLatin_c, chLatin_a, chLatin_l, chLatin_e, chNull };
static const XMLCh att_offset[] = { chLatin_o, chLatin_f, chLatin_f, chLatin_s, chLatin_e, chLatin_t, chNull };
static const XMLCh att_precision[] = { chLatin_p, chLatin_r, chLatin_e, chLatin_c, chLatin_i,
                                       chLatin_s, chLatin_i, chLatin_o, chLatin_n, chNull };
static const XMLCh att_allowHeterogeneousChildren[] = {
   chLatin_a, chLatin_l, chLatin_l, chLatin_o, chLatin_w, chLatin_H, chLatin_e, chLatin_t, chLatin_e,
   chLatin_r, chLatin_o, chLatin_g, chLatin_e, chLatin_n, chLatin_e, chLatin_o, chLatin_u, chLatin_s,
   chLatin_C, chLatin_h, chLatin_i, chLatin_l, chLatin_d, chLatin_r, chLatin_e, chLatin_n, chNull
};
static const XMLCh att_fileOffset[] = { chLatin_f, chLatin_i, chLatin_l, chLatin_e, chLatin_O, chLatin_f,
                                        chLatin_f, chLatin_s, chLatin_e, chLatin_t, chNull };

static const XMLCh att_type[] = { chLatin_t, chLatin_y, chLatin_p, chLatin_e, chNull };
static const XMLCh att_length[] = { chLatin_l, chLatin_e, chLatin_n, chLatin_g, chLatin_t, chLatin_h, chNull };
static const XMLCh att_recordCount[] = { chLatin_r, chLatin_e, chLatin_c, chLatin_o, chLatin_r, chLatin_d,
                                         chLatin_C, chLatin_o, chLatin_u, chLatin_n, chLatin_t, chNull };

inline int64_t convertStrToLL( const std::string &inStr )
{
#if defined( _MSC_VER )
   return _atoi64( inStr.c_str() );
#elif defined( __GNUC__ )
   return strtoll( inStr.c_str(), nullptr, 10 );
#else
#error "Need to define string to long long conversion for this compiler"
#endif
}

//=============================================================================
// E57FileInputStream

class E57FileInputStream : public BinInputStream
{
public:
   E57FileInputStream( CheckedFile *cf, uint64_t logicalStart, uint64_t logicalLength );
   ~E57FileInputStream() override = default;

   E57FileInputStream( const E57FileInputStream & ) = delete;
   E57FileInputStream &operator=( const E57FileInputStream & ) = delete;

   XMLFilePos curPos() const override
   {
      return ( logicalPosition_ );
   }
   XMLSize_t readBytes( XMLByte *const toFill, const XMLSize_t maxToRead ) override;
   const XMLCh *getContentType() const override
   {
      return nullptr;
   }

private:
   //??? lifetime of cf_ must be longer than this object!
   CheckedFile *cf_;
   uint64_t logicalStart_;
   uint64_t logicalLength_;
   uint64_t logicalPosition_;
};

E57FileInputStream::E57FileInputStream( CheckedFile *cf, uint64_t logicalStart, uint64_t logicalLength ) :
   cf_( cf ), logicalStart_( logicalStart ), logicalLength_( logicalLength ), logicalPosition_( logicalStart )
{
}

XMLSize_t E57FileInputStream::readBytes( XMLByte *const toFill, const XMLSize_t maxToRead )
{
   if ( logicalPosition_ > logicalStart_ + logicalLength_ )
   {
      return ( 0 );
   }

   int64_t available = logicalStart_ + logicalLength_ - logicalPosition_;
   if ( available <= 0 )
   {
      return ( 0 );
   }

   /// size_t and XMLSize_t should be compatible, should get compiler warning
   /// here if not
   size_t maxToRead_size = maxToRead;

   /// Be careful if size_t is smaller than int64_t
   size_t available_size;
   if ( sizeof( size_t ) >= sizeof( int64_t ) )
   {
      /// size_t is at least as big as int64_t
      available_size = static_cast<size_t>( available );
   }
   else
   {
      /// size_t is smaller than int64_t, Calc max that size_t can hold
      const int64_t size_max = std::numeric_limits<size_t>::max();

      /// read smaller of size_max, available
      ///??? redo
      if ( size_max < available )
      {
         available_size = static_cast<size_t>( size_max );
      }
      else
      {
         available_size = static_cast<size_t>( available );
      }
   }

   size_t readCount = std::min( maxToRead_size, available_size );

   cf_->seek( logicalPosition_ );
   cf_->read( reinterpret_cast<char *>( toFill ), readCount ); //??? cast ok?
   logicalPosition_ += readCount;
   return ( readCount );
}

//=============================================================================
// E57XmlFileInputSource

E57XmlFileInputSource::E57XmlFileInputSource( CheckedFile *cf, uint64_t logicalStart, uint64_t logicalLength ) :
   InputSource( "E57File",
                XMLPlatformUtils::fgMemoryManager ), //??? what if want to use our own
                                                     // memory
                                                     // manager?, what bufid is good?
   cf_( cf ), logicalStart_( logicalStart ), logicalLength_( logicalLength )
{
}

BinInputStream *E57XmlFileInputSource::makeStream() const
{
   return new E57FileInputStream( cf_, logicalStart_, logicalLength_ );
}

//=============================================================================
// E57XmlParser::ParseInfo

E57XmlParser::ParseInfo::ParseInfo() :
   nodeType( static_cast<NodeType>( 0 ) ), minimum( 0 ), maximum( 0 ), scale( 0 ), offset( 0 ),
   precision( static_cast<FloatPrecision>( 0 ) ), floatMinimum( 0 ), floatMaximum( 0 ), fileOffset( 0 ), length( 0 ),
   allowHeterogeneousChildren( false ), recordCount( 0 )
{
}

void E57XmlParser::ParseInfo::dump( int indent, std::ostream &os ) const
{
   os << space( indent ) << "nodeType:       " << nodeType << std::endl;
   os << space( indent ) << "minimum:        " << minimum << std::endl;
   os << space( indent ) << "maximum:        " << maximum << std::endl;
   os << space( indent ) << "scale:          " << scale << std::endl;
   os << space( indent ) << "offset:         " << offset << std::endl;
   os << space( indent ) << "precision:      " << precision << std::endl;
   os << space( indent ) << "floatMinimum:   " << floatMinimum << std::endl;
   os << space( indent ) << "floatMaximum:   " << floatMaximum << std::endl;
   os << space( indent ) << "fileOffset:     " << fileOffset << std::endl;
   os << space( indent ) << "length:         " << length << std::endl;
   os << space( indent ) << "allowHeterogeneousChildren: " << allowHeterogeneousChildren << std::endl;
   os << space( indent ) << "recordCount:    " << recordCount << std::endl;
   if ( container_ni )
   {
      os << space( indent ) << "container_ni:   <defined>" << std::endl;
   }
   else
   {
      os << space( indent ) << "container_ni:   <null>" << std::endl;
   }
   os << space( indent ) << "childText:      \"" << childText << "\"" << std::endl;
}

//=============================================================================
// E57XmlParser

E57XmlParser::E57XmlParser( ImageFileImplSharedPtr imf ) : imf_( imf ), xmlReader( nullptr )
{
}

E57XmlParser::~E57XmlParser()
{
   delete xmlReader;

   xmlReader = nullptr;

   XMLPlatformUtils::Terminate();
}

void E57XmlParser::init()
{
   // Initialize the XML4C2 system
   try
   {
      XMLPlatformUtils::Initialize();
   }
   catch ( const XMLException &ex )
   {
      /// Turn parser exception into E57Exception
      throw E57_EXCEPTION2( E57_ERROR_XML_PARSER_INIT,
                            "parserMessage=" + ustring( XMLString::transcode( ex.getMessage() ) ) );
   }

   xmlReader = XMLReaderFactory::createXMLReader(); //??? auto_ptr?

   if ( xmlReader == nullptr )
   {
      throw E57_EXCEPTION2( E57_ERROR_XML_PARSER_INIT, "could not create the xml reader" );
   }

   //??? check these are right
   xmlReader->setFeature( XMLUni::fgSAX2CoreValidation, true );
   xmlReader->setFeature( XMLUni::fgXercesDynamic, true );
   xmlReader->setFeature( XMLUni::fgSAX2CoreNameSpaces, true );
   xmlReader->setFeature( XMLUni::fgXercesSchema, true );
   xmlReader->setFeature( XMLUni::fgXercesSchemaFullChecking, true );
   xmlReader->setFeature( XMLUni::fgSAX2CoreNameSpacePrefixes, true );

   xmlReader->setContentHandler( this );
   xmlReader->setErrorHandler( this );
}

void E57XmlParser::parse( InputSource &inputSource )
{
   xmlReader->parse( inputSource );
}

void E57XmlParser::startElement( const XMLCh *const uri, const XMLCh *const localName, const XMLCh *const qName,
                                 const Attributes &attributes )
{
#ifdef E57_MAX_VERBOSE
   std::cout << "startElement" << std::endl;
   std::cout << space( 2 ) << "URI:       " << toUString( uri ) << std::endl;
   std::cout << space( 2 ) << "localName: " << toUString( localName ) << std::endl;
   std::cout << space( 2 ) << "qName:     " << toUString( qName ) << std::endl;

   for ( size_t i = 0; i < attributes.getLength(); i++ )
   {
      std::cout << space( 2 ) << "Attribute[" << i << "]" << std::endl;
      std::cout << space( 4 ) << "URI:       " << toUString( attributes.getURI( i ) ) << std::endl;
      std::cout << space( 4 ) << "localName: " << toUString( attributes.getLocalName( i ) ) << std::endl;
      std::cout << space( 4 ) << "qName:     " << toUString( attributes.getQName( i ) ) << std::endl;
      std::cout << space( 4 ) << "value:     " << toUString( attributes.getValue( i ) ) << std::endl;
   }
#endif
   /// Get Type attribute
   ustring node_type = lookupAttribute( attributes, att_type );

   //??? check to make sure not in primitive type (can only nest inside compound
   // types).

   ParseInfo pi;

   if ( node_type == "Integer" )
   {
#ifdef E57_MAX_VERBOSE
      std::cout << "got a Integer" << std::endl;
#endif
      //??? check validity of numeric strings
      pi.nodeType = E57_INTEGER;

      if ( isAttributeDefined( attributes, att_minimum ) )
      {
         ustring minimum_str = lookupAttribute( attributes, att_minimum );

         pi.minimum = convertStrToLL( minimum_str );
      }
      else
      {
         /// Not defined defined in XML, so defaults to E57_INT64_MIN
         pi.minimum = E57_INT64_MIN;
      }

      if ( isAttributeDefined( attributes, att_maximum ) )
      {
         ustring maximum_str = lookupAttribute( attributes, att_maximum );

         pi.maximum = convertStrToLL( maximum_str );
      }
      else
      {
         /// Not defined defined in XML, so defaults to E57_INT64_MAX
         pi.maximum = E57_INT64_MAX;
      }

      /// Push info so far onto stack
      stack_.push( pi );
   }
   else if ( node_type == "ScaledInteger" )
   {
#ifdef E57_MAX_VERBOSE
      std::cout << "got a ScaledInteger" << std::endl;
#endif
      pi.nodeType = E57_SCALED_INTEGER;

      //??? check validity of numeric strings
      if ( isAttributeDefined( attributes, att_minimum ) )
      {
         ustring minimum_str = lookupAttribute( attributes, att_minimum );

         pi.minimum = convertStrToLL( minimum_str );
      }
      else
      {
         /// Not defined defined in XML, so defaults to E57_INT64_MIN
         pi.minimum = E57_INT64_MIN;
      }

      if ( isAttributeDefined( attributes, att_maximum ) )
      {
         ustring maximum_str = lookupAttribute( attributes, att_maximum );

         pi.maximum = convertStrToLL( maximum_str );
      }
      else
      {
         /// Not defined defined in XML, so defaults to E57_INT64_MAX
         pi.maximum = E57_INT64_MAX;
      }

      if ( isAttributeDefined( attributes, att_scale ) )
      {
         ustring scale_str = lookupAttribute( attributes, att_scale );
         pi.scale = atof( scale_str.c_str() ); //??? use exact rounding library
      }
      else
      {
         /// Not defined defined in XML, so defaults to 1.0
         pi.scale = 1.0;
      }

      if ( isAttributeDefined( attributes, att_offset ) )
      {
         ustring offset_str = lookupAttribute( attributes, att_offset );
         pi.offset = atof( offset_str.c_str() ); //??? use exact rounding library
      }
      else
      {
         /// Not defined defined in XML, so defaults to 0.0
         pi.offset = 0.0;
      }

      /// Push info so far onto stack
      stack_.push( pi );
   }
   else if ( node_type == "Float" )
   {
#ifdef E57_MAX_VERBOSE
      std::cout << "got a Float" << std::endl;
#endif
      pi.nodeType = E57_FLOAT;

      if ( isAttributeDefined( attributes, att_precision ) )
      {
         ustring precision_str = lookupAttribute( attributes, att_precision );
         if ( precision_str == "single" )
         {
            pi.precision = E57_SINGLE;
         }
         else if ( precision_str == "double" )
         {
            pi.precision = E57_DOUBLE;
         }
         else
         {
            throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                                  "precisionString=" + precision_str + " fileName=" + imf_->fileName() +
                                     " uri=" + toUString( uri ) + " localName=" + toUString( localName ) +
                                     " qName=" + toUString( qName ) );
         }
      }
      else
      {
         /// Not defined defined in XML, so defaults to double
         pi.precision = E57_DOUBLE;
      }

      if ( isAttributeDefined( attributes, att_minimum ) )
      {
         ustring minimum_str = lookupAttribute( attributes, att_minimum );
         pi.floatMinimum = atof( minimum_str.c_str() ); //??? use exact rounding library
      }
      else
      {
         /// Not defined defined in XML, so defaults to E57_FLOAT_MIN or
         /// E57_DOUBLE_MIN
         if ( pi.precision == E57_SINGLE )
         {
            pi.floatMinimum = E57_FLOAT_MIN;
         }
         else
         {
            pi.floatMinimum = E57_DOUBLE_MIN;
         }
      }

      if ( isAttributeDefined( attributes, att_maximum ) )
      {
         ustring maximum_str = lookupAttribute( attributes, att_maximum );
         pi.floatMaximum = atof( maximum_str.c_str() ); //??? use exact rounding library
      }
      else
      {
         /// Not defined defined in XML, so defaults to FLOAT_MAX or DOUBLE_MAX
         if ( pi.precision == E57_SINGLE )
         {
            pi.floatMaximum = E57_FLOAT_MAX;
         }
         else
         {
            pi.floatMaximum = E57_DOUBLE_MAX;
         }
      }

      /// Push info so far onto stack
      stack_.push( pi );
   }
   else if ( node_type == "String" )
   {
#ifdef E57_MAX_VERBOSE
      std::cout << "got a String" << std::endl;
#endif
      pi.nodeType = E57_STRING;

      /// Push info so far onto stack
      stack_.push( pi );
   }
   else if ( node_type == "Blob" )
   {
#ifdef E57_MAX_VERBOSE
      std::cout << "got a Blob" << std::endl;
#endif
      pi.nodeType = E57_BLOB;

      //??? check validity of numeric strings

      /// fileOffset is required to be defined
      ustring fileOffset_str = lookupAttribute( attributes, att_fileOffset );

      pi.fileOffset = convertStrToLL( fileOffset_str );

      /// length is required to be defined
      ustring length_str = lookupAttribute( attributes, att_length );

      pi.length = convertStrToLL( length_str );

      /// Push info so far onto stack
      stack_.push( pi );
   }
   else if ( node_type == "Structure" )
   {
#ifdef E57_MAX_VERBOSE
      std::cout << "got a Structure" << std::endl;
#endif
      pi.nodeType = E57_STRUCTURE;

      /// Read name space decls, if e57Root element
      if ( toUString( localName ) == "e57Root" )
      {
         /// Search attributes for namespace declarations (only allowed in
         /// E57Root structure)
         bool gotDefault = false;
         for ( size_t i = 0; i < attributes.getLength(); i++ )
         {
            /// Check if declaring the default namespace
            if ( toUString( attributes.getQName( i ) ) == "xmlns" )
            {
#ifdef E57_VERBOSE
               std::cout << "declared default namespace, URI=" << toUString( attributes.getValue( i ) ) << std::endl;
#endif
               imf_->extensionsAdd( "", toUString( attributes.getValue( i ) ) );
               gotDefault = true;
            }

            /// Check if declaring a namespace
            if ( toUString( attributes.getURI( i ) ) == "http://www.w3.org/2000/xmlns/" )
            {
#ifdef E57_VERBOSE
               std::cout << "declared extension, prefix=" << toUString( attributes.getLocalName( i ) )
                    << " URI=" << toUString( attributes.getValue( i ) ) << std::endl;
#endif
               imf_->extensionsAdd( toUString( attributes.getLocalName( i ) ), toUString( attributes.getValue( i ) ) );
            }
         }

         /// If didn't declare a default namespace, have error
         if ( !gotDefault )
         {
            throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                                  "fileName=" + imf_->fileName() + " uri=" + toUString( uri ) +
                                     " localName=" + toUString( localName ) + " qName=" + toUString( qName ) );
         }
      }

      /// Create container now, so can hold children
      std::shared_ptr<StructureNodeImpl> s_ni( new StructureNodeImpl( imf_ ) );
      pi.container_ni = s_ni;

      /// After have Structure, check again if E57Root, if so mark attached so
      /// all children will be attached when added
      if ( toUString( localName ) == "e57Root" )
      {
         s_ni->setAttachedRecursive();
      }

      /// Push info so far onto stack
      stack_.push( pi );
   }
   else if ( node_type == "Vector" )
   {
#ifdef E57_MAX_VERBOSE
      std::cout << "got a Vector" << std::endl;
#endif
      pi.nodeType = E57_VECTOR;

      if ( isAttributeDefined( attributes, att_allowHeterogeneousChildren ) )
      {
         ustring allowHetero_str = lookupAttribute( attributes, att_allowHeterogeneousChildren );

         int64_t i64 = convertStrToLL( allowHetero_str );

         if ( i64 == 0 )
         {
            pi.allowHeterogeneousChildren = false;
         }
         else if ( i64 == 1 )
         {
            pi.allowHeterogeneousChildren = true;
         }
         else
         {
            throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                                  "allowHeterogeneousChildren=" + toString( i64 ) + "fileName=" + imf_->fileName() +
                                     " uri=" + toUString( uri ) + " localName=" + toUString( localName ) +
                                     " qName=" + toUString( qName ) );
         }
      }
      else
      {
         /// Not defined defined in XML, so defaults to false
         pi.allowHeterogeneousChildren = false;
      }

      /// Create container now, so can hold children
      std::shared_ptr<VectorNodeImpl> v_ni( new VectorNodeImpl( imf_, pi.allowHeterogeneousChildren ) );
      pi.container_ni = v_ni;

      /// Push info so far onto stack
      stack_.push( pi );
   }
   else if ( node_type == "CompressedVector" )
   {
#ifdef E57_MAX_VERBOSE
      std::cout << "got a CompressedVector" << std::endl;
#endif
      pi.nodeType = E57_COMPRESSED_VECTOR;

      /// fileOffset is required to be defined
      ustring fileOffset_str = lookupAttribute( attributes, att_fileOffset );

      pi.fileOffset = convertStrToLL( fileOffset_str );

      /// recordCount is required to be defined
      ustring recordCount_str = lookupAttribute( attributes, att_recordCount );

      pi.recordCount = convertStrToLL( recordCount_str );

      /// Create container now, so can hold children
      std::shared_ptr<CompressedVectorNodeImpl> cv_ni( new CompressedVectorNodeImpl( imf_ ) );
      cv_ni->setRecordCount( pi.recordCount );
      cv_ni->setBinarySectionLogicalStart(
         imf_->file_->physicalToLogical( pi.fileOffset ) ); //??? what if file_ is NULL?
      pi.container_ni = cv_ni;

      /// Push info so far onto stack
      stack_.push( pi );
   }
   else
   {
      throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                            "nodeType=" + node_type + " fileName=" + imf_->fileName() + " uri=" + toUString( uri ) +
                               " localName=" + toUString( localName ) + " qName=" + toUString( qName ) );
   }
#ifdef E57_MAX_VERBOSE
   pi.dump( 4 );
#endif
}

void E57XmlParser::endElement( const XMLCh *const uri, const XMLCh *const localName, const XMLCh *const qName )
{
#ifdef E57_MAX_VERBOSE
   std::cout << "endElement" << std::endl;
#endif

   /// Pop the node that just ended
   ParseInfo pi = stack_.top(); //??? really want to make a copy here?
   stack_.pop();
#ifdef E57_MAX_VERBOSE
   pi.dump( 4 );
#endif

   /// We should now have all the info we need to create the node
   NodeImplSharedPtr current_ni;

   switch ( pi.nodeType )
   {
      case E57_STRUCTURE:
      case E57_VECTOR:
         current_ni = pi.container_ni;
         break;
      case E57_COMPRESSED_VECTOR:
      {
         /// Verify that both prototype and codecs child elements were defined
         /// ???
         current_ni = pi.container_ni;
      }
      break;
      case E57_INTEGER:
      {
         /// Convert child text (if any) to value, else default to 0.0
         int64_t intValue;
         if ( pi.childText.length() > 0 )
         {
            intValue = convertStrToLL( pi.childText );
         }
         else
         {
            intValue = 0;
         }
         std::shared_ptr<IntegerNodeImpl> i_ni( new IntegerNodeImpl( imf_, intValue, pi.minimum, pi.maximum ) );
         current_ni = i_ni;
      }
      break;
      case E57_SCALED_INTEGER:
      {
         /// Convert child text (if any) to value, else default to 0.0
         int64_t intValue;
         if ( pi.childText.length() > 0 )
         {
            intValue = convertStrToLL( pi.childText );
         }
         else
         {
            intValue = 0;
         }
         std::shared_ptr<ScaledIntegerNodeImpl> si_ni(
            new ScaledIntegerNodeImpl( imf_, intValue, pi.minimum, pi.maximum, pi.scale, pi.offset ) );
         current_ni = si_ni;
      }
      break;
      case E57_FLOAT:
      {
         /// Convert child text (if any) to value, else default to 0.0
         double floatValue;
         if ( pi.childText.length() > 0 )
         {
            floatValue = atof( pi.childText.c_str() );
         }
         else
         {
            floatValue = 0.0;
         }
         std::shared_ptr<FloatNodeImpl> f_ni(
            new FloatNodeImpl( imf_, floatValue, pi.precision, pi.floatMinimum, pi.floatMaximum ) );
         current_ni = f_ni;
      }
      break;
      case E57_STRING:
      {
         std::shared_ptr<StringNodeImpl> s_ni( new StringNodeImpl( imf_, pi.childText ) );
         current_ni = s_ni;
      }
      break;
      case E57_BLOB:
      {
         std::shared_ptr<BlobNodeImpl> b_ni( new BlobNodeImpl( imf_, pi.fileOffset, pi.length ) );
         current_ni = b_ni;
      }
      break;
      default:
         throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "nodeType=" + toString( pi.nodeType ) +
                                                      " fileName=" + imf_->fileName() + " uri=" + toUString( uri ) +
                                                      " localName=" + toUString( localName ) +
                                                      " qName=" + toUString( qName ) );
   }
#ifdef E57_MAX_VERBOSE
   current_ni->dump( 4 );
#endif

   /// If first node in file ended, we are all done
   if ( stack_.empty() )
   {
      /// Top level should be Structure
      if ( current_ni->type() != E57_STRUCTURE )
      {
         throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                               "currentType=" + toString( current_ni->type() ) + " fileName=" + imf_->fileName() +
                                  " uri=" + toUString( uri ) + " localName=" + toUString( localName ) +
                                  " qName=" + toUString( qName ) );
      }
      imf_->root_ = std::static_pointer_cast<StructureNodeImpl>( current_ni );
      return;
   }

   /// Get next level up node (when entered function), which should be a
   /// container.
   NodeImplSharedPtr parent_ni = stack_.top().container_ni;

   if ( !parent_ni )
   {
      throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT, "fileName=" + imf_->fileName() + " uri=" + toUString( uri ) +
                                                         " localName=" + toUString( localName ) +
                                                         " qName=" + toUString( qName ) );
   }

   /// Add current node into parent at top of stack
   switch ( parent_ni->type() )
   {
      case E57_STRUCTURE:
      {
         std::shared_ptr<StructureNodeImpl> struct_ni = std::static_pointer_cast<StructureNodeImpl>( parent_ni );

         /// Add named child to structure
         struct_ni->set( toUString( qName ), current_ni );
      }
      break;
      case E57_VECTOR:
      {
         std::shared_ptr<VectorNodeImpl> vector_ni = std::static_pointer_cast<VectorNodeImpl>( parent_ni );

         /// Add unnamed child to vector
         vector_ni->append( current_ni );
      }
      break;
      case E57_COMPRESSED_VECTOR:
      {
         std::shared_ptr<CompressedVectorNodeImpl> cv_ni =
            std::static_pointer_cast<CompressedVectorNodeImpl>( parent_ni );
         ustring uQName = toUString( qName );

         /// n can be either prototype or codecs
         if ( uQName == "prototype" )
         {
            cv_ni->setPrototype( current_ni );
         }
         else if ( uQName == "codecs" )
         {
            if ( current_ni->type() != E57_VECTOR )
            {
               throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                                     "currentType=" + toString( current_ni->type() ) + " fileName=" + imf_->fileName() +
                                        " uri=" + toUString( uri ) + " localName=" + toUString( localName ) +
                                        " qName=" + toUString( qName ) );
            }
            std::shared_ptr<VectorNodeImpl> vi = std::static_pointer_cast<VectorNodeImpl>( current_ni );

            /// Check VectorNode is hetero
            if ( !vi->allowHeteroChildren() )
            {
               throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                                     "currentType=" + toString( current_ni->type() ) + " fileName=" + imf_->fileName() +
                                        " uri=" + toUString( uri ) + " localName=" + toUString( localName ) +
                                        " qName=" + toUString( qName ) );
            }

            cv_ni->setCodecs( vi );
         }
         else
         {
            /// Found unknown XML child element of CompressedVector, not
            /// prototype or codecs
            throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                                  +"fileName=" + imf_->fileName() + " uri=" + toUString( uri ) +
                                     " localName=" + toUString( localName ) + " qName=" + toUString( qName ) );
         }
      }
      break;
      default:
         /// Have bad XML nesting, parent should have been a container.
         throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT,
                               "parentType=" + toString( parent_ni->type() ) + " fileName=" + imf_->fileName() +
                                  " uri=" + toUString( uri ) + " localName=" + toUString( localName ) +
                                  " qName=" + toUString( qName ) );
   }
}

void E57XmlParser::characters( const XMLCh *const chars, const XMLSize_t length )
{
   (void)length;
//??? use length to make ustring
#ifdef E57_MAX_VERBOSE
   std::cout << "characters, chars=\"" << toUString( chars ) << "\" length=" << length << std::endl;
#endif
   /// Get active element
   ParseInfo &pi = stack_.top();

   /// Check if child text is allowed for current E57 element type
   switch ( pi.nodeType )
   {
      case E57_STRUCTURE:
      case E57_VECTOR:
      case E57_COMPRESSED_VECTOR:
      case E57_BLOB:
      {
         /// If characters aren't whitespace, have an error, else ignore
         ustring s = toUString( chars );
         if ( s.find_first_not_of( " \t\n\r" ) != std::string::npos )
         {
            throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT, "chars=" + toUString( chars ) );
         }
      }
      break;
      default:
         /// Append to any previous characters
         pi.childText += toUString( chars );
   }
}

void E57XmlParser::error( const SAXParseException &ex )
{
   throw E57_EXCEPTION2( E57_ERROR_XML_PARSER, "systemId=" + ustring( XMLString::transcode( ex.getSystemId() ) ) +
                                                  " xmlLine=" + toString( ex.getLineNumber() ) +
                                                  " xmlColumn=" + toString( ex.getColumnNumber() ) + " parserMessage=" +
                                                  ustring( XMLString::transcode( ex.getMessage() ) ) );
}

void E57XmlParser::fatalError( const SAXParseException &ex )
{
   throw E57_EXCEPTION2( E57_ERROR_XML_PARSER, "systemId=" + ustring( XMLString::transcode( ex.getSystemId() ) ) +
                                                  " xmlLine=" + toString( ex.getLineNumber() ) +
                                                  " xmlColumn=" + toString( ex.getColumnNumber() ) + " parserMessage=" +
                                                  ustring( XMLString::transcode( ex.getMessage() ) ) );
}

void E57XmlParser::warning( const SAXParseException &ex )
{
   /// Don't take any action on warning from parser, just report
   std::cerr << "**** XML parser warning: " << ustring( XMLString::transcode( ex.getMessage() ) ) << std::endl;
   std::cerr << "  Debug info:" << std::endl;
   std::cerr << "    systemId=" << XMLString::transcode( ex.getSystemId() ) << std::endl;
   std::cerr << ",   xmlLine=" << ex.getLineNumber() << std::endl;
   std::cerr << ",   xmlColumn=" << ex.getColumnNumber() << std::endl;
}

ustring E57XmlParser::toUString( const XMLCh *const xml_str )
{
   ustring u_str;
   if ( xml_str && *xml_str )
   {
      TranscodeToStr UTF8Transcoder( xml_str, "UTF-8" );
      u_str = ustring( reinterpret_cast<const char *>( UTF8Transcoder.str() ) );
   }
   return ( u_str );
}

ustring E57XmlParser::lookupAttribute( const Attributes &attributes, const XMLCh *attribute_name )
{
   XMLSize_t attr_index;
   if ( !attributes.getIndex( attribute_name, attr_index ) )
   {
      throw E57_EXCEPTION2( E57_ERROR_BAD_XML_FORMAT, "attributeName=" + toUString( attribute_name ) );
   }
   return ( toUString( attributes.getValue( attr_index ) ) );
}

bool E57XmlParser::isAttributeDefined( const Attributes &attributes, const XMLCh *attribute_name )
{
   XMLSize_t attr_index;
   return ( attributes.getIndex( attribute_name, attr_index ) );
}
