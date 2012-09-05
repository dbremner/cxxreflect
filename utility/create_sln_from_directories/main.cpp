
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This program generates a .sln file containing all of the CxxReflect projects.

#pragma warning(disable: 4244) // narrowing conversions

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <objbase.h>

#pragma comment(lib, "ole32.lib")

namespace mbe
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
    typedef std::tr2::sys::path   path_type;
    typedef std::set<path_type>   path_set;
    typedef std::string           string_type;
    typedef std::set<string_type> string_set;
    typedef std::ofstream         output_stream_type;

    using std::tr2::sys::directory_iterator;
    using std::tr2::sys::recursive_directory_iterator;

    string_type const folder_flavor_guid ("{2150E333-8FDC-42A3-9474-1A3956D46DE8}");
    string_type const vcxproj_flavor_guid("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}");
    string_type const miscellaneous_guid ("{2D96883D-3EC7-475A-8506-C13FAB2F3EBE}");

    string_set const project_file_extensions([]() -> string_set
    {
        string_set extensions;
        extensions.insert(".vcxproj");
        return extensions;
    }());

    auto create_guid() -> string_type
    {
        GUID guid((GUID()));
        if (FAILED(::CoCreateGuid(&guid)))
            throw std::runtime_error("failed to create guid");

        std::vector<wchar_t> guid_string(40);
        if (::StringFromGUID2(guid, guid_string.data(), guid_string.size()) == 0)
            throw std::runtime_error("failed to stringify guid");

        std::vector<char> narrow_guid_string(begin(guid_string), end(guid_string));
        return narrow_guid_string.data();
    }

    auto print_usage() -> void
    {
        std::cout << "Synchronizes a .sln file with the project files it references" << std::endl;
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
        string_type const& from_string(from.string());
        return to.string().substr(from_string.size() + (from_string.back() == '/' ? 0 : 1), string_type::npos);
    }

    template <typename Set, typename Element>
    auto set_contains(Set const& s, Element const& e) -> bool
    {
        return s.find(e) != s.end();
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

            mbe::Project^ msbuild_project(gcnew mbe::Project(
                gcnew sys::String(project_file.string().c_str()),
                msbuild_properties,
                nullptr));

            _guid = marshal_string(msbuild_project->GetPropertyValue("ProjectGuid"));
            _name = marshal_string(msbuild_project->GetPropertyValue("ProjectName"));

            sys::IEnumerable<mbe::ProjectItem^>^ project_configuration_items(msbuild_project->GetItems("ProjectConfiguration"));
            for each (mbe::ProjectItem^ project_configuration_item in project_configuration_items)
                _configurations.insert(marshal_string(project_configuration_item->EvaluatedInclude));

            // TODO
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
        os << '\n';
        os << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
        os << "# Visual Studio 2012\n";

        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            os << "Project(\"" << vcxproj_flavor_guid << "\") = "
               << "\"" << project.name()            << "\", "
               << "\"" << project.relative_path().file_string()   << "\", "
               << "\"" << project.guid()            << "\"\n"
               << "EndProject\n";
        });

        // Generate GUIDs for each virtual folder:
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

                folder_guids.insert(std::make_pair(current_path, create_guid()));
            }
        });

        // Generate fake projects for each folder:
        std::for_each(begin(folder_guids), end(folder_guids), [&](folder_guid_map::value_type const& folder_guid)
        {
            os << "Project(\"" << folder_flavor_guid << "\") = "
               << "\"" << folder_guid.first.leaf() << "_\", "
               << "\"" << folder_guid.first.leaf() << "_\", "
               << "\"" << folder_guid.second << "\"\n"
               << "EndProject\n";
        });

        // Generate a miscellaneous folder:
        os << "Project(\"" << folder_flavor_guid  << "\") = "
           << "\"miscellaneous_\", "
           << "\"miscellaneous_\", "
           << "\"" << miscellaneous_guid << "\"\n"
           << "\tProjectSection(SolutionItems) = preProject\n";
        string_set seen_root_files;
        std::for_each(directory_iterator(solution_root), directory_iterator(), [&](path_type const& path)
        {
            if (!is_directory(solution_root / path) &&
                !set_contains(seen_root_files, path) &&
                path.string().size() > 0 &&
                path.string()[0] != '\\' &&
                path.extension() != ".sln" &&
                path.extension() != ".swp" &&
                path.extension() != ".suo")
            {
                os << "\t\t" << path.filename() << " = " << path.filename() << "\n";
                seen_root_files.insert(path.filename());
            }
        });
        os << "\tEndProjectSection\n"
           << "EndProject\n";

        // Accumulate the complete set of configurations:
        string_set solution_configurations;
        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            std::for_each(begin(project.configurations()), end(project.configurations()), [&](string_type const& configuration)
            {
                solution_configurations.insert(configuration);
            });
        });

        // Generate the Global sections:
        os << "Global\n";
        os << "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
        std::for_each(begin(solution_configurations), end(solution_configurations), [&](string_type const& configuration)
        {
            os << "\t\t" << configuration << " = " << configuration << "\n";
        });
        os << "\tEndGlobalSection\n";

        os << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            std::for_each(begin(solution_configurations), end(solution_configurations), [&](string_type const& configuration)
            {
                if (set_contains(project.configurations(), configuration))
                {
                    os << "\t\t" << project.guid() << "." << configuration << ".ActiveCfg = " << configuration << "\n";
                    os << "\t\t" << project.guid() << "." << configuration << ".Build.0 = " << configuration << "\n";
                }
                else
                {
                    os << "\t\t" << project.guid() << "." << configuration << ".ActiveCfg = " << *project.configurations().begin() << "\n";
                }
            });
        });
        os << "\tEndGlobalSection\n";
        
        // I have no idea what this does...
        os << "\tGlobalSection(SolutionProperties) = preSolution\n"
           << "\t\tHideSolutionNode = FALSE\n"
           << "\tEndGlobalSection\n";

        // Nested projects and folders:
        os << "\tGlobalSection(NestedProjects) = preSolution\n";
        std::for_each(begin(projects), end(projects), [&](project_info const& project)
        {
            if (project.relative_path().has_parent_path() && project.relative_path().parent_path().has_parent_path())
                os << "\t\t" << project.guid() << " = " << folder_guids.find(project.relative_path().parent_path().parent_path())->second << "\n";
        });

        std::for_each(begin(folder_guids), end(folder_guids), [&](folder_guid_map::value_type const& folder_guid)
        {
            if (folder_guid.first.has_parent_path())
                os << "\t\t" << folder_guid.second << " = " << folder_guids.find(folder_guid.first.parent_path())->second << "\n";
        });

        os << "\tEndGlobalSection\n";
        
        os << "EndGlobal\n";
    }

    auto regenerate_solution(path_type const& solution_root, string_type const& solution_name) -> void
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

        regenerate_solution(arguments[1], arguments[2]);
    }
    catch (std::exception const& e)
    {
        std::cout << "Uh oh.  An exception occurred during execution :'(\n"
                  << e.what();

        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
