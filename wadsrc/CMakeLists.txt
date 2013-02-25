cmake_minimum_required( VERSION 2.4 )


add_custom_command( OUTPUT ${ZDOOM_OUTPUT_DIR}/gzdoom.pk3
	COMMAND ${ZIPDIR_EXE} -udf ${ZDOOM_OUTPUT_DIR}/gzdoom.pk3 ${CMAKE_CURRENT_SOURCE_DIR}/static
   DEPENDS zipdir ${CMAKE_CURRENT_SOURCE_DIR}/static )

add_custom_target( pk3 ALL
	DEPENDS ${ZDOOM_OUTPUT_DIR}/gzdoom.pk3 )

