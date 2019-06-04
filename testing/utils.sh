info()
{
    echo " ==> INFO: $1"
}

msg()
{
    echo "    $*"
}

error()
{
    yup_comm_stop_server

    echo " ==> ERROR: $1" >&2
    exit 1
}

get_cmd_num()
{
    echo "$1" | cut -d_ -f1
}

compare_files()
{
    diff -q "$1" "$2" 1>/dev/null 2>&1
    ret=$?
    [ $ret -ne 0 ] && msg "Files '$1' and '$2' differ" >&2 
    return $ret
}

list_commands_and_replies()
{
    dir="$1"

    one_reply='no'
    [ -f "$dir/reply" ] && one_reply='yes'

    ls "$dir" | grep '[0-9][0-9]*_cmd' | sort -n | while read c; do
	n=$(get_cmd_num $c)
	
	if [ $one_reply = 'yes' ]; then
	    echo $dir/$c $dir/reply
	else
	    echo $dir/$c $dir/${n}_reply
	fi
    done
}

init_tmp_dir()
{
    if [ ! -d "$1" ]; then
	mkdir "$1"
    else
	rm -f "$1"/* 2>/dev/null
    fi

    [ ! -w "$1" ] && error "Temporary directory '$1' is not writable"
}

check_retval()
{
    [ $1 -ne 0 ] && error "$2"
}

test()
{
    if [ $# -ne 1 ]; then
	echo 'test: ERROR: incorect number of arguments'
	return 1
    fi
    
    while read cmd blessed_reply; do
	msg "$cmd $blessed_reply"

	test_reply=$1/reply
	
	yup_comm_execute_dm_cmd $cmd $test_reply
	[ $? -ne 0 ] && error "bad reply for command"

	compare_files $test_reply $blessed_reply
	[ $? -ne 0 ] && return 1
    done

    return 0
}
