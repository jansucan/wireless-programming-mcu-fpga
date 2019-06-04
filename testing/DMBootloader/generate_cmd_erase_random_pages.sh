#!/bin/sh

# Generates the YUP commands for DMBootloader for erasing random pages

MIN_PAGE=1
MAX_PAGE=992
PAGE_COUNT=16
OUTDIR=4_erase_random_pages
MEMIMAGE=3_cmd_read_all_pages/memory_image

tmp_cmd=${OUTDIR}/0_erase
mkdir $tmp_cmd
cp $MEMIMAGE $tmp_cmd

for i in $(seq 1 $PAGE_COUNT); do
    # The same page numbers can possibly be generated
    virtual_page=$(( $RANDOM % $MAX_PAGE ))

    d=${OUTDIR}/${i}_erase
    mkdir ${d}
    
    o=${d}/1_cmd
    r=${d}/1_reply
    m_base=${OUTDIR}/$(( $i - 1 ))_erase/memory_image
    m=${d}/memory_image
    
    # Generate erase command and gradually construct images of virtual
    # address space with the random pages erased
    ./create_dmbootloader_cmd.sh erase $virtual_page $o
    # Each Erase command must complete with success
    printf '\x00' >$r
    # Memory image for this Erase command will be based on the memory image for the previous command
    cp $m_base $m
    { dd if=/dev/zero bs=1 count=512 | tr '\000' '\377' | \
    	  dd bs=1 of=$m seek=$(( $virtual_page * 512)) conv=notrunc; } 2>/dev/null

    echo "Generated $(( $i )) / $(( $PAGE_COUNT ))"
done

rm -rf $tmp_cmd

exit 0
