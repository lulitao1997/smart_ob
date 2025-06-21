# Find all header files
file(GLOB CONFIGURE_DEPENDS HEADER_FILES
    "*.h"
)

# Create an INTERFACE library since it's header-only
add_library(smart_ob_lib INTERFACE)

# Set include directories for the library
target_include_directories(smart_ob_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create the executable
add_executable(smart_ob
    main.cpp
)

# Link the executable with the library
target_link_libraries(smart_ob PRIVATE
    smart_ob_lib
)
