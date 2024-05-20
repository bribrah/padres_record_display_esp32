#pragma once

#define PADRES_TEAM_ID 135
#include <time.h>
#include <sys/time.h>
struct winLossRecord
{
    int wins;
    int losses;
};

struct gameScore
{
    int homeScore;
    int awayScore;
};

struct gameInfo
{
    String homeTeam;
    int homeTeamId;
    String homeProbablePitcher;
    
    String awayTeam;
    int awayTeamId;
    String awayProbablePitcher;

    int gameId;
    String startTime;
    int currentInning;
    String inningState;
    String lastPlay;
    bool populated = false;
};
