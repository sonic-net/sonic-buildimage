LOGOUT_MESSAGE_PATH="/etc/logout_message"
LOGOUT_MESSAGE=

if [[ -f "$LOGOUT_MESSAGE_PATH" ]]; then
    LOGOUT_MESSAGE=$(cat $LOGOUT_MESSAGE_PATH)
fi

if [[ -n "$LOGOUT_MESSAGE" ]]; then
    # Print logout message
    echo $LOGOUT_MESSAGE
fi

