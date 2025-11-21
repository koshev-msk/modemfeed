#!/bin/sh

# luci-app-smstools3 command handler by koshev-msk 2025

#SECTIONS=$(uci show smstools3 | awk -F [\.][\]\[\@=] '/=command/{print $3}')
SECTIONS=$(uci show smstools3 | awk -F [\.,=] '/=command/{print $2}')
PHONE=$(uci -q get smstools3.@root_phone[0].phone)

# Send SMS
send_sms() {
    local phone="$1"
    local message="$2"
    local modem="$3"

    if [ -n "$modem" ] && [ "$modem" != "" ]; then
        # if modem selected
        /usr/bin/send_sms "$phone" "$message" "$modem"
    else
        # if not
        /usr/bin/send_sms "$phone" "$message"
    fi
}

# smscommand function
smscmd(){
    local message_modem="$1"

    for s in $SECTIONS; do
        CMD="$(uci -q get smstools3.${s}.command)"
        MSG="$(echo $content)"
        COMMAND_MODEM=$(uci -q get smstools3.${s}.modem)

        # check recieved 
        case $MSG in
            *${CMD}*)
                # check modem
                if [ -n "$COMMAND_MODEM" ] && [ "$COMMAND_MODEM" != "" ]; then
                    if [ "$message_modem" != "$COMMAND_MODEM" ]; then
                        # if not from modem message
                        continue
                    fi
                fi
                # run commmand
                ANSWER=$(uci -q get smstools3.${s}.answer)
                if [ "$ANSWER" ]; then
                    send_sms "$PHONE" "$ANSWER" "$COMMAND_MODEM"
                fi
                EXEC=$(uci -q get smstools3.${s}.exec)
                DELAY=$(uci -q get smstools3.${s}.delay)
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
    message_modem=`grep "Modem:" $2 | awk -F ': ' '{printf $2}'`
    # check ROOT messages
    for n in ${PHONE}; do
        if [ "$from" -eq "$n" ]; then
            PHONE=$n
            smscmd "$message_modem"
        fi
    done
fi
