#include "MlbTeam.h"

MlbTeam::MlbTeam(int teamId)
    : teamId(teamId),
      divisionId(0),
      leagueId(0),
      fullName(""),
      teamName(""),
      teamAbbreviation(""),
      teamCity(""),
      teamDivision(""),
      teamLeague(""),
      teamVenue(""),
      streakType(""),
      lastPlay(""),
      record{0, 0},
      lastTenRecord{0, 0},
      nextOpponentRecord{0, 0},
      isPlaying(false),
      currentGame{},
      nextGame{},
      score{0, 0},
      gamesBackFromFirst(0),
      gamesBackFromWildcard(0),
      divisionRank(0),
      leagueRank(0),
      wildCardRank(0),
      streakNumber(0),
      runsAllowed(0),
      runsScored(0),
      opponent(nullptr)
{
}

MlbTeam::MlbTeam()
    : MlbTeam(0)
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
        if (opponent != nullptr)
        {
            delete opponent;
            opponent = nullptr;
        }
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
        if (!jsonObject["dates"].is<JsonArray>() || jsonObject["dates"].as<JsonArray>().size() == 0)
        {
            currentGame.populated = false;
            return false;
        }

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
        currentGame.populated = false;
        return false;
    }
}

void MlbTeam::getCurrentGameInfo(int &httpCode)
{
    Serial.println("Getting game score of game id: " + String(currentGame.gameId));
    gameScore score = {0, 0};
    currentGame.runnerOnFirst = false;
    currentGame.runnerOnFirstName = "";
    currentGame.runnerOnSecond = false;
    currentGame.runnerOnSecondName = "";
    currentGame.runnerOnThird = false;
    currentGame.runnerOnThirdName = "";
    currentGame.hasLastScoringPlay = false;
    currentGame.lastScoringPlay = "";

    // String url = "http://statsapi.mlb.com/api/v1/game/" + String(game.gameId) + "/linescore";
    String url = "http://statsapi.mlb.com/api/v1.1/game/" + String(currentGame.gameId) + "/feed/live";
    JsonDocument filters;
    filters["liveData"]["linescore"]["teams"] = true;
    filters["liveData"]["linescore"]["currentInning"] = true;
    filters["liveData"]["linescore"]["inningState"] = true;
    filters["liveData"]["linescore"]["outs"] = true;
    filters["liveData"]["linescore"]["balls"] = true;
    filters["liveData"]["linescore"]["strikes"] = true;
    filters["liveData"]["linescore"]["offense"]["first"] = true;
    filters["liveData"]["linescore"]["offense"]["second"] = true;
    filters["liveData"]["linescore"]["offense"]["third"] = true;
    filters["liveData"]["plays"]["currentPlay"] = true;
    filters["liveData"]["plays"]["scoringPlays"] = true;
    filters["liveData"]["plays"]["allPlays"][0]["result"]["description"] = true;
    filters["liveData"]["plays"]["allPlays"][0]["about"]["inning"] = true;
    filters["liveData"]["plays"]["allPlays"][0]["about"]["halfInning"] = true;
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

        JsonObject offense = jsonObject["liveData"]["linescore"]["offense"];
        currentGame.runnerOnFirst = offense["first"]["fullName"].is<const char *>();
        currentGame.runnerOnFirstName = currentGame.runnerOnFirst ? (const char *)offense["first"]["fullName"] : "";
        currentGame.runnerOnSecond = offense["second"]["fullName"].is<const char *>();
        currentGame.runnerOnSecondName = currentGame.runnerOnSecond ? (const char *)offense["second"]["fullName"] : "";
        currentGame.runnerOnThird = offense["third"]["fullName"].is<const char *>();
        currentGame.runnerOnThirdName = currentGame.runnerOnThird ? (const char *)offense["third"]["fullName"] : "";

        currentGame.hasLastPlay = false;
        currentGame.lastPlay = "";
        JsonArray allPlays = jsonObject["liveData"]["plays"]["allPlays"];
        if (allPlays.size() > 0)
        {
            JsonObject lastCompletedPlay = allPlays[allPlays.size() - 1];
            if (lastCompletedPlay["result"]["description"].is<const char *>())
            {
                currentGame.lastPlay = (const char *)lastCompletedPlay["result"]["description"];
                currentGame.hasLastPlay = true;
            }
        }
        else if (jsonObject["liveData"]["plays"]["currentPlay"]["result"]["description"].is<const char *>())
        {
            currentGame.lastPlay = (const char *)jsonObject["liveData"]["plays"]["currentPlay"]["result"]["description"];
            currentGame.hasLastPlay = true;
        }

        JsonArray scoringPlays = jsonObject["liveData"]["plays"]["scoringPlays"];
        if (scoringPlays.size() > 0)
        {
            int lastScoringPlayIndex = scoringPlays[scoringPlays.size() - 1];
            JsonObject lastScoringPlay = jsonObject["liveData"]["plays"]["allPlays"][lastScoringPlayIndex];
            if (lastScoringPlay["result"]["description"].is<const char *>())
            {
                currentGame.lastScoringPlay = (const char *)lastScoringPlay["result"]["description"];
                currentGame.hasLastScoringPlay = true;
            }
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

        JsonArray splitRecords = jsonObject["teams"][0]["record"]["records"]["splitRecords"];
        if (splitRecords.size() > 8)
        {
            lastTenRecord.wins = splitRecords[8]["wins"];
            lastTenRecord.losses = splitRecords[8]["losses"];
        }
        else
        {
            lastTenRecord = {0, 0};
        }
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
    nextOpponentRecord = {0, 0};
    nextGame.homeProbablePitcher = "Unknown";
    nextGame.awayProbablePitcher = "Unknown";
    if (httpCode == 200)
    {
        if (!jsonObject["teams"][0]["nextGameSchedule"]["dates"].is<JsonArray>())
        {
            return;
        }
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
                        if (jsonObject["gameData"]["probablePitchers"]["home"]["fullName"].is<const char *>())
                        {
                            nextGame.homeProbablePitcher = removeAccented((const char *)jsonObject["gameData"]["probablePitchers"]["home"]["fullName"]);
                        }
                        else
                        {
                            nextGame.homeProbablePitcher = "Unknown";
                        }
                        if (jsonObject["gameData"]["probablePitchers"]["away"]["fullName"].is<const char *>())
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
