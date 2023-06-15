// SPDX-FileCopyrightText: 2023 Infineon Technologies AG
//
// SPDX-License-Identifier: BSD-2-Clause

#include <stdio.h>


int main() {
    int ch;

    //FILE *f = fopen("hex.txt", "wb");

    setbuf(stdout, NULL);
    while ((ch = getchar()) != EOF) {
        printf("%02x", ch);
        //fprintf(f, "%02x", ch);
        //fflush(f);
    }

    //fclose(f);

    return 0;
}
