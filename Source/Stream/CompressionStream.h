/***********************************************************************
Author: Zihan Chen (vczh)
Licensed under https://github.com/vczh-libraries/License
***********************************************************************/

#ifndef VCZH_STREAM_COMPRESSIONSTREAM
#define VCZH_STREAM_COMPRESSIONSTREAM

#include "Interfaces.h"

namespace vl
{
	namespace stream
	{

/***********************************************************************
Compression
***********************************************************************/

		namespace lzw
		{
			static const vint						BufferSize = 1024;
			static const vint						MaxDictionarySize = 1 << 24;

			struct Code
			{
				typedef collections::PushOnlyAllocator<Code>			CodeAllocator;
				typedef collections::ByteObjectMap<Code>::Allocator		MapAllocator;

				vuint8_t							byte = 0;
				vint								code = -1;
				Code*								parent = 0;
				vint								size = 0;
				collections::ByteObjectMap<Code>	children;
			};
		}

		class LzwBase : public Object
		{
		protected:
			lzw::Code::CodeAllocator				codeAllocator;
			lzw::Code::MapAllocator					mapAllocator;
			lzw::Code*								root;
			vint									eofIndex = -1;
			vint									nextIndex = 0;
			vint									indexBits = 1;

			void									UpdateIndexBits();
			lzw::Code*								CreateCode(lzw::Code* parent, vuint8_t byte);

			LzwBase();
			LzwBase(bool (&existingBytes)[256]);
			~LzwBase();
		};

		/// <summary>An encoder to compress data using the Lzw algorithm.</summary>
		/// <remark>
		/// You are not recommended to compress data more than 1 mega bytes at once using the encoder directly.
		/// <see cref="CompressStream"/> and <see cref="DecompressStream"/> is recommended.
		/// </remark>
		class LzwEncoder : public LzwBase, public IEncoder
		{
		protected:
			IStream*								stream = 0;

			vuint8_t								buffer[lzw::BufferSize];
			vint									bufferUsedBits = 0;
			lzw::Code*								prefix;

			void									Flush();
			void									WriteNumber(vint number, vint bitSize);
		public:
			/// <summary>Create an encoder.</summary>
			LzwEncoder();
			/// <summary>Create an encoder, specifying what bytes will never appear in the data to compress.</summary>
			/// <param name="existingBytes">
			/// A filter array
			/// If existingBytes[x] == true, it means x will possibly appear.
			/// If existingBytes[x] == false, it means x will never appear.
			/// </param>
			/// <remark>
			/// The behavior is undefined, if existingBytes[x] == false, but byte x is actually in the data to compress.
			/// </remark>
			LzwEncoder(bool (&existingBytes)[256]);
			~LzwEncoder();

			void									Setup(IStream* _stream)override;
			void									Close()override;
			vint									Write(void* _buffer, vint _size)override;
		};
		
		/// <summary>An decoder to decompress data using the Lzw algorithm.</summary>
		/// <remark>
		/// You are not recommended to compress data more than 1 mega bytes at once using the encoder directly.
		/// <see cref="CompressStream"/> and <see cref="DecompressStream"/> is recommended.
		/// </remark>
		class LzwDecoder :public LzwBase, public IDecoder
		{
		protected:
			IStream*								stream = 0;
			collections::List<lzw::Code*>			dictionary;
			lzw::Code*								lastCode = 0;

			vuint8_t								inputBuffer[lzw::BufferSize];
			vint									inputBufferSize = 0;
			vint									inputBufferUsedBits = 0;

			collections::Array<vuint8_t>			outputBuffer;
			vint									outputBufferSize = 0;
			vint									outputBufferUsedBytes = 0;

			bool									ReadNumber(vint& number, vint bitSize);
			void									PrepareOutputBuffer(vint size);
			void									ExpandCodeToOutputBuffer(lzw::Code* code);
		public:
			/// <summary>Create a decoder.</summary>
			LzwDecoder();
			/// <summary>Create an encoder, specifying what bytes will never appear in the decompressed data.</summary>
			/// <param name="existingBytes">
			/// A filter array
			/// If existingBytes[x] == true, it means x will possibly appear.
			/// If existingBytes[x] == false, it means x will never appear.
			/// </param>
			/// <remark>
			/// The array "existingBytes" should exactly match the one given to <see cref="LzwEncoder"/>.
			/// </remark>
			LzwDecoder(bool (&existingBytes)[256]);
			~LzwDecoder();

			void									Setup(IStream* _stream)override;
			void									Close()override;
			vint									Read(void* _buffer, vint _size)override;
		};

/***********************************************************************
Helper Functions
***********************************************************************/

		/// <summary>Copy data from a <b>readable</b> input stream to a <b>writable</b> output stream.</summary>
		/// <returns>Data copied in bytes.</returns>
		/// <param name="inputStream">The <b>readable</b> input stream.</param>
		/// <param name="outputStream">The <b>writable</b> output stream.</param>
		extern vint						CopyStream(stream::IStream& inputStream, stream::IStream& outputStream);

		/// <summary>Compress data from a <b>readable</b> input stream to a <b>writable</b> output stream.</summary>
		/// <returns>Data copied in bytes.</returns>
		/// <param name="inputStream">The <b>readable</b> input stream.</param>
		/// <param name="outputStream">The <b>writable</b> output stream.</param>
		/// <remarks>
		/// Data is compressed in multiple batches,
		/// the is expected output stream to have data in multiple parts.
		/// In each part, the first 4 bytes is the data before compression in bytes.
		/// the rest is the compressed data.
		/// </remarks>
		extern void						CompressStream(stream::IStream& inputStream, stream::IStream& outputStream);

		/// <summary>Decompress data from a <b>readable</b> input stream (with compressed data) to a <b>writable</b> output stream (with uncompressed data).</summary>
		/// <returns>Data copied in bytes.</returns>
		/// <param name="inputStream">The <b>readable</b> input stream.</param>
		/// <param name="outputStream">The <b>writable</b> output stream.</param>
		/// <remarks>
		/// Data is compressed in multiple batches,
		/// the is expected input stream to have data in multiple parts.
		/// In each part, the first 4 bytes is the data before compression in bytes.
		/// the rest is the compressed data.
		/// </remarks>
		extern void						DecompressStream(stream::IStream& inputStream, stream::IStream& outputStream);
	}
}

#endif