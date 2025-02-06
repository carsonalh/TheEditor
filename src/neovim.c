#include "neovim.h"

#include <windows.h>
#include <stdio.h>
#include <msgpack.h>

DWORD neovim_thread_main(void *unused_param)
{
    PROCESS_INFORMATION pi = {0};
    // TODO: plumb the stdin and stdout pipes to receive rpc and start making buffers and sending/receiving events
    // make sure the RPC implementation is platform independent. not sure whether or not to use a library since
    // its probably a two-day project and the most current implemtnations i've found haven't been touched for
    // literally over a decade; how hard can a json-like packing library be?
    CreateProcessW(
            L"nvim.exe", L"-u NONE --embed", NULL, NULL,
            FALSE, 0, NULL, NULL,
            &(STARTUPINFOW) { sizeof (STARTUPINFOW) },
            &pi);

    /* creates buffer and serializer instance. */
    msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    /* serializes ["Hello", "MessagePack"]. */
    msgpack_pack_array(pk, 2);
    msgpack_pack_bin(pk, 5);
    msgpack_pack_bin_body(pk, "Hello", 5);
    msgpack_pack_bin(pk, 11);
    msgpack_pack_bin_body(pk, "MessagePack", 11);

    /* deserializes it. */
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    msgpack_unpack_return ret = msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL);

    /* prints the deserialized object. */
    msgpack_object obj = msg.data;
    msgpack_object_print(stdout, obj);  /*=> ["Hello", "MessagePack"] */

    /* cleaning */
    msgpack_unpacked_destroy(&msg);
    msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);

    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
