#
find_package(Threads REQUIRED)
find_package(Nng REQUIRED)

add_library ( hGw src/hGw.c )
target_include_directories ( hGw PUBLIC inc )
target_link_libraries(hGw PRIVATE
                          Nng
                          Threads::Threads)

