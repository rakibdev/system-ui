#include <iostream>

#include "../../src/services/media.h"
#include "../../src/utils/argparser.h"
#include "../../src/utils/log.h"

void usage() {
  Log::Table content = {
      {"media", "list", "List available players"},
      {"media", "play-pause|next|previous", "Control playback"},
      {"media", "progress <value>", "Set progress percentage"},
      {"media", "<command> --player <name>", "Target specific player"},
      {""},
      {"e.g."},
      {"media list"},
      {"media play-pause"}};
  Log::table(content);
}

std::string getPlayerName(const std::string& bus) {
  const std::string prefix = "org.mpris.MediaPlayer2.";
  return bus.starts_with(prefix) ? bus.substr(prefix.length()) : bus;
}

int main(int argc, char* argv[]) {
  if (argc <= 1 || std::string(argv[1]) == "--help") {
    usage();
    return 0;
  }

  ArgParser args(argc, argv);
  MediaService media;
  auto players = media.getPlayers();

  if (players.empty()) {
    Log::error("No active player found.");
    return args[0] == "list" ? 0 : 1;
  }

  if (args[0] == "list") {
    for (const auto& player : players) {
      std::string status;
      if (player->status == PlayerService::Playing)
        status = "Playing";
      else if (player->status == PlayerService::Paused)
        status = "Paused";
      else if (player->status == PlayerService::Stopped)
        status = "Stopped";

      std::cout << "Player: " << getPlayerName(player->bus) << std::endl;
      std::cout << "Status: " << status << std::endl;
      std::cout << "Title: " << (player->title.empty() ? "-" : player->title)
                << std::endl;
      std::cout << std::endl;
    }
    return 0;
  }

  std::string targetPlayer = args.value("--player");
  PlayerService* player = nullptr;

  if (targetPlayer.empty()) {
    player = players.front().get();
  } else {
    for (const auto& p : players) {
      if (getPlayerName(p->bus) == targetPlayer) {
        player = p.get();
        break;
      }
    }
    if (!player) {
      Log::error("Player not found: " + targetPlayer);
      return 1;
    }
  }

  if (args[0] == "progress") {
    if (args[1].empty()) {
      Log::error("Progress value required.");
      return 1;
    }
    int progress = std::stoi(args[1]);
    player->progress(progress);
  } else if (args[0] == "play-pause")
    player->playPause();
  else if (args[0] == "next")
    player->next();
  else if (args[0] == "previous")
    player->previous();

  return 0;
}
