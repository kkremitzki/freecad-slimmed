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

#if defined( _WIN32 )
#if defined( _MSC_VER )
#include <codecvt>
#include <io.h>
#elif defined( __GNUC__ )
#define _LARGEFILE64_SOURCE
#define __LARGE64_FILES
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#else
#error "no supported compiler defined"
#endif
#elif defined( __linux__ )
#define _LARGEFILE64_SOURCE
#define __LARGE64_FILES
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined( __APPLE__ )
#include <sys/types.h>
#include <unistd.h>
#else
#error "no supported OS platform defined"
#endif

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fcntl.h>

#include "CRC.h"

#include "CheckedFile.h"

//#define E57_CHECK_FILE_DEBUG
#ifdef E57_CHECK_FILE_DEBUG
#include <cassert>
#endif

#ifndef O_BINARY
constexpr int O_BINARY = 0;
#endif

using namespace e57;

// These extra definitions are required in C++11.
// In C++17, "static constexpr" is implicitly inline, so these are not required.
constexpr size_t CheckedFile::physicalPageSizeLog2;
constexpr size_t CheckedFile::physicalPageSize;
constexpr uint64_t CheckedFile::physicalPageSizeMask;
constexpr size_t CheckedFile::logicalPageSize;

/// Tool class to read buffer efficiently without
/// multiplying copy operations.
///
/// WARNING: pointer input is handled by user!
class e57::BufferView
{
public:
   /// @param[IN] input: filled buffer owned by caller.
   /// @param[IN] size: size of input
   BufferView( const char *input, uint64_t size ) : streamSize_( size ), stream_( input )
   {
   }

   uint64_t pos() const
   {
      return cursorStream_;
   }

   bool seek( uint64_t offset, int whence )
   {
      if ( whence == SEEK_CUR )
      {
         cursorStream_ += offset;
      }
      else if ( whence == SEEK_SET )
      {
         cursorStream_ = offset;
      }
      else if ( whence == SEEK_END )
      {
         cursorStream_ = streamSize_ - offset;
      }

      if ( cursorStream_ > streamSize_ )
      {
         cursorStream_ = streamSize_;
         return false;
      }

      return true;
   }

   void read( char *buffer, uint64_t count )
   {
      const uint64_t start = cursorStream_;
      for ( uint64_t i = 0; i < count; ++i )
      {
         buffer[i] = stream_[start + i];
         ++cursorStream_;
      }
   }

private:
   const uint64_t streamSize_;
   uint64_t cursorStream_ = 0;
   const char *stream_;
};

CheckedFile::CheckedFile( const ustring &fileName, Mode mode, ReadChecksumPolicy policy ) :
   fileName_( fileName ), checkSumPolicy_( policy )
{
   switch ( mode )
   {
      case ReadOnly:
         fd_ = open64( fileName_, O_RDONLY | O_BINARY, 0 );

         readOnly_ = true;

         physicalLength_ = lseek64( 0LL, SEEK_END );
         lseek64( 0, SEEK_SET );

         logicalLength_ = physicalToLogical( physicalLength_ );
         break;

      case WriteCreate:
         /// File truncated to zero length if already exists
         fd_ = open64( fileName_, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE | S_IREAD );
         break;

      case WriteExisting:
         fd_ = open64( fileName_, O_RDWR | O_BINARY, 0 );

         logicalLength_ = physicalToLogical( length( Physical ) ); //???
         break;
   }
}

CheckedFile::CheckedFile( const char *input, uint64_t size, ReadChecksumPolicy policy ) :
   fileName_( "<StreamBuffer>" ), checkSumPolicy_( policy )
{
   bufView_ = new BufferView( input, size );

   readOnly_ = true;

   physicalLength_ = lseek64( 0LL, SEEK_END );
   lseek64( 0, SEEK_SET );

   logicalLength_ = physicalToLogical( physicalLength_ );
}

int CheckedFile::open64( const ustring &fileName, int flags, int mode )
{
#if defined( _MSC_VER )
   // Handle UTF-8 file names - Windows requires conversion to UTF-16
   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
   std::wstring widePath = converter.from_bytes( fileName );

   int handle;
   int err = _wsopen_s( &handle, widePath.c_str(), flags, _SH_DENYNO, mode );
   if ( handle < 0 )
   {
      throw E57_EXCEPTION2( E57_ERROR_OPEN_FAILED, "err=" + toString( err ) + " fileName=" + fileName +
                                                      " flags=" + toString( flags ) + " mode=" + toString( mode ) );
   }
   return handle;
#elif defined( __GNUC__ )
   int result = ::open( fileName_.c_str(), flags, mode );
   if ( result < 0 )
   {
      throw E57_EXCEPTION2( E57_ERROR_OPEN_FAILED, "result=" + toString( result ) + " fileName=" + fileName +
                                                      " flags=" + toString( flags ) + " mode=" + toString( mode ) );
   }
   return result;
#else
#error "no supported compiler defined"
#endif
}

CheckedFile::~CheckedFile()
{
   try
   {
      close(); ///??? what if already closed?
   }
   catch ( ... )
   {
      //??? report?
   }
}

void CheckedFile::read( char *buf, size_t nRead, size_t /*bufSize*/ )
{
   //??? what if read past logical end?, or physical end?
   //??? need to keep track of logical length?
   //??? check bufSize OK

   const uint64_t end = position( Logical ) + nRead;
   const uint64_t logicalLength = length( Logical );

   if ( end > logicalLength )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "fileName=" + fileName_ + " end=" + toString( end ) +
                                                   " length=" + toString( logicalLength ) );
   }

   uint64_t page = 0;
   size_t pageOffset = 0;

   getCurrentPageAndOffset( page, pageOffset );

   size_t n = std::min( nRead, logicalPageSize - pageOffset );

   /// Allocate temp page buffer
   std::vector<char> page_buffer_v( physicalPageSize );
   char *page_buffer = &page_buffer_v[0];

   const auto checksumMod = static_cast<unsigned int>( std::nearbyint( 100.0 / checkSumPolicy_ ) );

   while ( nRead > 0 )
   {
      readPhysicalPage( page_buffer, page );

      switch ( checkSumPolicy_ )
      {
         case CHECKSUM_POLICY_NONE:
            break;

         case CHECKSUM_POLICY_ALL:
            verifyChecksum( page_buffer, page );
            break;

         default:
            if ( !( page % checksumMod ) || ( nRead < physicalPageSize ) )
            {
               verifyChecksum( page_buffer, page );
            }
            break;
      }

      memcpy( buf, page_buffer + pageOffset, n );

      buf += n;
      nRead -= n;
      pageOffset = 0;
      ++page;

      n = std::min( nRead, logicalPageSize );
   }

   /// When done, leave cursor just past end of last byte read
   seek( end, Logical );
}

void CheckedFile::write( const char *buf, size_t nWrite )
{
#ifdef E57_MAX_VERBOSE
   // cout << "write nWrite=" << nWrite << " position()="<< position() << std::endl;
   // //???
#endif
   if ( readOnly_ )
   {
      throw E57_EXCEPTION2( E57_ERROR_FILE_IS_READ_ONLY, "fileName=" + fileName_ );
   }

   uint64_t end = position( Logical ) + nWrite;

   uint64_t page = 0;
   size_t pageOffset = 0;

   getCurrentPageAndOffset( page, pageOffset );

   size_t n = std::min( nWrite, logicalPageSize - pageOffset );

   /// Allocate temp page buffer
   std::vector<char> page_buffer_v( physicalPageSize );
   char *page_buffer = &page_buffer_v[0];

   while ( nWrite > 0 )
   {
      const uint64_t physicalLength = length( Physical );

      if ( page * physicalPageSize < physicalLength )
      {
         readPhysicalPage( page_buffer, page );
      }

#ifdef E57_MAX_VERBOSE
      // cout << "  page_buffer[0] read: '" << page_buffer[0] << "'" << std::endl;
      // cout << "copy " << n << "bytes to page=" << page << " pageOffset=" <<
      // pageOffset << " buf='"; //??? for (size_t i=0; i < n; i++) cout <<
      // buf[i]; cout << "'" << std::endl;
#endif
      memcpy( page_buffer + pageOffset, buf, n );
      writePhysicalPage( page_buffer, page );
#ifdef E57_MAX_VERBOSE
      // cout << "  page_buffer[0] after write: '" << page_buffer[0] << "'" <<
      // std::endl; //???
#endif
      buf += n;
      nWrite -= n;
      pageOffset = 0;
      page++;
      n = std::min( nWrite, logicalPageSize );
   }

   if ( end > logicalLength_ )
   {
      logicalLength_ = end;
   }

   /// When done, leave cursor just past end of buf
   seek( end, Logical );
}

CheckedFile &CheckedFile::operator<<( const ustring &s )
{
   write( s.c_str(), s.length() ); //??? should be times size of uchar?
   return ( *this );
}

CheckedFile &CheckedFile::operator<<( int64_t i )
{
   std::stringstream ss;
   ss << i;
   return ( *this << ss.str() );
}

CheckedFile &CheckedFile::operator<<( uint64_t i )
{
   std::stringstream ss;
   ss << i;
   return ( *this << ss.str() );
}

CheckedFile &CheckedFile::operator<<( float f )
{
   //??? is 7 digits right number?
   return writeFloatingPoint( f, 7 );
}

CheckedFile &CheckedFile::operator<<( double d )
{
   //??? is 17 digits right number?
   return writeFloatingPoint( d, 17 );
}

template <class FTYPE> CheckedFile &CheckedFile::writeFloatingPoint( FTYPE value, int precision )
{
#ifdef E57_MAX_VERBOSE
   std::cout << "CheckedFile::writeFloatingPoint, value=" << value << " precision=" << precision << std::endl;
#endif

   std::stringstream ss;
   ss << std::scientific << std::setprecision( precision ) << value;

   /// Try to remove trailing zeroes and decimal point
   /// E.g. 1.23456000000000000e+005  ==> 1.23456e+005
   /// E.g. 2.00000000000000000e+005  ==> 2e+005

   ustring s = ss.str();
   const size_t len = s.length();

#ifdef E57_MAX_DEBUG
   ustring old_s = s;
#endif

   /// Split into mantissa and exponent
   /// E.g. 1.23456000000000000e+005  ==> "1.23456000000000000" + "e+005"
   ustring mantissa = s.substr( 0, len - 5 );
   ustring exponent = s.substr( len - 5, 5 );

   /// Double check that we understand the formatting
   if ( exponent[0] == 'e' )
   {
      /// Trim of any trailing zeros in mantissa
      while ( mantissa[mantissa.length() - 1] == '0' )
      {
         mantissa = mantissa.substr( 0, mantissa.length() - 1 );
      }

      /// Make one attempt to trim off trailing decimal point
      if ( mantissa[mantissa.length() - 1] == '.' )
      {
         mantissa = mantissa.substr( 0, mantissa.length() - 1 );
      }

      /// Reassemble whole floating point number
      /// Check if can drop exponent.
      if ( exponent == "e+000" )
      {
         s = mantissa;
      }
      else
      {
         s = mantissa + exponent;
      }
   }

   // Disable these checks because they compare floats using "!=" which is
   // invalid
#if 0 // E57_MAX_DEBUG
   /// Double check same value
   FTYPE old_value = static_cast<FTYPE>(atof(old_s.c_str()));
   FTYPE new_value = static_cast<FTYPE>(atof(s.c_str()));
   if (old_value != new_value)
      throw E57_EXCEPTION2(E57_ERROR_INTERNAL, "fileName=" + fileName_ + " oldValue=" + toString(old_value) + " newValue=" + toString(new_value));
   if (new_value != value)
      throw E57_EXCEPTION2(E57_ERROR_INTERNAL, "fileName=" + fileName_ + " newValue=" + toString(new_value) + " value=" + toString(value));
#endif

   return ( *this << s );
}

void CheckedFile::seek( uint64_t offset, OffsetMode omode )
{
   //??? check for seek beyond logicalLength_
   const auto pos = static_cast<int64_t>( omode == Physical ? offset : logicalToPhysical( offset ) );

#ifdef E57_MAX_VERBOSE
   // cout << "seek offset=" << offset << " omode=" << omode << " pos=" << pos
   // << std::endl; //???
#endif
   lseek64( pos, SEEK_SET );
}

uint64_t CheckedFile::lseek64( int64_t offset, int whence )
{
   if ( ( fd_ < 0 ) && bufView_ )
   {
      const auto uoffset = static_cast<uint64_t>( offset );

      if ( bufView_->seek( uoffset, whence ) )
      {
         return bufView_->pos();
      }

      throw E57_EXCEPTION2( E57_ERROR_LSEEK_FAILED, "fileName=" + fileName_ + " offset=" + toString( offset ) +
                                                       " whence=" + toString( whence ) );
   }

#if defined( _WIN32 )
#if defined( _MSC_VER ) || defined( __MINGW32__ ) //<rs 2010-06-16> mingw _is_ WIN32!
   __int64 result = _lseeki64( fd_, offset, whence );
#elif defined( __GNUC__ ) //<rs 2010-06-16> this most likely will not get
                          // triggered (cygwin != WIN32)?
#ifdef E57_MAX_DEBUG
   if ( sizeof( off_t ) != sizeof( offset ) )
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "sizeof(off_t)=" + toString( sizeof( off_t ) ) );
#endif
   int64_t result = ::lseek( fd_, offset, whence );
#else
#error "no supported compiler defined"
#endif
#elif defined( __linux__ )
   int64_t result = ::lseek64( fd_, offset, whence );
#elif defined( __APPLE__ )
   int64_t result = ::lseek( fd_, offset, whence );
#else
#error "no supported OS platform defined"
#endif

   if ( result < 0 )
   {
      throw E57_EXCEPTION2( E57_ERROR_LSEEK_FAILED, "fileName=" + fileName_ + " offset=" + toString( offset ) +
                                                       " whence=" + toString( whence ) +
                                                       " result=" + toString( result ) );
   }

   return static_cast<uint64_t>( result );
}

uint64_t CheckedFile::position( OffsetMode omode )
{
   /// Get current file cursor position
   const uint64_t pos = lseek64( 0LL, SEEK_CUR );

   if ( omode == Physical )
   {
      return pos;
   }

   return physicalToLogical( pos );
}

uint64_t CheckedFile::length( OffsetMode omode )
{
   if ( omode == Physical )
   {
      if ( readOnly_ )
      {
         return physicalLength_;
      }

      // Current file position
      uint64_t original_pos = lseek64( 0LL, SEEK_CUR );

      // End file position
      uint64_t end_pos = lseek64( 0LL, SEEK_END );

      // Restore original position
      lseek64( original_pos, SEEK_SET );

      return end_pos;
   }

   return logicalLength_;
}

void CheckedFile::extend( uint64_t newLength, OffsetMode omode )
{
#ifdef E57_MAX_VERBOSE
   // cout << "extend newLength=" << newLength << " omode="<< omode << std::endl;
   // //???
#endif
   if ( readOnly_ )
   {
      throw E57_EXCEPTION2( E57_ERROR_FILE_IS_READ_ONLY, "fileName=" + fileName_ );
   }

   uint64_t newLogicalLength = 0;

   if ( omode == Physical )
   {
      newLogicalLength = physicalToLogical( newLength );
   }
   else
   {
      newLogicalLength = newLength;
   }

   uint64_t currentLogicalLength = length( Logical );

   /// Make sure we are trying to make file longer
   if ( newLogicalLength < currentLogicalLength )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "fileName=" + fileName_ + " newLength=" + toString( newLogicalLength ) +
                                                   " currentLength=" + toString( currentLogicalLength ) );
   }

   /// Calc how may zero bytes we have to add to end
   uint64_t nWrite = newLogicalLength - currentLogicalLength;

   /// Seek to current end of file
   seek( currentLogicalLength, Logical );

   uint64_t page = 0;
   size_t pageOffset = 0;

   getCurrentPageAndOffset( page, pageOffset );

   /// Calc first write size (may be partial page)
   /// Watch out for different int sizes here.
   size_t n = 0;

   if ( nWrite < logicalPageSize - pageOffset )
   {
      n = static_cast<size_t>( nWrite );
   }
   else
   {
      n = logicalPageSize - pageOffset;
   }

   /// Allocate temp page buffer
   std::vector<char> page_buffer_v( physicalPageSize );
   char *page_buffer = &page_buffer_v[0];

   while ( nWrite > 0 )
   {
      const uint64_t physicalLength = length( Physical );

      if ( page * physicalPageSize < physicalLength )
      {
         readPhysicalPage( page_buffer, page );
      }

#ifdef E57_MAX_VERBOSE
      // cout << "extend " << n << "bytes on page=" << page << " pageOffset=" <<
      // pageOffset << std::endl;
      // //???
#endif
      memset( page_buffer + pageOffset, 0, n );
      writePhysicalPage( page_buffer, page );

      nWrite -= n;
      pageOffset = 0;
      ++page;

      if ( nWrite < logicalPageSize )
      {
         n = static_cast<size_t>( nWrite );
      }
      else
      {
         n = logicalPageSize;
      }
   }

   //??? what if loop above throws, logicalLength_ may be wrong
   logicalLength_ = newLogicalLength;

   /// When done, leave cursor at end of file
   seek( newLogicalLength, Logical );
}

void CheckedFile::close()
{
   if ( fd_ >= 0 )
   {
#if defined( _MSC_VER )
      int result = ::_close( fd_ );
#elif defined( __GNUC__ )
      int result = ::close( fd_ );
#else
#error "no supported compiler defined"
#endif
      if ( result < 0 )
      {
         throw E57_EXCEPTION2( E57_ERROR_CLOSE_FAILED, "fileName=" + fileName_ + " result=" + toString( result ) );
      }

      fd_ = -1;
   }

   if ( bufView_ )
   {
      delete bufView_;
      bufView_ = nullptr;

      // WARNING: do NOT delete buffer of bufView_ because
      // pointer is handled by user !!
   }
}

void CheckedFile::unlink()
{
   close();

   /// Try to remove the file, don't report a failure
   int result = std::remove( fileName_.c_str() ); //??? unicode support here
   (void)result;                                  // this maybe unused
#ifdef E57_MAX_VERBOSE
   if ( result < 0 )
   {
      std::cout << "std::remove() failed, result=" << result << std::endl;
   }
#endif
}

inline uint32_t swap_uint32( uint32_t val )
{
   val = ( ( val << 8 ) & 0xFF00FF00 ) | ( ( val >> 8 ) & 0xFF00FF );

   return ( val << 16 ) | ( val >> 16 );
}

/// Calc CRC32C of given data
uint32_t CheckedFile::checksum( char *buf, size_t size ) const
{
   static const CRC::Parameters<crcpp_uint32, 32> sCRCParams{ 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true };

   static const CRC::Table<crcpp_uint32, 32> sCRCTable = sCRCParams.MakeTable();

   auto crc = CRC::Calculate<crcpp_uint32, 32>( buf, size, sCRCTable );

   // (Andy) I don't understand why we need to swap bytes here
   crc = swap_uint32( crc );

   return crc;
}

void CheckedFile::verifyChecksum( char *page_buffer, size_t page )
{
   const uint32_t check_sum = checksum( page_buffer, logicalPageSize );
   const uint32_t check_sum_in_page = *reinterpret_cast<uint32_t *>( &page_buffer[logicalPageSize] );

   if ( check_sum_in_page != check_sum )
   {
      const uint64_t physicalLength = length( Physical );

      throw E57_EXCEPTION2( E57_ERROR_BAD_CHECKSUM,
                            "fileName=" + fileName_ + " computedChecksum=" + toString( check_sum ) +
                               " storedChecksum=" + toString( check_sum_in_page ) + " page=" + toString( page ) +
                               " length=" + toString( physicalLength ) );
   }
}

void CheckedFile::getCurrentPageAndOffset( uint64_t &page, size_t &pageOffset, OffsetMode omode )
{
   const uint64_t pos = position( omode );

   if ( omode == Physical )
   {
      page = pos >> physicalPageSizeLog2;
      pageOffset = static_cast<size_t>( pos & physicalPageSizeMask );
   }
   else
   {
      page = pos / logicalPageSize;
      pageOffset = static_cast<size_t>( pos - page * logicalPageSize );
   }
}

void CheckedFile::readPhysicalPage( char *page_buffer, uint64_t page )
{
#ifdef E57_MAX_VERBOSE
   // cout << "readPhysicalPage, page:" << page << std::endl;
#endif

#ifdef E57_CHECK_FILE_DEBUG
   const uint64_t physicalLength = length( Physical );

   assert( page * physicalPageSize < physicalLength );
#endif

   /// Seek to start of physical page
   seek( page * physicalPageSize, Physical );

   if ( ( fd_ < 0 ) && bufView_ )
   {
      bufView_->read( page_buffer, physicalPageSize );
      return;
   }

#if defined( _MSC_VER )
   int result = ::_read( fd_, page_buffer, physicalPageSize );
#elif defined( __GNUC__ )
   ssize_t result = ::read( fd_, page_buffer, physicalPageSize );
#else
#error "no supported compiler defined"
#endif

   if ( result < 0 || static_cast<size_t>( result ) != physicalPageSize )
   {
      throw E57_EXCEPTION2( E57_ERROR_READ_FAILED, "fileName=" + fileName_ + " result=" + toString( result ) );
   }
}

void CheckedFile::writePhysicalPage( char *page_buffer, uint64_t page )
{
#ifdef E57_MAX_VERBOSE
   // cout << "writePhysicalPage, page:" << page << std::endl;
#endif

   /// Append checksum
   uint32_t check_sum = checksum( page_buffer, logicalPageSize );
   *reinterpret_cast<uint32_t *>( &page_buffer[logicalPageSize] ) = check_sum; //??? little endian dependency

   /// Seek to start of physical page
   seek( page * physicalPageSize, Physical );

#if defined( _MSC_VER )
   int result = ::_write( fd_, page_buffer, physicalPageSize );
#elif defined( __GNUC__ )
   ssize_t result = ::write( fd_, page_buffer, physicalPageSize );
#else
#error "no supported compiler defined"
#endif

   if ( result < 0 )
   {
      throw E57_EXCEPTION2( E57_ERROR_WRITE_FAILED, "fileName=" + fileName_ + " result=" + toString( result ) );
   }
}
