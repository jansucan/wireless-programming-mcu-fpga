ATPROGRAM_EXE='/c/Program Files (x86)/Atmel/Studio/7.0/atbackend/atprogram.exe'
ATPROGRAM_OPTS='-t jtagice3 -i jtag -d at32uc3c2512c'
MCU_FLASH_BASE_ADDR=2147483648 # = 0x80000000

# Read FLASH memory of the MCU
#
# Usage: atprogram_read_flash  OUTPUT_FILE  FIRST_PAGE  PAGE_COUNT
#    OUTPUT_FILE   binary output file that will contain data from the FLASH
#    FIRST_PAGE    first page to be read
#    PAGE_COUNT    page count to be read from the FIRST_PAGE (including)
atprogram_read_flash()
{
    if [ $# -ne 3 ]; then
	echo 'atprogram_read_flash: ERROR: arguments' >&2
	return 1
    fi
    
    start_address=$(( $MCU_FLASH_BASE_ADDR + ( $2 * 512 ) ))
    bytes_to_read=$(( $3 * 512 ))

    "$ATPROGRAM_EXE" $ATPROGRAM_OPTS read --format bin -f "$(cygpath -w "$1")" \
		     -o $start_address -s $bytes_to_read 1>/dev/null
}

atprogram_write_bootloader()
{
    if [ $# -ne 1 ]; then
	echo 'atprogram_write_bootloader: ERROR: arguments' >&2
	return 1
    fi

    "$ATPROGRAM_EXE" $ATPROGRAM_OPTS program --chiperase --verify --format hex \
		     --file "$(cygpath -w "$1")" 1>/dev/null 2>&1
}

atprogram_protect_bootloader()
{
    if [ $# -ne 0 ]; then
	echo 'atprogram_protect_bootloader: ERROR: arguments' >&2
	return 1
    fi
    
    {
	# Set bootloader area protection
	"$ATPROGRAM_EXE" $ATPROGRAM_OPTS write  -fs --values FFF5FFFF
	[ $? -ne 0 ] && return 1
	# Write must fail
	"$ATPROGRAM_EXE" $ATPROGRAM_OPTS write -o $MCU_FLASH_BASE_ADDR --values 01020304
	[ $? -eq 0 ] && return 1 || return 0
    } 1>/dev/null 2>&1
}

atprogram_set_security_bit()
{
    if [ $# -ne 0 ]; then
	echo 'atprogram_set_security_bit: ERROR: arguments' >&2
	return 1
    fi
    
    {
	# Set security bit
	"$ATPROGRAM_EXE" $ATPROGRAM_OPTS secure
	[ $? -ne 0 ] && return 1
	# Write must fail with error code 16
	"$ATPROGRAM_EXE" $ATPROGRAM_OPTS write -o $MCU_FLASH_BASE_ADDR --values 01020304
	[ $? -eq 16 ] && return 0 || return 1
    } 1>/dev/null 2>&1
}
