#!/bin/sh

GPIO_PATH=/sys/class/gpio

SIMADDR_PIN=$(uci -q get simman.core.simaddr_gpio_pin)
[ -z "$SIMADDR_PIN" ] && {
	logger -t $tag "Not set SIMADDR_PIN" && exit 0
}

SIMDET0_PIN=$(uci -q get simman.core.simdet0_gpio_pin)
[ -z "$SIMDET0_PIN" ] && {
	logger -t $tag "Not set SIMDET0_PIN" && exit 0
}

SIMDET1_PIN=$(uci -q get simman.core.simdet1_gpio_pin)
[ -z "$SIMDET1_PIN" ] && {
	logger -t $tag "Not set SIMDET1_PIN" && exit 0
}

if [ ! -d "$GPIO_PATH/gpio$SIMADDR_PIN" ]; then
	echo $SIMADDR_PIN > $GPIO_PATH/export
	echo out > $GPIO_PATH/gpio$SIMADDR_PIN/direction
	logger -t $tag "Exporting gpio$SIMADDR_PIN"
fi

if [ ! -d "$GPIO_PATH/gpio$SIMDET0_PIN" ]; then
	echo $SIMDET0_PIN > $GPIO_PATH/export
	echo in > $GPIO_PATH/gpio$SIMDET0_PIN/direction
	logger -t $tag "Exporting gpio$SIMDET0_PIN"
fi

if [ ! -d "$GPIO_PATH/gpio$SIMDET1_PIN" ]; then
	echo $SIMDET1_PIN > $GPIO_PATH/export
	echo in > $GPIO_PATH/gpio$SIMDET1_PIN/direction
	logger -t $tag "Exporting gpio$SIMDET1_PIN"
fi

sim1=$(cat $GPIO_PATH/gpio$SIMDET0_PIN/value)
sim2=$(cat $GPIO_PATH/gpio$SIMDET1_PIN/value)
simaddr=$(cat $GPIO_PATH/gpio$SIMADDR_PIN/value)

#sim1act=""
#sim2act=""
#sim1av=""
#sim2av=""

if [ "$sim1" == "1" ]; then
	sim1av="NOT INSERTED"
else
	sim1av="INSERTED"
fi
if [ "$sim2" == "1" ]; then
	sim2av="NOT INSERTED"
else
	sim2av="INSERTED"
fi
if [ "$simaddr" == "1" ]; then
	sim2act=" (ACT)"
else
	sim1act=" (ACT)"
fi

echo "'1 $sim1av$sim1act  |  2 $sim2av$sim2act'"