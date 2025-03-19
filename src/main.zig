const std = @import("std");
const zenith = @import("zenith.zig");
const debug = @import("debug.zig");
const misc = @import("misc.zig");
pub fn main() !void {
    const stdout_file = std.io.getStdOut().writer();
    var bw = std.io.bufferedWriter(stdout_file);
    const stdout = bw.writer();

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const alloc = gpa.allocator();

    var aalloc = std.heap.ArenaAllocator.init(alloc);
    const palloc = aalloc.allocator();

    defer {
        _ = aalloc.reset(std.heap.ArenaAllocator.ResetMode.free_all);
        aalloc.deinit();
        _ = gpa.deinit();
    }

    var parser = zenith.Parser.init("three a = caller callee;", palloc);

    const errnode = parser.parse_node(); 
    if (errnode) |node| {

        try debug.print_node(stdout, node);
        const isexpr = switch (node.data) {
            .expr => true,
            else => false,
        };
        try stdout.print("first node is expr: {}\n", .{isexpr});

    } else |err| {
        try misc.printError(stdout, parser.code, parser.pos, err);
    }
    
    try bw.flush(); // don't forget to flush!
}
