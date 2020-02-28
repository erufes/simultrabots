#!/bin/bash

PLAYER_FILE=src/erus_coach
COACH_FILE=src/erus_player

START_FILE=start.sh

if [ ! -f "$PLAYER_FILE" ]; then 
	echo "Binário do jogador não encontrado!"
	exit 1
fi

if [ ! -f "$COACH_FILE" ]; then 
	echo "Binário do coach não encontrado!"
	exit 1
fi

if [ ! -f "$START_FILE" ]; then 
	echo "Script de início não encontrado!"
	exit 1
fi

echo "Preparando arquivo compactado..."
tar -zcvf simultrabots.tar.gz start.sh src/erus_coach src/erus_player
echo "Pronto! Agora é só enviar o arquivo \"simultrabots.tar.gz\"!"