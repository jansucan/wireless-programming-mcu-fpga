#!/bin/bash

. ./flashpro.sh

. ../utils.sh
. ../yup_comm.sh


TMPDIR=/tmp/testing
COM_PORT=COM5
FPGA_DATA_ARR_FROM='./fpga_data/TutorKit1FlashRom1_v1004_Top - 2018-05-28_ARR_FROM.stp'
FPGA_DATA_ARR='./fpga_data/TutorKit1FlashRom1_v1004_Top - 2018-05-28_ARR.stp'
FPGA_DATA_FROM='./fpga_data/FlashRom1kbitPattern01_80 - 2018-07-27.stp'

operator_msg()
{
    if [ $# -ne 1 -a $# -ne 2 ]; then
	error "operator_msg: invalid number of arguments"
    fi

    case "$1" in
	"DM" | "FPRO" | "")
	    ;;
	*)
	    error "operator_msg: invalid first argument: $1"
	    ;;
    esac

    case "$2" in
	"KIT_VJTAG_ON" | "KIT_VJTAG_OFF" | "")
	    ;;
	*)
	    error "operator_msg: invalid second argument: $2"
	    ;;
    esac

    action_index=1

    echo
    echo "+----------------------------------------------------------------+"
    echo "|                                                                |"

    if [ "$2" = "KIT_VJTAG_ON" ]; then
	echo "|  ${action_index}) On the Starter Kit switch on VJTAG and VPUMP voltages      |"
	echo "|    (JP9 and JP12, pins 2 and 3)                                |"
	echo "|                                                                |"
	action_index=$(( $action_index + 1 ))
    elif [ "$2" = "KIT_VJTAG_OFF" ]; then
	echo "|  ${action_index}) On the Starter Kit switch off VJTAG and VPUMP voltages     |"
	echo "|    (JP9 and JP12, pins 3 and 4)                                |"
	echo "|                                                                |"
	action_index=$(( $action_index + 1 ))
    fi

    if [ "$1" = "DM" ]; then
	echo "|  ${action_index}) Connect Diagnostic Module to the Starter Kit               |"
	echo "|                                                                |"
	action_index=$(( $action_index + 1 ))
    elif [ "$1" = "FPRO" ]; then
	echo "|  ${action_index}) Connect FlashPro4 to the Starter Kit                       |"
	echo "|                                                                |"
	action_index=$(( $action_index + 1 ))
    fi

    echo "|  ${action_index}) Press Enter to continue                                    |"
    echo "|                                                                |"
    echo "+----------------------------------------------------------------+"

    read
}

execute_DMAppFpgaProg()
{
    info "Executing DMAppFpgaProg"

    basedir=1_execute_DMAppFpgaProg
    
    operator_msg DM KIT_VJTAG_OFF
    
    yup_comm_execute_dm_cmd $basedir/1_cmd_get_id $TMPDIR/reply
    check_retval $? "Cannot get ID of a DM program"

    if cmp --silent $TMPDIR/reply $basedir/1_reply_DMAppFpgaProg ; then
	info "DMAppFpgaProg is already running"
	return
    elif ! cmp --silent $TMPDIR/reply $basedir/1_reply_DMBootloader ; then
	error "DMAppFpgaProg nor DMBootloader is running"
    fi

    info "Executing DMAppFpgaProg by DMBootloader"
    
    yup_comm_execute_dm_cmd $basedir/2_cmd_execute_DMAppFpgaProg $TMPDIR/reply
    check_retval $? "Cannot send command to DMBootloader for execution of DMAppFpgaProg"

    if ! cmp --silent $TMPDIR/reply $basedir/2_reply ; then
	error "Cannot execute DMAppFpgaProg by DMBootloader"
    fi

    info "OK"
}

test_command_properties()
{
    info "Testing basic command properties"

    basedir=2_basic
    
    list_commands_and_replies $basedir/1 | test $TMPDIR
    check_retval $?

    # Result of the gettext should be checked by size
    msg "./$basedir/2/25_cmd_get_text $TMPDIR/reply"
    yup_comm_execute_dm_cmd ./$basedir/2/25_cmd_get_text $TMPDIR/reply
    [ $(du -b $TMPDIR/reply | cut -f1) -ge 2 ]
    check_retval $?

    list_commands_and_replies $basedir/3 | test $TMPDIR
    check_retval $?
    
    info "OK"
}

test_write_verify_erase_array_from()
{
    info "Testing writing, verification and erasing of ARRAY and FROM"

    basedir=3_write_verify_erase_array_from
    
    operator_msg "" KIT_VJTAG_ON
    list_commands_and_replies $basedir/1_write | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/2_verify | test $TMPDIR
    check_retval $?
    
    operator_msg FPRO
    flashpro VERIFY "$FPGA_DATA_ARR_FROM" 0 $TMPDIR
    check_retval $?
    
    operator_msg DM
    list_commands_and_replies $basedir/3_erase | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/4_verify | test $TMPDIR
    check_retval $?
    
    operator_msg FPRO
    flashpro VERIFY "$FPGA_DATA_ARR_FROM" 1 $TMPDIR
    check_retval $?
    
    info "OK"
}

test_write_verify_erase_from()
{
    info "Testing writing, verification and erasing of FROM"

    basedir=4_write_verify_erase_from

    operator_msg DM
    list_commands_and_replies $basedir/1_write_array | test $TMPDIR
    check_retval $?    
    list_commands_and_replies $basedir/2_erase_from | test $TMPDIR
    check_retval $?

    operator_msg FPRO
    flashpro VERIFY "$FPGA_DATA_ARR" 0 $TMPDIR
    check_retval $?
    flashpro VERIFY "$FPGA_DATA_FROM" 1 $TMPDIR
    check_retval $?
    
    operator_msg DM
    list_commands_and_replies $basedir/3_write_from | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/4_verify_array | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/5_verify_from | test $TMPDIR
    check_retval $?

    operator_msg FPRO
    flashpro VERIFY "$FPGA_DATA_ARR" 0 $TMPDIR
    check_retval $?
    flashpro VERIFY "$FPGA_DATA_FROM" 0 $TMPDIR
    check_retval $?
    
    operator_msg DM
    list_commands_and_replies $basedir/6_erase_from | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/7_verify_array | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/8_verify_from | test $TMPDIR
    check_retval $?
    
    operator_msg FPRO
    flashpro VERIFY "$FPGA_DATA_ARR" 0 $TMPDIR
    check_retval $?
    flashpro VERIFY "$FPGA_DATA_FROM" 1 $TMPDIR
    check_retval $?
    
    info "OK"
}

test_write_verify_erase_array()
{
    info "Testing writing, verification and erasing of ARRAY"

    basedir=5_write_verify_erase_array

    operator_msg DM
    list_commands_and_replies $basedir/1_erase_array | test $TMPDIR
    check_retval $?    
    list_commands_and_replies $basedir/2_write_from | test $TMPDIR
    check_retval $?

    operator_msg FPRO
    flashpro VERIFY "$FPGA_DATA_ARR" 1 $TMPDIR
    check_retval $?
    flashpro VERIFY "$FPGA_DATA_FROM" 0 $TMPDIR
    check_retval $?
    
    operator_msg DM
    list_commands_and_replies $basedir/3_write_array | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/4_verify_array | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/5_verify_from | test $TMPDIR
    check_retval $?

    operator_msg FPRO
    flashpro VERIFY "$FPGA_DATA_ARR" 0 $TMPDIR
    check_retval $?
    flashpro VERIFY "$FPGA_DATA_FROM" 0 $TMPDIR
    check_retval $?
    
    operator_msg DM
    list_commands_and_replies $basedir/6_erase_array | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/7_verify_array | test $TMPDIR
    check_retval $?
    list_commands_and_replies $basedir/8_verify_from | test $TMPDIR
    check_retval $?
    
    operator_msg FPRO
    flashpro VERIFY "$FPGA_DATA_ARR" 1 $TMPDIR
    check_retval $?
    flashpro VERIFY "$FPGA_DATA_FROM" 0 $TMPDIR
    check_retval $?
    
    info "OK"
}

#-----------------------------------------------------------------------------
# MAIN
#-----------------------------------------------------------------------------

yup_comm_start_server $COM_PORT
check_retval $? "Cannot start YUP communication utility server"

init_tmp_dir $TMPDIR

execute_DMAppFpgaProg
test_command_properties
test_write_verify_erase_array_from
test_write_verify_erase_from
test_write_verify_erase_array

yup_comm_stop_server

exit 0
