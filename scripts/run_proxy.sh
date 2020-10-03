#!/bin/bash

function usage() {
    echo "Usage: $0 [ --mode <1/2, 1 - explicit only, 2 - transparent and explicit> ] [ --ip <proxy-ip> ] [ --port <proxy-port> ] [ --ca <ca certificate to use> ] [ -v to run with valgrind ] [ -g to run with gdb ] [ -h print help]" 1>&2
    exit 1
}

APP_NAME=$(grep -oP 'project\(\K[^\)]+' CMakeLists.txt)
IP=
PORT=5443
CA_CERTIFICATE=./scerts/ca_cert.pem
MODE=2
RUNNER=None

ALL_OPTS=($@)
OPTIONS=$(getopt -o vgh --long mode:,ip:,port:,ca: -n ${APP_NAME} -- "$@" 2> /dev/null)

eval set -- "${OPTIONS}"

while true; do
    case "$1" in
        --mode) MODE=$2; shift 2;;
        --ip) IP=$2; shift 2;;
        --port) PORT=$2; shift 2;;
        --ca) CA_CERTIFICATE=$2; shift 2;;
        -g) RUNNER=gdb; shift;;
        -v) RUNNER=valgrind; shift;;
        -h) usage;;
        *) break;;
    esac
done

if [[ "${MODE}" != "1" && "${MODE}" != "2" ]]; then
    echo "--mode accept only 1 or 2"
    exit 1
fi

if [ -z $IP ]; then
    echo "You must supply an ip to use"
    exit 1
fi

function add_rules() {
    sudo ip link add tls_proxy type dummy
    sudo ifconfig tls_proxy ${IP} netmask 255.255.255.0 up
    if [ "${MODE}" == "2" ]; then
    sudo iptables -t nat -A OUTPUT -p tcp -s ${IP}/32 --dport 443 -j ACCEPT
    sudo iptables -t nat -A OUTPUT -p tcp --dport 443 -j DNAT --to-destination ${IP}:${PORT}
    sudo iptables -t nat -A POSTROUTING -p tcp -d ${IP} --dport ${PORT} -j MASQUERADE
    sudo iptables -A FORWARD -p tcp -s ${IP} --dport 443 -m state --state NEW,ESTABLISHED,RELATED -j ACCEPT
    sudo iptables -A FORWARD -p tcp -d ${IP} --dport 443 -m state --state NEW,ESTABLISHED,RELATED -j ACCEPT
    fi
}

function delete_rules() {
    sudo ifconfig tls_proxy down
    sudo ip link delete tls_proxy
    if [ "${MODE}" == "2" ]; then
    sudo iptables -t nat -D OUTPUT -p tcp -s ${IP}/32 --dport 443 -j ACCEPT
    sudo iptables -t nat -D OUTPUT -p tcp --dport 443 -j DNAT --to-destination ${IP}:${PORT}
    sudo iptables -t nat -D POSTROUTING -p tcp -d ${IP} --dport ${PORT} -j MASQUERADE
    sudo iptables -D FORWARD -p tcp -s ${IP} --dport 443 -m state --state NEW,ESTABLISHED,RELATED -j ACCEPT
    sudo iptables -D FORWARD -p tcp -d ${IP} --dport 443 -m state --state NEW,ESTABLISHED,RELATED -j ACCEPT
    fi
}

function handle_ca_certificate() {
	CA_CERT_LOCATION=/usr/local/share/ca-certificates/${APP_NAME}.crt
	CA_CERT=${CA_CERTIFICATE}

	if [ ! -f ${CA_CERT_LOCATION} ]; then
		echo "Adding CA Cert to local store"
		sudo cp ${CA_CERT} ${CA_CERT_LOCATION}

		sudo update-ca-certificates
	fi
}

trap 'delete_rules; exit 0' SIGINT

add_rules
handle_ca_certificate

mkdir certs 2>/dev/null
rm -fr certs/* 2>/dev/null

APP=$(find . -name ${APP_NAME})

if [ -z ${APP} ]; then
    echo "tls-proxy binary was not found. Make sure to build the project before you trying to run it!"
else
	if [ "${RUNNER}" == "valgrind" ]; then
	    valgrind --leak-check=full --show-leak-kinds=all ./${APP} "${ALL_OPTS[@]}"
	elif  [ "${RUNNER}" == "gdb" ]; then
	    gdb ./${APP}
	else
    	./${APP} "${ALL_OPTS[@]}"
	fi
fi

delete_rules
