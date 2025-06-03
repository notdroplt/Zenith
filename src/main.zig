const std = @import("std");
const pipeline = @import("pipeline.zig");
const misc = @import("misc.zig");

pub fn main() !u8 {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const alloc = gpa.allocator();

    defer _ = gpa.deinit();

    return pipeline.pipeline("main.zenith", alloc);
}
