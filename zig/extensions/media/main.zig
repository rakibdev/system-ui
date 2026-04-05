const std = @import("std");
const media = @import("media");
const log = @import("log");

fn usage() void {
    const table =
        \\  media list                          List available players
        \\  media play-pause|next|previous      Control playback
        \\  media progress <value>              Set progress percentage
        \\  media <command> --player <name>     Target specific player
        \\
        \\  e.g.
        \\  media list
        \\  media play-pause
    ;
    std.fs.File.stdout().writeAll(table ++ "\n") catch {};
}

fn playerName(bus: []const u8) []const u8 {
    return if (std.mem.startsWith(u8, bus, "org.mpris.MediaPlayer2."))
        bus["org.mpris.MediaPlayer2.".len..]
    else
        bus;
}

pub fn main() !u8 {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const alloc = gpa.allocator();

    const args = try std.process.argsAlloc(alloc);
    defer std.process.argsFree(alloc, args);

    if (args.len <= 1 or std.mem.eql(u8, args[1], "--help")) {
        usage();
        return 0;
    }

    var svc = try media.Media.init(alloc);
    defer svc.deinit();

    const players = try svc.getPlayers();
    defer {
        for (players) |p| p.deinit();
        alloc.free(players);
    }

    const cmd = args[1];

    if (std.mem.eql(u8, cmd, "list")) {
        if (players.len == 0) {
            std.fs.File.stdout().writeAll("No active players.\n") catch {};
            return 0;
        }
        const stdout = std.fs.File.stdout();
        for (players) |p| {
            const status = switch (p.status) {
                .Playing => "Playing",
                .Paused => "Paused",
                .Stopped => "Stopped",
            };
            var buf: [512]u8 = undefined;
            const out = std.fmt.bufPrint(&buf, "Player: {s}\nStatus: {s}\nTitle:  {s}\n\n", .{
                playerName(p.dest),
                status,
                if (p.title.len > 0) p.title else "-",
            }) catch continue;
            stdout.writeAll(out) catch {};
        }
        return 0;
    }

    if (players.len == 0) {
        log.err("No active player found.");
        return 1;
    }

    // Resolve target player
    var target: *media.Player = players[0];
    for (args[2..]) |arg| {
        if (std.mem.eql(u8, arg, "--player")) break;
    } else {
        // check if --player was provided
        var i: usize = 2;
        while (i + 1 < args.len) : (i += 1) {
            if (std.mem.eql(u8, args[i], "--player")) {
                const name = args[i + 1];
                target = for (players) |p| {
                    if (std.mem.eql(u8, playerName(p.dest), name)) break p;
                } else {
                    log.err("Player not found.");
                    return 1;
                };
                break;
            }
        }
    }

    if (std.mem.eql(u8, cmd, "play-pause")) {
        target.playPause();
    } else if (std.mem.eql(u8, cmd, "next")) {
        target.next();
    } else if (std.mem.eql(u8, cmd, "previous")) {
        target.previous();
    } else if (std.mem.eql(u8, cmd, "progress")) {
        if (args.len < 3) {
            log.err("Progress value required.");
            return 1;
        }
        const val = std.fmt.parseInt(u8, args[2], 10) catch {
            log.err("Invalid progress value.");
            return 1;
        };
        target.setProgress(val);
    } else {
        log.err("Unknown command.");
        usage();
        return 1;
    }

    return 0;
}
