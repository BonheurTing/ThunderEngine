# move sln file
macro(MoveMSVCSolutionFile)
    set(SolutionBinaryPath ${CMAKE_SOURCE_DIR}/Intermediate/Build/${PROJECT_NAME}.sln)
    if(EXISTS ${SolutionBinaryPath})
        # Load solution file from bin-dir and change the relative references to 
        # project files so that the in memory copy is as if it had been built in 
        # the source dir.
        set(SolutionPrefix "Intermediate/Build/")
        file(READ ${SolutionBinaryPath} SlnContent)
        string(REGEX REPLACE 
            "\"([^\"]+).vcxproj\""
            "\"${SolutionPrefix}/\\1.vcxproj\"" 
            SlnContent
            "${SlnContent}")

        # Compare the updated contents with the existing source path sln, if it
        # exists and is the same we dont want to disturb VS by touching it.
        set(SlnOutPath ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.sln)
        set(OldContent "")
        if(EXISTS ${SlnOutPath})
            file(READ ${SlnOutPath} OldContent)
        endif()
        if(NOT OldContent STREQUAL SlnContent)
            file(WRITE ${SlnOutPath} ${SlnContent})
        endif()
    endif()
endmacro()

# link module
macro(LinkModuleFunc TargetName LinkModuleMode LinkModuleList)
    foreach(LinkModuleName IN LISTS ${LinkModuleList})
        message("${TargetName}  -link- ${LinkModuleName}")
        add_dependencies(${TargetName} ${LinkModuleName})
        target_link_libraries(${TargetName} ${LinkModuleMode} ${LinkModuleName})
        target_include_directories(${TargetName} ${LinkModuleMode} ${${LinkModuleName}DirPath}/Public) 
        # link和include的mode不是一样的吧？
        # 默认直接include全部Public
    endforeach()
endmacro()


# 设置全局的默认值以防有的模块的build文件没改
macro(ResetConfigureValue)
    set(IsSubMakeList False)
    set(IsEexcutableWindows False)

    set(CustomSourceFiles) # 自定义add target时需要的文件
    set(PrivateIncludePaths) # 自定义include的文件位置;
    set(AutoIncludePublicFile True) # 默认include全部public下面的文件和文件夹，第三方库的include可能要考虑不include public

    set(PrecompileHeaderImportList) # 预编译头，设置一些math库等几乎不会修改的头文件

    set(PublicDependencyModuleList) # 依赖的库，包含添加依赖、链接、include
    set(PrivateDependencyModuleList)
    set(InterfaceDependencyModuleList) #todo: 接口待测试
    
    set(PrivateIncludePathModuleList) # 只include，不链接不依赖，可能是一些纯头文件的模块，如Config

    set(PrivateThirdPartyIncludeList) # 第三方库 dll lib include
    set(PrivateThirdPartyNameList)
    set(PrivateThirdPartyPathList)
    #eastl submakelist 源码编译的第三方库
    # 第三方库像ue一样有build文件，依赖的适合感知不到来源
    # 第三方库的导出

    set(TargetCompileDefinitions)
endmacro()

# 预处理模块
macro(PreBuildModule)
    # 设置源文件夹位置
    set(SourceDirectoryPath Source)
    set(ModuleNameList)
    set(HeaderFilesList)
    set(ProjectBinaryOutputPath "${CMAKE_SOURCE_DIR}/Engine/Bin")
    set(ProjectArchiveOutputPath "${CMAKE_SOURCE_DIR}/Intermediate/Lib")
    set(ProjectIncludeOutputPath "${CMAKE_SOURCE_DIR}/Intermediate/Include")

    # 找到所有模块的位置
    file(GLOB_RECURSE BuildFileList ${SourceDirectoryPath}/*.Build)
    
    foreach(BuildFile IN LISTS BuildFileList)
        get_filename_component(ModuleFile ${BuildFile} NAME)
        string(FIND ${ModuleFile} ".Build" ModuleNameLength)
        string(SUBSTRING ${ModuleFile} 0 ${ModuleNameLength} ModuleName)
        get_filename_component(${ModuleName}DirPath ${BuildFile} DIRECTORY)
    endforeach()
endmacro()

macro(BuildModule)
    # 遍历添加模块
    foreach(BuildFile IN LISTS BuildFileList)
        set(CustomSourceFiles)
        set(IsSubMakeList False)
        include(${BuildFile})
        if(IsSubMakeList)
            add_subdirectory("${${ModuleName}DirPath}/Source")
            continue()
        endif()
        
        get_filename_component(BuildFileDir ${BuildFile} DIRECTORY)
        file(GLOB_RECURSE LibrarySourceFiles CMAKE_CONFIGURE_DEPENDS
            ${BuildFileDir}/*.cpp
            ${BuildFileDir}/*.c
            ${BuildFileDir}/*.cc
            ${BuildFileDir}/*.cxx
            ${BuildFileDir}/*.h
            ${BuildFileDir}/*.hpp
            ${SourceDirectoryPath}/${CustomSourceFiles}
        )

        if(${LinkMode} STREQUAL EXECUTABLE)
            add_executable(${ModuleName} ${LibrarySourceFiles})
            target_compile_features(${ModuleName} PRIVATE cxx_attribute_deprecated)
        else()
            add_library(${ModuleName} ${LinkMode} ${LibrarySourceFiles})
        endif()
        list(APPEND ModuleNameList ${ModuleName})

        #target_compile_definitions(foo PUBLIC FOO)
    endforeach()

    

    foreach(BuildFile IN LISTS BuildFileList)
        ResetConfigureValue()
        include(${BuildFile})
        if(IsSubMakeList)
            continue()
        endif()
        if(${IsEexcutableWindows})
            set_target_properties(${ModuleName} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
        endif()
        #get_filename_component(BuildFileDir ${BuildFile} DIRECTORY)
        set(BuildFileDir ${${ModuleName}DirPath})
        
        # 本模块的include
        if(${AutoIncludePublicFile})
            target_include_directories(${ModuleName} PUBLIC ${BuildFileDir}/Public) # 先默认public include，这是两码事
            list(APPEND HeaderFilesList ${BuildFileDir}/Public)
        endif()
        foreach(IncludePath IN LISTS PrivateIncludePaths)
            target_include_directories(${ModuleName} PRIVATE ${SourceDirectoryPath}/${IncludePath})
            list(APPEND HeaderFilesList ${SourceDirectoryPath}/${IncludePath})
        endforeach()
        
        # 预编译头
        set(PrecompileFileList)
        foreach(PrecompileFilePath IN LISTS PrecompileHeaderImportList)
            list(APPEND PrecompileFileList ${SourceDirectoryPath}/${PrecompileFilePath}) # add prefix
            #target_precompile_headers(${ModuleName} PUBLIC ${SourceDirectoryPath}/${PrecompileFilePath}) 
        endforeach()
        if(${LinkMode} STREQUAL SHARED)
            string(TOUPPER ${ModuleName} ModuleNameUpper)
            set(LibraryExportHeaderPath ${BuildFileDir}/Public/${ModuleName}.export.h)
            generate_export_header(${ModuleName}
                EXPORT_MACRO_NAME ${ModuleNameUpper}_API
                EXPORT_FILE_NAME ${LibraryExportHeaderPath})
            #message(STATUS "${LibraryExportHeaderPath}")
            list(APPEND PrecompileFileList ${LibraryExportHeaderPath})
            #target_precompile_headers(${ModuleName} PUBLIC ${LibraryExportHeaderPath})
        endif()
        target_precompile_headers(${ModuleName} PUBLIC ${PrecompileFileList}) # add prefix

        # 模块链接
        LinkModuleFunc(${ModuleName} PUBLIC PublicDependencyModuleList)
        LinkModuleFunc(${ModuleName} PRIVATE PrivateDependencyModuleList)
        LinkModuleFunc(${ModuleName} INTERFACE InterfaceDependencyModuleList)

        # 只include的模块s
        foreach(IncludeModule IN LISTS PrivateIncludePathModuleList)
            target_include_directories(${ModuleName} PRIVATE ${${IncludeModule}DirPath}/Public)
            list(APPEND HeaderFilesList ${${IncludeModule}DirPath}/Public)
        endforeach()

        
        # 第三方库 include dll lib
        foreach(IncludePath IN LISTS PrivateThirdPartyIncludeList)
            target_include_directories(${ModuleName} PRIVATE ${SourceDirectoryPath}/${IncludePath})
            list(APPEND HeaderFilesList ${SourceDirectoryPath}/${IncludePath})
        endforeach()
        if(PrivateThirdPartyNameList)
            foreach(FindPath IN LISTS PrivateThirdPartyPathList)
                #message("${SourceDirectoryPath}/${FindPath}")
                target_link_directories(${ModuleName} PRIVATE ${SourceDirectoryPath}/${FindPath})
            endforeach()
            foreach(ImportName IN LISTS PrivateThirdPartyNameList)
                target_link_libraries(${ModuleName} PRIVATE ${ImportName})
            endforeach()
        endif()
    endforeach()
endmacro()

macro(PostBuildModule)
    install(TARGETS ${ModuleNameList}
        CONFIGURATIONS Debug
        ARCHIVE DESTINATION "${ProjectArchiveOutputPath}/Debug"
        LIBRARY DESTINATION "${ProjectBinaryOutputPath}/Debug"
        RUNTIME DESTINATION "${ProjectBinaryOutputPath}/Debug")
    install(TARGETS ${ModuleNameList}
        CONFIGURATIONS Release
        ARCHIVE DESTINATION "${ProjectArchiveOutputPath}/Release"
        LIBRARY DESTINATION "${ProjectBinaryOutputPath}/Release"
        RUNTIME DESTINATION "${ProjectBinaryOutputPath}/Release")

    install(DIRECTORY ${HeaderFilesList} 
        CONFIGURATIONS Debug 
        DESTINATION "${ProjectIncludeOutputPath}/Debug")
    install(DIRECTORY ${HeaderFilesList} 
        CONFIGURATIONS Release 
        DESTINATION "${ProjectIncludeOutputPath}/Release")
endmacro()
