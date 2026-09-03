add_executable(sodium_smoke sodium_smoke.cpp)
target_link_libraries(sodium_smoke PRIVATE PasswordMagnets::core)

# Header-only container lives under src/; expose that tree to the test.
add_executable(hash_table_test hash_table_test.cpp)
target_include_directories(hash_table_test PRIVATE "${PROJECT_SOURCE_DIR}/src")

add_test(NAME sodium_smoke COMMAND sodium_smoke)
add_test(NAME hash_table_test COMMAND hash_table_test)
