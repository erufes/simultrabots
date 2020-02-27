/*
Copyright (c) 2000-2003, Jelle Kok, University of Amsterdam
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the University of Amsterdam nor the names of its
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*! \file PlayerTeams.cpp
<pre>
<b>File:</b>          PlayerTest.cpp
<b>Project:</b>       Robocup Soccer Simulation Team: UvA Trilearn
<b>Authors:</b>       Jelle Kok
<b>Created:</b>       10/12/2000
<b>Last Revision:</b> $ID$
<b>Contents:</b>      This file contains the class definitions for the
                      Player that are used to test the teams' high level
                      strategy.
<hr size=2>
<h2><b>Changes</b></h2>
<b>Date</b>             <b>Author</b>          <b>Comment</b>
10/12/2000        Jelle Kok       Initial version created
</pre>
*/

#include "Player.h"

#define ERUS_DEFAULT_FORMATION FT_433_OFFENSIVE
#define MIN_DIST_GOAL_THRESHOLD 10

/*!This method is the first complete simple team and defines the actions taken
   by all the players on the field (excluding the goalie). It is based on the
   high-level actions taken by the simple team FC Portugal that it released in
   2000. The players do the following:
   - if ball is kickable
       kick ball to goal (random corner of goal)
   - else if i am fastest player to ball
       intercept the ball
   - else
       move to strategic position based on your home position and pos ball */
SoccerCommand Player::deMeer5() {

    SoccerCommand soc(CMD_ILLEGAL);
    VecPosition posAgent = WM->getAgentGlobalPosition();
    VecPosition posBall = WM->getBallPos();
    int iTmp;

    if (WM->isBeforeKickOff()) {
        if (WM->isKickOffUs() && WM->getPlayerNumber() == 9) // 9 takes kick
        {
            if (WM->isBallKickable()) {
                VecPosition posGoal(PITCH_LENGTH / 2.0,
                                    (-1 + 2 * (WM->getCurrentCycle() % 2)) *
                                    0.4 * SS->getGoalWidth());
                soc = kickTo(posGoal, SS->getBallSpeedMax()); // kick maximal
                Log.log(100, "take kick off");
            } else {
                soc = intercept(false);
                Log.log(100, "move to ball to take kick-off");
            }
            ACT->putCommandInQueue(soc);
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            return soc;
        }
        if (formations->getFormation() != FT_INITIAL || // not in kickoff formation
            posAgent.getDistanceTo(WM->getStrategicPosition()) > 2.0) {
            formations->setFormation(FT_INITIAL); // go to kick_off formation
            ACT->putCommandInQueue(soc = teleportToPos(WM->getStrategicPosition()));
        } else // else turn to center
        {
            ACT->putCommandInQueue(soc = turnBodyToPoint(VecPosition(0, 0), 0));
            ACT->putCommandInQueue(alignNeckWithBody());
        }
    } else {
        formations->setFormation(FT_433_OFFENSIVE);
        soc.commandType = CMD_ILLEGAL;

        if (WM->getConfidence(OBJECT_BALL) < PS->getBallConfThr()) {
            ACT->putCommandInQueue(soc = searchBall());  // if ball pos unknown
            ACT->putCommandInQueue(alignNeckWithBody()); // search for it
        } else if (WM->isBallKickable()) // if kickable
        {
            // can we freely kick to enemy goal?
            // if so, will there be any enemy players in the way?

            // if not, can we pass the ball to someone else?
            VecPosition posGoal(PITCH_LENGTH / 2.0,
                                (-1 + 2 * (WM->getCurrentCycle() % 2)) * 0.4 * SS->getGoalWidth());

            soc = kickTo(posGoal, SS->getBallSpeedMax()); // kick maxima
            ACT->putCommandInQueue(soc);
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            Log.log(100, "kick ball");
        } else if (WM->getFastestInSetTo(OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp) == WM->getAgentObjectType() &&
                   !WM->isDeadBallThem()) { // if fastest to ball
            Log.log(100, "I am fastest to ball; can get there in %d cycles", iTmp);
            soc = intercept(false); // intercept the ball

            if (soc.commandType == CMD_DASH && // if stamina low
                WM->getAgentStamina().getStamina() <
                SS->getRecoverDecThr() * SS->getStaminaMax() + 200) {
                soc.dPower = 30.0 * WM->getAgentStamina().getRecovery(); // dash slow
                ACT->putCommandInQueue(soc);
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            } else // if stamina high
            {
                ACT->putCommandInQueue(soc); // dash as intended
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        } else if (posAgent.getDistanceTo(WM->getStrategicPosition()) >
                   1.5 + fabs(posAgent.getX() - posBall.getX()) / 10.0)
            // if not near strategic pos
        {
            if (WM->getAgentStamina().getStamina() > // if stamina high
                SS->getRecoverDecThr() * SS->getStaminaMax() + 800) {
                soc = moveToPos(WM->getStrategicPosition(),
                                PS->getPlayerWhenToTurnAngle());
                ACT->putCommandInQueue(soc); // move to strategic pos
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            } else // else watch ball
            {
                ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        } else if (fabs(WM->getRelativeAngle(OBJECT_BALL)) > 1.0) // watch ball
        {
            ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        } else // nothing to do
            ACT->putCommandInQueue(SoccerCommand(CMD_TURNNECK, 0.0));
    }
    return soc;
}

SoccerCommand Player::dummyBehaviour() {
    SoccerCommand soc(CMD_ILLEGAL);
    VecPosition posAgent = WM->getAgentGlobalPosition();
    if (WM->isBeforeKickOff()) {
        if (formations->getFormation() != FT_INITIAL || posAgent.getDistanceTo(WM->getStrategicPosition()) > 2.0) {
            formations->setFormation(FT_INITIAL);
            ACT->putCommandInQueue(soc = teleportToPos(WM->getStrategicPosition()));
        }
    }
    return soc;
}

/*!This method is a simple goalie based on the goalie of the simple Team of
   FC Portugal. It defines a rectangle in its penalty area and moves to the
   position on this rectangle where the ball intersects if you make a line
   between the ball position and the center of the goal. If the ball can
   be intercepted in the own penalty area the ball is intercepted and catched.
*/
SoccerCommand Player::deMeer5_goalie() {
    int i;

    SoccerCommand soc;
    VecPosition posAgent = WM->getAgentGlobalPosition();
    AngDeg angBody = WM->getAgentGlobalBodyAngle();

    // define the top and bottom position of a rectangle in which keeper moves
    static const VecPosition posLeftTop(-PITCH_LENGTH / 2.0 +
                                        0.7 * PENALTY_AREA_LENGTH,
                                        -PENALTY_AREA_WIDTH / 4.0);
    static const VecPosition posRightTop(-PITCH_LENGTH / 2.0 +
                                         0.7 * PENALTY_AREA_LENGTH,
                                         +PENALTY_AREA_WIDTH / 4.0);

    // define the borders of this rectangle using the two points.
    static Line lineFront = Line::makeLineFromTwoPoints(posLeftTop, posRightTop);
    static Line lineLeft = Line::makeLineFromTwoPoints(
            VecPosition(-50.0, posLeftTop.getY()), posLeftTop);
    static Line lineRight = Line::makeLineFromTwoPoints(
            VecPosition(-50.0, posRightTop.getY()), posRightTop);

    if (WM->isBeforeKickOff()) {
        if (formations->getFormation() != FT_INITIAL || // not in kickoff formation
            posAgent.getDistanceTo(WM->getStrategicPosition()) > 2.0) {
            formations->setFormation(FT_INITIAL); // go to kick_off formation
            ACT->putCommandInQueue(soc = teleportToPos(WM->getStrategicPosition()));
        } else // else turn to center
        {
            ACT->putCommandInQueue(soc = turnBodyToPoint(VecPosition(0, 0), 0));
            ACT->putCommandInQueue(alignNeckWithBody());
        }
        return soc;
    }

    if (WM->getConfidence(OBJECT_BALL) <
        PS->getBallConfThr()) {                                         // confidence ball too  low
        ACT->putCommandInQueue(searchBall()); // search ball
        ACT->putCommandInQueue(alignNeckWithBody());
    } else if (WM->getPlayMode() == PM_PLAY_ON || WM->isFreeKickThem() ||
               WM->isCornerKickThem()) {
        if (WM->isBallCatchable()) {
            ACT->putCommandInQueue(soc = catchBall());
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        } else if (WM->isBallKickable()) {
            soc = kickTo(VecPosition(0, posAgent.getY() * 2.0), 2.0);
            ACT->putCommandInQueue(soc);
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        } else if (WM->isInOwnPenaltyArea(getInterceptionPointBall(&i, true)) &&
                   WM->getFastestInSetTo(OBJECT_SET_PLAYERS, OBJECT_BALL, &i) ==
                   WM->getAgentObjectType()) {
            ACT->putCommandInQueue(soc = intercept(true));
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        } else {
            // make line between own goal and the ball
            VecPosition posMyGoal = (WM->getSide() == SIDE_LEFT)
                                    ? SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_L, SIDE_LEFT)
                                    : SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_RIGHT);
            Line lineBall = Line::makeLineFromTwoPoints(WM->getBallPos(), posMyGoal);

            // determine where your front line intersects with the line from ball
            VecPosition posIntersect = lineFront.getIntersection(lineBall);

            // outside rectangle, use line at side to get intersection
            if (posIntersect.isRightOf(posRightTop))
                posIntersect = lineRight.getIntersection(lineBall);
            else if (posIntersect.isLeftOf(posLeftTop))
                posIntersect = lineLeft.getIntersection(lineBall);

            if (posIntersect.getX() < -49.0)
                posIntersect.setX(-49.0);

            // and move to this position
            if (posIntersect.getDistanceTo(WM->getAgentGlobalPosition()) > 0.5) {
                soc = moveToPos(posIntersect, PS->getPlayerWhenToTurnAngle());
                ACT->putCommandInQueue(soc);
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            } else {
                ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        }
    } else if (WM->isFreeKickUs() == true || WM->isGoalKickUs() == true) {
        if (WM->isBallKickable()) {
            if (WM->getTimeSinceLastCatch() == 25 && WM->isFreeKickUs()) {
                // move to position with lesser opponents.
                if (WM->getNrInSetInCircle(OBJECT_SET_OPPONENTS,
                                           Circle(posRightTop, 15.0)) <
                    WM->getNrInSetInCircle(OBJECT_SET_OPPONENTS,
                                           Circle(posLeftTop, 15.0)))
                    soc.makeCommand(CMD_MOVE, posRightTop.getX(), posRightTop.getY(), 0.0);
                else
                    soc.makeCommand(CMD_MOVE, posLeftTop.getX(), posLeftTop.getY(), 0.0);
                ACT->putCommandInQueue(soc);
            } else if (WM->getTimeSinceLastCatch() > 28) {
                soc = kickTo(VecPosition(0, posAgent.getY() * 2.0), 2.0);
                ACT->putCommandInQueue(soc);
            } else if (WM->getTimeSinceLastCatch() < 25) {
                VecPosition posSide(0.0, posAgent.getY());
                if (fabs((posSide - posAgent).getDirection() - angBody) > 10) {
                    soc = turnBodyToPoint(posSide);
                    ACT->putCommandInQueue(soc);
                }
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        } else if (WM->isGoalKickUs()) {
            ACT->putCommandInQueue(soc = intercept(true));
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        } else
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
    } else {
        ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
        ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
    }
    return soc;
}

SoccerCommand Player::erus_attacker() {

    SoccerCommand soc(CMD_ILLEGAL);
    VecPosition posAgent = WM->getAgentGlobalPosition();
    VecPosition posBall = WM->getBallPos();
    int iTmp;

    ObjectT       prox = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES_NO_GOALIE,OBJECT_BALL,NULL,-1.0);
    ObjectT       prox2 = WM->getSecondClosestInSetTo(OBJECT_SET_TEAMMATES_NO_GOALIE,OBJECT_BALL,NULL,-1.0);
    ObjectT       enemy = WM->getClosestInSetTo( OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_UNKNOWN,NULL,-1.0);
    ObjectT       anterior;

    VecPosition   pos = WM->getGlobalPosition(prox);
    VecPosition   pos2 = WM->getGlobalPosition(prox2);

    if (WM->isBeforeKickOff()) {
        if (WM->isKickOffUs() && WM->getPlayerNumber() == 9) // 9 takes kick
        {
            if (WM->isBallKickable()) {
                VecPosition posGoal(PITCH_LENGTH / 2.0,
                                    (-1 + 2 * (WM->getCurrentCycle() % 2)) *
                                    0.4 * SS->getGoalWidth());
                soc = kickTo(posGoal, SS->getBallSpeedMax()); // kick maximal
                Log.log(100, "take kick off");
            } else {
                soc = intercept(false);
                Log.log(100, "move to ball to take kick-off");
            }
            ACT->putCommandInQueue(soc);
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            return soc;
        }
        if (formations->getFormation() != FT_INITIAL || // not in kickoff formation
            posAgent.getDistanceTo(WM->getStrategicPosition()) > 2.0) {
            formations->setFormation(FT_INITIAL); // go to kick_off formation
            ACT->putCommandInQueue(soc = teleportToPos(WM->getStrategicPosition()));
        } else // else turn to center
        {
            ACT->putCommandInQueue(soc = turnBodyToPoint(VecPosition(0, 0), 0));
            ACT->putCommandInQueue(alignNeckWithBody());
        }
    } else {
        formations->setFormation(ERUS_DEFAULT_FORMATION);
        soc.commandType = CMD_ILLEGAL;

        ObjectT enemyGoalie = WM->getOppGoalieType();
        VecPosition enemyGoaliePos = WM->getGlobalPosition(enemyGoalie);


        if (WM->getConfidence(OBJECT_BALL) < PS->getBallConfThr()) {
            ACT->putCommandInQueue(soc = searchBall());  // if ball pos unknown
            ACT->putCommandInQueue(alignNeckWithBody()); // search for it
        } else if (WM->isBallKickable()) // if kickable
        {
            VecPosition goleiro = WM->getGlobalPosition(OBJECT_OPPONENT_GOALIE),
                        traveDireita = SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_G_R_T, SIDE_LEFT),
                        traveEsquerda = SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_G_R_B, SIDE_LEFT);
            // Descobrir A parte em que o goleiro está protegendo mais
            Line gol = Line::makeLineFromTwoPoints(traveDireita, traveEsquerda);
            VecPosition meuPontoMaisProxGol = gol.getPointOnLineClosestTo(posAgent);
            (meuPontoMaisProxGol.getY() < traveDireita.getY()) ? meuPontoMaisProxGol.setY(traveDireita.getY() + 2) : 0;
            (meuPontoMaisProxGol.getY() > traveEsquerda.getY()) ? meuPontoMaisProxGol.setY(traveEsquerda.getY() - 2) : 0;
            if(i==0){
                anterior=prox;
            }
            if(i==0 || anterior!=prox)
                soc = directPass(pos, PASS_NORMAL);
            else
            {
                soc = directPass(pos2, PASS_NORMAL);
                i=0;
            }
            i++;
            ACT->putCommandInQueue( soc );
            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
        } else if (WM->getFastestInSetTo(OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp) == WM->getAgentObjectType() &&
                   !WM->isDeadBallThem()) { // if fastest to ball
            Log.log(100, "I am fastest to ball; can get there in %d cycles", iTmp);
            soc = intercept(false); // intercept the ball

            if (soc.commandType == CMD_DASH && // if stamina low
                WM->getAgentStamina().getStamina() <
                SS->getRecoverDecThr() * SS->getStaminaMax() + 200) {
                soc.dPower = 30.0 * WM->getAgentStamina().getRecovery(); // dash slow
                ACT->putCommandInQueue(soc);
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            } else // if stamina high
            {
                ACT->putCommandInQueue(soc); // dash as intended
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        } else if (posBall.getX() < SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_20, SIDE_LEFT).getX()){
            if (WM->getAgentStamina().getStamina() > // if stamina high
                    SS->getRecoverDecThr() * SS->getStaminaMax() + 800) {
                soc = moveToPos(VecPosition(posBall.getX(),WM->getStrategicPosition().getY()), enemyGoaliePos.getDirection());
                ACT->putCommandInQueue(soc);
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }else // else watch ball
            {
                ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        }
        else if (posAgent.getDistanceTo(WM->getStrategicPosition()) >
                   1.5 + fabs(posAgent.getX() - posBall.getX()) / 10.0)
            // if not near strategic pos
        {
            if (WM->getAgentStamina().getStamina() > // if stamina high
                SS->getRecoverDecThr() * SS->getStaminaMax() + 800) {
                soc = moveToPos(WM->getStrategicPosition(),
                                PS->getPlayerWhenToTurnAngle());
                ACT->putCommandInQueue(soc); // move to strategic pos
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            } else // else watch ball
            {
                ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        } else if (fabs(WM->getRelativeAngle(OBJECT_BALL)) > 1.0) // watch ball
        {
            ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        } else // nothing to do
            ACT->putCommandInQueue(SoccerCommand(CMD_TURNNECK, 0.0));
    }
    return soc;
}

VecPosition Player::best_goalie_kick(){
    VecPosition resp;

    // Passo 1: separar os dois aliados mais próximos como candidatos a recepção do chute
    ObjectT candidato1 = WM->getClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE),
            candidato2 = WM->getSecondClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE);

    if (candidato1 == OBJECT_ILLEGAL || candidato2 == OBJECT_ILLEGAL){
        VecPosition posAgent = WM->getAgentGlobalPosition();    // posição do próprio goleiro no mundo
        resp.setVecPosition(0, posAgent.getY() * 2.0);
    }else{
        // Passo 2: definir o mais livre
            // Versão 1: O mais longe de um inimigo
        /* double dist1, dist2;
        VecPosition inimigo1 = WM->getPosClosestOpponentTo(&dist1, candidato1), inimigo2 = WM->getPosClosestOpponentTo(&dist2, candidato2);
        VecPosition resp = (dist1 < dist2) ? WM->getGlobalPosition(candidato1) : WM->getGlobalPosition(candidato2); */
            // Versão 2: O que tem menos inimigos em um dado raio
        VecPosition c1 = WM->getGlobalPosition(candidato1), c2 = WM->getGlobalPosition(candidato2);
        int numInimigos1 = WM->getNrInSetInCircle(OBJECT_SET_OPPONENTS, Circle(c1, 10.0)), numInimigos2 = WM->getNrInSetInCircle(OBJECT_SET_OPPONENTS, Circle(c2, 10.0));
        VecPosition resp = (numInimigos1 < numInimigos2) ? c1 : c2;
            // Versão 3: O que tem o inimigo mais longe de mim
        /* ObjectT inimigo1 = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS, candidato1), inimigo2 = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS, candidato2);
        double dist1 = WM->getRelativeDistance(inimigo1), dist2 = WM->getRelativeDistance(inimigo2);
        VecPosition resp = (dist1 < dist2) ? WM->getGlobalPosition(candidato1) : WM->getGlobalPosition(candidato2); */
    }
    return resp;
};

// FIXME: Calcular nova posição de teleporte para cobrar o tiro livre, podendo ser qualquer uma dentro da penalty area
VecPosition Player::best_goalie_freekick(){
    VecPosition posLeftTop(-40.95, -10.08);
    VecPosition posRightTop(-40.95, 10.08);
    VecPosition pos;
    if (WM->getNrInSetInCircle(OBJECT_SET_OPPONENTS,
                                           Circle(posRightTop, 15.0)) <
                    WM->getNrInSetInCircle(OBJECT_SET_OPPONENTS,
                                           Circle(posLeftTop, 15.0)))
                    pos.setVecPosition(posRightTop.getX(), posRightTop.getY());
                else
                    pos.setVecPosition(posLeftTop.getX(), posLeftTop.getY());
    return pos;
};

SoccerCommand Player::erus_goalie(){
    // Inicialização de variaveis da função
    int i;                                                  // controle de fluxo (??)
    SoccerCommand soc;                                      // comando retornado no final
    VecPosition posAgent = WM->getAgentGlobalPosition();    // posição do próprio goleiro no mundo
    AngDeg angBody = WM->getAgentGlobalBodyAngle();         // angulo do corpo do goleiro em relação ao próprio mundo

    /* Definição do retangulo onde o goleiro se move em sua estratégia básica
       o retangulo é definido para o lado esquerdo e o lado defendido é usado
       como fator de conversão das coordanadas para as reais.
    */
    static const VecPosition posLeftTop( SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_40, SIDE_LEFT).getX() + 0.7 * (SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_50, SIDE_LEFT).getX() - SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_40, SIDE_LEFT).getX()),
                                         SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY() + 0.9 * (SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_T_10, SIDE_LEFT).getY() - SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY())),
                             posLeftBack(SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_T, SIDE_LEFT).getX(),
                                         SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY() + 0.9 * (SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_T_10, SIDE_LEFT).getY() - SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY())),
                             posRightTop(SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_40, SIDE_LEFT).getX() + 0.7 * (SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_50, SIDE_LEFT).getX() - SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_40, SIDE_LEFT).getX()),
                                         SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY() + 0.9 * (SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_B_10, SIDE_LEFT).getY() - SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY())),
                             posRightBack(SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_T, WM->getSide()).getX(),
                                         SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY() + 0.9 * (SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_B_10, SIDE_LEFT).getY() - SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY()));
    static Line lineFront = Line::makeLineFromTwoPoints(posLeftTop, posRightTop),
                lineLeft = Line::makeLineFromTwoPoints(posLeftBack, posLeftTop),
                lineRight = Line::makeLineFromTwoPoints(posRightBack, posRightTop);

    // Partida parada antes do inicio (Sem Alterações)
    if (WM->isBeforeKickOff()) {
        if (formations->getFormation() != FT_INITIAL || // not in kickoff formation
            posAgent.getDistanceTo(WM->getStrategicPosition()) > 2.0) {
            formations->setFormation(FT_INITIAL); // go to kick_off formation
            ACT->putCommandInQueue(soc = teleportToPos(WM->getStrategicPosition()));
        } else // else turn to center
        {
            ACT->putCommandInQueue(soc = turnBodyToPoint(VecPosition(0, 0), 0));
            ACT->putCommandInQueue(alignNeckWithBody());
        }
        return soc;
    }

    /* Jogo em execução !!! */
    formations->setFormation(ERUS_DEFAULT_FORMATION);                   //Formação da ERUS

    /* Bloco de procura pela bola
       Ocorre caso a confiança de saber onde a bola está é muito pequena
    */
    if (WM->getConfidence(OBJECT_BALL) <
        PS->getBallConfThr()) {               // confidence ball too low
        ACT->putCommandInQueue(searchBall()); // search ball
        ACT->putCommandInQueue(alignNeckWithBody());
    } // Fim do bloco de procura pela bola

    /* Bloco de casos de defesa
       Ocorre se:
            o jogo estiver em andamento
            ou for tiro livre inimigo
            ou for escanteio inimigo
    */
    else if (WM->getPlayMode() == PM_PLAY_ON || WM->isFreeKickThem() ||
               WM->isCornerKickThem()) {
        // Se eu posso pegar a bola
        if (WM->isBallCatchable()) {
            // Pego a bola (envia comando de catch ball)
            ACT->putCommandInQueue(soc = catchBall());
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        }
        // Caso não, se eu posso chutar a bola
        else if (WM->isBallKickable()) {
            // Chuto a bola (envia comando de kick ball to x)
            soc = directPass(this->best_goalie_kick(), PASS_NORMAL);
            ACT->putCommandInQueue(soc);
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        }
        // Caso não, verifica se é o caso de intercepção da bola
        else if (WM->isInOwnPenaltyArea(getInterceptionPointBall(&i, true)) &&
                   WM->getFastestInSetTo(OBJECT_SET_PLAYERS, OBJECT_BALL, &i) ==
                   WM->getAgentObjectType()) {
            // Intercepto a bola (envia comando de intercepção da bola)
            ACT->putCommandInQueue(soc = intercept(true));
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        }
        // Caso não, me posiciono da melhor maneira que eu puder (Não Alterado)
        else {
            // make line between own goal and the ball
            VecPosition posMyGoal = (WM->getSide() == SIDE_LEFT)
                                    ? SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_L, SIDE_LEFT)
                                    : SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_RIGHT);
            Line lineBall = Line::makeLineFromTwoPoints(WM->getBallPos(), posMyGoal);

            // determine where your front line intersects with the line from ball
            VecPosition posIntersect = lineFront.getIntersection(lineBall);

            // outside rectangle, use line at side to get intersection
            if (posIntersect.isRightOf(posRightTop))
                posIntersect = lineRight.getIntersection(lineBall);
            else if (posIntersect.isLeftOf(posLeftTop))
                posIntersect = lineLeft.getIntersection(lineBall);

            //if (posIntersect.getX() < -49.0)
            //    posIntersect.setX(-49.0);

            // and move to this position
            if (posIntersect.getDistanceTo(WM->getAgentGlobalPosition()) > 0.5) {
                soc = moveToPos(posIntersect, PS->getPlayerWhenToTurnAngle());
                ACT->putCommandInQueue(soc);
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            } else {
                ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        }

    } // Fim do bloco de casos de defesa

    /* Bloco de casos de volta da bola em jogo
       Ocorre se:
            for nosso tiro livre (free kick)
            ou for nosso tiro de meta (goal kick)
    */
    else if (WM->isFreeKickUs() == true || WM->isGoalKickUs() == true) {
        // Se eu já tenho a bola para chutar (eu cobrarei o tiro)
        if (WM->isBallKickable()) {
            // Quando o número de ciclos for exatos 25 desde o último catch e é nosso tiro livre
            // Neste caso, eu me teleporto para a posição de tiro livre após catch
            if (WM->getTimeSinceLastCatch() == 25 && WM->isFreeKickUs()) {
                VecPosition freekickPos = this->best_goalie_freekick();
                soc.makeCommand(CMD_MOVE, freekickPos.getX(), freekickPos.getY(), 0.0);
                ACT->putCommandInQueue(soc);
            }
            // Se já tem mais de 28 ciclos (tanto freekick quanto goalkick chutam aqui)
            else if (WM->getTimeSinceLastCatch() > 28) {
                soc = directPass(this->best_goalie_kick(), PASS_NORMAL);
                ACT->putCommandInQueue(soc);
            }
            // Se ainda não passaram 24 ciclos (apenas observo)
            else if (WM->getTimeSinceLastCatch() < 25) {
                VecPosition posSide(0.0, posAgent.getY());
                if (fabs((posSide - posAgent).getDirection() - angBody) > 10) {
                    soc = turnBodyToPoint(posSide);
                    ACT->putCommandInQueue(soc);
                }
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            }
        }
        // Caso não, se for tiro de meta, intercepto a bola
        // Entrar aqui significa que é tiro de meta mas ainda preciso buscar a bola
        else if (WM->isGoalKickUs()) {
            ACT->putCommandInQueue(soc = intercept(true));
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        }
        // Caso não
        else
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
    } // Fim do bloco de casos de volta da bola em jogo

    // Bloco de escape
    else {
        ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
        ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
    } // Fim do bloco de escape

    // Retorna o comando calculado
    return soc;
};
