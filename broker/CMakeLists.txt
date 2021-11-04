#
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Nng REQUIRED)
find_package(Glib REQUIRED)

add_library(cache src/cache.c)
target_include_directories(cache PUBLIC inc)
target_link_libraries(cache PRIVATE
                            Glib)

add_executable(gw src/gw.c)
target_link_libraries(gw PRIVATE
                           hGw
                           Nng
                           cache
                           uuid
                           hexdump
                           Threads::Threads)

add_executable(client src/client.c)
target_link_libraries(client PRIVATE
                               hGw
                               Nng
                               cache
                               uuid
                               hexdump
                               Threads::Threads)


add_executable(addsvr src/addsvr.c)
target_link_libraries(addsvr PRIVATE
                               hGw
                               Nng
                               cache
                               uuid
                               hexdump
                               Threads::Threads)




add_executable(addcli src/addcli.c)
target_link_libraries(addcli PRIVATE
                               hGw
                               Nng
                               cache
                               uuid
                               hexdump
                               Threads::Threads)

