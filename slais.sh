#!/bin/sh

# The script for automated installation of latest version of Squid
# Version: 0.1.0
# Date:    30-06-2014

trap clean INT TERM EXIT;
TMP_DIR="/tmp/squid.$(date +%s)";
PREFIX="/opt/squid";
CONF_FILE="${CONF_FILE:-"${PREFIX}/etc/squid.conf"}";
CONF_AUTH="${CONF_AUTH:-"${PREFIX}/etc/slais.conf"}";
CONF_ACCESS="${CONF_ACCESS:-"${PREFIX}/etc/http_access.conf"}";
NCSA_AUTH="${NCSA_AUTH:-"${PREFIX}/libexec/basic_ncsa_auth"}";
PASSWD_FILE_BASIC="${PASSWD_FILE_BASIC:-"${PREFIX}/etc/passwd"}";


clean() {
rm -rf "${TMP_DIR}";
}

put_aut_conf() {
CONF_FILE="${CONF_FILE:-${1}}";
CONF_AUTH="${CONF_AUTH:-${2}}";
CONF_ACCESS="${CONF_ACCESS:-${3}}";

cp "${CONF_FILE}" "${CONF_FILE}.$(date +%s)"
sed '/^[[:space:]]*[^#]*http_access/s/^\(.*\)$/\n# Commented by slais, '"$(date -u)"' #\n#slais# \1/' "${CONF_FILE}" > "${CONF_FILE}.tmp";
mv "${CONF_FILE}.tmp" "${CONF_FILE}";
echo "include ${CONF_AUTH}" >> "${CONF_FILE}";
echo "include ${CONF_ACCESS}" >> "${CONF_FILE}";
}



conf_auth() {
cat >"${1}"<<EOF
auth_param basic program ${NCSA_AUTH} ${PASSWD_FILE_BASIC}
acl aut_basic proxy_auth REQUIRED
forwarded_for off
EOF
touch "${PASSWD_FILE_BASIC}";
chown nobody:nogroup "${PASSWD_FILE_BASIC}";
conf_access "http_access deny !aut_basic" "http_access deny !Safe_ports" "http_access deny CONNECT !SSL_ports";
}

conf_access() {
CONF_ACCESS="${CONF_ACCESS:-${1}}";

while [ "${#}" -gt "0" ]; do
	echo "${1}" >> "${CONF_ACCESS}";
	shift;
done
}

find_version() {
wget "http://www.squid-cache.org/Versions" -O "${PWD}/versions";

awk '
/Stable[\ \t\v]*Versions/ { FLAG = 1; }
FLAG && /Development[\ \t\v]*Versions/ { FLAG = ""; }
FLAG && /Latest[\ \t\v]*Release/ { FOUND = 1; }
FOUND {
	if($0 ~ "href=") {
		FS = "<td>";
		$0 = $0;
		match($2, /href="[^"]*/);
		LINK = substr($2, RSTART, RLENGTH);
		sub(/href="[\ \t\v]*/, "", LINK);
		VERSION = $4;
		sub(/<\/td.*/, "", VERSION);
		printf "http://www.squid-cache.org/Versions/%ssquid-%s.tar.xz\n", LINK, VERSION;
		exit
		}
	}' "${PWD}/versions";
}


download() {
wget -c "${URL}" -P "${PWD}";
if [ $? -ne 0 ]; then
	exit;
fi
}

build() {
PREFIX="${PREFIX:-/opt/squid}";
if [ -e "${PREFX}" ]; then
	echo "Moving ${PREFIX} to ${PREFIX}.old"
#	mv "${PREFIX}" "${PREFIX}.old.$(date +%s)";
fi
	
apt-get -y build-dep squid3;
apt-get -y install gcc make build-essential;

ARCHIVE="${URL##*/}";
tar xvf "${ARCHIVE}";
SOURCE_DIR="${ARCHIVE%.tar.xz}";
cd "${SOURCE_DIR}";
./configure \
--prefix="${PREFIX}" \
--enable-auth \
--enable-auth-basic="DB,fake,getpwnam,NCSA,PAM" \
--enable-auth-digest="file"

if [ $? -ne 0 ]; then
	echo 1>&2 'ERROR!';
	echo 1>&2 'Configuration problems!';
	exit;
fi

make all;

if [ $? -ne 0 ]; then
	echo 1>&2 'ERROR!';
	echo 1>&2 'Make: failed!';
	exit;
fi

make install;

if [ $? -ne 0 ]; then
	echo 1>&2 'ERROR!';
	echo 1>&2 'Make install: failed!';
	exit
fi

chown -R nobody:nogroup "${PREFIX}/var";
chmod -R g+w "${PREFIX}/var/run";
}

rules() {
cat >"/etc/apparmor.d/squid.slais"<<EOF
#include <tunables/global>

${PREFIX}/sbin/squid {
  #include <abstractions/base>
  #include <abstractions/kerberosclient>
  #include <abstractions/nameservice>

  capability net_raw,
  capability setuid,
  capability setgid,
  capability sys_chroot,

  # pinger
  network inet raw,
  network inet6 raw,

  /etc/mtab r,
  @{PROC}/[0-9]*/mounts r,
  @{PROC}/mounts r,

  # squid configuration
  ${PREFIX}/etc/** r,
  ${PREFIX}/var/** rwk,
  ${PREFIX}/share/** r,
  ${PREFIX}/libexec/** Pixrm,
}
EOF

cat >"/etc/init/squid.slais.conf"<<EOF
# squid - SQUID HTTP proxy-cache
#

description     "HTTP proxy-cache"
author          "Chuck Short <zulcss@ubuntu.com>"

# The second "or" condition is to start squid in case it failed to start
# because no real interface was there.
start on runlevel [2345]
stop on runlevel [!2345]

respawn
normal exit 0

env CONFIG="${PREFIX}/etc/squid.conf"
env SQUID_ARGS="-YC"

script
        if [ -f /etc/default/squid ]; then
                . /etc/default/squid
        fi

        umask 027
        ulimit -n 65535
        exec ${PREFIX}/sbin/squid -N \$SQUID_ARGS -f \$CONFIG
end script
EOF
}

add_port() {
printf 2>&1 "Please, enter number of the port\n: ";
                read ANSWER;
                while netstat -lntu | grep -q >/dev/null 2>&1 ":${ANSWER}"; do
                        printf 2>&1 "Port $ANSWER already in use\nPlease, enter number of the port\n: ";
                        read ANSWER;
                done
                echo "http_port $ANSWER" >> "${CONF_FILE}";
                printf "Do you want add another port (y/n)?\n: ";
                case $ANSWER in
                        Y|y|yes|[Y|y][E|e][S|s])
                                                add_port;;
                esac
}

set_port() {
echo 2>&1 "The Squid port(s):";
sed -n 's/^[[:space:]]*[^#]*http_port[[:space:]]*\(.*\)/\1/p' "${CONF_FILE}";
printf 2>&1 "Do you want change the port (y/n)?\n: ";
read ANSWER;

case $ANSWER in
	Y|y|[Y|y][E|e][S|s])
		sed 's/^\([[:space:]]*[^#]*http_port\)/\n# Commented by slais, '"$(date -u)"' #\n#slais# \1/' "${CONF_FILE}" > "${CONF_FILE}.tmp";
                mv "${CONF_FILE}.tmp" "${CONF_FILE}";
		add_port;;
esac
}


# Main #

if [ ! -e "${TMP_DIR}" ]; then
	mkdir -p "${TMP_DIR}";
fi
cd $TMP_DIR;
URL=$(find_version);
download "${URL}";
build;
put_aut_conf "/opt/squid/etc/squid.conf" "/opt/squid/etc/slais.conf" "/opt/squid/etc/http_access.conf";
conf_auth "/opt/squid/etc/slais.conf";
apt-get -y install apache2-utils;
set_port;
rules;
service apparmor reload;
service squid.slais start;
# /opt/squid/sbin/squid -N -d 1 -YC -f /opt/squid/etc/squid.conf
# rm -rf "${TMP_DIR}";
