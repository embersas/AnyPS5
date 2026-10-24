add_library(libSceFreeTypeOt SHARED EXCLUDE_FROM_ALL Export.cpp ../libSceFreeType/src/OutlineBorders.cpp ../libSceFreeType/src/OutlineEmboldenExports.cpp)
target_include_directories(libSceFreeTypeOt PRIVATE ${LIBS_INCLUDE_DIR})
target_link_libraries(libSceFreeTypeOt PRIVATE libSceFreeType)
configure_windows_unwind(libSceFreeTypeOt)
