#!/bin/sh

# Generates commands to Write DMAppFpgaProg the the FLASH memory
# through DMBootloader

APPIMAGE=../../impl/DMAppFpgaProg/DMAppFpgaProg/Release/DMAppFpgaProg.bin
OUTDIR=7_write_DMAppFpgaProg

# Compute number of FLASH memory pages to write
APPSIZE=$(du -b $APPIMAGE | awk '{ print $1 }')
APPPAGES=$(( $APPSIZE / 512))
[ $(( $APPSIZE % 512 )) -ne 0 ] && APPPAGES=$(( $APPPAGES + 1 ))

for i in $(seq 1 $(( $APPPAGES ))); do
    virtual_page=$(( $i - 1 ))
    wr=${OUTDIR}/${i}_cmd
    
    # Generate write command
    ./create_dmbootloader_cmd.sh write $virtual_page $wr

    # Replace random data in the command with the page from application image
    dd bs=1 count=512 if=$APPIMAGE of=$wr skip=$(( $virtual_page * 512  )) seek=3 2>/dev/null
    
    echo "Generated $(( $i )) / $APPPAGES"
done

# Each Write command must complete with success
printf '\x00' >${OUTDIR}/reply

exit 0
