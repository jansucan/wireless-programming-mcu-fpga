YUP_COMM_SERVER=../yup-comm/yup-comm-server.exe
YUP_COMM_CLIENT=../yup-comm/yup-comm-client.exe

yup_comm_start_server()
{
    $YUP_COMM_SERVER $1 1>/dev/null 2>&1 &
    sleep 2
    $YUP_COMM_CLIENT ping 1>/dev/null 2>&1
}

yup_comm_stop_server()
{
    $YUP_COMM_CLIENT exit 1>/dev/null 2>&1
}

yup_comm_execute_dm_cmd()
{
    if [ $# -ne 2 ]; then
	echo 'yup_comm_execute_dm_cmd: ERROR: incorect number of arguments'
	return 1
    fi
    
    win_cmd_path="$(cygpath -w "$1" | sed 's,\\,\\\\,g')"
    win_reply_path="$(cygpath -w "$2" | sed 's,\\,\\\\,g')"

    $YUP_COMM_CLIENT "${win_cmd_path},${win_reply_path}" 1>/dev/null 2>&1
}
