
REM "//               Copyright James P. McNellis (james@jamesmcnellis.com) 2011 - 2012.               //"
REM "//                   Distributed under the Boost Software License, Version 1.0.                   //"
REM "//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //"

REM "Assembles an IL file, then embeds it into a .cpp file."

REM "%1 => Full path to .il file"
REM "%2 => Full path to .metadll file"
REM "%3 => Full path to .cpp file"
REM "%4 => Full path to EmbedFileInCpp.exe"
REM "%5 => Namespace qualified name of the array in the .cpp file"

ilasm /nologo /dll /noautoinherit /quiet /output=%2 %1
%4 %2 %3 %5


REM "That's it!"