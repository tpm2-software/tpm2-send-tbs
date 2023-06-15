#!/bin/bash

BUILD_DIR=build
TPM2_SEND_TBS=$BUILD_DIR/tpm2-send-tbs.exe
HEX=$BUILD_DIR/hex
UNHEX=$BUILD_DIR/unhex

set -xe


# TPM2_Startup: expect TPM_E_COMMAND_BLOCKED (0x80280400)
printf 80010000000c000001440000 | $TPM2_SEND_TBS > $BUILD_DIR/out.hex
[ "$(cat $BUILD_DIR/out.hex)" = "80010000000a80280400" ]
rm $BUILD_DIR/out.hex

# TPM2_GetRandom of 4 bytes
printf 80010000000c0000017b0004 | $TPM2_SEND_TBS

printf 80010000000c0000017b0004 | $TPM2_SEND_TBS

printf 80010000000c0000017b0004 | $TPM2_SEND_TBS

printf 80010000000c0000017b0004 | xxd -r -p | $TPM2_SEND_TBS --bin | xxd -p

printf 80010000000c0000017b0004 | xxd -r -p | $TPM2_SEND_TBS --bin | xxd -p

tpm2_getrandom -T "cmd: $HEX | $TPM2_SEND_TBS | $UNHEX" --hex 4

tpm2_getrandom -T "cmd: $HEX | $TPM2_SEND_TBS | $UNHEX" --hex 4

printf 80010000000c0000017b0004 > $BUILD_DIR/in.hex
$TPM2_SEND_TBS -i $BUILD_DIR/in.hex
rm -f $BUILD_DIR/in.hex

printf 80010000000c0000017b0004 > $BUILD_DIR/in.hex
$TPM2_SEND_TBS -i $BUILD_DIR/in.hex
rm -f $BUILD_DIR/in.hex

printf 80010000000c0000017b0004 | xxd -r -p > $BUILD_DIR/in.bin
$TPM2_SEND_TBS --bin -i $BUILD_DIR/in.bin | xxd -p
rm -f $BUILD_DIR/in.bin

printf 80010000000c0000017b0004 | xxd -r -p > $BUILD_DIR/in.bin
$TPM2_SEND_TBS --bin -i $BUILD_DIR/in.bin | xxd -p
rm -f $BUILD_DIR/in.bin

printf 80010000000c0000017b0004 | $TPM2_SEND_TBS -o $BUILD_DIR/out.hex
[ $(stat -c "%s" $BUILD_DIR/out.hex) -eq 32 ]
rm -f $BUILD_DIR/out.hex

printf 80010000000c0000017b0004 | $TPM2_SEND_TBS -o $BUILD_DIR/out.hex
[ $(stat -c "%s" $BUILD_DIR/out.hex) -eq 32 ]
rm -f $BUILD_DIR/out.hex

printf 80010000000c0000017b0004 | xxd -r -p | $TPM2_SEND_TBS --bin -o $BUILD_DIR/out.bin
[ $(stat -c "%s" $BUILD_DIR/out.bin) -eq 16 ]
rm -f $BUILD_DIR/out.bin

printf 80010000000c0000017b0004 | xxd -r -p | $TPM2_SEND_TBS --bin -o $BUILD_DIR/out.bin
[ $(stat -c "%s" $BUILD_DIR/out.bin) -eq 16 ]
rm -f $BUILD_DIR/out.bin

tpm2_getrandom -T "cmd: $HEX | $TPM2_SEND_TBS | $UNHEX" --hex 4

tpm2_getcap properties-fixed -T "cmd: $HEX | $TPM2_SEND_TBS -d | $UNHEX"

# TODO this will fail
tpm2_nvread 0x01C00002 -T "cmd: $HEX | $TPM2_SEND_TBS | $UNHEX"

set +x
echo "###########"
echo "# Success #"
echo "###########"
