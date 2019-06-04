#!/bin/sh

# Generates commands for whole DirectC operation. It expects to find
# the first command created by the user in output directory. It sends
# this command to DAppFpgaProg, reads reply, uses the reply to
# constructs the next command with data from .DAT file and sends the
# next command to DMAppFpgaProg. This is repeated until the reply
# contains result of the DirectC operation.

. ../utils.sh
. ../yup_comm.sh

COM_PORT='COM5'

extract_uint32()
{
    file=$1
    offset=$2

    n=0x$(dd bs=1 count=4 if=$file skip=$offset 2>/dev/null | \
	     hexdump -C | awk '{ print $5 $4 $3 $2 }')

    printf "%d\n" $n
}

usage()
{
    {
	echo "Usage: $0  DIR  DAT"
	echo "  DIR  directory with initial command and for generated commands"
	echo "  DAT  .DAT file for DirectC operation"
    } >&2
}

if [ $# -ne 2 ]; then
    echo "ERROR: invalid number of arguments" >&2
    usage
    exit 1
fi

dir="$1"
datfile="$2"

yup_comm_start_server $COM_PORT

while true; do

    next_cmd=$dir/$(ls "$dir" | grep '[0-9][0-9]*_cmd' | sort -n | tail -n1)
    next_n=$(basename $next_cmd | cut -d_ -f1)
    next_reply=$dir/${next_n}_reply

    yup_comm_execute_dm_cmd $next_cmd $next_reply
    [ $? -ne 0 ] && error "Cannot execute DMAppFpgaProg command $next_cmd"

    if [ $(du -b $next_reply | awk '{ printf $1 }') -ne 9 ]; then
	break
    fi
    
    offset=$(extract_uint32 $next_reply 1)
    size=$(extract_uint32 $next_reply 5)

    next_n=$(( $next_n + 1 ))

    echo "Generating command ${next_n}_reply"

    # Construct command Provide page data
    printf '\x04' >"$dir/${next_n}_cmd"
    dd bs=1 count=$size skip=$offset seek=1 \
       if="$datfile" of=$dir/${next_n}_cmd 2>/dev/null

done

yup_comm_stop_server
