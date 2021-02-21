#!/bin/sh

ACTION=$1
PKG="$2 $3 $4 $5 $6"

update_(){
	opkg update > /dev/null 2>&1
	MSG="List updated."
}

list_(){
	PKGS="$(opkg list-upgradable)"
	MSG="\`\`\`\n$PKGS\`\`\`"
	if [ "$PKGS" ]; then
		MSG="Packages to upgrade are:\n${MSG}"
	else
		MSG="Noting to upgrade"
	fi
}

upgrade_(){
	PKGS="$(opkg list-upgradable | awk '{print $1}')"
	opkg upgrade $(echo ${PKGS}) > /dev/null 2>&1
	MSG="Upgraded package(s):\`\`\`\n${PKGS}\`\`\`"
}

install_(){
	opkg ${ACTION} ${PKG} > /dev/null 2>&1
	MSG="Package(s): $PKG installed."
}

remove_(){
	opkg ${ACTION} ${PKG} > /dev/null 2>&1
	MSG="Package(s): $PKG removed."
}

case ${ACTION} in
	update)
		update_
		echo -en "$MSG"
	;;
	list-upgrade)
		list_
		echo -en "$MSG"
	;;
	run-upgrade)
		upgrade_
		echo -en "$MSG"
	;;
	install)
		install_
		echo -en "$MSG"
	;;
	remove)
		remove_
		echo -en "$MSG"
	;;
	*) echo -en " Usage: */opkg {update|install|remove|list-upgrade|run-upgrade} [package(s) max 5]*" ;;
esac
