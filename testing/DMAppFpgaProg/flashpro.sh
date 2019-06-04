FLASHPRO='/c/Microsemi/Program_Debug_v11.8/bin/flashpro.exe'

# Execute operation with FPGA by FlashPro4 HW programmer
#
# Usage: flashpro  ACTION  STP_FILE  RESULT  TMPDIR
#    ACTION        FlashPro action to execute
#    STP_FILE      .STP file with data for the FPGA
#    RESULT        expected result, 0 (success) or 1 (failure)
#    TMPDIR        temporary directory for FlashPro project and script
flashpro()
{
    if [ $# -ne 4 ]; then
	echo 'flashpro_verify: ERROR: arguments' >&2
	return 1
    fi

    project_name="flashpro_project"

    action="$1"
    stp_file="$2"    
    project_dir="$4/$project_name"
    script_file="$4/flashpro_script"
    log_file="$project_dir/${project_name}.log"
    expected_result="$3"
    [ $expected_result -ne 0 ] && expected_result=1
    
    rm -rf "$project_dir" "$script_file"

    cat << EOF > "$script_file"
new_project -name {flashpro_project} -location {$(cygpath -a -w "$project_dir")} -mode {single}

configure_flashpro4_prg -vpump {OFF} 

set_programming_file -file {$(cygpath -a -w "$stp_file")}
set_programming_action -action {$action}
run_selected_actions

close_project
EOF

    "$FLASHPRO" script:"$(cygpath -a -w "$script_file")" 1>/dev/null 2>&1

    if [ ! -f "$log_file" -o ! -r "$log_file" ]; then
	return 1;
    else
	grep "The 'run_selected_actions' command succeeded\." "$log_file" 1>/dev/null 2>&1
	# Convert non-zero last return value to 1
	[ $? -eq 0 ]
	[ $? -eq $expected_result ]
    fi
}
