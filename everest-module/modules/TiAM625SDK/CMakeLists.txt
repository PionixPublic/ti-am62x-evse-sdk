#
# AUTO GENERATED - MARKED REGIONS WILL BE KEPT
# template version 3
#

# module setup:
#   - ${MODULE_NAME}: module name
ev_setup_cpp_module()

# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    message(WARNING "Building dummy version of ti_rpmsg_char library")
    add_subdirectory(rpmsg_dummy_lib)
endif ()

add_library(pb_codec OBJECT)
target_sources(pb_codec 
    PRIVATE
        pb_codec/nanopb/pb_common.c
        pb_codec/nanopb/pb_decode.c
        pb_codec/nanopb/pb_encode.c
        pb_codec/high_to_low.pb.c
        pb_codec/low_to_high.pb.c
)

target_include_directories(pb_codec
    PUBLIC
        pb_codec
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ti_rpmsg_char
        pb_codec
)
# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1

target_sources(${MODULE_NAME}
    PRIVATE
        "board_support/board_support_ACImpl.cpp"
)

# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
# insert other things like install cmds etc here
# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
