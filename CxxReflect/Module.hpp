#ifndef CXXREFLECT_MODULE_HPP_
#define CXXREFLECT_MODULE_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect {

    /// \ingroup cxxreflect_public_interface
    ///
    /// @{





    class Module
    {
    public:

        typedef Detail::InstantiatingIterator<Metadata::RowReference, Type, Module> TypeIterator;

        Module();

        /// Gets the assembly that owns this module
        ///
        /// \nothrows
        Assembly GetAssembly() const;

        SizeType GetMetadataToken() const;

        StringReference GetName() const;
        StringReference GetPath() const;

        CustomAttributeIterator BeginCustomAttributes() const;
        CustomAttributeIterator EndCustomAttributes()   const;

        // TODO BeginFields()
        // TODO EndFields()

        // TODO BeginMethods()
        // TODO EndMethods()

        TypeIterator BeginTypes() const;
        TypeIterator EndTypes()   const;

        bool IsInitialized() const;
        bool operator!()     const;

        friend bool operator==(Module const& lhs, Module const& rhs);
        friend bool operator< (Module const& lhs, Module const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Module)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Module)

    public: // Internal Members

        Module(Detail::ModuleContext const* context, InternalKey);
        Module(Assembly const& assembly, SizeType moduleIndex, InternalKey);

        Detail::ModuleContext const& GetContext(InternalKey) const;

    private:

        void AssertInitialized() const;

        Detail::ValueInitialized<Detail::ModuleContext const*> _context;
    };

    /// @}

}

#endif
