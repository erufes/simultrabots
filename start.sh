#!/bin/bash
# instalar yum
# sudo apt install tcsh


# This script starts the UvA_Trilearn_2003 team. When player numbers are 
# supplied before the (optional) host-name and team-name arguments only
# these players are started, otherwise all players are started.
# Usage:   start.sh <player-numbers> <host-name> <team-name>
#
# Example: start.sh                 all players default host and name
# Example: start.sh machine         all players on host 'machine'
# Example: start.sh localhost UvA   all players on localhost and name 'UvA'
# Example: start.sh 127.0.0.1 UvA   all players on 127.0.0.1  and name 'UvA'
# Example: start.sh 1 2 3 4         players 1-4 on default host and name
# Example: start.sh 1 2 remote      players 1-2 on host 'remote'
# Example: start.sh 9 10 remote UvA players 9-10 on host remote and name 'UvA'
# Example: start.sh 0               start coach on default host 

wait=0
host="localhost"
team="ERUS" 
dir="src"
prog="${dir}/erus_player"
coach="${dir}/erus_coach"
pconf="${dir}/player.conf"
fconf="${dir}/formations.conf"
loglevel="100"

echo "*****************************************************************"
echo "* ERUS ULTRABOTS 2020 - Universidade Federal do Espírito Santo  *"
echo "*                                                               *"
echo "* Código original                                               *"
echo "*                       Jelle Kok                               *"
echo "*                       Nikos Vlassis                           *" 
echo "*                       Frans Groen                             *"
echo "*                       Remco de Boer                           *"
echo "*                       Mahdi Nami Damirchi                     *"
echo "*                                                               *"
echo "* Atualizações para IRONCup 2020                                *"
echo "*                       ERUS - Equipe de Robótica da UFES       *"
echo "* Licenciado pela GPLv2                                         *"
echo "*****************************************************************"

# Parsing de argumentos


# Instancia os jogadores
for i in {1..11}
do
	echo "Instanciando player ${i}..." 
	${prog} -log ${loglevel} -number ${i} -host ${host} -team ${team} -f ${fconf} -c ${pconf} -o ./logs/log_${i}.txt &
	sleep ${wait}
	echo "OK!"
done
sleep 1

echo "Todos os players instanciados"

echo "Instanciando coach..."
${coach} -host ${host} -team ${team} -f ${fconf} &
echo "OK!"

echo "Tudo ok no time"

# set i = 1
# while ( ${i} <12 )
#   ${prog} -log 100 -number ${i} -host ${host} -team ${team}  -f ${fconf} -c ${pconf} -o ./logs/log_${i} &
#   sleep $wait
#   @ i++
# end
# sleep 1
# ${coach} -host ${host} -team ${team} -f ${fconf}  &

