target_sources(libSceFreeType PRIVATE src/OutlineEmbolden.cpp)

if(BUILD_TESTING)
    add_test_executable(guest_freetype_embolden_tests ${LIBS_INCLUDE_DIR}/tests/GuestFreeTypeEmbolden.cpp)
    target_include_directories(guest_freetype_embolden_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(guest_freetype_embolden_tests PRIVATE libSceFreeType freetype libc ${CMAKE_DL_LIBS})
    add_dependencies(guest_freetype_embolden_tests libSceFreeTypeOl libSceFreeTypeOt)
    configure_windows_unwind(guest_freetype_embolden_tests)
    add_test(NAME guest_freetype_embolden COMMAND guest_freetype_embolden_tests)
    set_tests_properties(guest_freetype_embolden PROPERTIES TIMEOUT 20)
    if(WIN32)
        set_tests_properties(guest_freetype_embolden PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libSceFreeType>")
    endif()
    foreach(module IN ITEMS Ol Ot)
        add_test(NAME guest_freetype_embolden_${module}
            COMMAND guest_freetype_embolden_tests $<TARGET_FILE:libSceFreeType${module}> raw)
        set_tests_properties(guest_freetype_embolden_${module} PROPERTIES TIMEOUT 20)
        if(WIN32)
            set_tests_properties(guest_freetype_embolden_${module} PROPERTIES ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:libSceFreeType>")
        endif()
    endforeach()
endif()
