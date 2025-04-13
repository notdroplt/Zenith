const std = @import("std");
const zenith = @import("zenith.zig");
const misc = @import("misc.zig");

// until we have a build system, we'll just embed the file
const code = @embedFile("main.zenith");

pub fn main() !void {
    const stdout_file = std.io.getStdOut().writer();
    var bw = std.io.bufferedWriter(stdout_file);
    const stdout = bw.writer();

    var gpa = std.heap.GeneralPurposeAllocator(.{
        .verbose_log = false
    }){};
    const alloc = gpa.allocator();

    var aalloc = std.heap.ArenaAllocator.init(alloc);
    const palloc = aalloc.allocator();

    defer {
        _ = aalloc.reset(std.heap.ArenaAllocator.ResetMode.free_all);
        aalloc.deinit();
        _ = gpa.deinit();
    }

    var parser = zenith.Parser.init(code, palloc);

    const parsed = parser.parse_node();
    if (parsed) |node| {
        var analyzer = try zenith.Analyzer.init(palloc);
        const typed = analyzer.runAnalysis(node);

        if (typed) |_| {
            // try stdout.print("analysis successful\n", .{});
        } else |err| {
            _ = try misc.printError(stdout, parser.code, node.position, err);

        }
        
    } else |err| {
        try misc.printError(stdout, parser.code, parser.pos, err);
    }

    // try stdout.print("Zenithc [allocated {} bytes]\n", .{aalloc.queryCapacity()});

    try bw.flush();
}
