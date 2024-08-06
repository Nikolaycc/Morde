# Define the library
add_library(morde STATIC morde.c log.c)

# Specify include directories
target_include_directories(morde PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Link necessary libraries
target_link_libraries(morde pthread)
