//! Generate types for AST nodes

const std = @import("std");
const misc = @import("misc.zig");
const Node = @import("node.zig");
const Parser = @import("parser.zig");
const Type = @import("types.zig");
const Context = @import("context.zig").Context;
const Analyzer = @This();
/// Possible errors thrown by the analyzer only
pub const Error = error{
    /// This reference could not be found
    UndefinedReference,

    /// A ternary operator took a decision that is not a boolean
    NonBooleanDecision,

    /// Found type needs to have a compile time value
    RequiresValue,

    /// Two types could not be matched
    TypeMismatch,

    /// The compiler could not find any way to join two types together
    ImpossibleUnification,
} || Parser.Error || Type.Error;

// Bidirectional typing system?

// todo: verify that types match, or meet in case of casting for every
// operation

const TContext = Context(*Type);
allocator: std.mem.Allocator,
/// Use an arena for types to allow shallow copying
talloc: std.mem.Allocator,

context: TContext,
modules: TContext,
errctx: misc.ErrorContext = .{ .value = .NoContext },
castIndex: u32,

pub fn init(allocator: std.mem.Allocator, arena: std.mem.Allocator) !Analyzer {
    return Analyzer{
        .allocator = allocator,
        .talloc = arena,
        .context = TContext.init(allocator),
        .modules = TContext.init(allocator),
        .castIndex = 0,
    };
}

pub fn getContext(self: *Analyzer, path: misc.String) ?*TContext {
    return self.context.get(path);
}

pub fn getModules(self: *Analyzer, path: misc.String) ?*TContext {
    return self.modules.get(path);
}

pub fn deinit(self: *Analyzer) void {
    self.context.deinit(self.allocator);
    self.modules.deinit(self.allocator);
}

pub fn runAnalysis(self: *Analyzer, node: *Node) !*Node {
    return self.analyze(&self.context, node);
}

fn analyze(self: *Analyzer, currctx: *TContext, node: *Node) !*Node {
    switch (node.data) {
        .type => return node,
        .int => return self.analyzeInt(node),
        .dec => return self.analyzeDec(node),
        .str => return self.analyzeStr(node),
        .ref => return self.analyzeRef(currctx, node),
        .unr => return self.analyzeUnr(currctx, node),
        .bin => return self.analyzeBin(currctx, node),
        .call => return self.analyzeCall(currctx, node),
        .ter => return self.analyzeTernary(currctx, node),
        .expr => return self.analyzeExpr(currctx, node),
        .intr => return self.analyzeIntr(currctx, node),
        .range => return self.analyzeRange(currctx, node),
        .mod => return self.analyzeModule(currctx, node),
        else => return node,
    }
}

fn analyzeInt(self: *Analyzer, node: *Node) Error!*Node {
    node.ntype = try self.talloc.create(Type);
    node.ntype.?.* = Type.initInt(node.data.int, node.data.int, node.data.int);
    return node;
}

fn analyzeDec(self: *Analyzer, node: *Node) Error!*Node {
    node.ntype = try self.talloc.create(Type);
    node.ntype.?.* = Type.initFloat(node.data.dec, node.data.dec, node.data.dec);
    return node;
}

fn analyzeStr(self: *Analyzer, node: *Node) Error!*Node {
    var arr = try self.talloc.alloc(?Type, node.data.str.len);
    errdefer self.talloc.free(arr);

    const strType = try self.talloc.create(Type);

    strType.* = Type.initInt(0, 255, null);

    for (node.data.str, 0..) |c, i| {
        arr[i] = Type.initInt(c, c, c);
    }

    var arrtype = try self.talloc.create(Type);
    errdefer arrtype.deinit(self.talloc);

    arrtype.* = .{
        .pointerIndex = 0,
        .data = .{
            .array = .{
                .size = @intCast(node.data.str.len),
                .indexer = strType,
                .value = arr,
            },
        },
    };

    node.ntype = arrtype;
    return node;
}

fn analyzeRef(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const refType = currctx.get(node.data.ref);
    if (refType) |refT| {
        node.ntype = try refT.deepCopy(self.talloc);
        return node;
    }
    return error.UndefinedReference;
}

fn analyzeUnr(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    node.ntype = try self.talloc.create(Type);

    const rest = try analyze(self, currctx, node.data.unr.val);

    const unt = try self.talloc.create(Type);

    unt.* = Type.synthUnary(node.data.unr.op, rest.ntype.?.*) catch |err| {
        self.errctx.position = node.position;
        return err;
    };
    node.ntype = unt;
    return node;
}

fn analyzeBin(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const lhs = try analyze(self, currctx, node.data.bin.lhs);
    const rhs = try analyze(self, currctx, node.data.bin.rhs);

    node.ntype = try self.talloc.create(Type);

    node.ntype.?.* = Type.binOperate(self.talloc, node.data.bin.op, lhs.ntype.?.*, rhs.ntype.?.*) catch |err| {
        self.errctx.position = node.position;
        return err;
    };

    return node;
}

fn analyzeCall(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const caller = try analyze(self, currctx, node.data.call.caller);
    const callee = try analyze(self, currctx, node.data.call.callee);

    node.ntype = try self.talloc.create(Type);
    node.ntype = try Type.callFunction(caller.ntype.?.*, callee.ntype.?.*);
    return node;
}

fn analyzeTernary(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const cond = try analyze(self, currctx, node.data.ter.cond);
    if (cond.ntype == null or cond.ntype.?.data != .boolean) {
        self.errctx = .{
            .position = cond.position,
            .value = .{
                .NonBooleanDecision = .{ .condtype = cond.ntype.?.* },
            },
        };
        return error.NonBooleanDecision;
    }

    const btrue = try analyze(self, currctx, node.data.ter.btrue);
    const bfalse = try analyze(self, currctx, node.data.ter.bfalse);

    if (btrue.ntype.?.deepEqual(bfalse.ntype.?.*, false)) {
        return error.TypeMismatch;
    }

    node.ntype = btrue.ntype;
    return node;
}

fn analyzeIntr(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const intr = node.data.intr;
    var innerctx = TContext.init(self.allocator);
    defer innerctx.deinit(self.allocator);

    innerctx.parent = currctx;

    for (intr.intermediates) |inter| {
        inter.* = (try analyze(self, &innerctx, inter)).*;
    }

    intr.application.?.* = (try analyze(self, &innerctx, intr.application.?)).*;
    return node;
}

fn analyzeRange(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const range = node.data.range;
    const start = try analyze(self, currctx, range.start);
    const end = try analyze(self, currctx, range.end);
    const epsilon = try analyze(self, currctx, range.epsilon);

    if (!start.ntype.?.isValued())
        return error.RequiresValue;

    if (!end.ntype.?.isValued())
        return error.RequiresValue;

    if (!epsilon.ntype.?.isValued())
        return error.RequiresValue;

    if (start.ntype.?.data == .integer and end.ntype.?.data == .integer and epsilon.ntype.?.data == .integer) {
        const istart = start.ntype.?.data.integer.value;
        const iend = end.ntype.?.data.integer.value;
        //const ivalue = epsilon.ntype.?.data.integer.value;

        const trange = try self.talloc.create(Type);
        errdefer trange.deinit(self.talloc);

        trange.* = .{ .pointerIndex = 0, .data = .{ .integer = .{
            .start = istart.?,
            .end = iend.?,
            .value = null,
        } } };

        node.ntype = trange;
        return node;
    }

    if (start.ntype.?.data == .decimal and end.ntype.?.data == .decimal and epsilon.ntype.?.data == .decimal) {
        const dstart = start.ntype.?.data.decimal.value;
        const dend = end.ntype.?.data.decimal.value;

        const trange = try self.talloc.create(Type);
        errdefer trange.deinit(self.talloc);

        trange.* = .{ .pointerIndex = 0, .data = .{ .decimal = .{
            .start = dstart.?,
            .end = dend.?,
            .value = null,
        } } };

        node.ntype = trange;
        return node;
    }

    return error.TypeMismatch;
}

fn analyzeModule(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    // modules are always top level, we don't need to return useful nodes
    const mod = node.data.mod;

    if (mod.path[0] == '@') {
        // todo: compile interface modules
        return node;
    }

    if (mod.direction) {
        for (mod.nodes) |mnode| {
            const name = mnode.data.expr.name;
            const pmod = self.modules.getTree(mod.path, name);
            if (pmod) |p| {
                try currctx.add(name, p);
            } else return error.UndefinedReference;
        }

        return node;
    }
    // todo: check from build if file tree matches the module path
    for (mod.nodes) |mnode| {
        const pname = mnode.data.expr.name;
        const modnode = try analyze(self, currctx, mnode);
        const res = try modnode.ntype.?.deepCopy(self.allocator);
        try self.modules.addTree(mod.path, pname, res);
    }
    return node;
}

fn analyzeExpr(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    var expr = node.data.expr;
    var innerctx = TContext.init(self.allocator);
    defer innerctx.deinit(self.allocator);

    innerctx.parent = currctx;

    var freturn = try self.talloc.create(Type);
    freturn.* = .{
        .pointerIndex = 0,
        .data = .{
            .casting = .{ .index = @intCast(self.castIndex + expr.params.len) },
        },
    };

    // generate function type
    for (0..expr.params.len) |i| {
        const idx = expr.params.len - i - 1;
        const arg = expr.params[idx];
        const argT = try self.talloc.create(Type);
        argT.* = .{
            .pointerIndex = 0,
            .data = .{
                .casting = .{ .index = @intCast(idx) },
            },
            .paramIdx = @intCast(idx + 1),
        };

        arg.ntype = try argT.deepCopy(self.talloc);

        try innerctx.add(arg.data.ref, arg.ntype.?);

        const ft = try self.talloc.create(Type);
        errdefer ft.deinit(self.talloc);

        ft.* = .{
            .pointerIndex = 0,
            .data = .{
                .function = .{
                    .argument = argT,
                    .ret = freturn,
                    .body = null,
                },
            },
        };

        freturn = ft;
    }

    node.ntype = try freturn.deepCopy(self.talloc);

    self.castIndex += @intCast(expr.params.len + 1);

    if (expr.ntype) |res| {
        _ = try analyze(self, &innerctx, expr.ntype.?);
        errdefer res.deinit(self.talloc);
        node.ntype = try res.ntype.?.deepCopy(self.talloc);

        var paramVal = res.ntype.?;
        for (expr.params, 0..) |n, i| {
            std.debug.print("at param {}\n", .{i});
            if (!paramVal.isFunction()) break;
            n.ntype = try paramVal.data.function.argument.deepCopy(self.talloc);
            errdefer n.ntype.?.deinit(self.talloc);
            n.ntype.?.paramIdx = @intCast(i + 1);
        }
    }

    for (expr.params, 0..) |param, pidx|
        if (param.data == .ref) {
            const pname = param.data.ref;
            var ptype: *Type = undefined;
            if (node.ntype) |nt| {
                ptype = nt.getParameter(@intCast(pidx)) catch |err| {
                    self.errctx = .{
                        .position = param.position,
                        .value = .{
                            .UnknownParameter = .{
                                .func = expr.name,
                                .name = pname,
                            },
                        },
                    };
                    return err;
                };
            } else {
                ptype = try self.talloc.create(Type);
                ptype.* = Type.initIdxCasting(self.castIndex);
                self.castIndex += 1;
            }
            try innerctx.add(pname, ptype);
        };

    if (expr.expr != null) {
        _ = try analyze(self, &innerctx, expr.expr.?);
    }

    if (expr.ntype != null and expr.expr != null) {
        if (expr.ntype.?.ntype.?.deepEqual(expr.expr.?.ntype.?.*, false))
            return error.TypeMismatch;
    }

    try currctx.add(expr.name, freturn);

    return node;
}
