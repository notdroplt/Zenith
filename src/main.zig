const std = @import("std");
const zenith = @import("zenith.zig");
const misc = @import("misc.zig");

// until we have a build system, we'll just embed the file
const code = @embedFile("main.zenith");

pub fn main() !u8 {
    var gpa = std.heap.GeneralPurposeAllocator(.{
        .verbose_log = true,
    }){};
    const alloc = gpa.allocator();

    defer _ = gpa.deinit();

    return zenith.pipeline("main.zenith", alloc);
}
