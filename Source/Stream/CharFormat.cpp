/***********************************************************************
Author: Zihan Chen (vczh)
Licensed under https://github.com/vczh-libraries/License
***********************************************************************/

#include "CharFormat.h"
#if defined VCZH_MSVC
#include <windows.h>
#elif defined VCZH_GCC
#include <string.h>
#endif

namespace vl
{
	namespace stream
	{

/***********************************************************************
CharEncoder
***********************************************************************/

		void CharEncoder::Setup(IStream* _stream)
		{
			stream = _stream;
		}

		void CharEncoder::Close()
		{
		}

		vint CharEncoder::Write(void* _buffer, vint _size)
		{
			// prepare a buffer for input
			vint availableChars = (cacheSize + _size) / sizeof(wchar_t);
			vint availableBytes = availableChars * sizeof(wchar_t);
			bool needToFree = false;
			vuint8_t* unicode = nullptr;
			if (cacheSize > 0)
			{
				unicode = new vuint8_t[cacheSize + _size];
				memcpy(unicode, cacheBuffer, cacheSize);
				memcpy((vuint8_t*)unicode + cacheSize, _buffer, _size);
				needToFree = true;
			}
			else
			{
				unicode = (vuint8_t*)_buffer;
			}

#if defined VCZH_WCHAR_UTF16
			{
				// a surrogate pair must be written as a whole thing
				vuint16_t c = (vuint16_t)((wchar_t*)unicode)[availableChars - 1];
				if ((c & 0xFC00U) == 0xD800U)
				{
					availableChars -= 1;
					availableBytes -= sizeof(wchar_t);
				}
			}
#endif

			// write the buffer
			vint written = WriteString((wchar_t*)unicode, availableChars, needToFree) * sizeof(wchar_t);
			CHECK_ERROR(written == availableBytes, L"CharEncoder::Write(void*, vint)#Failed to write a complete string.");

			// cache the remaining
			cacheSize = cacheSize + _size - availableBytes;
			if (cacheSize > 0)
			{
				CHECK_ERROR(cacheSize <= sizeof(char32_t), L"CharEncoder::Write(void*, vint)#Unwritten text is too large to cache.");
				memcpy(cacheBuffer, unicode + availableBytes, cacheSize);
			}

			return written;
		}

/***********************************************************************
CharDecoder
***********************************************************************/

		void CharDecoder::Setup(IStream* _stream)
		{
			stream=_stream;
		}

		void CharDecoder::Close()
		{
		}

		vint CharDecoder::Read(void* _buffer, vint _size)
		{
			vuint8_t* writing = (vuint8_t*)_buffer;
			vint filledBytes = 0;

			// feed the cache first
			if (cacheSize > 0)
			{
				filledBytes = cacheSize < _size ? cacheSize : _size;
				memcpy(writing, cacheBuffer, cacheSize);
				_size -= filledBytes;
				writing += filledBytes;

				// adjust the cache if it is not fully consumed
				cacheSize -= filledBytes;
				if (cacheSize > 0)
				{
					memcpy(cacheBuffer, cacheBuffer + filledBytes, cacheSize);
				}

				if (_size == 0)
				{
					return filledBytes;
				}
			}

			// fill the buffer as many as possible
			while (_size >= sizeof(wchar_t))
			{
				vint availableChars = _size / sizeof(wchar_t);
				vint readBytes = ReadString((wchar_t*)writing, availableChars) * sizeof(wchar_t);
				if (readBytes == 0) break;
				filledBytes += readBytes;
				_size -= readBytes;
				writing += readBytes;
			}

			// cache the remaining wchar_t
			if (_size < sizeof(wchar_t))
			{
				wchar_t c;
				vint readChars = ReadString(&c, 1) * sizeof(wchar_t);
				if (readChars == sizeof(wchar_t))
				{
					vuint8_t* reading = (vuint8_t*)&c;
					memcpy(writing, reading, _size);
					filledBytes += _size;
					cacheSize = sizeof(wchar_t) - _size;
					memcpy(cacheBuffer, reading + _size, cacheSize);
				}
			}

			return filledBytes;
		}

/***********************************************************************
Mbcs
***********************************************************************/

		vint MbcsEncoder::WriteString(wchar_t* _buffer, vint chars, bool freeToUpdate)
		{
#if defined VCZH_MSVC
			vint length = WideCharToMultiByte(CP_THREAD_ACP, 0, _buffer, (int)chars, NULL, NULL, NULL, NULL);
			char* mbcs = new char[length];
			WideCharToMultiByte(CP_THREAD_ACP, 0, _buffer, (int)chars, mbcs, (int)length, NULL, NULL);
			vint result = stream->Write(mbcs, length);
			delete[] mbcs;
#elif defined VCZH_GCC
			WString w = WString::CopyFrom(_buffer, chars);
			AString a = wtoa(w);
			vint length = a.Length();
			vint result = stream->Write((void*)a.Buffer(), length);
#endif
			if (result != length)
			{
				Close();
				return 0;
			}
			return chars;
		}

		vint MbcsDecoder::ReadString(wchar_t* _buffer, vint chars)
		{
			char* source = new char[chars * 2];
			char* reading = source;
			vint readed = 0;
			while (readed < chars)
			{
				if (stream->Read(reading, 1) != 1)
				{
					break;
				}
#if defined VCZH_MSVC
				if (IsDBCSLeadByte(*reading))
#elif defined VCZH_GCC
				if ((vint8_t)*reading < 0)
#endif
				{
					if (stream->Read(reading + 1, 1) != 1)
					{
						break;
					}
					reading += 2;
				}
				else
				{
					reading++;
				}
				readed++;
			}
#if defined VCZH_MSVC
			MultiByteToWideChar(CP_THREAD_ACP, 0, source, (int)(reading - source), _buffer, (int)chars);
#elif defined VCZH_GCC
			AString a = AString::CopyFrom(source, (vint)(reading - source));
			WString w = atow(a);
			memcpy(_buffer, w.Buffer(), readed * sizeof(wchar_t));
#endif
			delete[] source;
			return readed;
		}

/***********************************************************************
Utf-16
***********************************************************************/

		vint Utf16Encoder::WriteString(wchar_t* _buffer, vint chars, bool freeToUpdate)
		{
#if defined VCZH_WCHAR_UTF16
			vint size = chars * sizeof(wchar_t);
			vint written = stream->Write(_buffer, size);
			if (written != size)
			{
				Close();
				return 0;
			}
			return chars;
#elif defined VCZH_WCHAR_UTF32
			WCharToUtfReader<char16_t> reader(_buffer, chars);
			while (char16_t c = reader.Read())
			{
				vint written = stream->Write(&c, sizeof(c));
				if (written != sizeof(c))
				{
					Close();
					return 0;
				}
			}
			if (reader.HasIllegalChar())
			{
				Close();
				return 0;
			}
			return chars;
#endif
		}

		vint Utf16Decoder::ReadString(wchar_t* _buffer, vint chars)
		{
#if defined VCZH_WCHAR_UTF16
			vint read = stream->Read(_buffer, chars * sizeof(wchar_t));
			CHECK_ERROR(read % sizeof(wchar_t) == 0, L"Utf16Decoder::ReadString(wchar_t*, vint)#Failed to read complete wchar_t characters.");
			return read / sizeof(wchar_t);
#elif defined VCZH_WCHAR_UTF32
			reader.Setup(stream);
			vint counter = 0;
			for (vint i = 0; i < chars; i++)
			{
				wchar_t c = reader.Read();
				if (!c) break;
				_buffer[i] = c;
				counter++;
			}
			return counter;
#endif
		}

/***********************************************************************
Utf-16-be
***********************************************************************/

		vint Utf16BEEncoder::WriteString(wchar_t* _buffer, vint chars, bool freeToUpdate)
		{
#if defined VCZH_WCHAR_UTF16
			if (freeToUpdate)
			{
				SwapBytesForUtf16BE(_buffer, chars);
				vint size = chars * sizeof(wchar_t);
				vint written = stream->Write(_buffer, size);
				SwapBytesForUtf16BE(_buffer, chars);
				if (written != size)
				{
					Close();
					return 0;
				}
				return chars;
			}
			else
			{
				vint counter = 0;
				for (vint i = 0; i < chars; i++)
				{
					wchar_t c = _buffer[i];
					SwapByteForUtf16BE(c);
					vint written = stream->Write(&c, sizeof(c));
					if (written != sizeof(c))
					{
						Close();
						return 0;
					}
					counter++;
				}
				return counter;
			}
#elif defined VCZH_WCHAR_UTF32
			WCharToUtfReader<char16_t> reader(_buffer, chars);
			while (char16_t c = reader.Read())
			{
				SwapByteForUtf16BE(c);
				vint written = stream->Write(&c, sizeof(c));
				if (written != sizeof(c))
				{
					Close();
					return 0;
				}
			}
			if (reader.HasIllegalChar())
			{
				Close();
				return 0;
			}
			return chars;
#endif
		}

		vint Utf16BEDecoder::ReadString(wchar_t* _buffer, vint chars)
		{
#if defined VCZH_WCHAR_UTF16
			vint read = stream->Read(_buffer, chars * sizeof(wchar_t));
			CHECK_ERROR(read % sizeof(wchar_t) == 0, L"Utf16Decoder::ReadString(wchar_t*, vint)#Failed to read complete wchar_t characters.");
			vint readChars = read / sizeof(wchar_t);
			SwapBytesForUtf16BE(_buffer, readChars);
			return readChars;
#elif defined VCZH_WCHAR_UTF32
			reader.Setup(stream);
			vint counter = 0;
			for (vint i = 0; i < chars; i++)
			{
				wchar_t c = reader.Read();
				if (!c) break;
				_buffer[i] = c;
				counter++;
			}
			return counter;
#endif
		}

/***********************************************************************
Utf8
***********************************************************************/

		vint Utf8Encoder::WriteString(wchar_t* _buffer, vint chars, bool freeToUpdate)
		{
#if defined VCZH_MSVC
			vint length = WideCharToMultiByte(CP_UTF8, 0, _buffer, (int)chars, NULL, NULL, NULL, NULL);
			char* mbcs = new char[length];
			WideCharToMultiByte(CP_UTF8, 0, _buffer, (int)chars, mbcs, (int)length, NULL, NULL);
			vint result = stream->Write(mbcs, length);
			delete[] mbcs;
			if (result != length)
			{
				Close();
				return 0;
			}
			return chars;
#elif defined VCZH_GCC
			WCharToUtfReader<char8_t> reader(_buffer, chars);
			while (char8_t c = reader.Read())
			{
				vint written = stream->Write(&c, sizeof(c));
				if (written != sizeof(c))
				{
					Close();
					return 0;
				}
			}
			if (reader.HasIllegalChar())
			{
				Close();
				return 0;
			}
			return chars;
#endif
		}

		vint Utf8Decoder::ReadString(wchar_t* _buffer, vint chars)
		{
			reader.Setup(stream);
			vint counter = 0;
			for (vint i = 0; i < chars; i++)
			{
				wchar_t c = reader.Read();
				if (!c) break;
				_buffer[i] = c;
				counter++;
			}
			return counter;
		}
	}
}
