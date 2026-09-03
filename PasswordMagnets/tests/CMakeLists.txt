add_executable(sodium_smoke sodium_smoke.cpp)
target_link_libraries(sodium_smoke PRIVATE PasswordMagnets::core)

# Header-only container and storage live under src/; expose that tree.
add_executable(hash_table_test hash_table_test.cpp)
target_include_directories(hash_table_test PRIVATE "${PROJECT_SOURCE_DIR}/src")

add_executable(vault_store_test vault_store_test.cpp)
target_include_directories(vault_store_test PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(vault_store_test PRIVATE PasswordMagnets::core)

add_test(NAME sodium_smoke COMMAND sodium_smoke)
add_test(NAME hash_table_test COMMAND hash_table_test)
add_test(NAME vault_store_test COMMAND vault_store_test)

# End-to-end persistence checkpoint: runs the passwordmagnets executable in
# its headless --checkpoint mode (no QApplication/window is started), which
# saves/loads an encrypted vault and exits non-zero on any failure.
add_test(NAME vault_checkpoint COMMAND passwordmagnets --checkpoint)
