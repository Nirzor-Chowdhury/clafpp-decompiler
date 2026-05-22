Visual Studio project changes (jdc.vcxproj)
==========================================

The existing jdc.vcxproj already hardcodes wxWidgets paths. We add libcurl
and nlohmann/json on top using the same pattern. Three things change:

  1. Add libcurl + nlohmann include dirs to AdditionalIncludeDirectories
     in each <ItemDefinitionGroup> (Debug|x64, Release|x64, Debug|Win32,
     Release|Win32 - the existing GUI build only enables x64, so the Win32
     entries are optional).

  2. Add libcurl import library to AdditionalDependencies in <Link>.

  3. Register the new src/llm/*.cpp source files and src/llm/*.h headers.

Recommended approach: install vcpkg (per-user, no admin needed) and let
vcpkg autodiscover the dependencies:

    vcpkg install curl[ssl] nlohmann-json --triplet x64-windows
    vcpkg integrate install

With vcpkg integration enabled, MSBuild discovers includes and libs
automatically and you only need step 3 below (registering the new files).

If you prefer manual installation, set up environment variables or hardcode
paths as shown.


-------------------------------------------------------------------------------
PATCH 1 -- AdditionalIncludeDirectories
-------------------------------------------------------------------------------
For each of the four <ItemDefinitionGroup> blocks (search for
"AdditionalIncludeDirectories"), change:

    <AdditionalIncludeDirectories>C:\wxWidgets-3.2.10\include;C:\wxWidgets-3.2.10\include\msvc;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>

to (if using vcpkg, no change needed - vcpkg handles it; otherwise):

    <AdditionalIncludeDirectories>C:\wxWidgets-3.2.10\include;C:\wxWidgets-3.2.10\include\msvc;C:\libs\curl\include;C:\libs\nlohmann-json\single_include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>

Also bump the C++ language standard. The existing file sets
<LanguageStandard_C> for the C side; add a matching <LanguageStandard> for C++:

    <LanguageStandard>stdcpp17</LanguageStandard>


-------------------------------------------------------------------------------
PATCH 2 -- libcurl link library
-------------------------------------------------------------------------------
Inside each <Link> block, ADD:

    <AdditionalDependencies>libcurl.lib;%(AdditionalDependencies)</AdditionalDependencies>

(Again, vcpkg + `vcpkg integrate install` makes this automatic.)

Also extend the AdditionalLibraryDirectories line to point at your libcurl
install dir, e.g.:

    <AdditionalLibraryDirectories>C:\wxWidgets-3.2.10\lib\vc_x64_lib;C:\libs\curl\lib;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>


-------------------------------------------------------------------------------
PATCH 3 -- Register new source files
-------------------------------------------------------------------------------
Find the existing <ItemGroup> that contains the ClCompile entries (around
line 158). Add the four LLM .cpp files in the same group:

    <ClCompile Include="src\llm\llmConfig.cpp" />
    <ClCompile Include="src\llm\ollama_interface.cpp" />
    <ClCompile Include="src\llm\llmCommentGenerator.cpp" />
    <ClCompile Include="src\llm\llmSettings.cpp" />

Find the <ItemGroup> with ClInclude entries (or add a new one if absent).
Register the headers so they appear in Solution Explorer:

    <ClInclude Include="src\llm\llmConfig.h" />
    <ClInclude Include="src\llm\ollama_interface.h" />
    <ClInclude Include="src\llm\llmCommentGenerator.h" />
    <ClInclude Include="src\llm\llmSettings.h" />


-------------------------------------------------------------------------------
PATCH 4 -- jdc.vcxproj.filters
-------------------------------------------------------------------------------
Open jdc.vcxproj.filters. Add a new filter to keep the LLM module organised:

    <ItemGroup>
      <Filter Include="src\llm">
        <UniqueIdentifier>{e3a1b5d0-4d2b-4f1a-8a9c-2b4a9c7e1234}</UniqueIdentifier>
      </Filter>
    </ItemGroup>

Then attach the new files to it:

    <ItemGroup>
      <ClCompile Include="src\llm\llmConfig.cpp"><Filter>src\llm</Filter></ClCompile>
      <ClCompile Include="src\llm\ollama_interface.cpp"><Filter>src\llm</Filter></ClCompile>
      <ClCompile Include="src\llm\llmCommentGenerator.cpp"><Filter>src\llm</Filter></ClCompile>
      <ClCompile Include="src\llm\llmSettings.cpp"><Filter>src\llm</Filter></ClCompile>
      <ClInclude Include="src\llm\llmConfig.h"><Filter>src\llm</Filter></ClInclude>
      <ClInclude Include="src\llm\ollama_interface.h"><Filter>src\llm</Filter></ClInclude>
      <ClInclude Include="src\llm\llmCommentGenerator.h"><Filter>src\llm</Filter></ClInclude>
      <ClInclude Include="src\llm\llmSettings.h"><Filter>src\llm</Filter></ClInclude>
    </ItemGroup>


-------------------------------------------------------------------------------
PATCH 5 -- runtime DLL (deployment)
-------------------------------------------------------------------------------
libcurl on Windows is typically shipped as a DLL. After Build, copy
libcurl.dll to the directory where jdc.exe lives. If you used vcpkg with
`integrate install`, this is automated for x64-windows builds.

Heads-up: on first run, Windows Defender / antivirus may flag the network
calls. The traffic is purely localhost; you can confirm via Wireshark.
