#ip r | awk '/default/{print "interface "$5": "$9}'
IP=$(nslookup myip.opendns.com resolver1.opendns.com | awk '/^[[:space:]]*Address( 1)?: / { print $2 }')
echo -en "[$IP](https://whois.ru/$IP)"
