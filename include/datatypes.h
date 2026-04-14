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
    String homeTeamAbbreviation;
    int homeTeamId;
    String homeProbablePitcher;

    String awayTeam;
    String awayTeamAbbreviation;
    int awayTeamId;
    String awayProbablePitcher;

    int gameId;
    String startTime;
    int currentInning;
    String inningState;
    String lastPlay;
    bool hasLastPlay;
    String lastScoringPlay;
    bool hasLastScoringPlay = false;
    bool runnerOnFirst = false;
    String runnerOnFirstName;
    bool runnerOnSecond = false;
    String runnerOnSecondName;
    bool runnerOnThird = false;
    String runnerOnThirdName;
    gameScore score;
    int outs;
    int balls;
    int strikes;

    bool populated = false;
};
