#ifndef CXXREFLECT_WINDOWSRUNTIMEUTILITIES_HPP
#define CXXREFLECT_WINDOWSRUNTIMEUTILITIES_HPP

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is a set of standalone Windows Runtime utilities that can be used even without the rest of
// the CxxReflect libraries.

#ifndef CXXREFLECT_WINDOWSRUNTIME_UTILITIES_STANDALONE
#include "CxxReflect/FundamentalUtilities.hpp"
#endif

#if defined(CXXREFLECT_WINDOWSRUNTIME_UTILITIES_STANDALONE) || defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION)

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#include <cor.h>
#include <hstring.h>
#include <rometadataresolution.h>
#include <winstring.h>
#include <wrl/client.h>

namespace CxxReflect { namespace WindowsRuntime { namespace Utility {

    #ifndef CXXREFLECT_WINDOWSRUNTIME_UTILITIES_STANDALONE
    typedef CxxReflect::HResultRuntimeError HResultError;
    #else
    class HResultError : public std::exception
    {
    public:

        HResultError(HRESULT const hr = E_FAIL)
            : _hr(hr)
        {
        }

        HRESULT GetError() const { return _hr; }

    private:

        HRESULT _hr;
    };
    #endif

	inline void ThrowOnFailure(HRESULT const hr)
	{
		if (hr < 0)
			HResultError(hr);
	}

	typedef unsigned SizeType;

} } }

namespace CxxReflect { namespace WindowsRuntime { namespace Utility {

    /// A std::wstring-like wrapper around HSTRING
    ///
    /// Useful for Windows Runtime interop code, this class provides most of the const parts of the
    /// std::wstring interface.  For mutation, it is recommended to convert to std::wstring, mutate,
    /// then convert back to SmartHString.
	class SmartHString
	{
	public:

		typedef wchar_t           value_type;
		typedef std::size_t       size_type;
		typedef std::ptrdiff_t    difference_type;

		typedef value_type const& reference;
		typedef value_type const& const_reference;
		typedef value_type const* pointer;
		typedef value_type const* const_pointer;

		typedef pointer           iterator;
		typedef const_pointer     const_iterator;

		typedef std::reverse_iterator<iterator>       reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		SmartHString()
			: _value()
		{
		}

		explicit SmartHString(const_pointer const s)
		{
			ThrowOnFailure(WindowsCreateString(s, Detail::ConvertInteger(::wcslen(s)), &_value));
		}

		SmartHString(SmartHString const& other)
		{
			ThrowOnFailure(WindowsDuplicateString(other._value, &_value));
		}

		SmartHString(SmartHString&& other)
			: _value(other._value)
		{
			other._value = nullptr;
		}

		SmartHString& operator=(SmartHString other)
		{
			swap(other);
			return *this;
		}

		SmartHString& operator=(SmartHString&& other)
		{
			ThrowOnFailure(WindowsDeleteString(_value));
			_value = other._value;
			other._value = nullptr;
			return *this;
		}

		~SmartHString()
		{
			WindowsDeleteString(_value);
		}

		void swap(SmartHString& other)
		{
			std::swap(_value, other._value);
		}

		const_iterator begin()  const { return get_buffer_begin(); }
		const_iterator end()    const { return get_buffer_end();   }
		const_iterator cbegin() const { return begin();            }
		const_iterator cend()   const { return end();              }

		const_reverse_iterator rbegin()  const { return reverse_iterator(get_buffer_end());   }
		const_reverse_iterator rend()    const { return reverse_iterator(get_buffer_begin()); }
		const_reverse_iterator crbegin() const { return rbegin();                             }
		const_reverse_iterator crend()   const { return rend();                               }

		size_type size()     const { return end() - begin();                       }
		size_type length()   const { return size();                                }
		size_type max_size() const { return std::numeric_limits<size_type>::max(); }
		size_type capacity() const { return size();                                }
		bool      empty()    const { return size() == 0;                           }

		const_reference operator[](size_type const n) const
		{
			return get_buffer_begin()[n];
		}

		const_reference at(size_type const n) const
		{
			if (n >= size())
				throw HResultError(E_BOUNDS);

			return get_buffer_begin()[n];
		}

		const_reference front() const { return *get_buffer_begin();     }
		const_reference back()  const { return *(get_buffer_end() - 1); }

		const_pointer c_str() const { return get_buffer_begin(); }
		const_pointer data()  const { return get_buffer_begin(); }

		// A reference proxy, returned by proxy(), that can be passed into a function expecting an
		// HSTRING*.  When the reference proxy is destroyed, it sets the value of the SmartHString
		// from which it was created.
		class ReferenceProxy
		{
		public:

			ReferenceProxy(SmartHString* const value)
				: _value(value), _proxy(value->_value)
			{
			}

			~ReferenceProxy()
			{
				if (_value->_value == _proxy)
					return;

				SmartHString newString;
				newString._value = _proxy;

				_value->swap(newString);
			}

			operator HSTRING*() { return &_proxy; }

		private:

			// Note that this type is copyable though it is not intended to be copied, aside from
			// when it is returned from SmartHString::proxy().
			ReferenceProxy& operator=(ReferenceProxy const&);

			HSTRING       _proxy;
			SmartHString* _value;
		};

		ReferenceProxy proxy()       { return ReferenceProxy(this); }
		HSTRING        value() const { return _value;               }

		friend bool operator==(SmartHString const& lhs, SmartHString const& rhs)
		{
			return compare(lhs, rhs) ==  0;
		}

		friend bool operator<(SmartHString const& lhs, SmartHString const& rhs)
		{
			return compare(lhs, rhs) == -1;
		}

		friend bool operator!=(SmartHString const& lhs, SmartHString const& rhs) { return !(lhs == rhs); }
		friend bool operator> (SmartHString const& lhs, SmartHString const& rhs) { return   rhs <  lhs ; }
		friend bool operator>=(SmartHString const& lhs, SmartHString const& rhs) { return !(lhs <  rhs); }
		friend bool operator<=(SmartHString const& lhs, SmartHString const& rhs) { return !(rhs <  lhs); }

	private:

		const_pointer get_buffer_begin() const
		{
			const_pointer const result(WindowsGetStringRawBuffer(_value, nullptr));
			return result == nullptr ? get_empty_string() : result;
		}

		const_pointer get_buffer_end() const
		{
			std::uint32_t length(0);
			const_pointer const first(WindowsGetStringRawBuffer(_value, &length));
			return first == nullptr ? get_empty_string() : first + length;
		}

		static const_pointer get_empty_string()
		{
			static const_pointer const value(L"");
			return value;
		}

		static int compare(SmartHString const& lhs, SmartHString const& rhs)
		{
			std::int32_t result(0);
			ThrowOnFailure(WindowsCompareStringOrdinal(lhs._value, rhs._value, &result));
			return result;
		}

		HSTRING _value;
	};





	/// An RAII wrapper for a callee-allocated, caller-destroyed array of HSTRING
    ///
    /// Several low-level Windows Runtime functions allocate an array of HSTRING and require the
    /// caller to destroy the HSTRINGs and the array.  This RAII container makes that pattern much
    /// more pleasant.
	class SmartHStringArray
	{
	public:

		SmartHStringArray()
			: _count(), _array()
		{
		}

		~SmartHStringArray()
		{
			std::for_each(Begin(), End(), [](HSTRING& s)
			{
				WindowsDeleteString(s);
			});

			CoTaskMemFree(_array);
		}

		DWORD&    GetCount() { return _count; }
		HSTRING*& GetArray() { return _array; }

		HSTRING* Begin() const { return _array;          }
		HSTRING* End()   const { return _array + _count; }

	private:

		SmartHStringArray(SmartHStringArray const&);
		SmartHStringArray& operator=(SmartHStringArray const&);

		DWORD    _count;
		HSTRING* _array;
	};

} } }

namespace CxxReflect { namespace WindowsRuntime { namespace Utility {

	struct CorEnumIterationPolicy
	{
		typedef SizeType InterfaceType;
		typedef SizeType ValueType;
		typedef SizeType BufferType;
		typedef SizeType ArgumentType;

		static SizeType Advance(InterfaceType&, HCORENUM&, BufferType&, ArgumentType);

		static ValueType Get(BufferType const&, SizeType);
	};

    template <
        typename TInterface,
        typename TValueType,
        HRESULT (__stdcall TInterface::*Function)(HCORENUM*, TValueType*, ULONG, ULONG*)
    >
    struct BaseNullaryCorEnumIterationPolicy
    {
        typedef TInterface                    InterfaceType;
		typedef TValueType                    ValueType;
		typedef std::array<ValueType, 128>    BufferType;
		typedef SizeType                      ArgumentType;

		static unsigned Advance(InterfaceType     & import,
								HCORENUM          & e,
								BufferType        & buffer,
								ArgumentType const argument)
		{
			ULONG count;
			ThrowOnFailure((import.*Function)(&e, buffer.data(), buffer.size(), &count));
			return count;
		}

		static ValueType Get(BufferType const& buffer, SizeType const index)
		{
			return buffer[index];
		}
    };

	template <
		typename TInterface,
		typename TValueType,
		typename TArgument,
		HRESULT (__stdcall TInterface::*Function)(HCORENUM*, TArgument, TValueType*, ULONG, ULONG*)
	>
	struct BaseUnaryCorEnumIterationPolicy
	{
		typedef TInterface                    InterfaceType;
		typedef TValueType                    ValueType;
		typedef std::array<ValueType, 128>    BufferType;
		typedef TArgument                     ArgumentType;

		static unsigned Advance(InterfaceType     & import,
								HCORENUM          & e,
								BufferType        & buffer,
								ArgumentType const  argument)
		{
			ULONG count;
			ThrowOnFailure((import.*Function)(&e, argument, buffer.data(), buffer.size(), &count));
			return count;
		}

		static ValueType Get(BufferType const& buffer, SizeType const index)
		{
			return buffer[index];
		}
	};





	template <typename TIterationPolicy>
	class CorEnumIterationContext
	{
	public:

		typedef typename TIterationPolicy                PolicyType;
		typedef typename TIterationPolicy::InterfaceType InterfaceType;
		typedef typename TIterationPolicy::ValueType     ValueType;
		typedef typename TIterationPolicy::BufferType    BufferType;
		typedef typename TIterationPolicy::ArgumentType  ArgumentType;

		CorEnumIterationContext(InterfaceType* const import, ArgumentType const argument = ArgumentType())
			: _import(import), _e(), _buffer(), _current(), _count(), _argument(argument)
		{
			Advance();
		}

		~CorEnumIterationContext()
		{
			Close();
		}

		void Close()
		{
			if (_e != nullptr)
			{
				_import->CloseEnum(_e);
				_e = nullptr;
			}
		}

		void Reset()
		{
			if (_e != nullptr)
			{
				ThrowOnFailure(_import->ResetEnum(_e, 0));
			}
		}

		void Advance()
		{
			if (_e != nullptr && _current != _count - 1)
			{
				++_current;
			}
			else
			{
				_count = PolicyType::Advance(*_import, _e, _buffer, _argument);
				_current = 0;
			}
		}

		ValueType Current() const
		{
			return PolicyType::Get(_buffer, _current);
		}

		bool IsEnd() const
		{
			return _current == _count;
		}

		friend bool operator==(CorEnumIterationContext const& lhs, CorEnumIterationContext const& rhs)
		{
			if (lhs._e != rhs._e)
				return false;

			if (lhs._current != rhs._current)
				return false;

			ULONG lhsCount(0);
			ULONG rhsCount(0);
			if (lhs._import->CountEnum(lhs._e, &lhsCount) != 0 ||
				rhs._import->CountEnum(rhs._e, &rhsCount) != 0 ||
				lhsCount != rhsCount)
				return false;

			return true;
		}

		friend bool operator!=(CorEnumIterationContext const& lhs, CorEnumIterationContext const& rhs)
		{
			return !(lhs == rhs);
		}

	private:

		CorEnumIterationContext(CorEnumIterationContext const&);
		CorEnumIterationContext& operator=(CorEnumIterationContext const&);

		InterfaceType* _import;
		HCORENUM       _e;
		BufferType     _buffer;
		SizeType       _count;
		SizeType       _current;
		ArgumentType   _argument;
	};





	/// An STL-compatible input iterator wrapper for HCORENUM
	template <typename TIterationPolicy>
	class CorEnumIterator
	{
	public:

		typedef typename TIterationPolicy                PolicyType;
		typedef CorEnumIterationContext<PolicyType>      ContextType;
		typedef typename TIterationPolicy::InterfaceType InterfaceType;
		typedef typename TIterationPolicy::ValueType     ValueType;
		typedef typename TIterationPolicy::BufferType    BufferType;
		typedef typename TIterationPolicy::ArgumentType  ArgumentType;

		typedef std::input_iterator_tag iterator_category;
		typedef ValueType               value_type;
		typedef std::ptrdiff_t          difference_type;
		typedef void                    pointer;
		typedef ValueType               reference;

		CorEnumIterator(ContextType* const context = nullptr)
			: _context(context)
		{
		}

		ValueType operator*() const
		{
			return _context->Current();
		}

		CorEnumIterator& operator++()
		{
			_context->Advance();
			return *this;
		}

		CorEnumIterator& operator++(int)
		{
			_context->Advance();
			return *this;
		}

		friend bool operator==(CorEnumIterator const& lhs, CorEnumIterator const& rhs)
		{
			bool const lhsIsEnd(lhs._context == nullptr || lhs._context->IsEnd());
			bool const rhsIsEnd(rhs._context == nullptr || rhs._context->IsEnd());

			if (lhsIsEnd && rhsIsEnd)
				return true;

			if (lhsIsEnd || rhsIsEnd)
				return false;

            // To be comparable, both iterators must point into the same range.  Since this is an
            // input iterator (and is thus single-pass), if neither iterator is an end iterator,
            // both iterators must point to the same element in the range.
            return true;
		}

		friend bool operator!=(CorEnumIterator const& lhs, CorEnumIterator const& rhs)
		{
			return !(lhs == rhs);
		}

	private:

		ContextType* _context;
	};





    //
    // IMetaDataImport Iterators
    //

    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport, mdFieldDef, mdTypeDef, &IMetaDataImport::EnumEvents
    > EventCorEnumIterationPolicy;

    typedef CorEnumIterationContext<EventCorEnumIterationPolicy> EventCorEnumIterationContext;
	typedef CorEnumIterator<EventCorEnumIterationPolicy>         EventCorEnumIterator;



	typedef BaseUnaryCorEnumIterationPolicy<
		IMetaDataImport, mdFieldDef, mdTypeDef, &IMetaDataImport::EnumFields
	> FieldCorEnumIterationPolicy;

	typedef CorEnumIterationContext<FieldCorEnumIterationPolicy> FieldCorEnumIterationContext;
	typedef CorEnumIterator<FieldCorEnumIterationPolicy>         FieldCorEnumIterator;



	typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport, mdInterfaceImpl, mdTypeDef, &IMetaDataImport::EnumInterfaceImpls
    > InterfaceImplCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<InterfaceImplCorEnumIteratorPolicy> InterfaceImplCorEnumIterationContext;
	typedef CorEnumIterator<InterfaceImplCorEnumIteratorPolicy>         InterfaceImplCorEnumIterator;



    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport, mdMemberRef, mdToken, &IMetaDataImport::EnumMemberRefs
    > MemberRefCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<MemberRefCorEnumIteratorPolicy> MemberRefCorEnumIterationContext;
	typedef CorEnumIterator<MemberRefCorEnumIteratorPolicy>         MemberRefCorEnumIterator;



    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport, mdToken, mdTypeDef, &IMetaDataImport::EnumMembers
    > MemberCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<MemberCorEnumIteratorPolicy> MemberCorEnumIterationContext;
	typedef CorEnumIterator<MemberCorEnumIteratorPolicy>         MemberCorEnumIterator;



	struct MethodImplCorEnumIteratorPolicy
	{
		typedef IMetaDataImport                                               InterfaceType;
		typedef std::pair<mdToken, mdToken>                                   ValueType;
		typedef std::pair<std::array<mdToken, 128>, std::array<mdToken, 128>> BufferType;
		typedef mdTypeDef                                                     ArgumentType;

		static unsigned Advance(InterfaceType     & import,
								HCORENUM          & e,
								BufferType        & buffer,
								ArgumentType const  argument)
		{
			ULONG count;
			ThrowOnFailure(import.EnumMethodImpls(
                &e,
                argument,
                buffer.first.data(),
                buffer.second.data(),
                static_cast<ULONG>(buffer.first.size()),
                &count));
			return count;
		}

		static ValueType Get(BufferType const& buffer, SizeType const index)
		{
			return std::make_pair(buffer.first[index], buffer.second[index]);
		}
	};

    typedef CorEnumIterationContext<MethodImplCorEnumIteratorPolicy> MethodImplCorEnumIterationContext;
	typedef CorEnumIterator<MethodImplCorEnumIteratorPolicy>         MethodImplCorEnumIterator;



    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport, mdMethodDef, mdTypeDef, &IMetaDataImport::EnumMethods
    > MethodCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<MethodCorEnumIteratorPolicy> MethodCorEnumIterationContext;
	typedef CorEnumIterator<MethodCorEnumIteratorPolicy>         MethodCorEnumIterator;



    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport, mdToken, mdMethodDef, &IMetaDataImport::EnumMethodSemantics
    > MethodSemanticsCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<MethodSemanticsCorEnumIteratorPolicy> MethodSemanticsCorEnumIterationContext;
	typedef CorEnumIterator<MethodSemanticsCorEnumIteratorPolicy>         MethodSemanticsCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataImport, mdModuleRef, &IMetaDataImport::EnumModuleRefs
    > ModuleRefCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<ModuleRefCorEnumIteratorPolicy> ModuleRefCorEnumIterationContext;
	typedef CorEnumIterator<ModuleRefCorEnumIteratorPolicy>         ModuleRefCorEnumIterator;



    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport, mdParamDef, mdMethodDef, &IMetaDataImport::EnumParams
    > ParamCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<ParamCorEnumIteratorPolicy> ParamCorEnumIterationContext;
	typedef CorEnumIterator<ParamCorEnumIteratorPolicy>         ParamCorEnumIterator;



    struct PermissionSetCorEnumIteratorPolicy
	{
		typedef IMetaDataImport               InterfaceType;
		typedef mdPermission                  ValueType;
		typedef std::array<mdPermission, 128> BufferType;
		typedef std::pair<mdToken, DWORD>     ArgumentType;

		static unsigned Advance(InterfaceType     & import,
								HCORENUM          & e,
								BufferType        & buffer,
								ArgumentType const  argument)
		{
			ULONG count;
			ThrowOnFailure(import.EnumPermissionSets(
                &e,
                argument.first,
                argument.second,
                buffer.data(),
                static_cast<ULONG>(buffer.size()),
                &count));
			return count;
		}

		static ValueType Get(BufferType const& buffer, SizeType const index)
		{
			return buffer[index];
		}
	};

    typedef CorEnumIterationContext<PermissionSetCorEnumIteratorPolicy> PermissionSetCorEnumIterationContext;
	typedef CorEnumIterator<PermissionSetCorEnumIteratorPolicy>         PermissionSetCorEnumIterator;



    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport, mdProperty, mdTypeDef, &IMetaDataImport::EnumProperties
    > PropertyCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<PropertyCorEnumIteratorPolicy> PropertyCorEnumIterationContext;
	typedef CorEnumIterator<PropertyCorEnumIteratorPolicy>         PropertyCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataImport, mdSignature, &IMetaDataImport::EnumSignatures
    > SignatureCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<SignatureCorEnumIteratorPolicy> SignatureCorEnumIterationContext;
	typedef CorEnumIterator<SignatureCorEnumIteratorPolicy>         SignatureCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataImport, mdTypeDef, &IMetaDataImport::EnumTypeDefs
    > TypeDefCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<TypeDefCorEnumIteratorPolicy> TypeDefCorEnumIterationContext;
	typedef CorEnumIterator<TypeDefCorEnumIteratorPolicy>         TypeDefCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataImport, mdTypeRef, &IMetaDataImport::EnumTypeRefs
    > TypeRefCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<TypeRefCorEnumIteratorPolicy> TypeRefCorEnumIterationContext;
	typedef CorEnumIterator<TypeRefCorEnumIteratorPolicy>         TypeRefCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataImport, mdTypeSpec, &IMetaDataImport::EnumTypeSpecs
    > TypeSpecCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<TypeSpecCorEnumIteratorPolicy> TypeSpecCorEnumIterationContext;
	typedef CorEnumIterator<TypeSpecCorEnumIteratorPolicy>         TypeSpecCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataImport, mdToken, &IMetaDataImport::EnumUnresolvedMethods
    > UnresolvedMethodCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<UnresolvedMethodCorEnumIteratorPolicy> UnresolvedMethodCorEnumIterationContext;
	typedef CorEnumIterator<UnresolvedMethodCorEnumIteratorPolicy>         UnresolvedMethodCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataImport, mdToken, &IMetaDataImport::EnumUserStrings
    > UserStringCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<UserStringCorEnumIteratorPolicy> UserStringCorEnumIterationContext;
	typedef CorEnumIterator<UserStringCorEnumIteratorPolicy>         UserStringCorEnumIterator;





    //
    // IMetaDataImport2 Iterators
    //

    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport2, mdGenericParamConstraint, mdGenericParam, &IMetaDataImport2::EnumGenericParamConstraints
    > GenericParamConstraintCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<GenericParamConstraintCorEnumIteratorPolicy> GenericParamConstraintEnumIterationContext;
	typedef CorEnumIterator<GenericParamConstraintCorEnumIteratorPolicy>         GenericParamConstraintEnumIterator;



    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport2, mdGenericParam, mdToken, &IMetaDataImport2::EnumGenericParams
    > GenericParamCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<GenericParamCorEnumIteratorPolicy> GenericParamEnumIterationContext;
	typedef CorEnumIterator<GenericParamCorEnumIteratorPolicy>         GenericParamEnumIterator;



    typedef BaseUnaryCorEnumIterationPolicy<
        IMetaDataImport2, mdMethodSpec, mdToken, &IMetaDataImport2::EnumMethodSpecs
    > MethodSpecCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<MethodSpecCorEnumIteratorPolicy> MethodSpecEnumIterationContext;
	typedef CorEnumIterator<MethodSpecCorEnumIteratorPolicy>         MethodSpecEnumIterator;





    //
    // IMetaDataAssemblyImport Iterators
    //

    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataAssemblyImport, mdAssemblyRef, &IMetaDataAssemblyImport::EnumAssemblyRefs
    > AssemblyRefCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<AssemblyRefCorEnumIteratorPolicy> AssemblyRefCorEnumIterationContext;
	typedef CorEnumIterator<AssemblyRefCorEnumIteratorPolicy>         AssemblyRefCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataAssemblyImport, mdExportedType, &IMetaDataAssemblyImport::EnumExportedTypes
    > ExportedTypeCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<ExportedTypeCorEnumIteratorPolicy> ExportedTypeCorEnumIterationContext;
	typedef CorEnumIterator<ExportedTypeCorEnumIteratorPolicy>         ExportedTypeCorEnumIterator;



    typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataAssemblyImport, mdFile, &IMetaDataAssemblyImport::EnumFiles
    > FileCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<FileCorEnumIteratorPolicy> FileCorEnumIterationContext;
	typedef CorEnumIterator<FileCorEnumIteratorPolicy>         FileCorEnumIterator;



     typedef BaseNullaryCorEnumIterationPolicy<
        IMetaDataAssemblyImport, mdManifestResource, &IMetaDataAssemblyImport::EnumManifestResources
    > ManifestResourceCorEnumIteratorPolicy;

    typedef CorEnumIterationContext<ManifestResourceCorEnumIteratorPolicy> ManifestResourceCorEnumIterationContext;
	typedef CorEnumIterator<ManifestResourceCorEnumIteratorPolicy>         ManifestResourceCorEnumIterator;

} } }

#endif // #if defined(CXXREFLECT_WINDOWSRUNTIME_UTILITIES_STANDALONE) || defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION)

#endif
