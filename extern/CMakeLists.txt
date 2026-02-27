# Vendored third-party libraries

# mINI - header-only INI file parser
add_library(mINI INTERFACE)
target_include_directories(mINI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/mINI)
# Enable case-sensitive mode for GKeyFile compatibility
target_compile_definitions(mINI INTERFACE MINI_CASE_SENSITIVE)

# cxxopts - header-only command line argument parser
add_library(cxxopts INTERFACE)
target_include_directories(cxxopts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cxxopts)
