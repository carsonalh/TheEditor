#include "neovim.h"
#include "msgpack/pack.h"
#include "nvim_rpc.h"

#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdbool.h>
#include <msgpack.h>

typedef struct {
    size_t len;
    char *buf;
} NbString;


DWORD neovim_thread_main(void *unused_param)
{
    SECURITY_ATTRIBUTES sa = {
        sizeof (SECURITY_ATTRIBUTES),
        .bInheritHandle = true,
    };

    HANDLE stdout_read, stdout_write,
           stdin_read, stdin_write,
           stderr_read, stderr_write;
    BOOL ok;
    const DWORD pipe_size = 0 /* default */;

    ok = CreatePipe(&stdout_read, &stdout_write, &sa, pipe_size);
    assert(ok && "stdout pipe");

    ok = CreatePipe(&stdin_read, &stdin_write, &sa, pipe_size);
    assert(ok && "stdin pipe");

    ok = CreatePipe(&stderr_read, &stderr_write, &sa, pipe_size);
    assert(ok && "stdin pipe");

    wchar_t cmdline[] = L"nvim.exe -u NONE --embed";
    PROCESS_INFORMATION pi = {0};
    ok = CreateProcessW(
            NULL, cmdline, NULL, NULL,
            FALSE, 0, NULL, NULL,
            &(STARTUPINFOW) {
                sizeof (STARTUPINFOW),
                .hStdInput = stdin_read,
                .hStdOutput = stdout_write,
                .hStdError = stderr_write,
            }, &pi);
    assert(ok && "create process");

    msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    // IMPORTANT: might have to use RPC events instead of calls for the nvim ui functions
    // The nvim source code does it this way, but there is no indication of this in the
    // documentation

    NbString name = { .buf = "nvim_ui_attach" };
    name.len = strlen(name.buf);
    // as per rpc
    msgpack_pack_array(pk, 3);
        msgpack_pack_uint32(pk, 2);
        msgpack_pack_str(pk, name.len);
        msgpack_pack_str_body(pk, name.buf, name.len);
        msgpack_pack_array(pk, 3);
            msgpack_pack_uint32(pk, 80);
            msgpack_pack_uint32(pk, 40);
            msgpack_pack_map(pk, 0);

    msgpack_zone mempool;
    msgpack_zone_init(&mempool, 2048);

    msgpack_object deserialized;
    msgpack_unpack(buffer->data, buffer->size, NULL, &mempool, &deserialized);

    /* print the deserialized object. */
    printf("sending the following call to nvim.exe --embed 's stdin\n");
    msgpack_object_print(stdout, deserialized);
    printf("\n");

    FILE *f = fopen("./msgpack.out", "wb");
    fwrite(buffer->data, sizeof(uint8_t), buffer->size, f);
    fclose(f);

    unsigned long written;
    // all readfile and writefile must have checks around them as they
    // are likely-to-fail operations
    ok = WriteFile(stdin_write, (void*)buffer->data, buffer->size, &written, NULL);
    assert(ok && "write nvim_ui_attach");

    buffer = msgpack_sbuffer_new();
    pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    name = (NbString) { .buf = "nvim_list_uis" };
    name.len = strlen(name.buf);
    // as per rpc
    msgpack_pack_array(pk, 3);
        msgpack_pack_uint32(pk, 1);
        msgpack_pack_uint32(pk, 1);
        msgpack_pack_str(pk, name.len);
        msgpack_pack_str_body(pk, name.buf, name.len);
        msgpack_pack_array(pk, 0);

    msgpack_zone_init(&mempool, 2048);

    // msgpack_object deserialized;
    msgpack_unpack(buffer->data, buffer->size, NULL, &mempool, &deserialized);

    /* print the deserialized object. */
    printf("sending the following call to nvim.exe --embed 's stdin\n");
    msgpack_object_print(stdout, deserialized);
    printf("\n");

    ok = WriteFile(stdin_write, (void*)buffer->data, buffer->size, &written, NULL);
    assert(ok && "write nvim_list_uis");

    if (ok && buffer->size == written) {
        printf("Sent %lu bytes to neovim\n", written);
        buffer->size = 0;
        ok = ReadFile(stdout_read, &buffer->data, buffer->alloc, (DWORD*)&buffer->size, NULL);
        printf("Done reading from nvim\n");
        // msgpack_zone mempool;
        msgpack_zone_init(&mempool, 2048);
        // msgpack_object deserialized;
        msgpack_unpack(buffer->data, buffer->size, NULL, &mempool, &deserialized);
        msgpack_object_print(stdout, deserialized);
        printf("\n");
        assert(ok);
    } else {
        fprintf(stderr, "Failed to write the data to neovim stdin\n");
    }
    // assert(ok && "write to neovim stdin");

    DWORD exit_code;
    ok = GetExitCodeProcess(pi.hProcess, &exit_code);
    assert(ok);

    if (exit_code == STILL_ACTIVE) {
        ok = TerminateProcess(pi.hProcess, 0);
        assert(ok);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return 0;
}
