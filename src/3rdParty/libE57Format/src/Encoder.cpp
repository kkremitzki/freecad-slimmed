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

#include <algorithm>
#include <cstring>

#include "CompressedVectorNodeImpl.h"
#include "Encoder.h"
#include "FloatNodeImpl.h"
#include "ImageFileImpl.h"
#include "IntegerNodeImpl.h"
#include "Packet.h"
#include "ScaledIntegerNodeImpl.h"
#include "SourceDestBufferImpl.h"

using namespace e57;

std::shared_ptr<Encoder> Encoder::EncoderFactory( unsigned bytestreamNumber,
                                                  std::shared_ptr<CompressedVectorNodeImpl> cVector,
                                                  std::vector<SourceDestBuffer> &sbufs, ustring & /*codecPath*/ )
{
   //??? For now, only handle one input
   if ( sbufs.size() != 1 )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "sbufsSize=" + toString( sbufs.size() ) );
   }

   SourceDestBuffer sbuf = sbufs.at( 0 );

   /// Get node we are going to encode from the CompressedVector's prototype
   NodeImplSharedPtr prototype = cVector->getPrototype();
   ustring path = sbuf.pathName();
   NodeImplSharedPtr encodeNode = prototype->get( path );

#ifdef E57_MAX_VERBOSE
   std::cout << "Node to encode:" << std::endl; //???
   encodeNode->dump( 2 );
#endif
   switch ( encodeNode->type() )
   {
      case E57_INTEGER:
      {
         std::shared_ptr<IntegerNodeImpl> ini =
            std::static_pointer_cast<IntegerNodeImpl>( encodeNode ); // downcast to correct type

         /// Get pointer to parent ImageFileImpl, to call bitsNeeded()
         ImageFileImplSharedPtr imf( encodeNode->destImageFile_ ); //??? should be function for this,
                                                                   // imf->parentFile()
                                                                   //--> ImageFile?

         unsigned bitsPerRecord = imf->bitsNeeded( ini->minimum(), ini->maximum() );

         //!!! need to pick smarter channel buffer sizes, here and elsewhere
         /// Construct Integer encoder with appropriate register size, based on
         /// number of bits stored.
         if ( bitsPerRecord == 0 )
         {
            std::shared_ptr<Encoder> encoder( new ConstantIntegerEncoder( bytestreamNumber, sbuf, ini->minimum() ) );

            return encoder;
         }

         if ( bitsPerRecord <= 8 )
         {
            std::shared_ptr<Encoder> encoder( new BitpackIntegerEncoder<uint8_t>(
               false, bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/, ini->minimum(), ini->maximum(), 1.0, 0.0 ) );
            return encoder;
         }

         if ( bitsPerRecord <= 16 )
         {
            std::shared_ptr<Encoder> encoder( new BitpackIntegerEncoder<uint16_t>(
               false, bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/, ini->minimum(), ini->maximum(), 1.0, 0.0 ) );
            return encoder;
         }

         if ( bitsPerRecord <= 32 )
         {
            std::shared_ptr<Encoder> encoder( new BitpackIntegerEncoder<uint32_t>(
               false, bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/, ini->minimum(), ini->maximum(), 1.0, 0.0 ) );
            return encoder;
         }

         std::shared_ptr<Encoder> encoder( new BitpackIntegerEncoder<uint64_t>(
            false, bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/, ini->minimum(), ini->maximum(), 1.0, 0.0 ) );
         return encoder;
      }

      case E57_SCALED_INTEGER:
      {
         std::shared_ptr<ScaledIntegerNodeImpl> sini =
            std::static_pointer_cast<ScaledIntegerNodeImpl>( encodeNode ); // downcast to correct type

         /// Get pointer to parent ImageFileImpl, to call bitsNeeded()
         ImageFileImplSharedPtr imf( encodeNode->destImageFile_ ); //??? should be function for this,
                                                                   // imf->parentFile()
                                                                   //--> ImageFile?

         unsigned bitsPerRecord = imf->bitsNeeded( sini->minimum(), sini->maximum() );

         //!!! need to pick smarter channel buffer sizes, here and elsewhere
         /// Construct ScaledInteger encoder with appropriate register size,
         /// based on number of bits stored.
         if ( bitsPerRecord == 0 )
         {
            std::shared_ptr<Encoder> encoder( new ConstantIntegerEncoder( bytestreamNumber, sbuf, sini->minimum() ) );

            return encoder;
         }

         if ( bitsPerRecord <= 8 )
         {
            std::shared_ptr<Encoder> encoder(
               new BitpackIntegerEncoder<uint8_t>( true, bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/,
                                                   sini->minimum(), sini->maximum(), sini->scale(), sini->offset() ) );
            return encoder;
         }

         if ( bitsPerRecord <= 16 )
         {
            std::shared_ptr<Encoder> encoder(
               new BitpackIntegerEncoder<uint16_t>( true, bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/,
                                                    sini->minimum(), sini->maximum(), sini->scale(), sini->offset() ) );
            return encoder;
         }

         if ( bitsPerRecord <= 32 )
         {
            std::shared_ptr<Encoder> encoder(
               new BitpackIntegerEncoder<uint32_t>( true, bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/,
                                                    sini->minimum(), sini->maximum(), sini->scale(), sini->offset() ) );
            return encoder;
         }

         std::shared_ptr<Encoder> encoder(
            new BitpackIntegerEncoder<uint64_t>( true, bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/, sini->minimum(),
                                                 sini->maximum(), sini->scale(), sini->offset() ) );
         return encoder;
      }

      case E57_FLOAT:
      {
         std::shared_ptr<FloatNodeImpl> fni =
            std::static_pointer_cast<FloatNodeImpl>( encodeNode ); // downcast to correct type

         //!!! need to pick smarter channel buffer sizes, here and elsewhere
         std::shared_ptr<Encoder> encoder(
            new BitpackFloatEncoder( bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/, fni->precision() ) );
         return encoder;
      }

      case E57_STRING:
      {
         std::shared_ptr<Encoder> encoder(
            new BitpackStringEncoder( bytestreamNumber, sbuf, DATA_PACKET_MAX /*!!!*/ ) );

         return encoder;
      }

      default:
      {
         throw E57_EXCEPTION2( E57_ERROR_BAD_PROTOTYPE, "nodeType=" + toString( encodeNode->type() ) );
      }
   }
}

Encoder::Encoder( unsigned bytestreamNumber ) : bytestreamNumber_( bytestreamNumber )
{
}

#ifdef E57_DEBUG
void Encoder::dump( int indent, std::ostream &os ) const
{
   os << space( indent ) << "bytestreamNumber:       " << bytestreamNumber_ << std::endl;
}
#endif

///================

BitpackEncoder::BitpackEncoder( unsigned bytestreamNumber, SourceDestBuffer &sbuf, unsigned outputMaxSize,
                                unsigned alignmentSize ) :
   Encoder( bytestreamNumber ),
   sourceBuffer_( sbuf.impl() ), outBuffer_( outputMaxSize ), outBufferFirst_( 0 ), outBufferEnd_( 0 ),
   outBufferAlignmentSize_( alignmentSize ), currentRecordIndex_( 0 )
{
}

unsigned BitpackEncoder::sourceBufferNextIndex()
{
   return ( sourceBuffer_->nextIndex() );
}

uint64_t BitpackEncoder::currentRecordIndex()
{
   return ( currentRecordIndex_ );
}

size_t BitpackEncoder::outputAvailable() const
{
   return outBufferEnd_ - outBufferFirst_;
}

void BitpackEncoder::outputRead( char *dest, const size_t byteCount )
{
#ifdef E57_MAX_VERBOSE
   std::cout << "BitpackEncoder::outputRead() called, dest=" << dest << " byteCount=" << byteCount << std::endl; //???
#endif

   /// Check we have enough bytes in queue
   if ( byteCount > outputAvailable() )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "byteCount=" + toString( byteCount ) +
                                                   " outputAvailable=" + toString( outputAvailable() ) );
   }

   /// Copy output bytes to caller
   memcpy( dest, &outBuffer_[outBufferFirst_], byteCount );

#ifdef E57_MAX_VERBOSE
   {
      unsigned i;
      for ( i = 0; i < byteCount && i < 20; i++ )
      {
         std::cout << "  outBuffer[" << outBufferFirst_ + i
                   << "]=" << static_cast<unsigned>( static_cast<unsigned char>( outBuffer_[outBufferFirst_ + i] ) )
                   << std::endl; //???
      }

      if ( i < byteCount )
      {
         std::cout << "  " << byteCount - 1 << " bytes unprinted..." << std::endl;
      }
   }
#endif

   /// Advance head pointer.
   outBufferFirst_ += byteCount;

   /// Don't slide remaining data down now, wait until do some more processing
   /// (that's when data needs to be aligned).
}

void BitpackEncoder::outputClear()
{
   outBufferFirst_ = 0;
   outBufferEnd_ = 0;
}

void BitpackEncoder::sourceBufferSetNew( std::vector<SourceDestBuffer> &sbufs )
{
   /// Verify that this encoder only has single input buffer
   if ( sbufs.size() != 1 )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "sbufsSize=" + toString( sbufs.size() ) );
   }

   sourceBuffer_ = sbufs.at( 0 ).impl();
}

size_t BitpackEncoder::outputGetMaxSize()
{
   /// Its size that matters here, not capacity
   return ( outBuffer_.size() );
}

void BitpackEncoder::outputSetMaxSize( unsigned byteCount )
{
   /// Ignore if trying to shrink buffer (queue might get messed up).
   if ( byteCount > outBuffer_.size() )
   {
      outBuffer_.resize( byteCount );
   }
}

void BitpackEncoder::outBufferShiftDown()
{
   /// Move data down closer to beginning of outBuffer_.
   /// But keep outBufferEnd_ as a multiple of outBufferAlignmentSize_.
   /// This ensures that writes into buffer can occur on natural boundaries.
   /// Otherwise some CPUs will fault.

   if ( outBufferFirst_ == outBufferEnd_ )
   {
      /// Buffer is empty, reset indices to 0
      outBufferFirst_ = 0;
      outBufferEnd_ = 0;
      return;
   }

   /// Round newEnd up to nearest multiple of outBufferAlignmentSize_.
   size_t newEnd = outputAvailable();
   size_t remainder = newEnd % outBufferAlignmentSize_;
   if ( remainder > 0 )
   {
      newEnd += outBufferAlignmentSize_ - remainder;
   }
   size_t newFirst = outBufferFirst_ - ( outBufferEnd_ - newEnd );
   size_t byteCount = outBufferEnd_ - outBufferFirst_;

   /// Double check round up worked
   if ( newEnd % outBufferAlignmentSize_ )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "newEnd=" + toString( newEnd ) +
                                                   " outBufferAlignmentSize=" + toString( outBufferAlignmentSize_ ) );
   }

   /// Be paranoid before memory copy
   if ( newFirst + byteCount > outBuffer_.size() )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "newFirst=" + toString( newFirst ) +
                                                   " byteCount=" + toString( byteCount ) +
                                                   " outBufferSize=" + toString( outBuffer_.size() ) );
   }

   /// Move available data down closer to beginning of outBuffer_.  Overlapping
   /// regions ok with memmove().
   memmove( &outBuffer_[newFirst], &outBuffer_[outBufferFirst_], byteCount );

   /// Update indexes
   outBufferFirst_ = newFirst;
   outBufferEnd_ = newEnd;
}

#ifdef E57_DEBUG
void BitpackEncoder::dump( int indent, std::ostream &os ) const
{
   Encoder::dump( indent, os );
   os << space( indent ) << "sourceBuffer:" << std::endl;
   sourceBuffer_->dump( indent + 4, os );
   os << space( indent ) << "outBuffer.size:           " << outBuffer_.size() << std::endl;
   os << space( indent ) << "outBufferFirst:           " << outBufferFirst_ << std::endl;
   os << space( indent ) << "outBufferEnd:             " << outBufferEnd_ << std::endl;
   os << space( indent ) << "outBufferAlignmentSize:   " << outBufferAlignmentSize_ << std::endl;
   os << space( indent ) << "currentRecordIndex:       " << currentRecordIndex_ << std::endl;
   os << space( indent ) << "outBuffer:" << std::endl;
   unsigned i;
   for ( i = 0; i < outBuffer_.size() && i < 20; i++ )
   {
      os << space( indent + 4 ) << "outBuffer[" << i
         << "]: " << static_cast<unsigned>( static_cast<unsigned char>( outBuffer_.at( i ) ) ) << std::endl;
   }
   if ( i < outBuffer_.size() )
   {
      os << space( indent + 4 ) << outBuffer_.size() - i << " more unprinted..." << std::endl;
   }
}
#endif

//================

BitpackFloatEncoder::BitpackFloatEncoder( unsigned bytestreamNumber, SourceDestBuffer &sbuf, unsigned outputMaxSize,
                                          FloatPrecision precision ) :
   BitpackEncoder( bytestreamNumber, sbuf, outputMaxSize,
                   ( precision == E57_SINGLE ) ? sizeof( float ) : sizeof( double ) ),
   precision_( precision )
{
}

uint64_t BitpackFloatEncoder::processRecords( size_t recordCount )
{
#ifdef E57_MAX_VERBOSE
   std::cout << "  BitpackFloatEncoder::processRecords() called, recordCount=" << recordCount << std::endl; //???
#endif

   /// Before we add any more, try to shift current contents of outBuffer_ down
   /// to beginning of buffer. This leaves outBufferEnd_ at a natural boundary.
   outBufferShiftDown();

   size_t typeSize = ( precision_ == E57_SINGLE ) ? sizeof( float ) : sizeof( double );

#ifdef E57_DEBUG
   /// Verify that outBufferEnd_ is multiple of typeSize (so transfers of floats
   /// are aligned naturally in memory).
   if ( outBufferEnd_ % typeSize )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL,
                            "outBufferEnd=" + toString( outBufferEnd_ ) + " typeSize=" + toString( typeSize ) );
   }
#endif

   /// Figure out how many records will fit in output.
   size_t maxOutputRecords = ( outBuffer_.size() - outBufferEnd_ ) / typeSize;

   /// Can't process more records than will safely fit in output stream
   if ( recordCount > maxOutputRecords )
   {
      recordCount = maxOutputRecords;
   }

   if ( precision_ == E57_SINGLE )
   {
      /// Form the starting address for next available location in outBuffer
      auto outp = reinterpret_cast<float *>( &outBuffer_[outBufferEnd_] );

      /// Copy floats from sourceBuffer_ to outBuffer_
      for ( unsigned i = 0; i < recordCount; i++ )
      {
         outp[i] = sourceBuffer_->getNextFloat();
#ifdef E57_MAX_VERBOSE
         std::cout << "encoding float: " << outp[i] << std::endl;
#endif
      }
   }
   else
   { /// E57_DOUBLE precision
      /// Form the starting address for next available location in outBuffer
      auto outp = reinterpret_cast<double *>( &outBuffer_[outBufferEnd_] );

      /// Copy doubles from sourceBuffer_ to outBuffer_
      for ( unsigned i = 0; i < recordCount; i++ )
      {
         outp[i] = sourceBuffer_->getNextDouble();
#ifdef E57_MAX_VERBOSE
         std::cout << "encoding double: " << outp[i] << std::endl;
#endif
      }
   }

   /// Update end of outBuffer
   outBufferEnd_ += recordCount * typeSize;

   /// Update counts of records processed
   currentRecordIndex_ += recordCount;

   return ( currentRecordIndex_ );
}

bool BitpackFloatEncoder::registerFlushToOutput()
{
   /// Since have no registers in encoder, return success
   return ( true );
}

float BitpackFloatEncoder::bitsPerRecord()
{
   return ( ( precision_ == E57_SINGLE ) ? 32.0F : 64.0F );
}

#ifdef E57_DEBUG
void BitpackFloatEncoder::dump( int indent, std::ostream &os ) const
{
   BitpackEncoder::dump( indent, os );
   if ( precision_ == E57_SINGLE )
   {
      os << space( indent ) << "precision:                E57_SINGLE" << std::endl;
   }
   else
   {
      os << space( indent ) << "precision:                E57_DOUBLE" << std::endl;
   }
}
#endif

//================

BitpackStringEncoder::BitpackStringEncoder( unsigned bytestreamNumber, SourceDestBuffer &sbuf,
                                            unsigned outputMaxSize ) :
   BitpackEncoder( bytestreamNumber, sbuf, outputMaxSize, 1 ),
   totalBytesProcessed_( 0 ), isStringActive_( false ), prefixComplete_( false ), currentCharPosition_( 0 )
{
}

uint64_t BitpackStringEncoder::processRecords( size_t recordCount )
{
#ifdef E57_MAX_VERBOSE
   std::cout << "  BitpackStringEncoder::processRecords() called, recordCount=" << recordCount << std::endl; //???
#endif

   /// Before we add any more, try to shift current contents of outBuffer_ down
   /// to beginning of buffer.
   outBufferShiftDown();

   /// Figure out how many bytes outBuffer can accept.
   size_t bytesFree = outBuffer_.size() - outBufferEnd_;

   /// Form the starting address for next available location in outBuffer
   char *outp = &outBuffer_[outBufferEnd_];
   unsigned recordsProcessed = 0;

   /// Don't start loop unless have at least 8 bytes for worst case string
   /// length prefix
   while ( recordsProcessed < recordCount && bytesFree >= 8 )
   { //??? should be able to proceed if only 1 byte free
      if ( isStringActive_ && !prefixComplete_ )
      {
         /// Calc the length prefix, either 1 byte or 8 bytes
         size_t len = currentString_.length();
         if ( len <= 127 )
         {
#ifdef E57_MAX_VERBOSE
            std::cout << "encoding short string: (len=" << len
                      << ") "
                         ""
                      << currentString_
                      << ""
                         ""
                      << std::endl;
#endif
            /// We can use the short length prefix: b0=0, b7-b1=len
            auto lengthPrefix = static_cast<uint8_t>( len << 1 );
            *outp++ = lengthPrefix;
            bytesFree--;
         }
         else
         {
#ifdef E57_DEBUG
            /// Double check have space
            if ( bytesFree < 8 )
            {
               throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "bytesFree=" + toString( bytesFree ) );
            }
#endif
#ifdef E57_MAX_VERBOSE
            std::cout << "encoding long string: (len=" << len
                      << ") "
                         ""
                      << currentString_
                      << ""
                         ""
                      << std::endl;
#endif
            /// We use the long length prefix: b0=1, b63-b1=len, and store in
            /// little endian order Shift the length and set the least
            /// significant bit, b0=1.
            uint64_t lengthPrefix = ( static_cast<uint64_t>( len ) << 1 ) | 1LL;
            *outp++ = static_cast<uint8_t>( lengthPrefix );
            *outp++ = static_cast<uint8_t>( lengthPrefix >> ( 1 * 8 ) );
            *outp++ = static_cast<uint8_t>( lengthPrefix >> ( 2 * 8 ) );
            *outp++ = static_cast<uint8_t>( lengthPrefix >> ( 3 * 8 ) );
            *outp++ = static_cast<uint8_t>( lengthPrefix >> ( 4 * 8 ) );
            *outp++ = static_cast<uint8_t>( lengthPrefix >> ( 5 * 8 ) );
            *outp++ = static_cast<uint8_t>( lengthPrefix >> ( 6 * 8 ) );
            *outp++ = static_cast<uint8_t>( lengthPrefix >> ( 7 * 8 ) );
            bytesFree -= 8;
         }
         prefixComplete_ = true;
         currentCharPosition_ = 0;
      }
      if ( isStringActive_ )
      {
         /// Copy as much string as will fit in outBuffer
         size_t bytesToProcess = std::min( currentString_.length() - currentCharPosition_, bytesFree );

         for ( size_t i = 0; i < bytesToProcess; i++ )
         {
            *outp++ = currentString_[currentCharPosition_ + i];
         }

         currentCharPosition_ += bytesToProcess;
         totalBytesProcessed_ += bytesToProcess;
         bytesFree -= bytesToProcess;

         /// Check if finished string
         if ( currentCharPosition_ == currentString_.length() )
         {
            isStringActive_ = false;
            recordsProcessed++;
         }
      }
      if ( !isStringActive_ && recordsProcessed < recordCount )
      {
         /// Get next string from sourceBuffer
         currentString_ = sourceBuffer_->getNextString();
         isStringActive_ = true;
         prefixComplete_ = false;
         currentCharPosition_ = 0;
#ifdef E57_MAX_VERBOSE
         std::cout << "getting next string, length=" << currentString_.length() << std::endl;
#endif
      }
   }

   /// Update end of outBuffer
   outBufferEnd_ = outBuffer_.size() - bytesFree;

   /// Update counts of records processed
   currentRecordIndex_ += recordsProcessed;

   return ( currentRecordIndex_ );
}

bool BitpackStringEncoder::registerFlushToOutput()
{
   /// Since have no registers in encoder, return success
   return ( true );
}

float BitpackStringEncoder::bitsPerRecord()
{
   /// Return average number of bits in strings + 8 bits for prefix
   if ( currentRecordIndex_ > 0 )
   {
      return ( 8.0f * totalBytesProcessed_ ) / currentRecordIndex_ + 8;
   }

   /// We haven't completed a record yet, so guess 100 bytes per record
   return 100 * 8.0f;
}

#ifdef E57_DEBUG
void BitpackStringEncoder::dump( int indent, std::ostream &os ) const
{
   BitpackEncoder::dump( indent, os );
   os << space( indent ) << "totalBytesProcessed:    " << totalBytesProcessed_ << std::endl;
   os << space( indent ) << "isStringActive:         " << isStringActive_ << std::endl;
   os << space( indent ) << "prefixComplete:         " << prefixComplete_ << std::endl;
   os << space( indent ) << "currentString:          " << currentString_ << std::endl;
   os << space( indent ) << "currentCharPosition:    " << currentCharPosition_ << std::endl;
}
#endif

//================================================================

template <typename RegisterT>
BitpackIntegerEncoder<RegisterT>::BitpackIntegerEncoder( bool isScaledInteger, unsigned bytestreamNumber,
                                                         SourceDestBuffer &sbuf, unsigned outputMaxSize,
                                                         int64_t minimum, int64_t maximum, double scale,
                                                         double offset ) :
   BitpackEncoder( bytestreamNumber, sbuf, outputMaxSize, sizeof( RegisterT ) )
{
   /// Get pointer to parent ImageFileImpl
   ImageFileImplSharedPtr imf( sbuf.impl()->destImageFile() ); //??? should be function for this,
                                                               // imf->parentFile()  --> ImageFile?

   isScaledInteger_ = isScaledInteger;
   minimum_ = minimum;
   maximum_ = maximum;
   scale_ = scale;
   offset_ = offset;
   bitsPerRecord_ = imf->bitsNeeded( minimum_, maximum_ );
   sourceBitMask_ = ( bitsPerRecord_ == 64 ) ? ~0 : ( 1ULL << bitsPerRecord_ ) - 1;
   registerBitsUsed_ = 0;
   register_ = 0;
}

template <typename RegisterT> uint64_t BitpackIntegerEncoder<RegisterT>::processRecords( size_t recordCount )
{
   //??? what are state guarantees if get an exception during transfer?
#ifdef E57_MAX_VERBOSE
   std::cout << "BitpackIntegerEncoder::processRecords() called, sizeof(RegisterT)=" << sizeof( RegisterT )
             << " recordCount=" << recordCount << std::endl;
   dump( 4 );
#endif
#ifdef E57_MAX_DEBUG
   /// Double check that register will hold at least one input records worth of
   /// bits
   if ( 8 * sizeof( RegisterT ) < bitsPerRecord_ )
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "bitsPerRecord=" + toString( bitsPerRecord_ ) );
#endif

   /// Before we add any more, try to shift current contents of outBuffer_ down
   /// to beginning of buffer. This leaves outBufferEnd_ at a natural boundary.
   outBufferShiftDown();

#ifdef E57_DEBUG
   /// Verify that outBufferEnd_ is multiple of sizeof(RegisterT) (so transfers
   /// of RegisterT are aligned naturally in memory).
   if ( outBufferEnd_ % sizeof( RegisterT ) )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "outBufferEnd=" + toString( outBufferEnd_ ) );
   }
   size_t transferMax = ( outBuffer_.size() - outBufferEnd_ ) / sizeof( RegisterT );
#endif

   /// Precalculate exact maximum number of records that will fit in output
   /// before overflow.
   size_t outputWordCapacity = ( outBuffer_.size() - outBufferEnd_ ) / sizeof( RegisterT );
   size_t maxOutputRecords =
      ( outputWordCapacity * 8 * sizeof( RegisterT ) + 8 * sizeof( RegisterT ) - registerBitsUsed_ - 1 ) /
      bitsPerRecord_;

   /// Number of transfers is the smaller of what was requested and what will
   /// fit.
   recordCount = std::min( recordCount, maxOutputRecords );
#ifdef E57_MAX_VERBOSE
   std::cout << "  outputWordCapacity=" << outputWordCapacity << " maxOutputRecords=" << maxOutputRecords
             << " recordCount=" << recordCount << std::endl;
#endif

   /// Form the starting address for next available location in outBuffer
   auto outp = reinterpret_cast<RegisterT *>( &outBuffer_[outBufferEnd_] );
   unsigned outTransferred = 0;

   /// Copy bits from sourceBuffer_ to outBuffer_
   for ( unsigned i = 0; i < recordCount; i++ )
   {
      int64_t rawValue;

      /// The parameter isScaledInteger_ determines which version of
      /// getNextInt64 gets called
      if ( isScaledInteger_ )
      {
         rawValue = sourceBuffer_->getNextInt64( scale_, offset_ );
      }
      else
      {
         rawValue = sourceBuffer_->getNextInt64();
      }

      /// Enforce min/max specification on value
      if ( rawValue < minimum_ || maximum_ < rawValue )
      {
         throw E57_EXCEPTION2( E57_ERROR_VALUE_OUT_OF_BOUNDS, "rawValue=" + toString( rawValue ) +
                                                                 " minimum=" + toString( minimum_ ) +
                                                                 " maximum=" + toString( maximum_ ) );
      }

      auto uValue = static_cast<uint64_t>( rawValue - minimum_ );

#ifdef E57_MAX_VERBOSE
      std::cout << "encoding integer rawValue=" << binaryString( rawValue ) << " = " << hexString( rawValue )
                << std::endl;
      std::cout << "                 uValue  =" << binaryString( uValue ) << " = " << hexString( uValue ) << std::endl;
#endif
#ifdef E57_DEBUG
      /// Double check that no bits outside of the mask are set
      if ( uValue & ~static_cast<uint64_t>( sourceBitMask_ ) )
      {
         throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "uValue=" + toString( uValue ) );
      }
#endif
      /// Mask off upper bits (just in case)
      uValue &= static_cast<uint64_t>( sourceBitMask_ );

      /// See if uValue bits will fit in register
      unsigned newRegisterBitsUsed = registerBitsUsed_ + bitsPerRecord_;
#ifdef E57_MAX_VERBOSE
      std::cout << "  registerBitsUsed=" << registerBitsUsed_ << "  newRegisterBitsUsed=" << newRegisterBitsUsed
                << std::endl;
#endif
      if ( newRegisterBitsUsed > 8 * sizeof( RegisterT ) )
      {
         /// Have more than one registers worth, fill register, transfer, then
         /// fill some more
         register_ |= static_cast<RegisterT>( uValue ) << registerBitsUsed_;
#ifdef E57_DEBUG
         /// Before transfer, double check address within bounds
         if ( outTransferred >= transferMax )
         {
            throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "outTransferred=" + toString( outTransferred ) + " transferMax" +
                                                         toString( transferMax ) );
         }
#endif
         outp[outTransferred] = register_;

         outTransferred++;

         register_ = static_cast<RegisterT>( uValue ) >> ( 8 * sizeof( RegisterT ) - registerBitsUsed_ );
         registerBitsUsed_ = newRegisterBitsUsed - 8 * sizeof( RegisterT );
      }
      else if ( newRegisterBitsUsed == 8 * sizeof( RegisterT ) )
      {
         /// Input will exactly fill register, insert value, then transfer
         register_ |= static_cast<RegisterT>( uValue ) << registerBitsUsed_;
#ifdef E57_DEBUG
         /// Before transfer, double check address within bounds
         if ( outTransferred >= transferMax )
         {
            throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "outTransferred=" + toString( outTransferred ) + " transferMax" +
                                                         toString( transferMax ) );
         }
#endif
         outp[outTransferred] = register_;

         outTransferred++;

         register_ = 0;
         registerBitsUsed_ = 0;
      }
      else
      {
         /// There is extra room in register, insert value, but don't do
         /// transfer yet
         register_ |= static_cast<RegisterT>( uValue ) << registerBitsUsed_;
         registerBitsUsed_ = newRegisterBitsUsed;
      }
#ifdef E57_MAX_VERBOSE
      std::cout << "  After " << outTransferred << " transfers and " << i + 1 << " records, encoder:" << std::endl;
      dump( 4 );
#endif
   }

   /// Update tail of output buffer
   outBufferEnd_ += outTransferred * sizeof( RegisterT );
#ifdef E57_DEBUG
   /// Double check end is ok
   if ( outBufferEnd_ > outBuffer_.size() )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "outBufferEnd=" + toString( outBufferEnd_ ) +
                                                   " outBuffersize=" + toString( outBuffer_.size() ) );
   }
#endif

   /// Update counts of records processed
   currentRecordIndex_ += recordCount;

   return ( currentRecordIndex_ );
}

template <typename RegisterT> bool BitpackIntegerEncoder<RegisterT>::registerFlushToOutput()
{
#ifdef E57_MAX_VERBOSE
   std::cout << "BitpackIntegerEncoder::registerFlushToOutput() called, "
                "sizeof(RegisterT)="
             << sizeof( RegisterT ) << std::endl;
   dump( 4 );
#endif
   /// If have any used bits in register, transfer to output, padded in MSBits
   /// with zeros to RegisterT boundary
   if ( registerBitsUsed_ > 0 )
   {
      if ( outBufferEnd_ < outBuffer_.size() - sizeof( RegisterT ) )
      {
         auto outp = reinterpret_cast<RegisterT *>( &outBuffer_[outBufferEnd_] );
         *outp = register_;
         register_ = 0;
         registerBitsUsed_ = 0;
         outBufferEnd_ += sizeof( RegisterT );
         return true; // flush succeeded  ??? is this used? correctly?
      }

      return false; // flush didn't complete (not enough room).
   }

   return true;
}

template <typename RegisterT> float BitpackIntegerEncoder<RegisterT>::bitsPerRecord()
{
   return ( static_cast<float>( bitsPerRecord_ ) );
}

#ifdef E57_DEBUG
template <typename RegisterT> void BitpackIntegerEncoder<RegisterT>::dump( int indent, std::ostream &os ) const
{
   BitpackEncoder::dump( indent, os );
   os << space( indent ) << "isScaledInteger:  " << isScaledInteger_ << std::endl;
   os << space( indent ) << "minimum:          " << minimum_ << std::endl;
   os << space( indent ) << "maximum:          " << maximum_ << std::endl;
   os << space( indent ) << "scale:            " << scale_ << std::endl;
   os << space( indent ) << "offset:           " << offset_ << std::endl;
   os << space( indent ) << "bitsPerRecord:    " << bitsPerRecord_ << std::endl;
   os << space( indent ) << "sourceBitMask:    " << binaryString( sourceBitMask_ ) << " " << hexString( sourceBitMask_ )
      << std::endl;
   os << space( indent ) << "register:         " << binaryString( register_ ) << " " << hexString( register_ )
      << std::endl;
   os << space( indent ) << "registerBitsUsed: " << registerBitsUsed_ << std::endl;
}
#endif

//================================================================

ConstantIntegerEncoder::ConstantIntegerEncoder( unsigned bytestreamNumber, SourceDestBuffer &sbuf, int64_t minimum ) :
   Encoder( bytestreamNumber ), sourceBuffer_( sbuf.impl() ), currentRecordIndex_( 0 ), minimum_( minimum )
{
}

uint64_t ConstantIntegerEncoder::processRecords( size_t recordCount )
{
#ifdef E57_MAX_VERBOSE
   std::cout << "ConstantIntegerEncoder::processRecords() called, recordCount=" << recordCount << std::endl;
   dump( 4 );
#endif

   /// Check that all source values are == minimum_
   for ( unsigned i = 0; i < recordCount; i++ )
   {
      int64_t nextInt64 = sourceBuffer_->getNextInt64();
      if ( nextInt64 != minimum_ )
      {
         throw E57_EXCEPTION2( E57_ERROR_VALUE_OUT_OF_BOUNDS,
                               "nextInt64=" + toString( nextInt64 ) + " minimum=" + toString( minimum_ ) );
      }
   }

   /// Update counts of records processed
   currentRecordIndex_ += recordCount;

   return ( currentRecordIndex_ );
}

unsigned ConstantIntegerEncoder::sourceBufferNextIndex()
{
   return ( sourceBuffer_->nextIndex() );
}

uint64_t ConstantIntegerEncoder::currentRecordIndex()
{
   return ( currentRecordIndex_ );
}

float ConstantIntegerEncoder::bitsPerRecord()
{
   /// We don't produce any output
   return ( 0.0 );
}

bool ConstantIntegerEncoder::registerFlushToOutput()
{
   return ( true );
}

size_t ConstantIntegerEncoder::outputAvailable() const
{
   /// We don't produce any output
   return 0;
}

void ConstantIntegerEncoder::outputRead( char * /*dest*/, const size_t byteCount )
{
   /// Should never request any output data
   if ( byteCount > 0 )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "byteCount=" + toString( byteCount ) );
   }
}

void ConstantIntegerEncoder::outputClear()
{
}

void ConstantIntegerEncoder::sourceBufferSetNew( std::vector<SourceDestBuffer> &sbufs )
{
   /// Verify that this encoder only has single input buffer
   if ( sbufs.size() != 1 )
   {
      throw E57_EXCEPTION2( E57_ERROR_INTERNAL, "sbufsSize=" + toString( sbufs.size() ) );
   }

   sourceBuffer_ = sbufs.at( 0 ).impl();
}

size_t ConstantIntegerEncoder::outputGetMaxSize()
{
   /// We don't produce any output
   return ( 0 );
}

void ConstantIntegerEncoder::outputSetMaxSize( unsigned /*byteCount*/ )
{
   /// Ignore, since don't produce any output
}

#ifdef E57_DEBUG
void ConstantIntegerEncoder::dump( int indent, std::ostream &os ) const
{
   Encoder::dump( indent, os );
   os << space( indent ) << "currentRecordIndex:  " << currentRecordIndex_ << std::endl;
   os << space( indent ) << "minimum:             " << minimum_ << std::endl;
   os << space( indent ) << "sourceBuffer:" << std::endl;
   sourceBuffer_->dump( indent + 4, os );
}
#endif
