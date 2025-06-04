const std = @import("std");
const zenith = @import("zenith.zig");
const misc = @import("misc.zig");

pub fn main() !u8 {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const alloc = gpa.allocator();

    defer _ = gpa.deinit();

    return zenith.pipeline("main.zenith", alloc);
}
