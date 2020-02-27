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
SoccerCommand Player::erus_attacker() {

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
