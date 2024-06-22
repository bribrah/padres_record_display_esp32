#include "MlbTeam.h"

MlbTeam::MlbTeam(int teamId)
{
    this->teamId = teamId;
}

MlbTeam::MlbTeam()
{
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
        getCurrentGameInfo(httpCode);
        currentGame.populated = true;
        delay(200);
    }
    else
    {
        currentGame.populated = false;
        getNextGame(httpCode);
    }
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
                if (opponent != nullptr)
                {
                    delete opponent;
                }
                opponent = new MlbTeam(currentGame.homeTeamId == teamId ? currentGame.awayTeamId : currentGame.homeTeamId);
                opponent->getTeamDetails(httpCode);
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

void MlbTeam::getCurrentGameInfo(int &httpCode)
{
    Serial.println("Getting game score of game id: " + String(currentGame.gameId));
    gameScore score = {0, 0};

    // String url = "http://statsapi.mlb.com/api/v1/game/" + String(game.gameId) + "/linescore";
    String url = "http://statsapi.mlb.com/api/v1.1/game/" + String(currentGame.gameId) + "/feed/live";
    JsonDocument filters;
    filters["liveData"]["linescore"]["teams"] = true;
    filters["liveData"]["linescore"]["currentInning"] = true;
    filters["liveData"]["linescore"]["inningState"] = true;
    filters["liveData"]["linescore"]["outs"] = true;
    filters["liveData"]["linescore"]["balls"] = true;
    filters["liveData"]["linescore"]["strikes"] = true;
    filters["liveData"]["plays"]["currentPlay"] = true;
    JsonDocument jsonObject = makeHttpRequest(url, filters, httpCode);
    if (httpCode == 200)
    {
        score.homeScore = jsonObject["liveData"]["linescore"]["teams"]["home"]["runs"];
        score.awayScore = jsonObject["liveData"]["linescore"]["teams"]["away"]["runs"];
        currentGame.currentInning = jsonObject["liveData"]["linescore"]["currentInning"];
        currentGame.inningState = (const char *)jsonObject["liveData"]["linescore"]["inningState"];
        currentGame.outs = jsonObject["liveData"]["linescore"]["outs"];
        currentGame.balls = jsonObject["liveData"]["linescore"]["balls"];
        currentGame.strikes = jsonObject["liveData"]["linescore"]["strikes"];
        currentGame.score = score;
        if (jsonObject["liveData"]["plays"]["currentPlay"]["result"].containsKey("description"))
        {
            currentGame.lastPlay = (const char *)jsonObject["liveData"]["plays"]["currentPlay"]["result"]["description"];
            currentGame.hasLastPlay = true;
        }
        else
        {
            currentGame.hasLastPlay = false;
        }
    }
    else
    {
        Serial.print("Error getting game score code: ");
        Serial.println(httpCode);
    }
}

void MlbTeam::getTeamDetails(int &httpCode)
{
    Serial.println("Getting team info of team id: " + String(teamId));
    String url = "http://statsapi.mlb.com/api/v1/teams/" + String(teamId) + "/?hydrate=standings";
    JsonDocument jsonObject = makeHttpRequest(url, httpCode);
    if (httpCode == 200)
    {
        fullName = (const char *)(jsonObject["teams"][0]["name"]);
        teamName = (const char *)(jsonObject["teams"][0]["teamName"]);
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

        lastTenRecord.wins = jsonObject["teams"][0]["record"]["records"]["splitRecords"][8]["wins"];
        lastTenRecord.losses = jsonObject["teams"][0]["record"]["records"]["splitRecords"][8]["losses"];
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
    nextGame.populated = false;
    if (httpCode == 200)
    {
        for (int i = 0; i < jsonObject["teams"][0]["nextGameSchedule"]["dates"].size(); i++)
        {
            int numGames = jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"].size();
            for (int j = 0; j < numGames; j++)
            {
                if (jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][j]["status"]["abstractGameState"] == "Preview")
                {
                    String link = (const char *)jsonObject["teams"][0]["nextGameSchedule"]["dates"][i]["games"][j]["link"];
                    url = "http://statsapi.mlb.com" + link;
                    JsonDocument filter;
                    filter["gameData"]["teams"] = true;
                    filter["gameData"]["probablePitchers"] = true;
                    filter["gameData"]["datetime"] = true;

                    jsonObject = makeHttpRequest(url, filter, httpCode);
                    if (httpCode == 200)
                    {

                        nextGame.homeTeam = (String)(const char *)jsonObject["gameData"]["teams"]["home"]["abbreviation"] + " " + (String)(const char *)jsonObject["gameData"]["teams"]["home"]["teamName"];
                        nextGame.homeTeamId = (int)jsonObject["gameData"]["teams"]["home"]["id"];
                        nextGame.homeTeamAbbreviation = (const char *)jsonObject["gameData"]["teams"]["home"]["abbreviation"];

                        nextGame.awayTeam = (String)(const char *)jsonObject["gameData"]["teams"]["away"]["abbreviation"] + " " + (String)(const char *)jsonObject["gameData"]["teams"]["away"]["teamName"];
                        nextGame.awayTeamId = (int)jsonObject["gameData"]["teams"]["away"]["id"];
                        nextGame.awayTeamAbbreviation = (const char *)jsonObject["gameData"]["teams"]["away"]["abbreviation"];

                        nextGame.gameId = (int)jsonObject["gamePk"];
                        if (jsonObject["gameData"]["probablePitchers"].containsKey("home"))
                        {
                            nextGame.homeProbablePitcher = removeAccented((const char *)jsonObject["gameData"]["probablePitchers"]["home"]["fullName"]);
                        }
                        else
                        {
                            nextGame.homeProbablePitcher = "Unknown";
                        }
                        if (jsonObject["gameData"]["probablePitchers"].containsKey("away"))
                        {
                            nextGame.awayProbablePitcher = removeAccented((const char *)jsonObject["gameData"]["probablePitchers"]["away"]["fullName"]);
                        }
                        else
                        {
                            nextGame.awayProbablePitcher = "Unknown";
                        }
                        Serial.print("Next game probable pitchers: ");
                        Serial.print(nextGame.homeProbablePitcher);
                        Serial.print(" vs ");
                        Serial.println(nextGame.awayProbablePitcher);

                        bool padresHome = nextGame.homeTeamId == teamId;
                        if (padresHome)
                        {
                            nextOpponentRecord.losses = (int)jsonObject["gameData"]["teams"]["away"]["record"]["losses"];
                            nextOpponentRecord.wins = (int)jsonObject["gameData"]["teams"]["away"]["record"]["wins"];
                        }
                        else
                        {
                            nextOpponentRecord.losses = (int)jsonObject["gameData"]["teams"]["home"]["record"]["losses"];
                            nextOpponentRecord.wins = (int)jsonObject["gameData"]["teams"]["home"]["record"]["wins"];
                        }

                        // game time format is 2024-04-30T01:40:00Z"
                        nextGame.startTime = mlbTimeToWestCoast((const char *)jsonObject["gameData"]["datetime"]["dateTime"]);

                        nextGame.populated = true;
                        break;
                    }
                }
            }
            if (nextGame.populated)
            {
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