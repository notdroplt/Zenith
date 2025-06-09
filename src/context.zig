const std = @import("std");
const misc = @import("misc.zig");

pub fn Context(comptime T: type) type {
    return struct {
        const ctxt = @This();

        parent: ?*ctxt = null,
        alloc: std.mem.Allocator,
        members: std.StringHashMap(T),
        children: std.StringHashMap(ctxt),

        pub fn init(alloc: std.mem.Allocator) ctxt {
            return ctxt{
                .alloc = alloc,
                .members = std.StringHashMap(T).init(alloc),
                .children = std.StringHashMap(ctxt).init(alloc),
            };
        }

        pub fn initTree(alloc: std.mem.Allocator, path: misc.String) !*ctxt {
            const ctx = ctxt.init(alloc);
            const parts = std.mem.splitScalar(u8, path, '/');
            var currentCtx = ctx;

            while (parts.next()) |part| {
                var subCtx = currentCtx.children.get(part);
                if (subCtx == null) {
                    subCtx = ctxt.init(alloc);
                    subCtx.parent = currentCtx.?;
                    try currentCtx.children.put(part, subCtx);
                }
                currentCtx = subCtx.?;
            }

            return ctx;
        }

        pub fn getTree(self: ctxt, path: misc.String, name: misc.String) ?T {
            var parts = std.mem.splitScalar(u8, path, '/');
            var currentCtx = self;

            while (parts.next()) |part| {
                std.debug.print("Getting part: {s}\n", .{part});
                const subCtx = currentCtx.children.get(part);
                if (subCtx == null) return null;
                currentCtx = subCtx.?;
            }

            std.debug.print("Getting member: {s}\n", .{name});
            return currentCtx.get(name);
        }

        pub fn addTree(self: *ctxt, path: misc.String, name: misc.String, value: T) !void {
            var parts = std.mem.splitScalar(u8, path, '/');
            var currentCtx = self;

            while (parts.next()) |part| {
                std.debug.print("Adding part: {s}\n", .{part});
                var subCtx = currentCtx.children.get(part);
                if (subCtx == null) {
                    subCtx = ctxt.init(self.alloc);
                    subCtx.?.parent = currentCtx;
                    std.debug.print("Creating new sub-context for part: {s}\n", .{part});
                    try currentCtx.children.put(part, subCtx.?);
                }
                currentCtx = &subCtx.?;
            }

            try currentCtx.add(name, value);
        }

        pub fn deinit(self: *ctxt, alloc: std.mem.Allocator) void {
            var it2 = self.children.iterator();
            while (it2.next()) |pair| {
                pair.value_ptr.*.deinit(alloc);
            }

            self.members.deinit();
            self.children.deinit();
        }

        pub fn get(self: ctxt, name: misc.String) ?T {
            return self.members.get(name) orelse if (self.parent) |p| p.get(name) else null;
        }

        pub fn add(self: *ctxt, name: misc.String, value: T) !void {
            std.debug.print("Adding member: {s} = {any}\n", .{name, value});
            return self.members.put(name, value);
        }
        
        pub fn isEmpty(self: ctxt) bool {
            return self.members.count() == 0;
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

    try ctx.addTree("a/b/c", "test", 0);

    try expect(false);
    // try expect(ctx.getTree("a/b/c", "test") != null);
    // try expectEqual(ctx.getTree("a/b/c", "test"), 0);
}
