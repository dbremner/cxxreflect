
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //





// This program generates a .sln file containing all of the CxxReflect projects.
//
// The Solution configuration and platform management UI in Visual Studio does not handle complex
// sets of build configurations well and is extremely difficult to use with the CxxReflect Solution
// because we have a large number of configurations (due to the incompatibility of /ZW and non-/ZW
// translation units) and because different projects only support a subset of the complete set of
// configurations.  Unloading or loading projects and adding or removing projects from the Solution
// can cause the IDE to mix up which projects get built under each configuration.
//
// For CxxReflect, this should be very simple:  each project supports a subset of configurations and
// we only want to build each project if the Solution configuration is supported by that project.
// So, we use this program to trawl the CxxReflect Solution directory for .vcxproj files, enumerate
// the configurations supported by each project, and generate a new Solution file that defines the
// correct set of Solution configurations and configures them correctly.





#pragma warning(disable: 4244) // Ignore narrowing conversion warnings

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <objbase.h>
#include <wincrypt.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")

namespace msbuild
{
    using namespace Microsoft::Build::Evaluation;
}

namespace sys
{
    using namespace System;
    using namespace System::Collections::Generic;
    using namespace System::Runtime::InteropServices;
}

namespace
{
    typedef unsigned char*        byte_iterator;
    typedef unsigned char const*  const_byte_iterator;
    typedef std::tr2::sys::path   path_type;
    typedef std::set<path_type>   path_set;
    typedef std::string           string_type;
    typedef std::set<string_type> string_set;
    typedef std::ofstream         output_stream_type;

    using std::tr2::sys::directory_iterator;
    using std::tr2::sys::recursive_directory_iterator;

    // Well-known GUIDs to be written to the Solution file
    string_type const folder_flavor_guid ("{2150E333-8FDC-42A3-9474-1A3956D46DE8}");
    string_type const vcxproj_flavor_guid("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}");
    string_type const miscellaneous_guid ("{2D96883D-3EC7-475A-8506-C13FAB2F3EBE}");

    // The supported set of project file extensions
    string_set const project_file_extensions([]() -> string_set
    {
        string_set extensions;
        extensions.insert(".vcxproj");
        return extensions;
    }());





    auto print_usage() -> void
    {
        std::cout << "Synchronizes a .sln file with the project files it references,\n"
                  << "\n"
                  << "create_sln_from_directories {0} {1}"
                  << "  {1}: The root directory of the Solution.\n"
                  << "  {2}: The name of the Solution file to be generated." << std::endl;
    }

    template <typename Set, typename Element>
    auto set_contains(Set const& s, Element const& e) -> bool
    {
        return s.find(e) != s.end();
    }

    auto create_guid(string_type const& text) -> string_type
    {
        class scope_guard
        {
        public:

            typedef std::function<void()> function_type;

            explicit scope_guard(function_type f)
                : _f(std::move(f))
            {
            }

            ~scope_guard()
            {
                if (_f != nullptr) { _f();}
            }

        private:

            function_type _f;
        };

        // Compute the SHA1 hash of the text:
        HCRYPTPROV provider(0);
        scope_guard cleanup_provider([&](){ if (provider) { ::CryptReleaseContext(provider, 0); } });
        if (!::CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
            throw std::runtime_error("failed to acquire cryptographic context");

        HCRYPTHASH hash(0);
        scope_guard cleanup_hash([&](){ if (hash) { ::CryptDestroyHash(hash); } });
        if (!::CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash))
            throw std::runtime_error("failed to create cryptographic hash");

        const_byte_iterator const first(reinterpret_cast<const_byte_iterator>(text.c_str()));
        const_byte_iterator const last (first + text.size());

        if (!::CryptHashData(hash, first, static_cast<DWORD>(last - first), 0))
            throw std::runtime_error("failed to hash data");

        std::array<unsigned char, 20> result = { 0 };
        DWORD resultLength(static_cast<DWORD>(result.size()));
        if (!::CryptGetHashParam(hash, HP_HASHVAL, result.data(), &resultLength, 0) || resultLength != 20)
            throw std::runtime_error("failed to obtain hash value");

        // Use the first 16 bytes of the SHA1 hash as the GUID and return the string:
        GUID guid;
        std::copy(begin(result), begin(result) + sizeof guid, reinterpret_cast<byte_iterator>(&guid));

        std::vector<wchar_t> guid_string(40);
        if (::StringFromGUID2(guid, guid_string.data(), static_cast<int>(guid_string.size())) == 0)
            throw std::runtime_error("failed to stringify guid");

        std::vector<char> narrow_guid_string(begin(guid_string), end(guid_string));
        return narrow_guid_string.data();
    }

    auto marshal_string(sys::String^ source) -> string_type
    {
        class string_marshaler
        {
        public:

            string_marshaler(sys::String^ source)
                : _characters(sys::Marshal::StringToHGlobalAnsi(source).ToPointer())
            {
            }

            ~string_marshaler()
            {
                sys::Marshal::FreeHGlobal(sys::IntPtr(_characters));
            }

            auto characters() -> string_type
            {
                return string_type(static_cast<char*>(_characters));
            }

        private:

            void* _characters;
        };

        return string_marshaler(source).characters();
    }

    auto make_relative_path(path_type const& from, path_type const& to) -> path_type
    {
        // This is a hack, but it is sufficient for creating the CxxReflect Solution
        string_type const& from_string(from.string());
        return to.string().substr(from_string.size() + (from_string.back() == '/' ? 0 : 1), string_type::npos);
    }

    auto enumerate_project_paths(path_type const& solution_root) -> path_set
    {
        path_set project_paths;

        std::for_each(recursive_directory_iterator(solution_root),
                      recursive_directory_iterator(),
                      [&](path_type const& current_path)
        {
            if (set_contains(project_file_extensions, current_path.extension()))
                project_paths.insert(current_path);
        });

        return project_paths;
    }





    class project_info
    {
    public:

        project_info(path_type const& solution_root, path_type const& project_file)
            : _absolute_path(project_file), 
              _relative_path(make_relative_path(solution_root, project_file))
        {
            sys::IDictionary<sys::String^, sys::String^>^ msbuild_properties(gcnew sys::Dictionary<sys::String^, sys::String^>());
            msbuild_properties->Add(L"SolutionDir", gcnew sys::String(solution_root.string().c_str()));

            msbuild::Project^ msbuild_project(gcnew msbuild::Project(
                gcnew sys::String(project_file.string().c_str()),
                msbuild_properties,
                nullptr));

            _guid = marshal_string(msbuild_project->GetPropertyValue("ProjectGuid"));
            _name = marshal_string(msbuild_project->GetPropertyValue("ProjectName"));

            sys::IEnumerable<msbuild::ProjectItem^>^ project_configuration_items(msbuild_project->GetItems("ProjectConfiguration"));
            for each (msbuild::ProjectItem^ project_configuration_item in project_configuration_items)
                _configurations.insert(marshal_string(project_configuration_item->EvaluatedInclude));
        }

        auto absolute_path()  const -> path_type   const& { return _absolute_path;  }
        auto relative_path()  const -> path_type   const& { return _relative_path;  }
        auto guid()           const -> string_type const& { return _guid;           }
        auto name()           const -> string_type const& { return _name;           }
        auto configurations() const -> string_set  const& { return _configurations; }

    private:

        path_type   _absolute_path;
        path_type   _relative_path;

        string_type _guid;
        string_type _name;

        string_set  _configurations;
    };

    typedef std::vector<project_info> project_info_sequence;

    auto create_project_infos(path_type const& solution_root, path_set const& project_paths) -> project_info_sequence
    {
        std::vector<project_info> project_infos;
        std::transform(begin(project_paths), end(project_paths), back_inserter(project_infos), [&](string_type const& project_path)
        {
            return project_info(solution_root, project_path);
        });

        return project_infos;
    }





    auto write_solution_file(output_stream_type& os, path_type const& solution_root, project_info_sequence const& projects) -> void
    {
        // Write the header:
        os << '\n';
        os << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
        os << "# Visual Studio 2012\n";

        // Write Project entries for each Project to be defined in the Solution:
        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            os << "Project(\"" << vcxproj_flavor_guid           << "\") = "
               << "\"" << project.name()                        << "\", "
               << "\"" << project.relative_path().file_string() << "\", "
               << "\"" << project.guid()                        << "\"\n"
               << "EndProject\n";
        });

        // Each virtual folder also requires a Project entry.  For real Projects, we use the GUID
        // defined in the Project.  For virtual folders, we generate new GUIDs each time this
        // program is run.
        typedef std::map<path_type, string_type> folder_guid_map;
        folder_guid_map folder_guids;
        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            path_type current_path(project.relative_path().parent_path());
            while (current_path.has_parent_path())
            {
                current_path = current_path.parent_path();
                if (set_contains(folder_guids, current_path))
                    continue;

                folder_guids.insert(std::make_pair(current_path, create_guid(current_path.string())));
            }
        });

        // Write Project entries for each virtual folder:
        std::for_each(begin(folder_guids), end(folder_guids), [&](folder_guid_map::value_type const& folder_guid)
        {
            os << "Project(\"" << folder_flavor_guid << "\") = "
               << "\"" << folder_guid.first.leaf()   << "_\", "
               << "\"" << folder_guid.first.leaf()   << "_\", "
               << "\"" << folder_guid.second         << "\"\n"
               << "EndProject\n";
        });

        // Write a Project entry for the Miscellaneous virtual folder, and include all of the files
        // from the root directory of the Solution:
        os << "Project(\"" << folder_flavor_guid  << "\") = "
           << "\"miscellaneous_\", "
           << "\"miscellaneous_\", "
           << "\"" << miscellaneous_guid << "\"\n"
           << "\tProjectSection(SolutionItems) = preProject\n";

        string_set seen_root_files;
        std::for_each(directory_iterator(solution_root), directory_iterator(), [&](path_type const& path)
        {
            if (!is_directory(solution_root / path)  &&
                !set_contains(seen_root_files, path) &&
                path.string().size() > 0             &&
                path.string()[0] != '\\'             &&
                path.extension() != ".sln"           &&
                path.extension() != ".swp"           &&
                path.extension() != ".suo")
            {
                os << "\t\t" << path.filename() << " = " << path.filename() << "\n";
                seen_root_files.insert(path.filename());
            }
        });

        os << "\tEndProjectSection\n"
           << "EndProject\n";

        // Accumulate the complete set of Configurations to be defined by the Solution:
        string_set solution_configurations;
        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            std::for_each(begin(project.configurations()), end(project.configurations()), [&](string_type const& configuration)
            {
                solution_configurations.insert(configuration);
            });
        });

        // Start the Global group that defines all of the configurations and mappings
        os << "Global\n";

        // Write the Solution Configurations:
        os << "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
        std::for_each(begin(solution_configurations), end(solution_configurations), [&](string_type const& configuration)
        {
            os << "\t\t" << configuration << " = " << configuration << "\n";
        });
        os << "\tEndGlobalSection\n";

        // Write the Project-to-Solution Configuration mappings:
        os << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            std::for_each(begin(solution_configurations), end(solution_configurations), [&](string_type const& configuration)
            {
                // If the Project supports this configuration, we generate both an ActiveCfg and a
                // Build.0 entry; otherwise we generate only an ActiveCfg entry.  It appears that
                // ActiveCfg specifies which configuration appears as selected in the UI in Visual
                // Studio; Build.0 specifies which configuration is actually built.
                //
                // Visual Studio will muck with the Project file if there is no ActiveCfg, which is
                // why we always generate it.  If there is only an ActiveCfg but no Build.0, the
                // Project will not be built in that Solution configuration.
                if (set_contains(project.configurations(), configuration))
                {
                    os << "\t\t" << project.guid() << "." << configuration << ".ActiveCfg = " << configuration << "\n";
                    os << "\t\t" << project.guid() << "." << configuration << ".Build.0 = "   << configuration << "\n";
                }
                else
                {
                    // It doesn't matter which configuration we pick here because it won't be built:
                    os << "\t\t" << project.guid() << "." << configuration << ".ActiveCfg = " << *project.configurations().begin() << "\n";
                }
            });
        });
        os << "\tEndGlobalSection\n";
        
        // Write the Solution Properties Global Section.  I have no idea what this does.
        os << "\tGlobalSection(SolutionProperties) = preSolution\n"
           << "\t\tHideSolutionNode = FALSE\n"
           << "\tEndGlobalSection\n";

        // Write the Project nesting nodes.  Note that we don't generate a virtual folder for the
        // physical folder in which each Project is defined:  this is because each Project is
        // defined in its own physical folder.
        os << "\tGlobalSection(NestedProjects) = preSolution\n";
        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            if (!project.relative_path().has_parent_path())
                return;
            
            path_type const& parent_path(project.relative_path().parent_path());
            if (parent_path.has_parent_path())
            {
                path_type const& grandparent_path(parent_path.parent_path());
                os << "\t\t" << project.guid() << " = " << folder_guids.find(grandparent_path)->second << "\n";
            }
            else
            {
                os << "\t\t" << project.guid() << " = " << folder_guids.find(parent_path)->second << "\n";
            }
        });

        // Write the virtual folder nesting nodes:
        std::for_each(begin(folder_guids), end(folder_guids), [&](folder_guid_map::value_type const& folder_guid)
        {
            if (!folder_guid.first.has_parent_path())
                return;

            path_type const& parent_path(folder_guid.first.parent_path());
                os << "\t\t" << folder_guid.second << " = " << folder_guids.find(parent_path)->second << "\n";
        });

        os << "\tEndGlobalSection\n";
        
        // End the Global section; this is the end of the Solution file.  If all went well, we
        // should be finished.
        os << "EndGlobal\n";
    }

    auto create_solution_file(path_type const& solution_root, string_type const& solution_name) -> void
    {
        path_set const project_paths(enumerate_project_paths(solution_root));

        project_info_sequence const project_infos(create_project_infos(solution_root, project_paths));

        output_stream_type os((solution_root / path_type(solution_name)).string());
        write_solution_file(os, solution_root, project_infos);
    }
}

auto main(int argc, char** argv) -> int
{
    try
    {
        std::vector<std::string> const arguments(argv, argv + argc);
        if (arguments.size() != 3)
            return print_usage(), EXIT_FAILURE;

        create_solution_file(arguments[1], arguments[2]);
    }
    catch (std::exception const& e)
    {
        std::cout << "Uh oh.  An exception occurred during execution :'(\n"
                  << e.what();

        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cout << "Uh oh.  An unknown exception occurred during execution :'(\n";

        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
