cmake_minimum_required ( VERSION 3.17 )

PROJECT ( xinfc )

add_executable (xinfc-wsc src/xinfc-wsc.c )
target_include_directories (xinfc-wsc PRIVATE "include")
target_compile_options ( xinfc-wsc PRIVATE "-Wall" )

if (${XINFC_DUMMY_OUT})
	target_compile_definitions ( xinfc-wsc PRIVATE "-DXINFC_DUMMY_OUT" )
endif()

INSTALL(TARGETS xinfc-wsc RUNTIME DESTINATION usr/bin)