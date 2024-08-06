# Add test executable
add_executable(test_main test_main.c)

# Link the test executable with the library
target_link_libraries(test_main morde)