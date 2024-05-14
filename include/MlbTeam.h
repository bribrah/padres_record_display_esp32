#pragma once

#include "utils.h"
#include <Arduino.h>
#include "datatypes.h"
#include <ArduinoJson.h>
#include "utils.h"
#include "datatypes.h"

class MlbTeam
{
public:
    int teamId;
    int divisionId;
    int leagueId;
    String teamName;
    String teamAbbreviation;
    String teamCity;
    String teamDivision;
    String teamLeague;
    String teamVenue;
    String streakType;

    winLossRecord record;
    bool isPlaying;
    gameInfo currentGame;
    gameInfo nextGame;
    gameScore score;

    int gamesBackFromFirst;
    int gamesBackFromWildcard;
    int divisionRank;
    int leagueRank;
    int wildCardRank;
    int streakNumber;
    int runsAllowed;
    int runsScored;

    MlbTeam(int teamId);

    void update();
    void begin();

    bool getIsCurrentlyPlaying(int &httpCode);
    gameScore getGameScore(gameInfo game, int &httpCode);

    void getTeamDetails(int &httpCode);
    void getNextGame(int &httpCode);
    void getDivisionStandings(int &httpCode);
    void printAllInfo();
};