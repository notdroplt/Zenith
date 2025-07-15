const std = @import("std");
const misc = @import("misc.zig");
const Type = @import("types.zig");

const Context = @This();

parent: ?*Context = null,
members: std.StringHashMapUnmanaged(std.ArrayListUnmanaged(*Type)) = .{},
children: std.StringHashMapUnmanaged(Context) = .{},

/// Add a value in the current context
pub fn add(self: *Context, alloc: std.mem.Allocator, name: misc.String, value: *Type) !void {
    if (self.members.getPtr(name)) |vals| {
        for (vals.items) |v| {
            if (v.deepEqual(value.*, true)) return;
        }
        try vals.append(alloc, value);
        return;
    }
    try self.members.put(alloc, name, .{});
    try self.members.getPtr(name).?.append(alloc, value);

}

/// Add a value at a path, creating child contexts as needed.
pub fn addTree(self: *Context, allocator: std.mem.Allocator, path: misc.String, name: misc.String, value: *Type) !void {
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
    try ctx.add(allocator, name, value);
}

/// Extends a context with multiple types
pub fn extend(self: *Context, alloc: std.mem.Allocator, name: misc.String, value: []*Type) !void {
    for (value) |v| try self.add(alloc, name, v);
}

/// Get a value by path and name, searching up the parent chain
/// if not found locally.
pub fn getTree(self: *Context, path: misc.String, name: misc.String) ?[]*Type {
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
pub fn get(self: *Context, name: misc.String) ?[]*Type {
    var p: ?*Context = self;

    while (p) |ctx| : (p = ctx.parent)
        if (ctx.members.get(name)) |val|
            return val.items;

    return null;
}

pub fn deinit(self: *Context, allocator: std.mem.Allocator) void {
    var it = self.children.valueIterator();
    var mit = self.members.valueIterator();
    while (it.next()) |v| 
        v.deinit(allocator);
    
    while (mit.next()) |v| 
        v.deinit(allocator);
    
    self.members.deinit(allocator);
    self.children.deinit(allocator);
}

pub fn isEmpty(self: *Context) bool {
    return self.members.count() == 0 and self.children.count() == 0;
}

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;

test Context {
    const alloc = std.testing.allocator;
    var ctx = Context{};
    defer ctx.deinit(alloc);

    var val = Type.initBool(null);

    try expect(ctx.isEmpty());
    try ctx.add(alloc, "test", &val);
    try expect(!ctx.isEmpty());

    try expect(ctx.get("test") != null);
    try expectEqual(1, ctx.get("test").?.len);
    try expectEqual(&val, ctx.get("test").?[0]);

    try ctx.addTree(alloc, "a/b/c", "test", &val);
    try expect(ctx.getTree("a/b/c", "test") != null);
    try expectEqual(1, ctx.getTree("a/b/c", "test").?.len);
    try expectEqual(&val, ctx.getTree("a/b/c", "test").?[0]);
}
