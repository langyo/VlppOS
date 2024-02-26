/***********************************************************************
Author: Zihan Chen (vczh)
Licensed under https://github.com/vczh-libraries/License
***********************************************************************/

#ifndef VCZH_STREAM_ENCODING_CHARFORMAT_UTFENCODING
#define VCZH_STREAM_ENCODING_CHARFORMAT_UTFENCODING

#include "CharEncodingBase.h"

namespace vl
{
	namespace stream
	{

/***********************************************************************
UtfStreamConsumer<T>
***********************************************************************/

		template<typename T>
		class UtfStreamConsumer : public Object
		{
		protected:
			IStream*				stream = nullptr;

			T Consume()
			{
				T c;
				vint size = stream->Read(&c, sizeof(c));
				if (size != sizeof(c)) return 0;
				return c;
			}
		public:
			void Setup(IStream* _stream)
			{
				stream = _stream;
			}

			bool HasIllegalChar() const
			{
				return false;
			}
		};

/***********************************************************************
UtfStreamToStreamReader<TFrom, TTo>
***********************************************************************/

		template<typename TFrom, typename TTo>
		class UtfStreamToStreamReader : public encoding::UtfToUtfReaderBase<TFrom, TTo, UtfStreamConsumer<TFrom>>
		{
		public:
			void Setup(IStream* _stream)
			{
				this->internalReader.Setup(_stream);
			}

			encoding::UtfCharCluster SourceCluster() const
			{
				return this->internalReader.SourceCluster();
			}
		};

		template<typename TFrom, typename TTo>
			requires(std::is_same_v<TFrom, char32_t> || std::is_same_v<TTo, char32_t>)
		class UtfStreamToStreamReader<TFrom, TTo> : public encoding::UtfToUtfReaderBase<TFrom, TTo, UtfStreamConsumer<TFrom>>
		{
		};

/***********************************************************************
Unicode General
***********************************************************************/

		template<typename T>
		struct MaxPossibleCodePoints
		{
			static const vint		Value = encoding::UtfConversion<T>::BufferLength;
		};

		template<>
		struct MaxPossibleCodePoints<char32_t>
		{
			static const vint		Value = 1;
		};

		template<typename TNative, typename TExpect>
		class UtfGeneralEncoder : public CharEncoderBase
		{
		protected:
			vuint8_t						cacheBuffer[sizeof(TExpect) * MaxPossibleCodePoints<TExpect>::Value];
			vint							cacheSize = 0;

		public:

			vint							Write(void* _buffer, vint _size) override;
		};

		template<typename TNative, typename TExpect>
		class UtfGeneralDecoder : public CharDecoderBase
		{
			using TStreamReader = UtfStreamToStreamReader<TNative, TExpect>;
		protected:
			vuint8_t						cacheBuffer[sizeof(TExpect)];
			vint							cacheSize = 0;
			TStreamReader					reader;

		public:

			void							Setup(IStream* _stream) override;
			vint							Read(void* _buffer, vint _size) override;
		};

/***********************************************************************
Unicode General (without conversion)
***********************************************************************/

		template<typename T>
		class UtfGeneralEncoder<T, T> : public CharEncoderBase
		{
		public:
			vint							Write(void* _buffer, vint _size) override;
		};

		template<typename T>
		class UtfGeneralDecoder<T, T> : public CharDecoderBase
		{
		public:
			vint							Read(void* _buffer, vint _size) override;
		};

#if defined VCZH_WCHAR_UTF16

		template<>
		class UtfGeneralEncoder<char16_t, wchar_t> : public UtfGeneralEncoder<wchar_t, wchar_t> {};

		template<>
		class UtfGeneralEncoder<wchar_t, char16_t> : public UtfGeneralEncoder<wchar_t, wchar_t> {};

#elif defined VCZH_WCHAR_UTF32

		template<>
		class UtfGeneralEncoder<char32_t, wchar_t> : public UtfGeneralEncoder<wchar_t, wchar_t> {};

		template<>
		class UtfGeneralEncoder<wchar_t, char32_t> : public UtfGeneralEncoder<wchar_t, wchar_t> {};

#endif

/***********************************************************************
Unicode General (extern templates)
***********************************************************************/

		extern template class UtfGeneralEncoder<wchar_t, wchar_t>;
		extern template class UtfGeneralEncoder<wchar_t, char8_t>;
		extern template class UtfGeneralEncoder<wchar_t, char16_t>;
		extern template class UtfGeneralEncoder<wchar_t, char16be_t>;
		extern template class UtfGeneralEncoder<wchar_t, char32_t>;

		extern template class UtfGeneralEncoder<char8_t, wchar_t>;
		extern template class UtfGeneralEncoder<char8_t, char8_t>;
		extern template class UtfGeneralEncoder<char8_t, char16_t>;
		extern template class UtfGeneralEncoder<char8_t, char16be_t>;
		extern template class UtfGeneralEncoder<char8_t, char32_t>;

		extern template class UtfGeneralEncoder<char16_t, wchar_t>;
		extern template class UtfGeneralEncoder<char16_t, char8_t>;
		extern template class UtfGeneralEncoder<char16_t, char16_t>;
		extern template class UtfGeneralEncoder<char16_t, char16be_t>;
		extern template class UtfGeneralEncoder<char16_t, char32_t>;

		extern template class UtfGeneralEncoder<char16be_t, wchar_t>;
		extern template class UtfGeneralEncoder<char16be_t, char8_t>;
		extern template class UtfGeneralEncoder<char16be_t, char16_t>;
		extern template class UtfGeneralEncoder<char16be_t, char16be_t>;
		extern template class UtfGeneralEncoder<char16be_t, char32_t>;

		extern template class UtfGeneralEncoder<char32_t, wchar_t>;
		extern template class UtfGeneralEncoder<char32_t, char8_t>;
		extern template class UtfGeneralEncoder<char32_t, char16_t>;
		extern template class UtfGeneralEncoder<char32_t, char16be_t>;
		extern template class UtfGeneralEncoder<char32_t, char32_t>;

		extern template class UtfGeneralDecoder<wchar_t, wchar_t>;
		extern template class UtfGeneralDecoder<wchar_t, char8_t>;
		extern template class UtfGeneralDecoder<wchar_t, char16_t>;
		extern template class UtfGeneralDecoder<wchar_t, char16be_t>;
		extern template class UtfGeneralDecoder<wchar_t, char32_t>;

		extern template class UtfGeneralDecoder<char8_t, wchar_t>;
		extern template class UtfGeneralDecoder<char8_t, char8_t>;
		extern template class UtfGeneralDecoder<char8_t, char16_t>;
		extern template class UtfGeneralDecoder<char8_t, char16be_t>;
		extern template class UtfGeneralDecoder<char8_t, char32_t>;

		extern template class UtfGeneralDecoder<char16_t, wchar_t>;
		extern template class UtfGeneralDecoder<char16_t, char8_t>;
		extern template class UtfGeneralDecoder<char16_t, char16_t>;
		extern template class UtfGeneralDecoder<char16_t, char16be_t>;
		extern template class UtfGeneralDecoder<char16_t, char32_t>;

		extern template class UtfGeneralDecoder<char16be_t, wchar_t>;
		extern template class UtfGeneralDecoder<char16be_t, char8_t>;
		extern template class UtfGeneralDecoder<char16be_t, char16_t>;
		extern template class UtfGeneralDecoder<char16be_t, char16be_t>;
		extern template class UtfGeneralDecoder<char16be_t, char32_t>;

		extern template class UtfGeneralDecoder<char32_t, wchar_t>;
		extern template class UtfGeneralDecoder<char32_t, char8_t>;
		extern template class UtfGeneralDecoder<char32_t, char16_t>;
		extern template class UtfGeneralDecoder<char32_t, char16be_t>;
		extern template class UtfGeneralDecoder<char32_t, char32_t>;
	}
}

#endif
