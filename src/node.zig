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
        bfalse: *Node
    },

    /// Function call
    call: struct { 
        /// Caller node
        caller: *Node, 
        
        /// Callee node
        callee: *Node
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
        direction: bool
    },

    /// Range definition
    range: struct { 
        /// Start of the range
        start: *Node, 

        /// End of the range
        end: *Node, epsilon: *Node
    },

    /// Aggregate definition
    aggr: struct { 
        /// Name of the aggregate
        name: misc.String, 
        
        /// Child nodes
        children: []*Node
    },

    /// Sum definition
    sum: []*Node,

    /// Intermediate representation
    intr: struct { 
        /// Intermediate nodes
        intermediates: []*Node, 
        
        /// Application node
        application: ?*Node
    },
},

/// Check if two nodes are equal.
pub fn eql(self: Node, other: Node) bool {
    if (std.meta.activeTag(self) != std.meta.activeTag(other))
        return false;

    const od = other.data;

    switch (self.data) {
        .int => |v| return v == od.int,
        .dec => |v| return v == od.dec,
        .str => |v| return std.mem.eql(u8, v, od.str),
        .ref => |v| return std.mem.eql(u8, v, od.ref),
        .unr => |v| return v.op == od.unr.op and v.val.eql(od.unr.val),
        .bin => |v| return v.op == od.bin.op and v.lhs.eql(od.bin.lhs) and v.rhs.eql(od.bin.rhs),
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
            if (!std.mem.eql(v.name, od.aggr.name) or v.children.len != od.aggr.children.len)
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
                alloc.destroy(node);
            }
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
                alloc.destroy(inter);
            }
            alloc.free(v.intermediates);
            if (v.application) |app|
                app.deinit(alloc);
        },
    }

    alloc.destroy(self);
}

