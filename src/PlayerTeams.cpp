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

SoccerCommand Player::erus_midfielder(  )
{
  SoccerCommand soc(CMD_ILLEGAL);
  VecPosition posAgent = WM->getAgentGlobalPosition();
  VecPosition posBall = WM->getBallPos();
  int iTmp;

  ObjectT ally1 = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES_NO_GOALIE, OBJECT_BALL, NULL, -1.0);
  ObjectT ally2 = WM->getSecondClosestInSetTo(OBJECT_SET_TEAMMATES_NO_GOALIE, OBJECT_BALL, NULL, -1.0);
  ObjectT prev;
  ObjectT opponent1 = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS, OBJECT_BALL, NULL, -1.0);
  ObjectT opponent2 = WM->getSecondClosestInSetTo(OBJECT_SET_OPPONENTS, OBJECT_BALL, NULL, -1.0);

  VecPosition al1 = WM->getGlobalPosition(ally1);
  VecPosition al2 = WM->getGlobalPosition(ally2);
  VecPosition previous = WM->getGlobalPosition(prev);
  VecPosition opp1 = WM->getGlobalPosition(opponent1);
  VecPosition opp2 = WM->getGlobalPosition(opponent2);
  VecPosition posGoal(PITCH_LENGTH / 2.0, (-1 + 2 * (WM->getCurrentCycle() % 2)) * 0.4 * SS -> getGoalWidth());

  if( WM->isBeforeKickOff( ) )
  {
    if( WM->isKickOffUs( ) && WM->getPlayerNumber() == 9 ) // 9 takes kick
    {
      if(  WM->isBallInOurPossesion())
      {
      }
      else
      {
        soc = intercept( false );
        Log.log( 100, "move to ball to take kick-off" );
      }
      ACT->putCommandInQueue( soc );
      ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      return soc;
    }
    if( formations->getFormation() != FT_INITIAL || // not in kickoff formation
        posAgent.getDistanceTo( WM->getStrategicPosition() ) > 2.0 )
    {
      formations->setFormation( FT_INITIAL );       // go to kick_off formation
      ACT->putCommandInQueue( soc=teleportToPos( WM->getStrategicPosition() ));
    }
    else                                            // else turn to center
    {
      ACT->putCommandInQueue( soc=turnBodyToPoint( VecPosition( 0, 0 ), 0 ) );
      ACT->putCommandInQueue( alignNeckWithBody( ) );
    }
  }
  else
  {
    formations->setFormation( ERUS_DEFAULT_FORMATION );
    soc.commandType = CMD_ILLEGAL;

    if( WM->getConfidence( OBJECT_BALL ) < PS->getBallConfThr() )
    {
      ACT->putCommandInQueue( soc = searchBall() );   // if ball pos unknown
      ACT->putCommandInQueue( alignNeckWithBody( ) ); // search for it
    }
  else if(WM->isBallKickable()){
    if(WM->isFreeKickUs() || WM->isCornerKickUs() || WM->isKickInUs() || (WM->getPlayMode() == PM_INDIRECT_FREE_KICK_LEFT && WM->getSide() == SIDE_LEFT) || ( WM->getPlayMode() == PM_INDIRECT_FREE_KICK_RIGHT && WM->getSide() == SIDE_RIGHT)){
            if(WM->getFastestInSetTo(OBJECT_SET_TEAMMATES_NO_GOALIE, OBJECT_BALL, &iTmp) == WM->getAgentObjectType() &&
                   !WM->isDeadBallThem()){
                if (WM->isBallKickable()) {
                    soc = directPass(WM->getGlobalPosition(WM->getClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE)), PASS_NORMAL);
                    Log.log(100, "Dando um chutão");
                    ACT->putCommandInQueue(soc);
                } else {
                    soc = intercept(false);
                    Log.log(100, "Teleportando");
                    ACT->putCommandInQueue(soc);
                }
            }
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
    }

    else if( ((posBall.getDistanceTo(opp1) < 2.0) && (posBall.getDistanceTo(opp2) < 2.0) ||
    ((posBall.getDistanceTo(opp1) < 1.0) || (posBall.getDistanceTo(opp2) < 1.0))))
    {
      if(WM -> getNrInSetInCircle(OBJECT_SET_OPPONENTS, Circle(al1, 2.0 )) < 2){
        if(WM -> getPlayerType(ally1) == PT_MIDFIELDER_CENTER || WM -> getPlayerType(ally1) == PT_MIDFIELDER_WING){
            if(WM->getConfidence(ally1) < PS->getPlayerConfThr( )){
                ACT->putCommandInQueue(turnNeckToObject(ally1, soc));
            }
            else{
                prev = ally1;
                soc = directPass(al1, PASS_FAST);
                ACT->putCommandInQueue(soc);
            }
        }
      }
      else if(WM -> getNrInSetInCircle(OBJECT_SET_OPPONENTS, Circle(al2, 2.0)) < 2){
        if((WM -> getPlayerType(ally2) == PT_MIDFIELDER_CENTER || WM ->getPlayerType(ally2) == PT_MIDFIELDER_WING)){
            if(WM->getConfidence(ally2) < PS->getPlayerConfThr( )){
                ACT->putCommandInQueue(turnNeckToObject(ally2, soc));
            }
            else{
                prev = ally2;
                soc = directPass(al2, PASS_FAST);
                ACT->putCommandInQueue(soc);
            }
        }
      }
    }
    else{
      if(WM -> getNrInSetInCircle(OBJECT_SET_OPPONENTS, Circle(previous, 2.0)) >= 2){
        if((WM -> getPlayerType(ally1) == PT_ATTACKER || WM -> getPlayerType(ally1) == PT_ATTACKER_WING) && ally1!=prev){
            if(WM->getConfidence(ally1) < PS->getPlayerConfThr( )){
            ACT->putCommandInQueue(turnNeckToObject(ally1, soc));
            }
            else{
            soc = directPass(al1, PASS_FAST);
            ACT->putCommandInQueue(soc);
            }
        }
        else if((WM -> getPlayerType(ally2) == PT_ATTACKER || WM -> getPlayerType(ally2) == PT_ATTACKER_WING) && ally2!=prev){
            if(WM->getConfidence(ally1) < PS->getPlayerConfThr( )){
            ACT->putCommandInQueue(turnNeckToObject(ally2, soc));
            }
            else{
            soc = directPass(al2, PASS_FAST);
            ACT->putCommandInQueue(soc);
            }
        }
      }
      else{
        if((posBall.getDistanceTo(opp1) < 2.5) || (posBall.getDistanceTo(opp2) < 2.5)){
            if(WM->getConfidence(prev) < PS->getPlayerConfThr( )){
                ACT->putCommandInQueue(turnNeckToObject(prev, soc));
            }
            else{
                soc = directPass(previous, PASS_NORMAL);
                ACT->putCommandInQueue(soc);
            }
        }
        else if( WM-> getTimeSinceLastCatch() > 0 && WM->getTimeSinceLastCatch() < 50){
          if((WM -> getPlayerType(ally1) == PT_ATTACKER || WM -> getPlayerType(ally1) == PT_ATTACKER_WING) && ally1!=prev){
            if(WM->getConfidence(ally1) < PS->getPlayerConfThr( )){
                ACT->putCommandInQueue(turnNeckToObject(ally1, soc));
            }
            else{
                soc = directPass(al1, PASS_NORMAL);
                ACT->putCommandInQueue(soc);
            }
          }
          else if((WM -> getPlayerType(ally2) == PT_ATTACKER || WM -> getPlayerType(ally2) == PT_ATTACKER_WING) && ally2!=prev){
            if(WM->getConfidence(ally2) < PS->getPlayerConfThr( )){
                ACT->putCommandInQueue(turnNeckToObject(ally1, soc));
            }
            else{
                soc = directPass(al2, PASS_NORMAL);
                ACT->putCommandInQueue(soc);
            }
          }
        }
        else if(posBall.getDistanceTo(al1) < 5.0 && (al1.getDistanceTo(posGoal) < posBall.getDistanceTo(posGoal))){
            soc = directPass(al1, PASS_NORMAL);
            ACT->putCommandInQueue(soc);
        }
        else if(posBall.getDistanceTo(al2) < 5.0 && (al2.getDistanceTo(posGoal) < posBall.getDistanceTo(posGoal))){
            soc = directPass(al2, PASS_NORMAL);
            ACT->putCommandInQueue(soc);
        }
        else if(posBall.getDistanceTo(posGoal) < 15){
            Line target_raycast = Line::makeLineFromTwoPoints(posBall, posGoal);
          soc = dribble(target_raycast.getBCoefficient(), DRIBBLE_FAST);
          ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
          ACT->putCommandInQueue(soc);
        }
        else if(WM -> getNrInSetInCircle(OBJECT_SET_OPPONENTS, Circle(al1, 6)) < 2) {
            soc = directPass(al1, PASS_FAST);
            ACT->putCommandInQueue(soc);
        }
        else{
            soc = directPass(al2, PASS_FAST);
            ACT->putCommandInQueue(soc);
        }
      }
    }
  }
  else if( WM->getFastestInSetTo( OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp )
              == WM->getAgentObjectType()  && !WM->isDeadBallThem() )
    {                                                // if fastest to ball
      Log.log( 100, "I am fastest to ball; can get there in %d cycles", iTmp );
      soc = intercept( false );                      // intercept the ball

      if( soc.commandType == CMD_DASH &&             // if stamina low
          WM->getAgentStamina().getStamina() <
             SS->getRecoverDecThr()*SS->getStaminaMax()+200 )
      {
        soc.dPower = 30.0 * WM->getAgentStamina().getRecovery(); // dash slow
        ACT->putCommandInQueue( soc );
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
      else                                           // if stamina high
      {
        ACT->putCommandInQueue( soc );               // dash as intended
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
     }
     else if( posAgent.getDistanceTo(WM->getStrategicPosition()) >
                  1.5 + fabs(posAgent.getX()-posBall.getX())/15.0)
                                                  // if not near strategic pos
     {
       if( WM->getAgentStamina().getStamina() >     // if stamina high
                            SS->getRecoverDecThr()*SS->getStaminaMax()+800 )
       {
         soc = moveToPos(WM->getStrategicPosition(),
                         PS->getPlayerWhenToTurnAngle());
         ACT->putCommandInQueue( soc );            // move to strategic pos
         ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
       }
       else                                        // else watch ball
       {
         ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
         ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
       }
     }
     else if( fabs( WM->getRelativeAngle( OBJECT_BALL ) ) > 1.0 ) // watch ball
     {
       ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
       ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
     }
     else
     {
       if(WM -> isBallInOurPossesion()){
        ACT->putCommandInQueue( SoccerCommand(CMD_TURNNECK,0.0) );
      }
       else{
        soc = mark(opponent1, 0.5, MARK_BALL );
      }                    // nothing to do
       ACT->putCommandInQueue( soc);
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

SoccerCommand Player::erus_defense(  )
{

  SoccerCommand soc(CMD_ILLEGAL);

  int           iTmp;

  ObjectT       prox = WM->getClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE,NULL);
  ObjectT       prox2 = WM->getSecondClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE,NULL);
  ObjectT       enemy = WM->getClosestRelativeInSet( OBJECT_SET_OPPONENTS,NULL);
  ObjectT       enemy2 = WM->getSecondClosestRelativeInSet( OBJECT_SET_OPPONENTS,NULL);

  VecPosition   posAgent = WM->getAgentGlobalPosition();
  VecPosition   posBall  = WM->getBallPos();
  VecPosition   pos = WM->getGlobalPosition(prox);
  VecPosition   pos2 = WM->getGlobalPosition(prox2);

  Line          lineTeam = Line::makeLineFromTwoPoints( WM->getBallPos(),pos);
  Line          lineTeam2 = Line::makeLineFromTwoPoints( WM->getBallPos(),pos2);
  Line          lineEnemy = Line::makeLineFromTwoPoints( WM->getBallPos(),enemy);
  Line          lineEnemy2 = Line::makeLineFromTwoPoints( WM->getBallPos(),enemy2);

  VecPosition   int1 = lineTeam.getIntersection( lineEnemy );
  VecPosition   int2 = lineTeam.getIntersection( lineEnemy2 );
  VecPosition   int3 = lineTeam2.getIntersection( lineEnemy );
  VecPosition   int4 = lineTeam2.getIntersection( lineEnemy2 );


  if( WM->isBeforeKickOff( ) )
  {
    if( WM->isKickOffUs( ) && WM->getPlayerNumber() == 9 ) // 9 takes kick
    {
      if(  WM->isBallInOurPossesion())
      {

      }
      else
      {
        soc = intercept( false );
        Log.log( 100, "move to ball to take kick-off" );
      }
      ACT->putCommandInQueue( soc );
      ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      return soc;
    }
    if( formations->getFormation() != FT_INITIAL || // not in kickoff formation
        posAgent.getDistanceTo( WM->getStrategicPosition() ) > 2.0 )
    {
      formations->setFormation( FT_INITIAL );       // go to kick_off formation
      ACT->putCommandInQueue( soc=teleportToPos( WM->getStrategicPosition() ));
    }
    else                                            // else turn to center
    {
      ACT->putCommandInQueue( soc=turnBodyToPoint( VecPosition( 0, 0 ), 0 ) );
      ACT->putCommandInQueue( alignNeckWithBody( ) );
    }
  }
  else
  {
    formations->setFormation( FT_433_OFFENSIVE );
    soc.commandType = CMD_ILLEGAL;

    if( WM->getConfidence( OBJECT_BALL ) < PS->getBallConfThr() )
    {
      ACT->putCommandInQueue( soc = searchBall() );   // if ball pos unknown
      ACT->putCommandInQueue( alignNeckWithBody( ) ); // search for it
    }
    else if(  WM->isBallKickable())
    {
        if ((WM->getPlayerType(prox) == PT_MIDFIELDER_CENTER || WM->getPlayerType(prox) == PT_MIDFIELDER_WING) && int1.getX()!=0.0 && int1.getY()!=0.0 && int2.getX()!=0.0 && int2.getY()!=0.0 )
        {
            soc = directPass(prox, PASS_NORMAL);
            ACT->putCommandInQueue( soc );
            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
        }
        else if ((WM->getPlayerType(prox) == PT_ATTACKER || WM->getPlayerType(prox) == PT_ATTACKER_WING) && int1.getX()!=0.0 && int1.getY()!=0.0 && int2.getX()!=0.0 && int2.getY()!=0.0 )
        {
            soc = directPass(prox, PASS_NORMAL);
            ACT->putCommandInQueue( soc );
            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
        }
        else if ((WM->getPlayerType(prox2) == PT_MIDFIELDER_CENTER || WM->getPlayerType(prox2) == PT_MIDFIELDER_WING) && int3.getX()!=0.0 && int3.getY()!=0.0 && int4.getX()!=0.0 && int4.getY()!=0.0 )
        {
            soc = directPass(prox2, PASS_NORMAL);
            ACT->putCommandInQueue( soc );
            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
        }
        else if ((WM->getPlayerType(prox2) == PT_ATTACKER || WM->getPlayerType(prox2) == PT_ATTACKER_WING) && int3.getX()!=0.0 && int3.getY()!=0.0 && int4.getX()!=0.0 && int4.getY()!=0.0 )
        {
            soc = directPass(prox2, PASS_NORMAL);
            ACT->putCommandInQueue( soc );
            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
        }
        else if ((WM->getPlayerType(prox) == PT_DEFENDER_SWEEPER) && int1.getX()!=0.0 && int1.getY()!=0.0 && int2.getX()!=0.0 && int2.getY()!=0.0 )
        {
            soc = directPass(prox, PASS_NORMAL);
            ACT->putCommandInQueue( soc );
            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
        }
        else if ((WM->getPlayerType(prox2) == PT_DEFENDER_SWEEPER) && int3.getX()!=0.0 && int3.getY()!=0.0 && int4.getX()!=0.0 && int4.getY()!=0.0 )
        {
            soc = directPass(prox2, PASS_NORMAL);
            ACT->putCommandInQueue( soc );
            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
        }
        else if(int1.getX()!=0.0 && int1.getY()!=0.0 && int2.getX()!=0.0 && int2.getY()!=0.0 )
        {
            soc = directPass(prox, PASS_NORMAL);
            ACT->putCommandInQueue( soc );
            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
        }
        else {
            VecPosition posGoal( PITCH_LENGTH/2.0,
                                 (-1 + 2*(WM->getCurrentCycle()%2)) *
                                 0.4 * SS->getGoalWidth() );
            soc = kickTo( posGoal, SS->getBallSpeedMax() ); // kick maximal
            Log.log( 100, "take kick off" );
        }
    }
    else if( WM->getFastestInSetTo( OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp )
    == WM->getAgentObjectType()  && !WM->isDeadBallThem() )
    {                                                // if fastest to ball
      Log.log( 100, "I am fastest to ball; can get there in %d cycles", iTmp );
      soc = intercept( false );                      // intercept the ball

      if( soc.commandType == CMD_DASH &&             // if stamina low
          WM->getAgentStamina().getStamina() <
             SS->getRecoverDecThr()*SS->getStaminaMax()+200 )
      {
        soc.dPower = 30.0 * WM->getAgentStamina().getRecovery(); // dash slow
        ACT->putCommandInQueue( soc );
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
      else                                           // if stamina high
      {
        ACT->putCommandInQueue( soc );               // dash as intended
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
     }
     else if( posAgent.getDistanceTo(WM->getStrategicPosition()) >
                  1.5 + fabs(posAgent.getX()-posBall.getX())/5.0)
                                                  // if not near strategic pos
     {
       if( WM->getAgentStamina().getStamina() >     // if stamina high
                            SS->getRecoverDecThr()*SS->getStaminaMax()+800 )
       {
         soc = moveToPos(WM->getStrategicPosition(),
                         PS->getPlayerWhenToTurnAngle());
         ACT->putCommandInQueue( soc );            // move to strategic pos
         ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
       }
       else                                        // else watch ball
       {
         ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
         ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
       }
     }
     else if( fabs( WM->getRelativeAngle( OBJECT_BALL ) ) > 1.0 ) // watch ball
     {
       ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
       ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
     }
     else if (WM->isBallInOurPossesion())
     {
       ACT->putCommandInQueue( SoccerCommand(CMD_TURNNECK,0.0) );
     }
     else
     {
        soc = mark(enemy, 0.5, MARK_BALL );                    // nothing to do
       ACT->putCommandInQueue( soc);
     }
   }
  return soc;
}


SoccerCommand Player::erus_attacker() {

    SoccerCommand soc(CMD_ILLEGAL);
    VecPosition posAgent = WM->getAgentGlobalPosition();
    VecPosition posBall = WM->getBallPos();
    int iTmp;
    PlayModeT jogo = WM->getPlayMode();

    /* ObjectT       prox = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES_NO_GOALIE,OBJECT_BALL,NULL,-1.0);
    VecPosition   pos = WM->getGlobalPosition(prox); */

    if (WM->isBeforeKickOff()) {
        if (WM->isKickOffUs() && WM->getPlayerNumber() == 9) // 9 takes kick
        {
            if (WM->isBallKickable()) {
                // Pass to closest ally, instead of just kicking off randomly
                // Re-check this, so it won't pass backwards!
                // Maybe pass to north-facing or south-facing players...?
                ObjectT closestPlayer = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES, posAgent);
                // TODO: Magic numbers!
                //ObjectT playera = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES, VecPosition(-10, -10));
                //ObjectT playerb = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES, VecPosition(-10, 10));
                // Talvez achar alguma forma de randomizar para qual player passar a bola aqui seria bom!
                // soc = directPass(WM->getGlobalPosition(closestPlayer), PASS_NORMAL);
                Log.log(100, "kicking off to player at position " + WM->getGlobalPosition(closestPlayer).str() + "\n");
                soc = directPass(WM->getGlobalPosition(closestPlayer), PASS_NORMAL);
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
            Log.log(100, "Searching for ball...");
        } else if(WM->isFreeKickUs() || WM->isCornerKickUs() || WM->isKickInUs() || (WM->getPlayMode() == PM_INDIRECT_FREE_KICK_LEFT && WM->getSide() == SIDE_LEFT) || ( WM->getPlayMode() == PM_INDIRECT_FREE_KICK_RIGHT && WM->getSide() == SIDE_RIGHT)){
            if(WM->getFastestInSetTo(OBJECT_SET_TEAMMATES_NO_GOALIE, OBJECT_BALL, &iTmp) == WM->getAgentObjectType() &&
                   !WM->isDeadBallThem()){
                if (WM->isBallKickable()) {
                    soc = directPass(WM->getGlobalPosition(WM->getClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE)), PASS_NORMAL);
                    Log.log(100, "Dando um chutão");
                    ACT->putCommandInQueue(soc);
                } else {
                    soc = intercept(false);
                    Log.log(100, "Teleportando");
                    ACT->putCommandInQueue(soc);
                }
            }
            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
        } else if (WM->isBallKickable()) // if kickable
        {
            Log.log(100, "A bola é chutável pra mim.");
            VecPosition posGoleiro = WM->getGlobalPosition(OBJECT_OPPONENT_GOALIE),
                    posTraveDireita = SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_G_R_T, SIDE_LEFT),
                    posTraveEsquerda = SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_G_R_B, SIDE_LEFT);
            // Se a posição do goleiro for inválida, possivelmente não há goleiro, ou não sabemos a posição do goleiro
            if (!WM->isValidPosition(posGoleiro)) {
                // ???
                Log.log(100, "Não achei o goleiro inimigo!");
            }
            // Descobrir a parte em que o goleiro está protegendo mais
            Line gol = Line::makeLineFromTwoPoints(posTraveDireita, posTraveEsquerda);
            VecPosition meuPontoMaisProxGol = gol.getPointOnLineClosestTo(posAgent);
            Line target_raycast = Line::makeLineFromTwoPoints(posAgent, meuPontoMaisProxGol);
            if (meuPontoMaisProxGol.getY() < posTraveDireita.getY()) {
                meuPontoMaisProxGol.setY(posTraveDireita.getY() + 2);
            }
            if (meuPontoMaisProxGol.getY() > posTraveEsquerda.getY()) {
                meuPontoMaisProxGol.setY(posTraveEsquerda.getY() - 2);
            }

            double distToMe1, distToMe2, minhaDistGol = meuPontoMaisProxGol.getDistanceTo(posAgent);

            // Se a gente tá longe do gol, tenta se aproximar primeiro!
            if (minhaDistGol > 20) {
                /* e checar se é seguro driblar*/
                // se temos ao menos dois inimigos por perto, não driblamos, mas passamos
                if (WM->getNrInSetInCircle(OBJECT_SET_OPPONENTS, Circle(posAgent, 8)) > 1) {

                    // Só para teste:
                    ObjectT pass_tgt = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES, posAgent);
                    soc = directPass(WM->getGlobalPosition(pass_tgt), PASS_NORMAL);
                    ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
                    ACT->putCommandInQueue(soc);
                    Log.log(100, "Tentando passar para um colega");
                    return soc;

                    ObjectT candidato1 = WM->getClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE, &distToMe1),
                            candidato2 = WM->getSecondClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE, &distToMe2);
                    double distToGoal1 = WM->getGlobalPosition(candidato1).getDistanceTo(
                            gol.getPointOnLineClosestTo(WM->getGlobalPosition(candidato1))),
                            distToGoal2 = WM->getGlobalPosition(candidato2).getDistanceTo(
                            gol.getPointOnLineClosestTo(WM->getGlobalPosition(candidato2)));

                    ObjectT aliado = OBJECT_ILLEGAL;

                    // Se alguém estiver mais próximo de mim do que eu estou do gol e mais próximo do gol do que eu estou do gol, é o aliado que vai ganhar o chute
                    if (distToMe1 < minhaDistGol && distToGoal1 < minhaDistGol) {
                        aliado = candidato1;
                    } else if (distToMe2 < minhaDistGol && distToGoal2 < minhaDistGol) {
                        aliado = candidato2;
                    }
                    // Se existe aliado com melhores condições de chute:
                    if (aliado != OBJECT_ILLEGAL) {
                        Log.log(100, "Há um aliado com condições melhores de chute. Passando para ele...");
                        VecPosition aliadoPontoMaisProxGol = gol.getPointOnLineClosestTo(WM->getGlobalPosition(aliado));
                        if (WM->isValidPosition(posGoleiro)) {
                            soc = directPass(WM->getGlobalPosition(aliado), PASS_NORMAL);
                            ACT->putCommandInQueue(soc);
                            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
                            return soc;
                        }
                        if ((posGoleiro.getY() < SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_LEFT).getY() &&
                             aliadoPontoMaisProxGol.getY() >=
                             SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_LEFT).getY()) ||
                            (posGoleiro.getY() > SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_LEFT).getY() &&
                             aliadoPontoMaisProxGol.getY() <=
                             SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_LEFT).getY())) {
                            soc = directPass(WM->getGlobalPosition(aliado), PASS_NORMAL);
                            ACT->putCommandInQueue(soc);
                            ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
                            return soc;
                        }
                    } else if (
                            (posGoleiro.getY() < SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_LEFT).getY() &&
                             posAgent.getY() >= SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_LEFT).getY()) ||
                            (posGoleiro.getY() > SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_LEFT).getY() &&
                             posAgent.getY() <= SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_LEFT).getY())) {
                        // kick!
                        soc = directPass(meuPontoMaisProxGol, PASS_NORMAL);
                        Log.log(100, "Chutando pro gol!");
                    } else {
                        // Se eu não tinha boas condições de chute, considerar um dos aliados mesmo assim
                        aliado = (distToGoal1 < distToGoal2) ? candidato1 : candidato2;
                        Log.log(100, "Sem condições de chutar isso ae, vou passar pra alguém");
                        // se o aliado tem chance de gol, passe!!
                        /* if (aliado !=OBJECT_ILLEGAL){
                            VecPosition aliadoPontoMaisProxGol = gol.getPointOnLineClosestTo(WM->getGlobalPosition(aliado));
                            if ( ( goleiro.getY() < enemyGoaliePos.getY() && aliadoPontoMaisProxGol.getY() >= enemyGoaliePos.getY() ) || ( goleiro.getY() > enemyGoaliePos.getY() && aliadoPontoMaisProxGol.getY() <= enemyGoaliePos.getY() ) ){
                                // Passe!
                                soc = directPass(WM->getGlobalPosition(aliado), PASS_NORMAL);
                            }
                        }else */{
                            soc = dashToPoint(VecPosition(meuPontoMaisProxGol.getX(),
                                                          SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_40,
                                                                                             SIDE_LEFT).getY()));
                        }
                    }
                } else {
                    // Checar a quantidade de stamina para determinar o tipo de drible!
                    soc = dribble(target_raycast.getBCoefficient(), DRIBBLE_FAST);
                    ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
                    ACT->putCommandInQueue(soc);
                    Log.log(100, "Tentando me aproximar do gol driblando");
                    Log.log(100, "Ângulo: %.2lf", target_raycast.getBCoefficient());
                    return soc;
                }
            } else {
                soc = kickTo(meuPontoMaisProxGol, SS->getBallSpeedMax());
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
                ACT->putCommandInQueue(soc);
                Log.log(100, "Chutando pro gol!");
                return soc;

            }


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
        } else if (posBall.getX() < SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_20, SIDE_LEFT).getX()) {
            if (WM->getAgentStamina().getStamina() > // if stamina high
                SS->getRecoverDecThr() * SS->getStaminaMax() + 800) {
                soc = moveToPos(VecPosition(posBall.getX(), WM->getStrategicPosition().getY()),
                                enemyGoaliePos.getDirection());
                ACT->putCommandInQueue(soc);
                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL, soc));
            } else // else watch ball
            {
                ACT->putCommandInQueue(soc = turnBodyToObject(OBJECT_BALL));
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

VecPosition Player::best_goalie_kick() {
    VecPosition resp;

    // Passo 1: separar os dois aliados mais próximos como candidatos a recepção do chute
    ObjectT candidato1 = WM->getClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE),
            candidato2 = WM->getSecondClosestRelativeInSet(OBJECT_SET_TEAMMATES_NO_GOALIE);

    if (candidato1 == OBJECT_ILLEGAL || candidato2 == OBJECT_ILLEGAL) {
        VecPosition posAgent = WM->getAgentGlobalPosition();    // posição do próprio goleiro no mundo
        resp.setVecPosition(0, posAgent.getY() * 2.0);
    } else {
        // Passo 2: definir o mais livre
        // Versão 1: O mais longe de um inimigo
        /* double dist1, dist2;
        VecPosition inimigo1 = WM->getPosClosestOpponentTo(&dist1, candidato1), inimigo2 = WM->getPosClosestOpponentTo(&dist2, candidato2);
        VecPosition resp = (dist1 < dist2) ? WM->getGlobalPosition(candidato1) : WM->getGlobalPosition(candidato2); */
        // Versão 2: O que tem menos inimigos em um dado raio
        VecPosition c1 = WM->getGlobalPosition(candidato1), c2 = WM->getGlobalPosition(candidato2);
        int numInimigos1 = WM->getNrInSetInCircle(OBJECT_SET_OPPONENTS,
                                                  Circle(c1, 10.0)), numInimigos2 = WM->getNrInSetInCircle(
                OBJECT_SET_OPPONENTS, Circle(c2, 10.0));
        VecPosition resp = (numInimigos1 < numInimigos2) ? c1 : c2;
        // Versão 3: O que tem o inimigo mais longe de mim
        /* ObjectT inimigo1 = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS, candidato1), inimigo2 = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS, candidato2);
        double dist1 = WM->getRelativeDistance(inimigo1), dist2 = WM->getRelativeDistance(inimigo2);
        VecPosition resp = (dist1 < dist2) ? WM->getGlobalPosition(candidato1) : WM->getGlobalPosition(candidato2); */
    }
    return resp;
};

// FIXME: Calcular nova posição de teleporte para cobrar o tiro livre, podendo ser qualquer uma dentro da penalty area
VecPosition Player::best_goalie_freekick() {
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

SoccerCommand Player::erus_goalie() {
    // Inicialização de variaveis da função
    int i;                                                  // controle de fluxo (??)
    SoccerCommand soc;                                      // comando retornado no final
    VecPosition posAgent = WM->getAgentGlobalPosition();    // posição do próprio goleiro no mundo
    AngDeg angBody = WM->getAgentGlobalBodyAngle();         // angulo do corpo do goleiro em relação ao próprio mundo

    /* Definição do retangulo onde o goleiro se move em sua estratégia básica
       o retangulo é definido para o lado esquerdo e o lado defendido é usado
       como fator de conversão das coordanadas para as reais.
    */
    static const VecPosition posLeftTop(SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_40, SIDE_LEFT).getX() + 0.7 *
                                                                                                                   (SoccerTypes::getGlobalPositionFlag(
                                                                                                                           OBJECT_FLAG_T_L_50,
                                                                                                                           SIDE_LEFT).getX() -
                                                                                                                    SoccerTypes::getGlobalPositionFlag(
                                                                                                                            OBJECT_FLAG_T_L_40,
                                                                                                                            SIDE_LEFT).getX()),
                                        SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY() + 0.9 *
                                                                                                                (SoccerTypes::getGlobalPositionFlag(
                                                                                                                        OBJECT_FLAG_L_T_10,
                                                                                                                        SIDE_LEFT).getY() -
                                                                                                                 SoccerTypes::getGlobalPositionFlag(
                                                                                                                         OBJECT_FLAG_L_0,
                                                                                                                         SIDE_LEFT).getY())),
            posLeftBack(SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_T, SIDE_LEFT).getX(),
                        SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY() + 0.9 *
                                                                                                (SoccerTypes::getGlobalPositionFlag(
                                                                                                        OBJECT_FLAG_L_T_10,
                                                                                                        SIDE_LEFT).getY() -
                                                                                                 SoccerTypes::getGlobalPositionFlag(
                                                                                                         OBJECT_FLAG_L_0,
                                                                                                         SIDE_LEFT).getY())),
            posRightTop(SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_T_L_40, SIDE_LEFT).getX() + 0.7 *
                                                                                                   (SoccerTypes::getGlobalPositionFlag(
                                                                                                           OBJECT_FLAG_T_L_50,
                                                                                                           SIDE_LEFT).getX() -
                                                                                                    SoccerTypes::getGlobalPositionFlag(
                                                                                                            OBJECT_FLAG_T_L_40,
                                                                                                            SIDE_LEFT).getX()),
                        SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY() + 0.9 *
                                                                                                (SoccerTypes::getGlobalPositionFlag(
                                                                                                        OBJECT_FLAG_L_B_10,
                                                                                                        SIDE_LEFT).getY() -
                                                                                                 SoccerTypes::getGlobalPositionFlag(
                                                                                                         OBJECT_FLAG_L_0,
                                                                                                         SIDE_LEFT).getY())),
            posRightBack(SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_T, WM->getSide()).getX(),
                         SoccerTypes::getGlobalPositionFlag(OBJECT_FLAG_L_0, SIDE_LEFT).getY() + 0.9 *
                                                                                                 (SoccerTypes::getGlobalPositionFlag(
                                                                                                         OBJECT_FLAG_L_B_10,
                                                                                                         SIDE_LEFT).getY() -
                                                                                                  SoccerTypes::getGlobalPositionFlag(
                                                                                                          OBJECT_FLAG_L_0,
                                                                                                          SIDE_LEFT).getY()));
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

// TODO: Reimplementar isso aqui no futuro eventualmente...

// SoccerCommand Player::erus_attacker() {
//     SoccerCommand soc(CMD_ILLEGAL);

//     VecPosition posAgent = WM->getAgentGlobalPosition();
//     VecPosition posBall = WM->getBallPos();

//     // This integer holds how much cycles the closest ally need
//     // to reach the ball
//     int iCyclesNeeded;

//     // Before match begins
//     if (WM->isBeforeKickOff()) {
//         // move (teleport) to formation
//         // #9 passes to closest friendly player
//     } else {
//         formations->setFormation(ERUS_DEFAULT_FORMATION);
//         soc.commandType = CMD_ILLEGAL;

//         // Search for ball
//         if (WM->getConfidence(OBJECT_BALL) < PS->getBallConfThr()) {
//             ACT->putCommandInQueue(soc = searchBall());
//             ACT->putCommandInQueue(alignNeckWithBody());
//         }
//             // if we know where the ball is, we can kick it!
//         else if (WM->isBallKickable()) {
//             // old idea: kick to goal directly
//             // new architecture:
//             // raycast to goal; can our kick reach it?
//             // if it can't, can we approach it? are there any obstacles?
//             // if there are, can we pass to someone in a better position?
//             // if we can't, then we messed up; retreat to midfielders
//             // however, if our kick can't reach the goal,
//             // but there are no obstacles, can we approach the goal?
//             // if we can, we do it.
//             // if we can't approach, we'll try to pass to someone on a
//             // better position;

//             /* if the distance to goal is good enough... */
//             if (posAgent.getDistanceTo(WM->getPosOpponentGoal()) < 10.0) {
//                 /* ...we can try to keep on keeping up */
//                 // make line between own goal and the ball

//                 // Enemy goalie stuff
//                 ObjectT enemyGoalie = WM->getOppGoalieType();
//                 VecPosition enemyGoaliePos = WM->getGlobalPosition(enemyGoalie);

//                 // Enemy goal stuff
//                 VecPosition posGoalTarget = (WM->getSide() != SIDE_LEFT)
//                                         ? SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_L, SIDE_LEFT)
//                                         : SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_RIGHT);

//                 // Our casts
//                 Line rcast = Line::makeLineFromTwoPoints(posBall, posGoalTarget);
//                 Line rcast_goalie = Line::makeLineFromTwoPoints(posBall, enemyGoaliePos);

//                 if (/* raycast to goal, goalie position ok */ true) {
//                     /* generate kick arc (left and right from goal) */
//                     /* check where it'll be the hardest for the goalie to catch*/
//                     /* maybe check if some teammate can kick better than us */
//                     if (/* are the conditions optimal? */ true) {
//                         /* kick! */
//                     }
//                 } else /* if the goalie is messing with our kick */
//                 {
//                     if (/* there is some teammate on a decent position*/ true) {
//                         /* try to pass */
//                     } else /* if there are no teammates positioned */
//                     {

//                         /* (we have to trust that our teammate is on the best
//                          *  possible position)
//                          */

//                         /* return ball to midfielders */
//                         /* (or clear ball) */
//                     }
//                 }
//             } else /* if we're not close enough to goal to kick confidently */
//             {
//                 if (/* can we approach the goal safely? */ true) {
//                     /* dribble with the ball to approach the goal */
//                 } else /* there are some kind of obstacle (probably an enemy)
//                  on the way*/
//                 {
//                     if (/* can we dribble the obstacle safely? */ true) {
//                         /* then let's try it */
//                     } else /* if we're not confident that dribbling is the best action */
//                     {
//                         if (/* are there any allies on a good position? */ true) {
//                             /* pass ball to ally */
//                         } else /* there are no allies on good positions */
//                         {
//                             /* return ball to midfielders */
//                             /* (or clear ball) */
//                         }
//                     }
//                 }
//             }
//         }
//         else /* if ball is not kickable */
//         // if we'll get to the ball the fastest from our allies and
//         // the game state allows we to try to move to it
//         if(WM->getFastestInSetTo(OBJECT_SET_TEAMMATES, OBJECT_BALL, &iCyclesNeeded) == WM->getAgentObjectType()
//            && !WM->isDeadBallThem()) {

//         }
//     }
//     return soc;
// }
