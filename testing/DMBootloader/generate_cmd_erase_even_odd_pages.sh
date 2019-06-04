#!/bin/sh

# Generates the YUP commands for DMBootloader for erasing even and odd
# pages

MIN_PAGE=1
MAX_PAGE=992
EVEN_OUTDIR=5_erase_even_pages
ODD_OUTDIR=6_erase_odd_pages
MEMIMAGE=3_cmd_read_all_pages/memory_image

cp $MEMIMAGE ${EVEN_OUTDIR}
cp $MEMIMAGE ${ODD_OUTDIR}

cmd_num=1
for i in $(seq $MIN_PAGE $MAX_PAGE); do
    virtual_page=$(( $i - 1 ))
    e=${EVEN_OUTDIR}/${cmd_num}_cmd
    o=${ODD_OUTDIR}/${cmd_num}_cmd

    # Generate erase command and gradually construct images of virtual
    # address space with even and odd pages erased
    if [ $(( $virtual_page % 2 )) -eq 0 ]; then
	./create_dmbootloader_cmd.sh erase $virtual_page $e
	{ dd if=/dev/zero bs=1 count=512 | tr '\000' '\377' | \
	      dd bs=1 of=${EVEN_OUTDIR}/memory_image seek=$(( $virtual_page * 512)) conv=notrunc; } 2>/dev/null
    else
	./create_dmbootloader_cmd.sh erase $virtual_page $o
	{ dd if=/dev/zero bs=1 count=512 | tr '\000' '\377' | \
	      dd bs=1 of=${ODD_OUTDIR}/memory_image seek=$(( $virtual_page * 512)) conv=notrunc; } 2>/dev/null
	cmd_num=$(( $cmd_num + 1 ))
    fi

    echo "Generated $(( $i )) / $(( $MAX_PAGE ))"
done

# Each Erase command must complete with success
printf '\x00' >${EVEN_OUTDIR}/reply
printf '\x00' >${ODD_OUTDIR}/reply

exit 0
