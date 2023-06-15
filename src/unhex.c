// SPDX-FileCopyrightText: 2023 Infineon Technologies AG
//
// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdio.h>


int main(){
    unsigned char c = 0;
    char buf[2];

    //FILE *f = fopen("unhex.txt", "wb");

    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    while (1) {
        fread(buf, 1, 2, stdin);
        if (feof(stdin)) {
            return 0;
        }
        if (ferror(stdin)) {
            return 1;
        }
        sscanf(buf, "%02hhx", &c);
        printf("%c", c);
        //fprintf(f, "%c", c);
        //fflush(f);
    }

    //fclose(f);

    return 0;
}
