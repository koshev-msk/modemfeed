#!/bin/sh

SECTIONS=$(uci show smstools3 | awk -F [\]\[\@=] '/=command/{print $3}')
PHONE=$(uci -q get smstools3.@root_phone[0].phone)


smscmd(){
        for s in $SECTIONS; do
                CMD="$(uci -q get smstools3.@command[$s].command)"
                MSG="$(echo $content)"
                case $CMD in
                        *${MSG}*)
                                ANSWER=$(uci -q get smstools3.@command[$s].answer)
                                if [ "$ANSWER" ]; then
                                        /usr/bin/sendsms $PHONE "$ANSWER"
                                fi
                                EXEC=$(uci -q get smstools3.@command[$s].exec)
                                DELAY=$(uci -q get smstools3.@command[$s].delay)
                                if [ $DELAY ]; then
                                        sleep $DELAY && $EXEC &
                                else
                                        $EXEC
                                fi
                        ;;
                esac
        done
}

# parse incoming message
if [ "$1" == "RECEIVED" ]; then
	from=`grep "From:" $2 | awk -F ': ' '{printf $2}'`
	content=$(sed -e '1,/^$/ d' < "$2")
	# check ROOT messages
	for n in ${PHONE}; do
		if [ "$from" -eq "$n" ]; then
			PHONE=$n
			smscmd
		else
			exit 0
		fi
	done
fi

