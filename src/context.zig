const std = @import("std");
const misc = @import("misc.zig");

pub fn Context(comptime T: type) type {
    return struct {
        const Self = @This();

        parent: ?*Self = null,
        members: std.StringHashMapUnmanaged(T) = .{},
        children: std.StringHashMapUnmanaged(Self) = .{},

        /// Add a value in the current context
        pub inline fn add(self: *Self, alloc: std.mem.Allocator, name: misc.String, value: T) !void {
            try self.members.put(alloc, name, value);
        }

        /// Add a value at a path, creating child contexts as needed.
        pub fn addTree(self: *Self, allocator: std.mem.Allocator, path: misc.String, name: misc.String, value: T) !void {
            var ctx = self;
            var it = std.mem.splitScalar(u8, path, '/');
            while (it.next()) |segment| {
                if (ctx.children.getPtr(segment)) |child| {
                    ctx = child;
                } else {
                    try ctx.children.put(allocator, segment, .{ .parent = ctx });
                    ctx = ctx.children.getPtr(segment).?;
                }
            }
            try ctx.members.put(allocator, name, value);
        }

        /// Get a value by path and name, searching up the parent chain
        /// if not found locally.
        pub fn getTree(self: *Self, path: misc.String, name: misc.String) ?T {
            var ctx  = self;
            var it = std.mem.splitScalar(u8, path, '/');
            while (it.next()) |segment| {
                if (ctx.children.getPtr(segment)) |child| {
                    ctx = child;
                } else return null;
            }
            return ctx.get(name);
        }

        /// Get a value by name, searching up the parent chain.
        pub fn get(self: *Self, name: misc.String) ?T {
            var p: ?*Self = self;

            while (p) |ctx| : (p = ctx.parent)
                if (ctx.members.get(name)) |val|
                    return val;

            return null;
        }

        pub fn deinit(self: *Self, allocator: std.mem.Allocator) void {
            var it = self.children.iterator();
            while (it.next()) |pair| {
                pair.value_ptr.deinit(allocator);
            }
            self.members.deinit(allocator);
            self.children.deinit(allocator);
        }

        pub fn isEmpty(self: *Self) bool {
            return !self.members.count() and !self.children.count();
        }
    };
}

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;

test "Context - is empty" {
    const alloc = std.testing.allocator;
    var ctx = Context(u8).init(alloc);
    defer ctx.deinit(alloc);

    try expect(ctx.isEmpty());
    try ctx.add("test", 0);
    try expect(!ctx.isEmpty());
}

test "Context - get/add" {
    const alloc = std.testing.allocator;
    var ctx = Context(u8).init(alloc);
    defer ctx.deinit(alloc);

    try ctx.add("test", 0);
    try expect(ctx.get("test") != null);
    try expectEqual(0, ctx.get("test"));
}

test "Context - getTree/addTree" {
    const alloc = std.testing.allocator;
    var ctx = Context(u8).init(alloc);
    defer ctx.deinit(alloc);

    try ctx.addTree(alloc, "a/b/c", "test", 0);
    try expect(ctx.getTree("a/b/c", "test") != null);
    try expectEqual(ctx.getTree("a/b/c", "test"), 0);
}
