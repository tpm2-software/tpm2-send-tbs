// SPDX-FileCopyrightText: 2023 Infineon Technologies AG
//
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <windows.h>
#include <tbs.h>

#ifndef TPM_VERSION_20
#define TPM_VERSION_20 2
#endif

#define TPM2_MAX_COMMAND_SIZE 4096
#define TPM2_HEADER_SIZE 10
#define TPM2_RESPONSE_SIZE_OFFSET 2

#define ARG_IN "-i"
#define ARG_IN_LONG "--in"
#define ARG_OUT "-o"
#define ARG_OUT_LONG "--out"
#define ARG_BIN "-b"
#define ARG_BIN_LONG "--bin"
#define ARG_DEBUG "-d"
#define ARG_DEBUG_LONG "--debug"
#define PATH_DEFAULT "-"

#define BUF_TO_UINT32(buffer) \
    ( \
        ((uint32_t)((buffer)[0]) << 24) | \
        ((uint32_t)((buffer)[1]) << 16) | \
        ((uint32_t)((buffer)[2]) << 8) | \
        ((uint32_t)(buffer)[3]) \
    )


#if _MSC_VER
    #pragma warning(push)
    /* disable "nonstandard extension used: nameless struct/union" */
    #pragma warning(disable: 4201)
#endif

typedef struct tdTBS_CONTEXT_PARAMS2__ {
    UINT32 version;
    union {
        struct {
            UINT32 requestRaw : 1;
            UINT32 includeTpm12 : 1;
            UINT32 includeTpm20 : 1;
        };
        UINT32 asUINT32;
    };
} TBS_CONTEXT_PARAMS2__;

#if _MSC_VER
    #pragma warning(pop)
#endif


int parse_args(int argc, char* argv[], char **file_in_path, char **file_out_path, int *hex, int *debug) {
    char *arg;

    /* set defaults */
    *file_in_path = PATH_DEFAULT;
    *file_out_path = PATH_DEFAULT;
    *hex = 1;
    *debug = 0;

    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (strcmp(arg, ARG_IN) == 0 || strcmp(arg, ARG_IN_LONG) == 0) {
            if (i + 1 >= argc) {
                goto end_invalid_number_of_args;
            }
            *file_in_path = argv[i + 1];
            i++;
        } else if (strcmp(arg, ARG_OUT) == 0 || strcmp(arg, ARG_OUT_LONG) == 0) {
            if (i + 1 >= argc) {
                goto end_invalid_number_of_args;
            }
            *file_out_path = argv[i + 1];
            i++;
        } else if (strcmp(arg, ARG_BIN) == 0 || strcmp(arg, ARG_BIN_LONG) == 0) {
            *hex = 0;
        } else if (strcmp(arg, ARG_DEBUG) == 0 || strcmp(arg, ARG_DEBUG_LONG) == 0) {
            *debug = 1;
        } else {
            fprintf(stderr, "Invalid argument: %s\n", arg);
            goto end_print_usage;
        }
    }

    return 0;

end_invalid_number_of_args:
    fprintf(stderr, "Invalid number of arguments.\n");
end_print_usage:
    fprintf(stderr, "\nUsage:\n  %s [" ARG_DEBUG_LONG "] [" ARG_BIN_LONG "] [" ARG_IN_LONG " <input file>] [" ARG_OUT_LONG " <output file>]\n", argv[0]);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  " ARG_DEBUG "/" ARG_DEBUG_LONG "               Print debug information to stderr\n");
    fprintf(stderr, "  " ARG_BIN "/" ARG_BIN_LONG   "                 Print as hex stream (instead of binary)\n");
    fprintf(stderr, "  " ARG_IN "/" ARG_IN_LONG    " <input file>     File commands are read from (default stdin)\n");
    fprintf(stderr, "  " ARG_OUT "/" ARG_OUT_LONG   " <output file>   File responses are written to (default: stderr)\n");
    return -1;
}

void dump(const char *const s, const uint8_t *buf, size_t len) {
    fprintf(stderr, "%s[%zu]: ", s, len);
    for (size_t i = 0; i < len; i++) {
        fprintf(stderr, "%02x", buf[i]);
        if (i < len - 1) {
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

int input_init(FILE **file_in, char *file_in_path) {
    int result;

    if (strcmp(file_in_path, PATH_DEFAULT) == 0) {
        *file_in = stdin;
    } else {
        result = fopen_s(file_in, file_in_path, "rb");
        if (result != 0) {
            perror("Error opening input file");
            return result;
        }
    }

    return 0;
}

void input_cleanup(FILE *file_in) {
    if (file_in != stdout) {
        fclose(file_in);
    }
}

/*
 * Turns hex string to binary. buf_hexstr and buf_bin might be the same buffer.
 */
int hex_to_bin(const char *const buf_hexstr, size_t *buf_hexstr_len, uint8_t* buf_bin) {
    int result;

    if (*buf_hexstr_len % 2 != 0) {
        fprintf(stderr, "Hex-string does not have even length\n");
        return -1;
    }

    for (size_t i = 0; i < *buf_hexstr_len / 2; ++i) {
        result = sscanf_s(buf_hexstr + (i * 2), "%02hhx", buf_bin + i);
        if (result != 1) {
            fprintf(stderr, "Hex-string is invalid\n");
            dump("Hex-String: ", (const uint8_t *) buf_hexstr, *buf_hexstr_len);
            dump("Bin-Buf: ", buf_bin, i);
            return -1;
        }
    }

    *buf_hexstr_len /= 2;
    return 0;
}

int input_read(BYTE *tx_buf, UINT32 *tx_buf_len, FILE *file_in, int hex, int *eof) {
    int result;
    size_t offset;
    int cmd_rsp_size;
    size_t read_size = TPM2_HEADER_SIZE;
    uint8_t tmp[TPM2_MAX_COMMAND_SIZE];

    *eof = 0;

    if (hex) {
        read_size *= 2;
    }

    offset = fread(tx_buf, 1, read_size, file_in);
    if (feof(file_in)) {
        /* regular exit point when input pipe is closed */
        *eof = 1;
        result = 0;
        goto cleanup_file;
    }
    if (ferror(file_in)) {
        perror("Error reading from input file");
        result = -1;
        goto cleanup_file;
    }
    if (hex) {
        result = hex_to_bin((const char *const) tx_buf, &offset, tmp);
        memcpy(tx_buf, tmp, offset);
        if (result) {
            return -1;
        }
    }

    cmd_rsp_size = BUF_TO_UINT32(tx_buf + TPM2_RESPONSE_SIZE_OFFSET);
    read_size = cmd_rsp_size - TPM2_HEADER_SIZE;
    if (hex) {
        read_size *= 2;
    }
    if (read_size) {
        read_size = fread(tx_buf + offset, 1, read_size, file_in);
    }
    if (feof(file_in)) {
        fprintf(stderr, "Unexpected EOF when reading input file\n");
        result = -1;
        goto cleanup_file;
    }
    if (ferror(file_in)) {
        perror("Error reading from input file");
        result = -1;
        goto cleanup_file;
    }
    if (hex) {
        result = hex_to_bin((const char *const) tx_buf + offset, &read_size, tmp);
        memcpy(tx_buf + offset, tmp, read_size);
        if (result) {
            return -1;
        }
    }
    offset += read_size;

    *tx_buf_len = (UINT32) offset;
    if (*tx_buf_len == 0) {
        *eof = 1;
    }
    result = 0;

cleanup_file:
    if (file_in != stdin) {
        fclose(file_in);
    }
    return result;
}

int output_init(FILE **file_out, char *file_out_path) {
    int result;

    if (strcmp(file_out_path, PATH_DEFAULT) == 0) {
        *file_out = stdout;
    } else {
        result = fopen_s(file_out, file_out_path, "wb");
        if (result != 0) {
            perror("Error opening output file");
            return result;
        }
    }

    return 0;
}

void output_cleanup(FILE *file_out) {
    if (file_out != stdout) {
        fclose(file_out);
    }
}

int output_write(FILE *file_out, BYTE *buf, UINT32 buf_len, int hex) {
    if (!hex) {
        fwrite(buf, sizeof(BYTE), buf_len, file_out);
        if (ferror(file_out)) {
            perror("Error writing to output file");
            return -1;
        }
    } else {
        for (size_t i = 0; i < buf_len; i++) {
            fprintf(file_out, "%02x", buf[i]);
            if (ferror(file_out)) {
                perror("Error writing to output file");
                return -1;
            }
        }
    }

    fflush(file_out);

    return 0;
}

int transceive_init(TBS_HCONTEXT *handle_ctx) {
    int result;
    TBS_CONTEXT_PARAMS2__ ctx_params = {
        .version = TPM_VERSION_20,
        .includeTpm12 = 0,
        .includeTpm20 = 1,
    };

    result = Tbsi_Context_Create((TBS_CONTEXT_PARAMS *) &ctx_params, handle_ctx);
    if (result != TBS_SUCCESS) {
        fprintf(stderr, "Failed when attempting to create TBS context: %04x\n", result);
    }

    return result;
}

void transceive_cleanup(TBS_HCONTEXT handle_ctx) {
    Tbsip_Context_Close(handle_ctx);
}

int transceive(TBS_HCONTEXT *handle_ctx, BYTE *tx_buf, UINT32 tx_buf_len, BYTE *rx_buf, UINT32 *rx_buf_len) {
    TBS_RESULT result;

    result = Tbsip_Submit_Command(
        handle_ctx,
        TBS_COMMAND_LOCALITY_ZERO,
        TBS_COMMAND_PRIORITY_NORMAL,
        tx_buf,
        tx_buf_len,
        rx_buf,
        rx_buf_len);
    if (result != TBS_SUCCESS) {
        fprintf(stderr, "Failed when attempting to submit TBS context: %04x\n", result);
    }

    return result;
}


int main(int argc, char **argv) {
    int result;
    int eof;

    int hex, debug;
    char *file_in_path;
    char *file_out_path;
    FILE *file_in;
    FILE *file_out;
    TBS_HCONTEXT handle_ctx;

    /* we need double-sized buffer in case hex strings are used */
    BYTE tx_buf[TPM2_MAX_COMMAND_SIZE * 2] = {0};
    UINT32 tx_buf_len;
    BYTE rx_buf[TPM2_MAX_COMMAND_SIZE * 2] = {0};
    UINT32 rx_buf_len = sizeof(rx_buf);

    result = parse_args(argc, argv, &file_in_path, &file_out_path, &hex, &debug);
    if (result != 0) {
        goto end;
    }

    result = input_init(&file_in, file_in_path);
    if (result != 0) {
        goto end;
    }

    result = output_init(&file_out, file_out_path);
    if (result != 0) {
        goto cleanup_input;
    }

    result = transceive_init(&handle_ctx);
    if (result != 0) {
        goto cleanup_output;
    }

    while (1) {
        result = input_read(tx_buf, &tx_buf_len, file_in, hex, &eof);
        if (result != 0 || eof) {
            goto cleanup_transceive;
        }

        if (debug) {
            dump("read cmd", tx_buf, tx_buf_len);
        }
        result = transceive(handle_ctx, tx_buf, tx_buf_len, rx_buf, &rx_buf_len);
        if (result != 0) {
            goto cleanup_transceive;
        }

        if (debug) {
            dump("send rsp", rx_buf, rx_buf_len);
        }
        result = output_write(file_out, rx_buf, rx_buf_len, hex);
        if (result != 0) {
            goto cleanup_transceive;
        }
    }

cleanup_transceive:
    transceive_cleanup(handle_ctx);
cleanup_output:
    output_cleanup(file_out);
cleanup_input:
    input_cleanup(file_in);

end:
    return result;
}
