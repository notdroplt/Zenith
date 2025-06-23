//! Generate types for AST nodes

const std = @import("std");
const misc = @import("misc.zig");
const Lexer = @import("lexer.zig");
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

/// misc allocator
allocator: std.mem.Allocator,

/// Use an arena for types to allow shallow copying
talloc: std.mem.Allocator,

/// Context carrying globals and locals
context: TContext = .{},

/// Modules loaded
modules: TContext = .{},

/// Error context
errctx: misc.ErrorContext = .{ .value = .NoContext },

/// Index of casting types
castIndex: u32 = 0,

/// cast substitutions
substitutions: std.HashMapUnmanaged(Type, *Type, Type.TypeContext, std.hash_map.default_max_load_percentage) = .{},



/// Initialize an analyzer
pub fn init(allocator: std.mem.Allocator, arena: std.mem.Allocator) !Analyzer {
    return .{
        .allocator = allocator,
        .talloc = arena,
    };
}

/// Reset function specific state on an allocator
fn reset(self: *Analyzer) void {
    self.castIndex = 0;
    var it = self.substitutions.keyIterator();
    while (it.next()) |key| {
        _ = self.substitutions.remove(key.*);
    }
}

/// Get a context given a path
pub fn getContext(self: *Analyzer, path: misc.String) ?*TContext {
    return self.context.get(path);
}

/// Get modules inside the Analyzer
pub fn getModules(self: *Analyzer, path: misc.String) ?*TContext {
    return self.modules.get(path);
}

// Deinit the Analyzer
pub fn deinit(self: *Analyzer) void {
    self.context.deinit(self.allocator);
    self.modules.deinit(self.allocator);
}

const debug = @import("debug.zig");

/// Analyze nodes
pub fn runAnalysis(self: *Analyzer, nodes: []*Node) !void {
    for (nodes) |node| {
        _ = try self.analyze(&self.context, node);
        try debug.printType(std.io.getStdOut().writer(), node.ntype.?.*);
        self.reset();
    }
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
    const int = node.data.int;
    node.ntype = try self.talloc.create(Type);
    node.ntype.?.* = Type.initInt(int, int, int);
    return node;
}

fn analyzeDec(self: *Analyzer, node: *Node) Error!*Node {
    const dec = node.data.dec;
    node.ntype = try self.talloc.create(Type);
    node.ntype.?.* = Type.initFloat(dec, dec, dec);
    return node;
}

fn analyzeStr(self: *Analyzer, node: *Node) Error!*Node {
    const arr = try self.talloc.alloc(?Type, node.data.str.len);
    errdefer self.talloc.free(arr);

    const strType = try self.talloc.create(Type);

    strType.* = Type.initInt(0, 255, null);

    for (node.data.str, arr) |c, *i| {
        i.* = Type.initInt(c, c, c);
    }

    const arrtype = try self.talloc.create(Type);

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
    self.errctx = .{
        .position = node.position,
        .value = .{
            .UnknownReference = .{ .name = node.data.ref },
        },
    };
    return error.UndefinedReference;
}

fn undefinedOperation(self: *Analyzer, tok: Lexer.Tokens, lhs: ?*Type, rhs: *Type, pos: misc.Pos) !void {
    self.errctx = .{
        .position = pos, 
        .value = .{ 
            .UndefinedOperation = .{ 
                .lhs = lhs,
                .token = tok,
                .rhs = rhs,
            },
        },
    };
    return error.UndefinedOperation;
}

fn minMax(comptime T: type, x1: T, x2: T, y1: T, y2: T, f: fn(comptime type, T, T) T) struct { T, T } {
    if (T == bool) {
        return .{ false, true };
    } else {
        const d1 = f(T, x1, y1);
        const d2 = f(T, x1, y2);
        const d3 = f(T, x2, y1);
        const d4 = f(T, x2, y2);

        return .{ @min(d1, d2, d3, d4), @max(d1, d2, d3, d4) };
    }
}

fn analyzeUnr(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const rest = try analyze(self, currctx, node.data.unr.val);

    const op = node.data.unr.op;
    const val = rest.ntype.?;

    const unt = try rest.ntype.?.deepCopy(self.talloc);

    // TODO: add operations to arrays :skull:
    switch (op) {
        .Amp => unt.*.pointerIndex += 1,
        .Plus => {},
        .Star => {
            if (val.data == .casting and val.pointerIndex == 0) {
                self.castIndex += 1;
                unt.* = Type.initIdxCasting(self.castIndex);
                unt.*.pointerIndex += 1; // become pointer
                try self.substitutions.put(self.allocator, val.*, unt);
            } else if (val.pointerIndex == 0)
                try self.undefinedOperation(.Star, null, val, node.position);
            
            unt.*.pointerIndex -= 1;
        },
        .Bang => {
            if (val.data == .casting) {
                unt.* = Type.initBool(null);
                try self.substitutions.put(self.allocator, val.*, unt);
            } else if (val.data != .boolean) {
                try self.undefinedOperation(.Bang, null, val, node.position);
            } else {
                if (val.data.boolean) |vbool|
                    unt.* = Type.initBool(!vbool);
                unt.* = Type.initBool(null);
            }
        },
        .Til => {
            if (val.data == .casting) {
                unt.* = Type.initInt(std.math.minInt(i64), std.math.maxInt(i64), null);
                try self.substitutions.put(self.allocator, val.*, unt);
            } else if (val.data != .integer) {
                try self.undefinedOperation(.Til, null, val, node.position);
            } else {
                const int = val.data.integer;
                const min = @min(~int.start, ~int.end);
                const max = @max(~int.start, ~int.end);
                const value: ?i64 = if (int.value) |i| ~i else null;
                unt.* = Type.initInt(min, max, value);
            }
        },
        .Min => {
            if (val.data == .casting) {
                // TODO: should we do nothing and hope for the best??
            } else if (val.data == .integer) {
                const int = val.data.integer;
                const min = @min(-int.start, -int.end);
                const max = @max(-int.start, -int.end);
                const value: ?i64 = if (int.value) |i| -i else null;
                unt.* = Type.initInt(min, max, value);
            } else if (val.data == .decimal) {
                const flt = val.data.decimal;
                const min = @min(-flt.start, -flt.end);
                const max = @max(-flt.start, -flt.end);
                const value: ?f64 = if (flt.value) |i| -i else null;
                unt.* = Type.initFloat(min, max, value);
            } else {
                try self.undefinedOperation(.Min, null, val, node.position);
            }
        },
        else => try self.undefinedOperation(op, null, val, node.position)
    }

    if (val.data == .casting) instantiate(rest, val, unt);

    node.ntype = unt;
    return node;
}


fn analyzeBin(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const lhs = try analyze(self, currctx, node.data.bin.lhs);
    const rhs = try analyze(self, currctx, node.data.bin.rhs);
    const op = node.data.bin.op;

    const tlhs = lhs.ntype.?;
    const trhs = lhs.ntype.?;

    var bint = try self.talloc.create(Type);

    if (op != .Hash and op != .Arrow and op != .Dot) {
        if (trhs.data == .casting) {
            // worst case (happens the most often) is both trhs and tlhs
            // being cast types
            try self.substitutions.put(self.allocator, trhs.*, tlhs);
            instantiate(rhs, trhs, tlhs);

        } else if (tlhs.data == .casting) {
            try self.substitutions.put(self.allocator, tlhs.*, trhs);
            instantiate(lhs, tlhs, trhs);
        }
    }

    // substitution mapping MIGHT be unused but who knows without testing
    // 

    if (op == .Hash) {
        if (tlhs.data != .array and trhs.data != .integer) {
            try self.undefinedOperation(.Hash, tlhs, trhs, node.position);
        }

        if(tlhs.isValued() and trhs.isValued()) {
            bint.* = tlhs.data.array.value[@intCast(trhs.data.integer.value.?)].?;
        } else {
            bint = try tlhs.data.array.indexer.deepCopy(self.allocator);
        }

        node.ntype.? = bint;
        return node;
    }

    if (op == .Arrow) {
        node.ntype.?.* = .{
            .data = .{
                .function = .{
                    .argument = tlhs,
                    .ret = trhs
                }
            }
        };
        return node;
    }

    if (op == .Dot) {
        //TODO: add aggregate deconstruction
        try self.undefinedOperation(op, tlhs, trhs, node.position);
    }

    if (std.meta.activeTag(tlhs.data) != std.meta.activeTag(trhs.data)) {
        try self.undefinedOperation(.Plus, tlhs, trhs, node.position);
    }

    switch (tlhs.data) {
        .casting => {},
        .integer => |ia| {
            const ib = tlhs.data.integer;
            switch (op) {
                .Plus => bint.* = intMinMax(misc.add, ia, ib),
                .Min => bint.* = intMinMax(misc.sub, ia, ib),
                .Star => bint.* = intMinMax(misc.mul, ia, ib),
                .Bar => {
                    if (ib.value != null and ib.value.? == 0){
                        self.errctx = .{
                            .position =  node.position,
                            .value = .DivisionByZero
                        };
                        return error.DivisionByZero;
                    }
                    // todo: add "possible division by zero"
                    const mm = minMax(i64, ia.start, ia.end, ib.start, ib.end, misc.div);
                    bint.* = Type.initInt(mm[0], mm[1], binopt(i64, ia.value, ib.value, misc.div));
                },
                .Per => {
                    if (ib.value != null and ib.value.? == 0) {
                        self.errctx = .{
                            .position =  node.position,
                            .value = .DivisionByZero
                        };   
                        return error.DivisionByZero;
                    }
                    // todo: add "possible division by zero"
                    bint.* = Type.initInt(0, ia.end - 1, binopt(i64, ia.value, ib.value, misc.mod));
                },
                .Pipe => bint.* = intMinMax(misc.pipe, ia, ib),
                .Amp => bint.* = intMinMax(misc.amp, ia, ib),
                .Hat => bint.* = intMinMax(misc.hat, ia, ib),
                .Lsh => bint.* = intMinMax(misc.lsh, ia, ib),
                .Rsh => bint.* = intMinMax(misc.rsh, ia, ib),
                .Cequ => bint.* = doBoolMinMax(i64, misc.equ, ia, ib),
                .Cneq => bint.* = doBoolMinMax(i64, misc.neq, ia, ib),
                .Cge => bint.* = doBoolMinMax(i64, misc.gte, ia, ib),
                .Cgt => bint.* = doBoolMinMax(i64, misc.gt, ia, ib),
                .Cle => bint.* = doBoolMinMax(i64, misc.lte, ia, ib),
                .Clt => bint.* = doBoolMinMax(i64, misc.lt, ia, ib),
                else => try self.undefinedOperation(op, tlhs, trhs, node.position),
            }
        },
        .decimal => |fa| {
            const fb = tlhs.data.decimal;
            switch (op) {
                .Plus => bint.* = fltMinMax(misc.add, fa, fb),
                .Min => bint.* = fltMinMax(misc.sub, fa, fb),
                .Star => bint.* = fltMinMax(misc.mul, fa, fb),
                .Bar => {
                    if (fb.value != null and fb.value.? == 0){
                        self.errctx = .{
                            .position =  node.position,
                            .value = .DivisionByZero
                        };
                        return error.DivisionByZero;
                    }
                    // todo: add "possible division by zero"
                    const mm = minMax(f64, fa.start, fa.end, fb.start, fb.end, misc.div);
                    bint.* = Type.initFloat(mm[0], mm[1], binopt(f64, fa.value, fb.value, misc.div));
                },
                .Cequ => bint.* = doBoolMinMax(f64, misc.equ, fa, fb),
                .Cneq => bint.* = doBoolMinMax(f64, misc.neq, fa, fb),
                .Cge => bint.* = doBoolMinMax(f64, misc.gte, fa, fb),
                .Cgt => bint.* = doBoolMinMax(f64, misc.gt, fa, fb),
                .Cle => bint.* = doBoolMinMax(f64, misc.lte, fa, fb),
                .Clt => bint.* = doBoolMinMax(f64, misc.lt, fa, fb),
                else => try self.undefinedOperation(op, tlhs, trhs, node.position),
            }
        },
        else => try self.undefinedOperation(op, tlhs, trhs, node.position)
    }

    node.ntype = bint;
    return node;
}

fn analyzeCall(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    const caller = try analyze(self, currctx, node.data.call.caller);
    const callee = try analyze(self, currctx, node.data.call.callee);

    if (caller.ntype.?.data != .function) {
        self.errctx = .{
            .position = node.position,
            .value = .InvalidCall,
        };
        return error.TypeMismatch;
    }

    const argtype = caller.ntype.?.data.function.argument;
 
    if (argtype.data == .casting) {
        instantiate(caller, argtype, callee.ntype.?);
    } else if (!argtype.deepEqual(callee.ntype.?.*, false)) {
        self.errctx = .{
            .position = node.position,
            .value = .InvalidCall,
        };
        return error.TypeMismatch;
    }

    node.ntype = try caller.ntype.?.data.function.argument.deepCopy(self.allocator);
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
    var innerctx = TContext{};
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

        trange.* = .{
            .pointerIndex = 0,
            .data = .{
                .integer = .{
                    .start = istart.?,
                    .end = iend.?,
                    .value = null,
                },
            },
        };

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
                try currctx.add(self.allocator, name, p);
            } else return error.UndefinedReference;
        }

        return node;
    }
    // todo: check from build if file tree matches the module path
    for (mod.nodes) |mnode| {
        const pname = mnode.data.expr.name;
        const modnode = try analyze(self, currctx, mnode);
        const res = try modnode.ntype.?.deepCopy(self.allocator);
        try self.modules.addTree(self.allocator, mod.path, pname, res);
    }
    return node;
}

fn analyzeExpr(self: *Analyzer, currctx: *TContext, node: *Node) Error!*Node {
    var expr = node.data.expr;
    var innerctx = TContext{};
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

        try innerctx.add(self.allocator, arg.data.ref, arg.ntype.?);

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
            if (paramVal.data != .function) break;
            expr.params[i].ntype = try paramVal.data.function.argument.deepCopy(self.talloc);
            errdefer n.ntype.?.deinit(self.talloc);
            expr.params[i].ntype.?.paramIdx = @intCast(i + 1);
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
                ptype.paramIdx = @intCast(pidx + 1);
            } else {
                ptype = try self.talloc.create(Type);
                ptype.* = Type.initIdxCasting(self.castIndex);
                ptype.paramIdx = @intCast(pidx + 1);
                self.castIndex += 1;
            }
            try innerctx.add(self.allocator, pname, ptype);
        };

    if (expr.expr) |res|
        _ = try analyze(self, &innerctx, res);

    if (expr.ntype != null and expr.expr != null) {
        if (expr.ntype.?.ntype.?.deepEqual(expr.expr.?.ntype.?.*, false))
            return error.TypeMismatch;
    }

    var it = self.substitutions.iterator();
    while(it.next()) |val| {
        instantiate(node, val.key_ptr, val.value_ptr.*);
    }


    try currctx.add(self.allocator, expr.name, freturn);

    return node;
}

fn binopt(comptime T: type, a: ?T, b: ?T, func: anytype) ?@TypeOf(func(T, a.?, b.?)) {
    if (a == null or b == null) return null;
    return func(T, a.?, b.?);
}

fn intMinMax(func: anytype, ma: anytype, mb: anytype) Type {
    const mm = minMax(i64, ma.start, ma.end, mb.start, mb.end, func);
    const val = binopt(i64, ma.value, mb.value, func);
    return Type.initInt(mm[0], mm[1], val);
}

fn fltMinMax(func: anytype, ma: anytype, mb: anytype) Type {
    const mm = minMax(f64, ma.start, ma.end, mb.start, mb.end, func);
    const val = binopt(f64, ma.value, mb.value, func);
    return Type.initFloat(mm[0], mm[1], val);
}

fn doBoolMinMax(comptime T: type, func: anytype, ma: anytype, mb: anytype) Type {
    return Type.initBool(binopt(T, ma.value, mb.value, func));
}

/// given a node tree, instantiate "find" with "replace"
fn instantiate(node: *Node, find: *Type, replace: *Type) void {
    if (node.ntype) |nt| {
        if (nt.deepEqual(find.*, true)) {
            node.ntype.? = replace;
        }
    }

    switch (node.data) {
        .int, .dec, .str, .ref, .mod, .type => {},
        .unr => |v| {
            instantiate(v.val, find, replace);
        },
        .bin => |v| {
            instantiate(v.lhs, find, replace);
            instantiate(v.rhs, find, replace);
        },
        .ter => |v| {
            instantiate(v.cond, find, replace);
            instantiate(v.btrue, find, replace);
            instantiate(v.bfalse, find, replace);
        },
        .call => |v| {
            instantiate(v.caller, find, replace);
            instantiate(v.callee, find, replace);
        },
        .intr => |v| {
            for (v.intermediates) |i| 
                instantiate(i, find, replace);

            if (v.application) |app|
                instantiate(app, find, replace);
        },
        .sum => |v| {
            for (v) |s|
                instantiate(s, find, replace);
        },
        .aggr => |v| {
            for (v.children) |c|
                instantiate(c, find, replace);
        },
        .range => |v| {
            instantiate(v.start, find, replace);
            instantiate(v.end, find, replace);
            instantiate(v.epsilon, find, replace);
        },
        .expr => |v| {
            for (v.params) |p|
                instantiate(p, find, replace);

            if (v.ntype) |nt|
                instantiate(nt, find, replace);
            
            if (v.expr) |expr|
                instantiate(expr, find, replace);
        },
    }
}

// get a function parameter given index
