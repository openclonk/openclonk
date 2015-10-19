add_library(tinyxml
	tinystr.cpp
	tinystr.h
	tinyxml.cpp
	tinyxml.h
	tinyxmlerror.cpp
	tinyxmlparser.cpp
)

if(GTK3_FOUND)
	target_include_directories(tinyxml PRIVATE ${GTK3_INCLUDE_DIRS})
endif()
