const std = @import("std");
const misc = @import("misc.zig");

pub fn Context(comptime T: type) type {
    return struct {
        const Self = @This();

        parent: ?*Self = null,
        members: std.StringHashMap(T),
        children: std.StringHashMap(Self),

        pub fn init(allocator: std.mem.Allocator) Self {
            return Self{
                .members = std.StringHashMap(T).init(allocator),
                .children = std.StringHashMap(Self).init(allocator),
            };
        }

        pub fn add(self: *Self, name: misc.String, value: T) !void {
            try self.members.put(name, value);
        }

        /// Add a value at a path, creating child contexts as needed.
        pub fn addTree(self: *Self, allocator: std.mem.Allocator, path: misc.String, name: misc.String, value: T) !void {
            var ctx = self;
            var it = std.mem.splitScalar(u8, path, '/');
            while (it.next()) |segment| {
                if (segment.len == 0) continue;
                if (ctx.children.getEntry(segment)) |child| {
                    ctx = child.value_ptr;
                } else {
                    var new_child = Self.init(allocator);
                    new_child.parent = ctx;
                    try ctx.children.put(segment, new_child);
                    ctx = ctx.children.getEntry(segment).?.value_ptr;
                }
            }
            try ctx.members.put(name, value);
        }

        /// Get a value by path and name, searching up the parent chain if not found locally.
        pub fn getTree(self: *Self, path: misc.String, name: misc.String) ?T {
            var ctx: *Self = self;
            var it = std.mem.splitScalar(u8, path, '/');
            while (it.next()) |segment| {
                if (segment.len == 0) continue;
                if (ctx.children.getEntry(segment)) |child| {
                    ctx = child.value_ptr;
                } else {
                    return null;
                }
            }
            return ctx.get(name);
        }

        /// Get a value by name, searching up the parent chain.
        pub fn get(self: *Self, name: misc.String) ?T {
            if (self.members.get(name)) |val| {
                return val;
            } else if (self.parent) |p| {
                return p.get(name);
            } else {
                return null;
            }
        }

        pub fn deinit(self: *Self, allocator: std.mem.Allocator) void {
            var it = self.children.iterator();
            while (it.next()) |pair| {
                pair.value_ptr.deinit(allocator);
            }
            self.members.deinit();
            self.children.deinit();
        }

        pub fn isEmpty(self: *Self) bool {
            return self.members.count() == 0 and self.children.count() == 0;
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
