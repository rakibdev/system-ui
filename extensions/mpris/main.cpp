
if (args[0] == "media") {
  MediaController media;
  auto players = media.getPlayers();
  if (players.empty()) {
    Log::error("No active player found.");
    return 1;
  } else {
    std::unique_ptr<PlayerController> &player = players.front();
    if (args[1] == "play-pause") player->playPause();
    if (args[1] == "next") player->next();
    if (args[1] == "previous") player->previous();
    if (args[1] == "progress") player->progress(std::stoi(args[2]));
    return 0;
  }
}
