/***********************************************************************
Author: Zihan Chen (vczh)
Licensed under https://github.com/vczh-libraries/License
***********************************************************************/

#ifndef VCZH_STREAM_SERIALIZATION
#define VCZH_STREAM_SERIALIZATION

#include "CharFormat.h"

namespace vl
{
	namespace stream
	{
/***********************************************************************
Serialization
***********************************************************************/

		namespace internal
		{
			template<typename T>
			struct Reader
			{
				stream::IStream&			input;
				T							context;

				Reader(stream::IStream& _input)
					:input(_input)
					, context(nullptr)
				{
				}
			};
				
			template<typename T>
			struct Writer
			{
				stream::IStream&			output;
				T							context;

				Writer(stream::IStream& _output)
					:output(_output)
					, context(nullptr)
				{
				}
			};

			using ContextFreeReader = Reader<void*>;
			using ContextFreeWriter = Writer<void*>;

			template<typename T>
			struct Serialization
			{
				template<typename TIO>
				static void IO(TIO& io, T& value);
			};

			template<typename TValue, typename TContext>
			Reader<TContext>& operator<<(Reader<TContext>& reader, TValue& value)
			{
				Serialization<TValue>::IO(reader, value);
				return reader;
			}

			template<typename TValue, typename TContext>
			Writer<TContext>& operator<<(Writer<TContext>& writer, TValue& value)
			{
				Serialization<TValue>::IO(writer, value);
				return writer;
			}

/***********************************************************************
Serialization (integers)
***********************************************************************/

			template<typename T>
			struct Serialization_POD
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, T& value)
				{
					if (reader.input.Read(&value, sizeof(value)) != sizeof(value))
					{
						CHECK_FAIL(L"Deserialization failed.");
					}
				}

				template<typename TContext>
				static void IO(Writer<TContext>& writer, T& value)
				{
					if (writer.output.Write(&value, sizeof(value)) != sizeof(value))
					{
						CHECK_FAIL(L"Serialization failed.");
					}
				}
			};

			template<typename TValue, typename TData>
			struct Serialization_DefaultConversion
			{
				static TValue ToValue(TData data)
				{
					return (TValue)data;
				}

				static TData FromValue(TValue value)
				{
					return (TData)value;
				}
			};

			template<typename TValue, typename TData, typename TConversion = Serialization_DefaultConversion<TValue, TData>>
			struct Serialization_Conversion
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, TValue& value)
				{
					TData data;
					Serialization<TData>::IO(reader, data);
					value = TConversion::ToValue(data);
				}

				template<typename TContext>
				static void IO(Writer<TContext>& writer, TValue& value)
				{
					TData data = TConversion::FromValue(value);
					Serialization<TData>::IO(writer, data);
				}
			};

			template<>
			struct Serialization<vint64_t> : Serialization_POD<vint64_t> {};

			template<>
			struct Serialization<vuint64_t> : Serialization_POD<vuint64_t> {};

			template<>
			struct Serialization<vint32_t> : Serialization_Conversion<vint32_t, vint64_t> {};

			template<>
			struct Serialization<vuint32_t> : Serialization_Conversion<vuint32_t, vuint64_t> {};

			template<>
			struct Serialization<vint16_t> : Serialization_POD<vint16_t> {};

			template<>
			struct Serialization<vuint16_t> : Serialization_POD<vuint16_t> {};

			template<>
			struct Serialization<vint8_t> : Serialization_POD<vint8_t> {};

			template<>
			struct Serialization<vuint8_t> : Serialization_POD<vuint8_t> {};

/***********************************************************************
Serialization (chars)
***********************************************************************/

			template<>
			struct Serialization<char> : Serialization_Conversion<char, vint64_t> {};

			template<>
			struct Serialization<wchar_t> : Serialization_Conversion<wchar_t, vint64_t> {};

			template<>
			struct Serialization<char8_t> : Serialization_Conversion<char8_t, vint64_t> {};

			template<>
			struct Serialization<char16_t> : Serialization_Conversion<char16_t, vint64_t> {};

			template<>
			struct Serialization<char32_t> : Serialization_Conversion<char32_t, vint64_t> {};

/***********************************************************************
Serialization (floats)
***********************************************************************/

			template<>
			struct Serialization<double> : Serialization_POD<double> {};

			template<>
			struct Serialization<float> : Serialization_POD<float> {};

			template<>
			struct Serialization<bool> : Serialization_Conversion<bool, vint8_t, Serialization<bool>>
			{
				static bool ToValue(vint8_t data)
				{
					return data == -1;
				}

				static vint8_t FromValue(bool value)
				{
					return value ? -1 : 0;
				}
			};

/***********************************************************************
Serialization (generic types)
***********************************************************************/

			template<typename T>
			struct Serialization<Ptr<T>>
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, Ptr<T>& value)
				{
					bool notNull = false;
					reader << notNull;
					if (notNull)
					{
						value = new T;
						Serialization<T>::IO(reader, *value.Obj());
					}
					else
					{
						value = 0;
					}
				}
					
				template<typename TContext>
				static void IO(Writer<TContext>& writer, Ptr<T>& value)
				{
					bool notNull = value;
					writer << notNull;
					if (notNull)
					{
						Serialization<T>::IO(writer, *value.Obj());
					}
				}
			};

			template<typename T>
			struct Serialization<Nullable<T>>
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, Nullable<T>& value)
				{
					bool notNull = false;
					reader << notNull;
					if (notNull)
					{
						T data;
						Serialization<T>::IO(reader, data);
						value = Nullable<T>(data);
					}
					else
					{
						value = Nullable<T>();
					}
				}
				
				template<typename TContext>	
				static void IO(Writer<TContext>& writer, Nullable<T>& value)
				{
					bool notNull = value;
					writer << notNull;
					if (notNull)
					{
						T data = value.Value();
						Serialization<T>::IO(writer, data);
					}
				}
			};

/***********************************************************************
Serialization (strings)
***********************************************************************/

			template<>
			struct Serialization<WString>
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, WString& value)
				{
					vint count = -1;
					reader << count;
					if (count > 0)
					{
						MemoryStream stream;
						reader << (IStream&)stream;
						Utf8Decoder decoder;
						decoder.Setup(&stream);

						collections::Array<wchar_t> stringBuffer(count + 1);
						vint stringSize = decoder.Read(&stringBuffer[0], count * sizeof(wchar_t));
						stringBuffer[stringSize / sizeof(wchar_t)] = 0;

						value = &stringBuffer[0];
					}
					else
					{
						value = L"";
					}
				}
					
				template<typename TContext>
				static void IO(Writer<TContext>& writer, WString& value)
				{
					vint count = value.Length();
					writer << count;
					if (count > 0)
					{
						MemoryStream stream;
						{
							Utf8Encoder encoder;
							encoder.Setup(&stream);
							encoder.Write((void*)value.Buffer(), count * sizeof(wchar_t));
						}
						writer << (IStream&)stream;
					}
				}
			};

/***********************************************************************
Serialization (collections)
***********************************************************************/

			template<typename T>
			struct Serialization<collections::List<T>>
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, collections::List<T>& value)
				{
					vint32_t count = -1;
					reader << count;
					value.Clear();
					for (vint i = 0; i < count; i++)
					{
						T t;
						reader << t;
						value.Add(t);
					}
				}
					
				template<typename TContext>
				static void IO(Writer<TContext>& writer, collections::List<T>& value)
				{
					vint32_t count = (vint32_t)value.Count();
					writer << count;
					for (vint i = 0; i < count; i++)
					{
						writer << value[i];
					}
				}
			};

			template<typename T>
			struct Serialization<collections::Array<T>>
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, collections::Array<T>& value)
				{
					vint32_t count = -1;
					reader << count;
					value.Resize(count);
					for (vint i = 0; i < count; i++)
					{
						reader << value[i];
					}
				}
					
				template<typename TContext>
				static void IO(Writer<TContext>& writer, collections::Array<T>& value)
				{
					vint32_t count = (vint32_t)value.Count();
					writer << count;
					for (vint i = 0; i < count; i++)
					{
						writer << value[i];
					}
				}
			};

			template<typename K, typename V>
			struct Serialization<collections::Dictionary<K, V>>
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, collections::Dictionary<K, V>& value)
				{
					vint32_t count = -1;
					reader << count;
					value.Clear();
					for (vint i = 0; i < count; i++)
					{
						K k;
						V v;
						reader << k << v;
						value.Add(k, v);
					}
				}
					
				template<typename TContext>
				static void IO(Writer<TContext>& writer, collections::Dictionary<K, V>& value)
				{
					vint32_t count = (vint32_t)value.Count();
					writer << count;
					for (vint i = 0; i < count; i++)
					{
						K k = value.Keys()[i];
						V v = value.Values()[i];
						writer << k << v;
					}
				}
			};

			template<typename K, typename V>
			struct Serialization<collections::Group<K, V>>
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, collections::Group<K, V>& value)
				{
					vint32_t count = -1;
					reader << count;
					value.Clear();
					for (vint i = 0; i < count; i++)
					{
						K k;
						collections::List<V> v;
						reader << k << v;
						for (vint j = 0; j < v.Count(); j++)
						{
							value.Add(k, v[j]);
						}
					}
				}
					
				template<typename TContext>
				static void IO(Writer<TContext>& writer, collections::Group<K, V>& value)
				{
					vint32_t count = (vint32_t)value.Count();
					writer << count;
					for (vint i = 0; i < count; i++)
					{
						K k = value.Keys()[i];
						collections::List<V>& v = const_cast<collections::List<V>&>(value.GetByIndex(i));
						writer << k << v;
					}
				}
			};

/***********************************************************************
Serialization (MISC)
***********************************************************************/

			template<>
			struct Serialization<stream::IStream>
			{
				template<typename TContext>
				static void IO(Reader<TContext>& reader, stream::IStream& value)
				{
					vint32_t count = 0;
					reader.input.Read(&count, sizeof(count));

					if (count > 0)
					{
						vint length = 0;
						collections::Array<vuint8_t> buffer(count);
						value.SeekFromBegin(0);
						length = reader.input.Read(&buffer[0], count);
						if (length != count)
						{
							CHECK_FAIL(L"Deserialization failed.");
						}
						length = value.Write(&buffer[0], count);
						if (length != count)
						{
							CHECK_FAIL(L"Deserialization failed.");
						}
						value.SeekFromBegin(0);
					}
				}
					
				template<typename TContext>
				static void IO(Writer<TContext>& writer, stream::IStream& value)
				{
					vint32_t count = (vint32_t)value.Size();
					writer.output.Write(&count, sizeof(count));

					if (count > 0)
					{
						vint length = 0;
						collections::Array<vuint8_t> buffer(count);
						value.SeekFromBegin(0);
						length = value.Read(&buffer[0], count);
						if (length != count)
						{
							CHECK_FAIL(L"Serialization failed.");
						}
						length = writer.output.Write(&buffer[0], count);
						if (length != count)
						{
							CHECK_FAIL(L"Serialization failed.");
						}
						value.SeekFromBegin(0);
					}
				}
			};

/***********************************************************************
Serialization (macros)
***********************************************************************/

#define BEGIN_SERIALIZATION(TYPE)\
			template<>\
			struct Serialization<TYPE>\
			{\
				template<typename TIO>\
				static void IO(TIO& op, TYPE& value)\
				{\
					op\

#define SERIALIZE(FIELD)\
					<< value.FIELD\

#define END_SERIALIZATION\
					;\
				}\
			};\

#define SERIALIZE_ENUM(TYPE)\
			template<>\
			struct Serialization<TYPE>\
			{\
				template<typename TContext>\
				static void IO(Reader<TContext>& reader, TYPE& value)\
				{\
					vint32_t v = 0;\
					Serialization<vint32_t>::IO(reader, v);\
					value = (TYPE)v;\
				}\
				template<typename TContext>\
				static void IO(Writer<TContext>& writer, TYPE& value)\
				{\
					vint32_t v = (vint32_t)value;\
					Serialization<vint32_t>::IO(writer, v);\
				}\
			};\

		}
	}
}

#endif
