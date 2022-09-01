#ip r | awk '/default/{print "interface "$5": "$9}'
nslookup myip.opendns.com resolver1.opendns.com | awk '/^Address( 1)?: / { print $3 }'
