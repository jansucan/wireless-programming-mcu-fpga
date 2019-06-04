#!/bin/bash

. ./atprogram.sh

. ../utils.sh
. ../yup_comm.sh


TMPDIR=/tmp/testing
COM_PORT=COM5
DMBOOTLOADER_HEX="../../impl/DMBootloader/DMBootloader/Release/DMBootloader.hex"

test_command_properties()
{
    info "Testing basic command properties"
    
    list_commands_and_replies 1_basic | test $TMPDIR

    check_retval $?
    info "OK"
}

test_write_all_pages()
{
    info "Testing writing of all pages in virtual page space"
    
    list_commands_and_replies 2_cmd_write_all_pages | test $TMPDIR

    check_retval $?
    info "OK"
}

test_read_all_pages()
{
    info "Testing reading of all pages in virtual page space"

    basedir=3_cmd_read_all_pages
    
    list_commands_and_replies $basedir | test $TMPDIR
    check_retval $?

    # Check memory content by HW programmer
    atprogram_read_flash $TMPDIR/memory_image 32 992
    compare_files $TMPDIR/memory_image $basedir/memory_image
    [ $? -ne 0 ] && error "JTAGICE3 HW programmer has read a different FLASH content"

    info "OK"
}

test_erase_random_pages()
{
    info "Testing erasing of random pages in virtual page space"

    basedir=4_erase_random_pages

    for i in $(seq 1 16); do
	cmd_dir=$basedir/${i}_erase
	
	list_commands_and_replies $cmd_dir | test $TMPDIR
	check_retval $?

	# Check memory content by HW programmer
	atprogram_read_flash $TMPDIR/memory_image 32 992
	compare_files $TMPDIR/memory_image $cmd_dir/memory_image
	[ $? -ne 0 ] && error "JTAGICE3 HW programmer has read a different FLASH content"
    done
    
    info "OK"
}

test_erase_even_pages()
{
    info "Testing erasing of even pages in virtual page space"

    basedir=5_erase_even_pages
    
    list_commands_and_replies $basedir | test $TMPDIR
    check_retval $?

    # Check memory content by HW programmer
    atprogram_read_flash $TMPDIR/memory_image 32 992
    compare_files $TMPDIR/memory_image $basedir/memory_image
    [ $? -ne 0 ] && error "JTAGICE3 HW programmer has read a different FLASH content"

    info "OK"
}

test_erase_odd_pages()
{
    info "Testing erasing of odd pages in virtual page space"

    basedir=6_erase_odd_pages
    
    list_commands_and_replies $basedir | test $TMPDIR
    check_retval $?
    
    # Check memory content by HW programmer
    atprogram_read_flash $TMPDIR/memory_image 32 992
    compare_files $TMPDIR/memory_image $basedir/memory_image
    [ $? -ne 0 ] && error "JTAGICE3 HW programmer has read a different FLASH content"

    info "OK"
}

test_write_DMAppFpgaProg()
{
    info "Testing writing of DMAppFpgaProg"
    
    list_commands_and_replies 7_write_DMAppFpgaProg | test $TMPDIR

    check_retval $?
    info "OK"
}


test_execute_DMAppFpgaProg()
{
    info "Testing execution of DMAppFpgaProg"

    list_commands_and_replies 8_exec_DMAppFpgaProg | test $TMPDIR
    check_retval $?
    
    # It's needed to wait for DMAppFpgaProg initialization to complete
    sleep 4

    list_commands_and_replies 9_check_DMAppFpgaProg | test $TMPDIR

    check_retval $?
    info "OK"
}


#-----------------------------------------------------------------------------
# MAIN
#-----------------------------------------------------------------------------

info "Programming DMBootloader into the MCU"
atprogram_write_bootloader $DMBOOTLOADER_HEX
check_retval $? "Cannot program DMBootloader into the FLASH memory of the MCU"
info "OK"

info "Locking FLASH pages of DMBootloader's memory area through BOOTPROT fuses"
atprogram_protect_bootloader
check_retval $? "Cannot lock FLASH pages of the DMBootloader's memory area"
info "OK"

# Wait for DMBootloader to complete initialization of DM
sleep 4

yup_comm_start_server $COM_PORT
check_retval $? "Cannot start YUP communication utility server"

init_tmp_dir $TMPDIR

test_command_properties
test_write_all_pages
test_read_all_pages
test_erase_random_pages
# Restore the virtual address space data
test_write_all_pages
test_erase_even_pages
# Restore the virtual address space data
test_write_all_pages
test_erase_odd_pages
test_write_DMAppFpgaProg
test_execute_DMAppFpgaProg

yup_comm_stop_server

info "Setting security bit to improve DMBootloader's protection"
atprogram_set_security_bit
check_retval $? "Cannot set the security bit"
info "OK"

exit 0
