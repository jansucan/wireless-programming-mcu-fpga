#!/bin/sh

# Generates the Erase, Read, and Write YUP commands for DMBootloader.
# Write command is generated with random data of the page.

cmd_write_header()
{
    little_endian=$(printf "%04X\n" $2 | sed 's,^\(..\)\(..\)$,\2\1,')
    
    printf '\x'$1 >"$file"
    printf '\x'$(echo $little_endian | cut -c1-2) >>"$3"
    printf '\x'$(echo $little_endian | cut -c3-4) >>"$3"
}

usage()
{
    {
	echo "Usage: $0  CMD  PAGE_NUMBER  OUT_FILE"
	echo "  CMD          write, read, or erase"
	echo "  PAGE_NUMBER  16-bit page number"
	echo "  OUT_FILE     file to which the command's binary data will be written"
    } >&2
    exit 1
}

[ $# -ne 3 ] && usage

cmd=$1
page_num=$2
file="${3}"

case "$cmd" in
    write)
	cmd_write_header 00 $page_num "$file"
        ;;
    read)
	cmd_write_header 02 $page_num "$file"
        ;;
    erase)
	cmd_write_header 01 $page_num "$file"
        ;;
    *)
        echo "Unknown command requested" >&2
	usage
esac

[ "$1" = "write" ] && dd if=/dev/urandom of="$file" bs=1 count=512 seek=3 2>/dev/null

exit 0
