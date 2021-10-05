add_library(enable_gnu_cpp17 INTERFACE)
target_compile_options(enable_gnu_cpp17 INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -D_SYS__PTHREADTYPES_H_)
target_link_libraries(app PRIVATE enable_gnu_cpp17)
