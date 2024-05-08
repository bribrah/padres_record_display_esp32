#include "MlbTeam.h"

MlbTeam::MlbTeam(int teamId)
{
    this->teamId = teamId;
}

void MlbTeam::update()
{
    int httpCode;
    getTeamDetails(httpCode);
    delay(200);
    this->isPlaying = getIsCurrentlyPlaying(httpCode);
    delay(200);
    if (this->isPlaying)
    {
        this->score = getGameScore(this->currentGame, httpCode);
        delay(200);
    }
    getNextGame(httpCode);
    delay(200);

    // printAllInfo();
}

bool MlbTeam::getIsCurrentlyPlaying(int &httpCode)
{
    Serial.println("Checking if game is in progress of team id: " + String(teamId));
    bool isPlaying = false;

    String url = "http://statsapi.mlb.com/api/v1/schedule/games/?sportId=1";
    JsonDocument jsonObject = makeHttpRequest(url, httpCode);
    if (httpCode == 200)
    {
        JsonArray games = jsonObject["dates"][0]["games"];
        Serial.println("Number of games: " + String(games.size()));
        for (int i = 0; i < games.size(); i++)
        {
            String gameState = games[i]["status"]["abstractGameState"];
            if (gameState == "Live" && ((int)games[i]["teams"]["home"]["team"]["id"] == teamId || (int)games[i]["teams"]["away"]["team"]["id"] == teamId))
            {
                Serial.println("Game is in progress");
                currentGame.homeTeam = (const char *)games[i]["teams"]["home"]["team"]["name"];
                currentGame.homeTeamId = (int)games[i]["teams"]["home"]["team"]["id"];
                currentGame.awayTeam = (const char *)games[i]["teams"]["away"]["team"]["name"];
                currentGame.awayTeamId = (int)games[i]["teams"]["away"]["team"]["id"];
                currentGame.gameId = (int)games[i]["gamePk"];
                currentGame.populated = true;
                return true;
            }
        }
        return false;
    }
    else
    {
        Serial.print("Error getting is currently playing code : ");
        Serial.println(httpCode);
        return false;
    }
}

gameScore MlbTeam::getGameScore(gameInfo game, int &httpCode)
{
    Serial.println("Getting game score of game id: " + String(game.gameId));
    gameScore score = {0, 0};

    String url = "http://statsapi.mlb.com/api/v1/game/" + String(game.gameId) + "/linescore";
    JsonDocument jsonObject = makeHttpRequest(url, httpCode);
    if (httpCode == 200)
    {
        score.homeScore = jsonObject["teams"]["home"]["runs"];
        score.awayScore = jsonObject["teams"]["away"]["runs"];
        currentGame.currentInning = jsonObject["currentInning"];
        currentGame.inningState = (const char *)jsonObject["inningState"];
        return score;
    }
    else
    {
        Serial.print("Error getting game score code: ");
        Serial.println(httpCode);
        return score;
    }
}

void MlbTeam::getTeamDetails(int &httpCode)
{
    Serial.println("Getting team info of team id: " + String(teamId));
    String url = "http://statsapi.mlb.com/api/v1/teams/" + String(teamId) + "/?hydrate=standings";
    JsonDocument jsonObject = makeHttpRequest(url, httpCode);
    if (httpCode == 200)
    {
        teamName = (const char *)(jsonObject["teams"][0]["name"]);
        teamAbbreviation = (const char *)(jsonObject["teams"][0]["abbreviation"]);
        teamCity = (const char *)(jsonObject["teams"][0]["locationName"]);
        teamDivision = (const char *)(jsonObject["teams"][0]["division"]["name"]);
        teamLeague = (const char *)(jsonObject["teams"][0]["league"]["name"]);
        teamVenue = (const char *)(jsonObject["teams"][0]["venue"]["name"]);
        int wins = jsonObject["teams"][0]["record"]["leagueRecord"]["wins"];
        int losses = jsonObject["teams"][0]["record"]["leagueRecord"]["losses"];
        gamesBackFromFirst = jsonObject["teams"][0]["record"]["divisionGamesBack"];
        gamesBackFromWildcard = jsonObject["teams"][0]["record"]["wildCardGamesBack"];
        leagueRank = jsonObject["teams"][0]["record"]["leagueRank"];
        divisionRank = jsonObject["teams"][0]["record"]["divisionRank"];
        wildCardRank = jsonObject["teams"][0]["record"]["wildCardRank"];
        streakType = (const char *)jsonObject["teams"][0]["record"]["streak"]["streakType"];
        streakNumber = jsonObject["teams"][0]["record"]["streak"]["streakNumber"];
        runsAllowed = jsonObject["teams"][0]["record"]["runsAllowed"];
        runsScored = jsonObject["teams"][0]["record"]["runsScored"];

        divisionId = jsonObject["teams"][0]["division"]["id"];
        leagueId = jsonObject["teams"][0]["league"]["id"];
        record = {wins, losses};
    }
    else
    {
        Serial.print("Error getting team details code: ");
        Serial.println(httpCode);
    }
}

void MlbTeam::getNextGame(int &httpCode)
{
    String url = "http://statsapi.mlb.com/api/v1/teams/" + String(teamId) + "/?hydrate=nextSchedule";
    JsonDocument jsonObject = makeHttpRequest(url, httpCode);

    if (httpCode == 200)
    {
        for (int i = 0; i < jsonObject["teams"][0]["nextGameSchedule"]["dates"].size(); i++)
        {
            if (jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][0]["status"]["abstractGameState"] == "Preview")
            {
                nextGame.homeTeam = (const char *)jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][0]["teams"]["home"]["team"]["name"];
                nextGame.homeTeamId = (int)jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][0]["teams"]["home"]["team"]["id"];
                nextGame.awayTeam = (const char *)jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][0]["teams"]["away"]["team"]["name"];
                nextGame.awayTeamId = (int)jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][0]["teams"]["away"]["team"]["id"];
                nextGame.gameId = (int)jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][0]["gamePk"];
                // game time format is 2024-04-30T01:40:00Z"
                nextGame.startTime = mlbTimeToWestCoast(jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][0]["gameDate"]);
                nextGame.populated = true;
                break;
            }
        }
    }
    else
    {
        Serial.print("Error getting next game code: ");
        Serial.println(httpCode);
    }
}

void MlbTeam::getDivisionStandings(int &httpCode)
{
    String url = "https://statsapi.mlb.com/api/v1/standings?leagueId=" + String(leagueId);
    JsonDocument jsonObject = makeHttpRequest(url, httpCode);
    if (httpCode == 200)
    {
        
    }
    else
    {
        Serial.print("Error getting division standings code: ");
        Serial.println(httpCode);
    }
}

void MlbTeam::printAllInfo()
{
    Serial.println("Team Name: " + teamName);
    Serial.println("Team ID: " + teamId);
    Serial.println("Team Abbreviation: " + teamAbbreviation);
    Serial.println("Team City: " + teamCity);
    Serial.println("Team Division: " + teamDivision);
    Serial.println("Team League: " + teamLeague);
    Serial.println("Team Venue: " + teamVenue);
    // Serial.println("Record: " + (const char *)record.wins + " - " + record.losses);
    Serial.println("Games Back From First: " + gamesBackFromFirst);
    Serial.println("Games Back From Wildcard: " + gamesBackFromWildcard);
    Serial.println("Division Rank: " + divisionRank);
    Serial.println("League Rank: " + leagueRank);
    Serial.println("Wild Card Rank: " + wildCardRank);
    Serial.println("Streak Type: " + streakType);
    Serial.println("Streak Number: " + streakNumber);
    Serial.println("Runs Allowed: " + runsAllowed);
    Serial.println("Runs Scored: " + runsScored);
    Serial.println("Is Playing: " + isPlaying);
    if (isPlaying)
    {
        Serial.println("Current Game: " + currentGame.homeTeam + " vs " + currentGame.awayTeam);
        // Serial.println("Current Game Score: " + score.homeScore + " - " + score.awayScore);
    }
    else
    {
        Serial.println("No game currently being played");
    }
    // Serial.println("Next Game Time: " + asctime(&nextGame.startTime));
}