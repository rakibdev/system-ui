import media;
import argparser;
import log;

import std;

void usage() {
  std::println("  {:<35} {}", "media list", "List available players");
  std::println("  {:<35} {}", "media play-pause|next|previous", "Control playback");
  std::println("  {:<35} {}", "media progress <value>", "Set progress percentage");
  std::println("  {:<35} {}", "media <command> --player <name>", "Target specific player");
  std::println("");
  std::println("  e.g.");
  std::println("  media list");
  std::println("  media play-pause");
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

      std::println("Player: {}", getPlayerName(player->bus));
      std::println("Status: {}", status);
      std::println("Title: {}", player->title.empty() ? "-" : player->title);
      std::println("");
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
