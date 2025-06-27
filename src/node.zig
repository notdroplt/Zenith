//! Define a node in the abstract syntax tree.

const std = @import("std");
const misc = @import("misc.zig");
const Type = @import("types.zig");
const Lexer = @import("lexer.zig");
const Node = @This();

/// Current node source position
position: misc.Pos = .{},

/// Node Type
ntype: ?*Type = null,

/// Node Data
data: union(enum) {
    /// Node only has a type
    type: void,

    /// Integer literal
    int: i64,

    /// Decimal literal
    dec: f64,

    /// String literal
    str: misc.String,

    /// Reference to a variable or function
    ref: misc.String,

    /// Array of elements
    arr: []*Node,

    /// Unary operation
    unr: struct {
        /// Node operand
        val: *Node,

        /// Unary operator
        op: Lexer.Tokens,
    },

    /// Binary operation
    bin: struct {
        /// Left-hand side node
        lhs: *Node,

        /// Right-hand side node
        rhs: *Node,

        /// Binary operator
        op: Lexer.Tokens,
    },

    /// Ternary operation
    ter: struct {
        /// Condition node
        cond: *Node,

        /// True branch node
        btrue: *Node,

        /// False branch node
        bfalse: *Node,
    },

    /// Function call
    call: struct {
        /// Caller node
        caller: *Node,

        /// Callee node
        callee: *Node,
    },

    /// Expression, either ntype or expr node need to not be null
    expr: struct {
        /// Name of the expression
        name: misc.String,

        /// Parameters (max 8)
        params: []*Node,

        /// Optional type node
        ntype: ?*Node,

        /// Optional expression node
        expr: ?*Node,
    },

    /// Module definition
    mod: struct {
        /// Module path
        path: misc.String,

        /// Nodes interface
        nodes: []*Node,

        /// Import direction
        direction: bool,
    },

    /// Range definition
    range: struct {
        /// Start of the range
        start: *Node,

        /// End of the range
        end: *Node,
        epsilon: *Node,
    },

    /// Aggregate definition
    aggr: struct {
        /// Name of the aggregate
        name: misc.String,

        /// Child nodes
        children: []*Node,
    },

    /// Sum definition
    sum: []*Node,

    /// Intermediate representation
    intr: struct {
        /// Intermediate nodes
        intermediates: []*Node,

        /// Application node
        application: ?*Node,
    },
},

/// Check if two nodes are equal.
pub fn eql(self: Node, other: *const Node) bool {
    if (std.meta.activeTag(self.data) != std.meta.activeTag(other.data))
        return false;

    const od = other.data;

    switch (self.data) {
        .type => return true,
        .int => |v| return v == od.int,
        .dec => |v| return v == od.dec,
        .str => |v| return std.mem.eql(u8, v, od.str),
        .ref => |v| return std.mem.eql(u8, v, od.ref),
        .arr => |v| {
            if (v.len != od.arr.len) return false;
            for (v, od.arr) |v1, v2|
                if (!v1.eql(v2))
                    return false;
            return true;
        },
        .unr => |v| return std.meta.activeTag(v.op) == od.unr.op and v.val.eql(od.unr.val),
        .bin => |v| return std.meta.activeTag(v.op) == od.bin.op and v.lhs.eql(od.bin.lhs) and v.rhs.eql(od.bin.rhs),
        .ter => |v| return v.cond.eql(od.ter.cond) and v.btrue.eql(od.ter.btrue) and v.bfalse.eql(od.ter.bfalse),
        .call => |v| return v.caller.eql(od.call.caller) and v.callee.eql(od.call.callee),
        .expr => |v| {
            if (!std.mem.eql(u8, v.name, od.expr.name)) return false;

            if (v.params.len != od.expr.params.len) return false;

            for (v.params, od.expr.params) |p1, p2|
                if (!p1.eql(p2))
                    return false;

            if ((v.ntype == null) != (od.expr.ntype == null) or (v.expr == null) != (od.expr.expr == null))
                return false;

            return (if (v.ntype) |a| a.eql(od.expr.ntype.?) else true) and
                (if (v.expr) |a| a.eql(od.expr.expr.?) else true);
        },
        .mod => |v| {
            if (!std.mem.eql(u8, v.path, od.mod.path) or v.direction != od.mod.direction or v.nodes.len != od.mod.nodes.len)
                return false;
            for (v.nodes, od.mod.nodes) |n1, n2|
                if (!n1.eql(n2))
                    return false;
            return true;
        },
        .range => |v| return v.start.eql(od.range.start) and v.end.eql(od.range.end) and v.epsilon.eql(od.range.epsilon),
        .aggr => |v| {
            if (!std.mem.eql(u8,v.name, od.aggr.name) or v.children.len != od.aggr.children.len)
                return false;

            for (v.children, od.aggr.children) |c1, c2|
                if (!c1.eql(c2))
                    return false;

            return true;
        },
        .sum => |v| {
            if (v.len != od.sum.len)
                return false;

            for (v, od.sum) |v1, v2|
                if (!v1.eql(v2))
                    return false;

            return true;
        },
        .intr => |v| {
            if ((v.application == null) != (od.intr.application == null) or v.intermediates.len != od.intr.intermediates.len)
                return false;
            for (v.intermediates, od.intr.intermediates) |it1, it2|
                if (!it1.eql(it2))
                    return false;
            return true;
        },
    }
}

/// Deinit the node and free its resources.
pub fn deinit(self: *Node, alloc: std.mem.Allocator) void {
    switch (self.data) {
        .type, .int, .dec, .str, .ref => {},
        .arr => |v| {
            for (v) |i|
                i.deinit(alloc);
        },
        .unr => |v| v.val.deinit(alloc),
        .bin => |v| {
            v.lhs.deinit(alloc);
            v.rhs.deinit(alloc);
        },
        .ter => |v| {
            v.cond.deinit(alloc);
            v.btrue.deinit(alloc);
            v.bfalse.deinit(alloc);
        },
        .call => |v| {
            v.caller.deinit(alloc);
            v.callee.deinit(alloc);
        },
        .expr => |v| {
            for (v.params) |param| {
                param.deinit(alloc);
            }
            alloc.free(v.params);

            if (v.ntype) |nt| nt.deinit(alloc);

            if (v.expr) |expr| expr.deinit(alloc);
        },
        .mod => |v| {
            for (v.nodes) |node| {
                node.deinit(alloc);
            }
            alloc.free(v.nodes);
        },
        .range => |v| {
            v.start.deinit(alloc);
            v.end.deinit(alloc);
            v.epsilon.deinit(alloc);
        },
        .aggr => |v| {
            for (v.children) |child| child.deinit(alloc);

            alloc.free(v.children);
        },
        .sum => |v| {
            for (v) |child| child.deinit(alloc);
        },
        .intr => |v| {
            for (v.intermediates) |inter| {
                inter.deinit(alloc);
            }
            alloc.free(v.intermediates);
            if (v.application) |app|
                app.deinit(alloc);
        },
    }

    alloc.destroy(self);
}

/// Integer node factory
pub fn initInt(alloc: std.mem.Allocator, position: misc.Pos, value: i64) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .int = value },
    };
    return node;
}

/// Decimal node factory
pub fn initDec(alloc: std.mem.Allocator, position: misc.Pos, value: f64) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .dec = value },
    };
    return node;
}

/// String node factory
pub fn initStr(alloc: std.mem.Allocator, position: misc.Pos, value: misc.String) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .str = value },
    };
    return node;
}

pub fn initArr(alloc: std.mem.Allocator, position: misc.Pos, value: []*Node) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{.arr = value}
    };
    return node;
}

/// Reference node factory
pub fn initRef(alloc: std.mem.Allocator, position: misc.Pos, value: misc.String) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .ref = value },
    };
    return node;
}

/// Unary node factory
pub fn initUnr(alloc: std.mem.Allocator, position: misc.Pos, operator: Lexer.Tokens, value: *Node) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{
            .unr = .{
                .val = value,
                .op = operator,
            },
        },
    };
    return node;
}

/// Binary node factory
pub fn initBin(alloc: std.mem.Allocator, position: misc.Pos, operator: Lexer.Tokens, lhs: *Node, rhs: *Node) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{
            .bin = .{
                .lhs = lhs,
                .rhs = rhs,
                .op = operator,
            },
        },
    };
    return node;
}

pub fn initTer(alloc: std.mem.Allocator, position: misc.Pos, cond: *Node, btrue: *Node, bfalse: *Node) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{
            .ter = .{
                .cond = cond,
                .btrue = btrue,
                .bfalse = bfalse,
            },
        },
    };
    return node;
}

/// Call node factory
pub fn initCall(alloc: std.mem.Allocator, position: misc.Pos, caller: *Node, callee: *Node) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .call = .{ .caller = caller, .callee = callee } },
    };
    return node;
}

/// Expression node factory
pub fn initExpr(alloc: std.mem.Allocator, position: misc.Pos, name: misc.String, params: []*Node, exprType: ?*Node, expr: ?*Node) !*Node {
    const node = try alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .expr = .{ .name = name, .params = params, .ntype = exprType, .expr = expr } },
    };
    return node;
}

/// Calculate a weight for how many instructions would a node be in a tree, 
/// allowing for 
pub fn weight(self: *const Node) usize {
    return switch (self.data) {
        .type, .range, .aggr, .sum, .mod => 0,
        .int, .dec, .str, .ref => 1,
        .unr => |v| 1 + v.val.weight(),
        .bin => |v| 2 + v.lhs.weight() + v.rhs.weight(),
        .ter => |v| 3 + v.cond.weight() + v.btrue.weight() + v.bfalse.weight(),
        .call => |v| 4 + v.caller.weight() + v.callee.weight(),
        .expr => |v| blk: {
            var sum: usize = 1;
            for (v.params) |param| sum += param.weight();
            if (v.ntype) |nt| sum += nt.weight();
            if (v.expr) |ex| sum += ex.weight();
            break :blk sum;
        },
        .intr => |v| blk: {
            var sum: usize = 2;
            for (v.intermediates) |i| sum += i.weight();
            if (v.application) |app| sum += app.weight();
            break :blk sum;
        },
    };
}

pub fn isInline(self: *const Node) bool {
    return self.weight() < 69; //TODO: find a better weight
}

test Node {
    const alloc = std.testing.allocator;

    const pos = misc.Pos{ .index = 1, .span = 1 };

    const int1 = try Node.initInt(alloc, pos, 42);
    defer int1.deinit(alloc);

    const int2 = try Node.initInt(alloc, pos, 42);
    defer int2.deinit(alloc);

    const int3 = try Node.initInt(alloc, pos, 69);
    defer int3.deinit(alloc);

    try std.testing.expect(int1.eql(int2));
    try std.testing.expect(int2.eql(int1)); // should commutative
    try std.testing.expect(!int1.eql(int3));

}
