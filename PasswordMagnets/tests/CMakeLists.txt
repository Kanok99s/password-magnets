add_executable(sodium_smoke sodium_smoke.cpp)
target_link_libraries(sodium_smoke PRIVATE PasswordMagnets::core)

add_test(NAME sodium_smoke COMMAND sodium_smoke)
