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

#pragma once

#include "Common.h"

namespace e57
{
   class ImageFileImpl;

   class SourceDestBufferImpl : public std::enable_shared_from_this<SourceDestBufferImpl>
   {
   public:
      SourceDestBufferImpl( ImageFileImplWeakPtr destImageFile, const ustring &pathName, const size_t capacity,
                            bool doConversion = false, bool doScaling = false );

      template <typename T> void setTypeInfo( T *base, size_t stride = sizeof( T ) );

      SourceDestBufferImpl( ImageFileImplWeakPtr destImageFile, const ustring &pathName, StringList *b );

      ImageFileImplWeakPtr destImageFile() const
      {
         return destImageFile_;
      }

      ustring pathName() const
      {
         return pathName_;
      }
      MemoryRepresentation memoryRepresentation() const
      {
         return memoryRepresentation_;
      }
      void *base() const
      {
         return base_;
      }
      StringList *ustrings() const
      {
         return ustrings_;
      }
      bool doConversion() const
      {
         return doConversion_;
      }
      bool doScaling() const
      {
         return doScaling_;
      }
      size_t stride() const
      {
         return stride_;
      }
      size_t capacity() const
      {
         return capacity_;
      }
      unsigned nextIndex() const
      {
         return nextIndex_;
      }
      void rewind()
      {
         nextIndex_ = 0;
      }

      int64_t getNextInt64();
      int64_t getNextInt64( double scale, double offset );
      float getNextFloat();
      double getNextDouble();
      ustring getNextString();
      void setNextInt64( int64_t value );
      void setNextInt64( int64_t value, double scale, double offset );
      void setNextFloat( float value );
      void setNextDouble( double value );
      void setNextString( const ustring &value );

      void checkCompatible( const std::shared_ptr<SourceDestBufferImpl> &newBuf ) const;

#ifdef E57_DEBUG
      void dump( int indent = 0, std::ostream &os = std::cout );
#endif

   private:
      template <typename T> void _setNextReal( T inValue );

      void checkState_() const; /// Common routine to check that constructor
                                /// arguments were ok, throws if not

      //??? verify alignment
      ImageFileImplWeakPtr destImageFile_;
      ustring pathName_;                          /// Pathname from CompressedVectorNode to source/dest
                                                  /// object, e.g. "Indices/0"
      MemoryRepresentation memoryRepresentation_; /// Type of element (e.g. E57_INT8, E57_UINT64,
                                                  /// DOUBLE...)
      char *base_ = nullptr;                      /// Address of first element, for non-ustring buffers
      size_t capacity_ = 0;                       /// Total number of elements in array
      bool doConversion_ = false;                 /// Convert memory representation to/from disk representation
      bool doScaling_ = false;                    /// Apply scale factor for integer type
      size_t stride_ = 0;                         /// Distance between each element (different than size_
                                                  /// if elements not contiguous)
      unsigned nextIndex_ = 0;                    /// Number of elements that have been set (dest
                                                  /// buffer) or read (source buffer) since rewind().
      StringList *ustrings_ = nullptr;            /// Optional array of ustrings (used if
                                                  /// memoryRepresentation_==E57_USTRING) ???ownership
   };
}
