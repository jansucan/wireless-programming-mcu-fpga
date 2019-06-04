#!/bin/sh

# Generates the YUP commands for DMBootloader for writing every page
# of the virtual page space with random data, and correct replies for
# reading of the pages

MIN_PAGE=1
MAX_PAGE=992
WRITE_OUTDIR=2_cmd_write_all_pages
READ_OUTDIR=3_cmd_read_all_pages

for i in $(seq $MIN_PAGE $MAX_PAGE); do
    virtual_page=$(( $i - 1 ))
    wr=${WRITE_OUTDIR}/${i}_cmd
    rd=${READ_OUTDIR}/${i}_cmd
    rd_reply=${READ_OUTDIR}/${i}_reply
    
    # Generate Write command
    ./create_dmbootloader_cmd.sh write $virtual_page $wr

    # Generate corresponding Read command
    ./create_dmbootloader_cmd.sh read $virtual_page $rd

    # Generate expected reply to the read command
    printf '\x00' >$rd_reply
    dd bs=1 if=$wr of=$rd_reply skip=3 seek=1 2>/dev/null    

    # Construct virtual space memory image for comparing with data
    # read HW programmer
    dd bs=1 if=$wr of=$READ_OUTDIR/memory_image skip=3 seek=$(( $virtual_page * 512)) 2>/dev/null
    
    echo "Generated $(( $i )) / $(( $MAX_PAGE ))"
done

# Each Write command must complete with success
printf '\x00' >${WRITE_OUTDIR}/reply

exit 0
