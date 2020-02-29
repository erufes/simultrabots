#!/bin/bash

# Variáveis
WAIT_TIME=0
HOST="localhost"
TEAM="ERUS"
DIR="src"
PLAYER_BIN="${DIR}/erus_player"
COACH_BIN="${DIR}/erus_coach"
P_CONF="${DIR}/player.conf"
F_CONF="${DIR}/formations.conf"
LOGLEVEL="100"

# Mostra a tela de ajuda com os parâmetros suportados
show_help () {
	echo "*********************************************************************"
	echo "* Lista de parâmetros:                                              *"
	echo "*                                                                   *"
	echo "* --h | --help          Mostra a ajuda (esta tela)                  *"
	echo "*  -h | --host          Define o host do jogo. Padrão: localhost    *"
	echo "*  -t | --team          Define o nome do time. Padrão: ERUS         *"
	echo "*  -d | --dir           Define a pasta raiz do projeto. Padrão: src *"
	echo "*  -l | --log           Define o nível de logging. Padrão: 100      *"
	echo "*********************************************************************"
}

# Mostra a tela de "boas vindas"
show_welcome () {
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
}

# Mostra os dados da execução
show_data () {
	echo ""
	echo "Iniciando no host ${HOST}"
	echo "Nome do time: ${TEAM}"
	echo "Diretório base: ${DIR}"
	echo ""
}

# Parsing de argumentos
POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
	--h|--help)
	show_help
	shift
	shift
	exit 0
	;;
    -h|--host)
    HOST="$2"
    shift # past argument
    shift # past value
    ;;
    -t|--team)
    TEAM="$2"
    shift # past argument
    shift # past value
    ;;
    -d|--dir)
    DIR="$2"
    shift # past argument
    shift # past value
    ;;
	-l|--log)
	LOGLEVEL="$2"
	shift
	shift
	;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# Script "padrão"

show_welcome

show_data

# Instancia os jogadores
for i in {1..11}
do
	echo "Instanciando player ${i}..."
	${PLAYER_BIN} -log ${LOGLEVEL} -number ${i} -host ${HOST} -team ${TEAM} -f ${F_CONF} -c ${P_CONF} -o "./logs/log_${i}.txt" &
	sleep ${WAIT_TIME}
	echo "OK!"
done
sleep 1

echo "Todos os players instanciados"

echo "Instanciando coach..."
${COACH_BIN} -host ${HOST} -team ${TEAM} -f ${F_CONF} &
echo "OK!"

echo "Tudo ok no time"
