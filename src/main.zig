const std = @import("std");
const zenith = @import("zenith.zig");

pub fn main() !u8 {
    var gpa = std.heap.GeneralPurposeAllocator(.{
        //.verbose_log = true
    }){};
    const alloc = gpa.allocator();
    const start_time = std.time.nanoTimestamp();

    defer _ = gpa.deinit();

    const res = try zenith.pipeline("main.zenith", alloc);
    const end_time = std.time.nanoTimestamp();

    const delta = end_time - start_time;
    std.debug.print("done in {}ns\n", .{delta});
    return res;

}
