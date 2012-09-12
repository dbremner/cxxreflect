
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// NOTE:  THIS HEADER IS FOR DOCUMENTATION ONLY.  IT SHOULD NEVER BE INCLUDED IN ANY TRANSLATION
// UNIT.  IT HAS A .HPP EXTENSION FOR SYNTAX COLORING AND DOXYGEN GENERATION ONLY.

#error This header should not be included in any translation unit. :'(

/// \mainpage Getting Started Guide
///
/// CxxReflect is a native reflection library for use with CLI metadata files, like those used by
/// the .NET Framework (managed assemblies) and the Windows Runtime (Windows Metadata, or WinMD
/// files).  It's composed of several component libraries:
///
/// * **Core**:  Core utility types and functions that complement the C++ Standard Library and are
///   shared by the other components of the library
///
/// * **Metadata**:  A library for loading CLI metadata databases and for parsing signatures.  This
///   library ingests the raw binary metadata database and presents a relational database interface
///   for accessing the contents of the database.
///
/// * **Reflection**:  A library built atop the Metadata library that loads assemblies and presents
///   the metadata from the assemblies via an interface very similar to the .NET reflection APIs.
///
/// * **Windows Runtime**:  A library that integrates the Reflection and Metadata libraries with the
///   Windows Runtime for loading types from a packaged application.  This allows for runtime
///   reflection of inspectable objects and for dynamic invocation and instantiation via metadata.
///
/// This Getting Started Guide focuses on usage with the Windows Runtime.  Future documentation will
/// describe how the library may be used with ordinary CLI assemblies without the Windows Runtime.
///
///
///
/// # Copyright
///
/// **Copyright James P. McNellis 2011 - 2012.**
///
/// **Distributed under the Boost Software License, Version 1.0.**
///
/// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
///
///
/// # Getting the Sources
///
/// The CxxReflect library sources are hosted on CodePlex at http://cxxreflect.codeplex.com.  The
/// sources are accessible via Mercurial at the following location:
///
///     https://hg.codeplex.com/cxxreflect
///
/// You can either pull the latest sources from Mercurial, or you can download a zip file containing
/// the latest sources by clicking on the "Download" link on the following page:
///
///     http://cxxreflect.codeplex.com/SourceControl/list/changesets
///
/// At this time there is no binary release of the library; you must build it yourself (though, this
/// process is quite painless).
///
///
///
/// # Building the Sources
///
/// There are three top-level folders in the CxxReflect sources:
///
/// * **cxxreflect**: Contains the entire CxxReflect library
///
/// * **utility**: Contains a set of utility projects, some of which are required to build the
///   CxxReflect library itself
///
/// * **test**: Contains a handful of test projects that make use of the CxxReflect library,
///   including unit tests and other test programs
///
/// The projects can only be built using the Visual Studio 2012 Release Candidate.  All of the
/// projects share common configuration settings, defined in cxxreflect\cxxreflect.props.
///
/// You'll want to make sure to change the settings to match the ones that you are using in your
/// projects (especially settings like Link-Time Code Generation, and for Debug builds, the iterator
/// debugging settings).  CxxReflect uses default settings by default, so if you haven't changed any
/// important settings in your project, you shouldn't have to reconfigure CxxReflect.
///
/// There are four solution configurations:  Debug, Debug(ZW), Release, and Release(ZW).  If you are
/// using the library in a C++/CX project, you'll need to build and link against the appropriate ZW
/// configuration of the CxxReflect library (the configurations are called ZW because /ZW is the
/// compiler flag that enables C++/CX).
///
/// If you are using low-level C++ (C++ without hats), you'll need to use one of the non-ZW
/// configurations.  If you try to link with the wrong configuration, you'll get a linker error.
///
/// Before building one of the /ZW configurations, please build one of the non-/ZW configurations.
///
/// Currently, CxxReflect can only be built as a static library.
/// 
///
///
/// # Using the Library in a C++/CX Project
///
/// If you've built one of the ZW configurations of CxxReflect, you'll need to add the following to
/// your C++/CX project's configuration to use the CxxReflect library:
///
/// * Under Linker -> General -> Additional Library Directories, add the directory in which the 
///   CxxReflect.lib file is located.
///
/// * Under Linker -> Input -> Additional Dependencies, add CxxReflect.lib to the list.
///
/// * Under C/C++ -> General -> Additional Include Directories, add the directory that contains the
///   CxxReflect solution.  This should be one directory above where the CxxReflect header files are
///   located.
///
/// (Note that, if your are confused, you can take a look at what the WRTestApp project does; it
/// makes use of the CxxReflect library to look at and use some of the types from WRLibrary.)
///
/// To use the library, you need only include a single header file:
///
///     #include <cxxreflect/cxxreflect.hpp>
///
/// This includes the entire CxxReflect library.  For simplicity, most of the rest of the example
/// code also assumes that you've defined a namespace named `cxr`, defined as follows:
///
///     namespace cxr
///     {
///         using namespace cxxreflect::core;
///         using namespace cxxreflect::metadata;
///         using namespace cxxreflect::reflection;
///         using namespace cxxreflect::windows_runtime;
///     }
///
/// You are free to use the CxxReflect library however you see fit in your own code; we use this
/// alias for brevity only.
///
/// The first thing you'll need to do in your program is start initialization of the global Windows
/// Runtime type universe.  This is done by calling the following function as early as you can:
///
///     cxr::begin_initialization();
///
/// This will start initialization of the global Windows Runtime type universe on a worker thread
/// and will return immediately.  In a XAML application, it is recommended that you call this in the
/// constructor of your Application type.  If your project has a `main` function, you should begin
/// this initialization as early as you can in `main`.
///
/// Most of the CxxReflect API calls will block until the Windows Runtime type universe has finished
/// initializing.  This is a fairly quick process, but in C++/CX, you cannot ever block on an STA
/// thread, so you must be careful not to call the CxxReflect API directly from the UI thread (or any
/// other STA).  To make this easy, CxxReflect provides function to help you out:
///
///     cxr::when_initialized_call([&]
///     {
///         // Code goes here
///     });
///
/// `when_initialized_call` will enqueue the provided function for execution when initialization is
/// complete.  Note that the provided function is not marshaled back to the calling thread, so you
/// will need to do that marshaling yourself, if you need.
///
///
///
/// # Well, what can I do with it?
///
/// It's probably easier to start with what you can't do:
///
/// * CxxReflect only works on Windows Runtime types, so you can't use it on ordinary C++ types.  It
///   supports Windows Runtime reference types and value types.
///
/// * CxxReflect only works with public types, so you cannot inspect anything that is declared
///   private.
///
/// * CxxReflect only works with types from Windows Runtime components.  It does not work with types
///   defined in an executable (.exe) file.
///
/// Eventually CxxReflect will be able to provide some support for private types and for types from
/// executables, but for the moment, these are the restrictions.
///
/// There are also some features that are not well-supported or not yet supported, notably, generics
/// are not yet supported and ARM function invocation is not yet supported.  There are also some
/// parts of the type system that are also not supported, like properties and events.  All of these
/// will be supported eventually; these features just haven't been implemented yet.
///
/// The following examples will demonstrate some of the functionality and features that are working
/// today.
///
///
///
/// # Example Usage
///
/// All of the following examples assume usage of a C++/CX Windows Runtime component in which we
/// have some types defined and a C++/CX application that makes use of these types.  For brevity,
/// we assume that you've already included the CxxReflect header and that you've already initialized
/// the type system.  We assume that the name of the Windows Runtime component is "WRLibrary".
///
/// 
///
/// ## Getting Type Information
///
/// Ok, we've written our library, and we've got this awesome type that does awesome things:
///
///     namespace WRLibrary
///     {
///         public ref class MyAwesomeType
///         {
///         public:
///     
///             void DoSomethingAwesome()     { }
///             void DoSomethingLessAwesome() { }
///             void DoSomethingMoreAwesome() { }
///     
///         };
///     }
///
/// How would we get information about its type?  Well, we start by getting its `type` object,
/// which, much like `System.Type` in the .NET Framework, provides information about its type:
///
///     cxr::type const awesome_type(cxr::get_type(L"WRLibrary.MyAwesomeType"));
///
/// We can walk this type's type hierarchy, though the type hierarchy for this type isn't very
/// interesting:
///
///     std::wcout << L"Type hierarchy of WRLibrary.MyAwesomeType:\n";
///     cxr::type base(awesome_type);
///     while (base)
///     {
///         std::wcout << base.full_name().c_str() << <"\n";
///
///         base = base.base_type();
///     }
///
/// This will print to the console:
///
///     Type hierarchy of WRLibrary.MyAwesomeType:
///     WRLibrary.MyAwesomeType
///     Platform.Object
///
/// We can also enumerate the type's interfaces:
///
///     std::wcout << L"Interfaces implemented by WRLibrary.MyAwesomeType:\n";
///     std::for_each(awesome_type.begin_interfaces(), awesome_type.end_interfaces(), [&](cxr::type const& iface)
///     {
///         std::wcout << iface.full_name().c_str() << L"\n";
///     });
///
/// Again, this type isn't particularly interesting, but this will print to the console:
///
///     Interfaces implemented by WRLibrary.MyAwesomeType:
///     WRLibrary.__IMyAwesomeTypePublicNonVirtuals
///
/// (`__IMyAwesomeTypePublicNonVirtuals` is an interface fabricated by the C++ compiler.)
///
/// We can enumerate the type's methods too:
///
///     std::wcout << L"Methods of WRLibrary.MyAwesomeType:\n";
///     auto const first_method(awesome_type.begin_methods(cxr::binding_attribute::all_instance));
///     auto const last_method(awesome_type.end_methods());
///     std::for_each(first_method, last_method, [&](cxr::method const& m)
///     {
///         std::wcout << m.name() << L"\n";
///     });
///
/// Which prints:
///
///     Methods of WRLibrary.MyAwesomeType:
///     DoSomethingAwesome
///     DoSomethingLessAwesome
///     DoSomethingMoreAwesome
///
/// There is much other information that you can get from the `type` object, but to keep from boring
/// you, I'll stop here and let you explore on your own.  Much of the core functionality works for
/// types, interfaces, and methods.  As noted above, generics are not well supported, nor are events
/// or properties.
///
///
///
/// ## Enumerating the Enumerators of an Enumeration
///
/// Let's say that we've defined an enumeration in our component for specifying the days of the
/// week:
///
///     namespace WRLibrary
///     {
///         public enum class DayOfWeek
///         {
///             Sunday    = 0,
///             Monday    = 1,
///             Tuesday   = 2,
///             Wednesday = 3,
///             Thursday  = 4,
///             Friday    = 5,
///             Saturday  = 6
///         };
///     }
///
/// This is how we would get this list of enumerators and print their names and values:
///
///     auto enumerators(cxr::get_enumerators(cxr::get_type(L"WRLibrary.DayOfWeek")));
/// 
///     // The order of the enumerators is unspecified, so we'll sort them by value:
///     std::sort(begin(enumerators), end(enumerators), cxr::enumerator_unsigned_value_less_than());
/// 
///     // Print the enumerators:
///     std::for_each(begin(enumerators), end(enumerators), [&](cxr::enumerator const& e)
///     {
///         std::wcout << e.name().c_str() << L": " << e.unsigned_value() << L"\n";
///     });
///
/// This code will output to the debug console:
///
///     Sunday:  0
///     Monday:  1
///     Tuesday:  2
///     Wednesday:  3
///     Thursday:  4
///     Friday:  5
///     Saturday:  6
///
/// That's not very pretty looking, but that's the extent of my output-beautifying abilities.  Plus,
/// it demonstrates the capability pretty clearly, no?
///
///
///
/// ## Getting the Implementers of an Interface
///
/// The following example will get all of the implementers of `IDependencyObject`:
///
///     auto const dependency_object_types = cxr::get_implementers<IDependencyObject>();
///
/// It is left as an exercise to the reader to print the results or otherwise use them.  :-)
///
/// A word of warning:  This is currently very, very slow for types defined in the Windows platform.
/// Future work is planned to substantially improve the filtering of types to make this much faster.
///
///
///
/// ## Basic Object Creation
///
/// Inspecting types is all well and good, but instantiating types is much more fun.  We'll need an
/// interface and a few implementers for this:
///
///     namespace WRLibrary
///     {
///         public interface class IProvideANumber
///         {
///             default::int32 GetNumber();
///         };
///     
///         public ref class ProviderOfZero sealed : IProvideANumber
///         {
///         public:
///     
///             default::int32 GetNumber() { return 0; }
///         };
///     
///         public ref class ProviderOfOne sealed : IProvideANumber
///         {
///         public:
///     
///             default::int32 GetNumber() { return 1; }
///         };
///     
///         public ref class ProviderOfTheAnswer sealed : IProvideANumber
///         {
///         public:
///     
///             default::int32 GetNumber() { return 42; }
///         };
///     }
///
/// These types are very simple, but they are sufficient to demonstrate some basic object creation.
///
///     // This will get all the implementers of IProvideANumber, instantiate them, and print their
///     // type and number:
///     auto const types(cxr::get_implementers<WRLibrary::IProvideANumber>());
///     std::for_each(begin(types), end(types), [](cxr::type const& type)
///     {
///         if (!cxr::is_default_constructible(type))
///             return;
/// 
///         auto const instance(cxr::create_instance<WRLibrary::IProvideANumber>(type));
///
///         std::wcout << cxr::get_type_of(instance).full_name().c_str() << L": " << instance->GetNumber() << L"\n";
///     });
///
/// If you run this, it will print to the debug console:
///
///     WRLibrary.ProviderOfZero: 0
///     WRLibrary.ProviderOfOne: 1
///     WRLibrary.ProviderOfTheAnswer: 42
///
/// Nice!
///
///
///
/// ## Object Construction with Constructor Arguments
///
/// The above example only instantiates default-constructible objects, which it turns out, is pretty
/// easy in the Windows Runtime, even without using CxxReflect (oh, maybe I shouldn't have said
/// that! :-D).  Instantiation via default constructor basically boils down to a call to the
/// platform API function `RoActivateInstance`, which is a lot like `CoCreateInstance` except that
/// it takes a type name instead of a GUID.
///
/// Instantiation with constructor arguments is a bit more difficult, but we can handle that too.
/// We'll use another implementer of `IProvideANumber` to demonstrate:
///
///     namespace WRLibrary
///     {
///         public ref class UserProvidedNumber sealed : IProvideANumber
///         {
///         public:
///     
///             UserProvidedNumber(default::int32 value)
///                 : _value(value)
///             {
///             }
/// 
///             default::int32 GetNumber() { return _value; }
/// 
///         private:
/// 
///             default::int32 _value;
///         };
///     }
///
/// To instantiate this type, we'll need to pass an integer to its constructor.  This is done using
/// another overload of CxxReflect's `CreateInstance`:
///
///     cxr::type const type(cxr::get_type(L"WRLibrary.UserProvidedNumber"));
///     for (int i(0); i < 3; ++i)
///     {
///         auto const instance(cxr::create_instance<WRLibrary::IProvideANumber>(type, i));
///
///         std::wcout << cxr::get_type_of(instance).full_name().c_str() << L": " << instance->GetNumber() << L"\n";
///     }
///
/// This program will output:
///
///     WRLibrary.UserProvidedNumber:  0
///     WRLibrary.UserProvidedNumber:  1
///     WRLibrary.UserProvidedNumber:  2
///
/// Note that this example requires dynamic function invocation, which is currently only supported 
/// on x86 and x64.  ARM support is forthcoming.  Further, dynamic invocation is currently only
/// supported for object construction.  Eventually we will also support dynamic invocation of
/// arbitrary methods on types.
///
///
///
/// # A Few Comments
///
/// **CxxReflect is a work in progress.**  It has bugs, and in some areas it still doesn't perform
/// very well.  In debug builds, the library makes heavy use of assertions to verify assumptions;
/// if you hit one of these assertions or if you find a bug, please report it on the CodePlex site,
/// with a project or binary that demonstrates the problem, if possible.
///
/// **Why can't I do _________?**  If you have feature requests, please open an issue on the
/// CodePlex site.  Feature requests get top priority (after fixing known bugs, of course).
///
/// **Can I use CxxReflect from multiple threads?**  CxxReflect may be called concurrently from
/// multiple threads.  It is internally synchronized.
///
/// **Why is it called CxxReflect?** It was actually called C++Reflect, but the +'s fell over, so
/// now it's CxxReflect.  Also, the C++ compiler will get angry with you if you try using a + in a
/// namespace name.
///
/// I'll be blogging about the development of CxxReflect on my blog, with other examples and some
/// discussion as to what is required to make various features work:
///
///     http://seaplusplus.com
///
/// Happy Reflecting!
///
