cmake_minimum_required(VERSION 3.28)
project(cs202_24_prj_operating_student C)

set(CMAKE_C_STANDARD 11)

include_directories(done)
include_directories(provided/src)
include_directories(provided/tests/unit)

add_executable(cs202_24_prj_operating_student
        done/error.c
        done/error.h
        done/http_net.h
        done/http_prot.h
        done/image_content.h
        done/image_dedup.h
        done/imgfs.h
        done/imgfs_list.c
        done/imgfs_server_service.h
        done/imgfs_tools.c
        done/imgfscmd.c
        done/imgfscmd_functions.c
        done/imgfscmd_functions.h
        done/socket_layer.h
        done/util.c
        done/util.h
        provided/src/error.c
        provided/src/error.h
        provided/src/http-test-server.c
        provided/src/http_net.h
        provided/src/http_prot.h
        provided/src/image_content.h
        provided/src/image_dedup.h
        provided/src/imgfs.h
        provided/src/imgfs_server_service.h
        provided/src/imgfs_tools.c
        provided/src/imgfscmd.c
        provided/src/imgfscmd_functions.c
        provided/src/imgfscmd_functions.h
        provided/src/socket_layer.h
        provided/src/util.c
        provided/src/util.h
        provided/tests/unit/test.h
        provided/tests/unit/unit-test-imgfscreate.c
        provided/tests/unit/unit-test-imgfsdelete.c
        provided/tests/unit/unit-test-imgfslist.c
        provided/tests/unit/unit-test-imgfsstruct.c
        provided/tests/unit/unit-test-imgfstools.c
        done/imgfs_create.c
        done/imgfs_delete.c
        done/image_dedup.c
        done/image_content.c
        done/tests/unit/unit-test-imgfscontent.c
        done/imgfs_insert.c
        done/imgfs_read.c
        done/socket_layer.c
        done/http_net.c
        done/tcp-test-server.c
        done/http-test-server.c
        done/imgfs_server_service.c
        done/imgfs_server.c
        done/tests/unit/unit-test-imgfsread.c
        done/http_prot.c
        done/tests/unit/unit-test-http.c
        provided/src/http_net.c)
