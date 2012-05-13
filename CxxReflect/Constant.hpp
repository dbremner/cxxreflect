#ifndef CXXREFLECT_CONSTANT_HPP_
#define CXXREFLECT_CONSTANT_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect {

    /// \ingroup cxxreflect_public_interface
    ///
    /// @{

    /// A constant value from metadata, usually associated with a field, property, or parameter
    class Constant
    {
    public:

        enum class Kind
        {
            /// Indicates the constant has an unknown kind and attempts to get its value will fail
            Unknown,

            Boolean,
            Char,
            Int8,
            UInt8,
            Int16,
            UInt16,
            Int32,
            UInt32,
            Int64,
            UInt64,
            Float,
            Double,
            String,

            /// Indicates the constant has class type, which means its value is a nullptr
            Class
        };

        Constant();

        /// Gets the `Kind` of this constant.
        ///
        /// If this object is uninitialized or if there is an error, returns `Unknown`.
        ///
        /// \nothrows
        /// \todo Make sure that this actually won't throw :-)
        Kind GetKind() const;

        bool          AsBoolean() const;
        wchar_t       AsChar()    const;
        std::int8_t   AsInt8()    const;
        std::uint8_t  AsUInt8()   const;
        std::int16_t  AsInt16()   const;
        std::uint16_t AsUInt16()  const;
        std::int32_t  AsInt32()   const;
        std::uint32_t AsUInt32()  const;
        std::int64_t  AsInt64()   const;
        std::uint64_t AsUInt64()  const;
        float         AsFloat()   const;
        double        AsDouble()  const;

        // TODO StringReference AsString() const;
        
        bool IsInitialized() const;

    public: // Internal Members

        /// Constructs a `Constant` object from the given row in the **Constant** table
        ///
        /// \todo More doxygen
        Constant(Metadata::FullReference const& constant, InternalKey);

        /// Gets the `Constant` for the specified `parent`
        ///
        /// The `parent` must be a reference to a row in the **Field**, **Property**, or **Param**
        /// table.  It must be initialized and must refer to a valid row in a valid database.
        ///
        /// Note that not all rows in those three tables own constant values; if `parent` does not
        /// have a constant, an empty, uninitialized `Constant` is returned.
        ///
        /// \param   parent The row for which to get its constant value
        /// \returns The constant for the specified `parent`
        /// \throws  RuntimeError If `parent` is not initialized or is invalid
        static Constant For(Metadata::FullReference const& parent, InternalKey);

    private:

        void AssertInitialized() const;

        Metadata::ConstantRow GetConstantRow() const;

        /// A reference to the row for this constant in the **Constant** table
        Metadata::FullReference _constant;
    };

    /// @}

}

#endif
