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

    /// The import statement given was not assigned from/to
    EmptyImport,

    /// Could not find imported symbol
    UndefinedExternSymbol,
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

/// Cast substitutions
substitutions: std.HashMapUnmanaged(Type, *Type, Type.TypeContext, std.hash_map.default_max_load_percentage) = .{},

/// Allow undefined references to be treated as castings
inDefinition: bool = false,

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
    self.substitutions.deinit(self.allocator);
}

/// Analyze nodes
pub fn runAnalysis(self: *Analyzer, nodes: []*Node) !void {
    for (nodes) |node| {
        try self.analyze(&self.context, node);
        self.reset();
    }
}

fn analyze(self: *Analyzer, currctx: *TContext, node: *Node) !void {
    try switch (node.data) {
        .type => {},
        .int => self.analyzeInt(node),
        .dec => self.analyzeDec(node),
        .str => self.analyzeStr(node),
        .arr => self.analyzeArr(currctx, node),
        .ref => self.analyzeRef(currctx, node),
        .unr => self.analyzeUnr(currctx, node),
        .bin => self.analyzeBin(currctx, node),
        .call => self.analyzeCall(currctx, node),
        .ter => self.analyzeTernary(currctx, node),
        .expr => self.analyzeExpr(currctx, node),
        .intr => self.analyzeIntr(currctx, node),
        .range => self.analyzeRange(currctx, node),
        .mod => self.analyzeModule(currctx, node),
        .sum => self.analyzeSum(currctx, node),
        .aggr => self.analyzeAggr(currctx, node),
        else => unreachable,
    };
}

fn analyzeInt(self: *Analyzer, node: *Node) Error!void {
    const int = node.data.int;
    node.ntype = try self.talloc.create(Type);
    node.ntype.?.* = Type.initInt(int, int, int);
}

fn analyzeDec(self: *Analyzer, node: *Node) Error!void {
    const dec = node.data.dec;
    node.ntype = try self.talloc.create(Type);
    node.ntype.?.* = Type.initFloat(dec, dec, dec);
}

fn analyzeStr(self: *Analyzer, node: *Node) Error!void {
    const arr = try self.talloc.alloc(?*Type, node.data.str.len);
    errdefer self.talloc.free(arr);

    const strType = try self.talloc.create(Type);

    strType.* = Type.initInt(0, 255, null);

    for (node.data.str, arr) |c, *i| {
        i.* = try self.talloc.create(Type);
        i.*.?.* = Type.initInt(c, c, c);
    }

    const arrtype = try self.talloc.create(Type);

    arrtype.* = .{
        .data = .{
            .array = .{
                .size = @intCast(node.data.str.len),
                .indexer = strType,
                .value = arr,
            },
        },
    };

    node.ntype = arrtype;
}

fn analyzeArr(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    const arr = try self.talloc.alloc(?*Type, node.data.arr.len);
    errdefer self.talloc.free(arr);

    self.castIndex += 1;
    var res = Type.initCasting(self.castIndex);

    for (node.data.arr, 0..) |v, i| {
        try self.analyze(currctx, v);
        arr[i] = v.ntype;

        // first can be literally anything
        if (i == 0) {
            res = v.ntype.?.*;
            continue;
        }

        const join = Type.join(res, v.ntype.?.*);
        if (join == .Impossible) {
            self.errctx = .{
                .position = v.position,
                .value = .{
                    .DisjointTypes = .{
                        .t1 = res,
                        .t2 = v.ntype.?.*,
                    },
                },
            };
            return error.DisjointTypes;
        }

        res = switch (join) {
            .New => |new| new,
            .Left => res,
            .Right => v.ntype.?.*,
            .Impossible => unreachable,
        };
    }

    const idxr = try self.talloc.create(Type);
    idxr.* = res;

    if (!res.isTemplated()) self.castIndex -= 1;

    const arrt = try self.talloc.create(Type);
    arrt.* = .{
        .data = .{
            .array = .{
                .indexer = idxr,
                .size = @intCast(node.data.arr.len),
                .value = arr,
            },
        },
    };

    node.ntype = arrt;
}

fn analyzeRef(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    const refType = currctx.get(node.data.ref);
    if (refType) |refT| {
        node.ntype = refT;
        return;
    }

    if (self.inDefinition) {
        node.ntype = try self.talloc.create(Type);
        self.castIndex += 1;
        node.ntype.?.* = Type.initCasting(self.castIndex);
        return;
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

fn minMax(comptime T: type, x1: T, x2: T, y1: T, y2: T, f: fn (comptime type, T, T) T) struct { T, T } {
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

fn analyzeUnr(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    try analyze(self, currctx, node.data.unr.val);
    const rest = node.data.unr.val;

    const op = node.data.unr.op;
    const val = rest.ntype.?;

    const unt = try self.talloc.create(Type);

    // TODO: add operations to arrays :skull:
    switch (op) {
        .Amp => unt.* = .{
            .data = .{ .pointer = val },
        },
        .Plus => {},
        .Star => {
            if (val.data == .casting) {
                self.castIndex += 1;
                unreachable;
                // FIXME: make pointer casting work
                // unt.* = Type.initCasting(self.castIndex);
                // unt.*.pointerIndex += 1; // become pointer
                // try self.substitutions.put(self.allocator, val.*, unt);
            } else if (val.data != .pointer)
                try self.undefinedOperation(.Star, null, val, node.position);

            node.ntype = val.data.pointer;
            return;
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
        else => try self.undefinedOperation(op, null, val, node.position),
    }

    if (val.data == .casting) instantiate(rest, val, unt);

    node.ntype = unt;
}

fn analyzeBin(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    try analyze(self, currctx, node.data.bin.lhs);
    try analyze(self, currctx, node.data.bin.rhs);
    const lhs = node.data.bin.lhs;
    const rhs = node.data.bin.rhs;
    const op = node.data.bin.op;

    const tlhs = lhs.ntype.?;
    const trhs = rhs.ntype.?;

    var bint = try self.talloc.create(Type);

    if (op != .Hash and op != .Arrow and op != .Dot) {
        if (trhs.data == .casting) {
            // worst case (happens the most often) is both trhs and tlhs
            // being cast types

            try self.substitutions.put(self.allocator, trhs.*, tlhs);
            instantiate(rhs, trhs, tlhs);
            instantiateContext(currctx, trhs, tlhs);
        } else if (tlhs.data == .casting) {
            try self.substitutions.put(self.allocator, tlhs.*, trhs);
            instantiate(lhs, tlhs, trhs);
            instantiateContext(currctx, tlhs, trhs);
        }
    }

    if (op == .Hash) {
        if (tlhs.data != .array and trhs.data != .integer) {
            try self.undefinedOperation(.Hash, tlhs, trhs, node.position);
        }

        if (tlhs.isValued() and trhs.isValued()) {
            node.ntype = tlhs.data.array.value[@intCast(trhs.data.integer.value.?)].?;
        } else {
            self.talloc.destroy(bint);
            bint = tlhs.data.array.indexer;
        }

        node.ntype.? = bint;
        return;
    }

    if (op == .Arrow) {
        node.ntype = try self.talloc.create(Type);
        node.ntype.?.* = .{
            .data = .{
                .function = .{ .argument = tlhs, .ret = trhs },
            },
        };
        return;
    }

    if (op == .Dot) {
        //TODO: add aggregate deconstruction
        try self.undefinedOperation(op, tlhs, trhs, node.position);
    }

    if (std.meta.activeTag(tlhs.data) != std.meta.activeTag(trhs.data)) {
        try self.undefinedOperation(.Plus, tlhs, trhs, node.position);
    }

    switch (tlhs.data) {
        .casting => {
            switch (op) {
                .Per, .Pipe, .Amp, .Hat, .Lsh, .Rsh => {
                    bint.* = Type.initInt(std.math.minInt(i64), std.math.maxInt(i64), null);
                    instantiate(lhs, tlhs, bint);
                    instantiate(rhs, tlhs, bint);
                    instantiateContext(currctx, tlhs, bint);
                },

                .Cequ, .Cneq, .Cge, .Cgt, .Cle, .Clt => {
                    bint.* = Type.initBool(null);
                },
                else => {
                    bint.* = tlhs.*;
                },
            }
        },
        .integer => |ia| {
            const ib = trhs.data.integer;
            switch (op) {
                .Plus => bint.* = intMinMax(misc.add, ia, ib),
                .Min => bint.* = intMinMax(misc.sub, ia, ib),
                .Star => bint.* = intMinMax(misc.mul, ia, ib),
                .Bar => {
                    if (ib.value != null and ib.value.? == 0) {
                        self.errctx = .{ .position = node.position, .value = .DivisionByZero };
                        return error.DivisionByZero;
                    }
                    // todo: add "possible division by zero"
                    const mm = minMax(i64, ia.start, ia.end, ib.start, ib.end, misc.div);
                    bint.* = Type.initInt(mm[0], mm[1], binopt(i64, ia.value, ib.value, misc.div));
                },
                .Per => {
                    if (ib.value != null and ib.value.? == 0) {
                        self.errctx = .{ .position = node.position, .value = .DivisionByZero };
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
                    if (fb.value != null and fb.value.? == 0) {
                        self.errctx = .{ .position = node.position, .value = .DivisionByZero };
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
        else => try self.undefinedOperation(op, tlhs, trhs, node.position),
    }

    node.ntype = bint;
}

fn analyzeCall(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    try analyze(self, currctx, node.data.call.caller);
    try analyze(self, currctx, node.data.call.callee);
    const caller = node.data.call.caller;
    const callee = node.data.call.callee;

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

    node.ntype = caller.ntype.?.data.function.ret;
}

fn analyzeTernary(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    try analyze(self, currctx, node.data.ter.cond);
    const cond = node.data.ter.cond.ntype;
    if (cond == null or cond.?.data != .boolean) {
        self.errctx = .{
            .position = node.data.ter.cond.position,
            .value = .{
                .NonBooleanDecision = .{
                    .condtype = cond.?.*,
                },
            },
        };
        return error.NonBooleanDecision;
    }

    try analyze(self, currctx, node.data.ter.btrue);
    try analyze(self, currctx, node.data.ter.bfalse);
    const btrue = node.data.ter.btrue.ntype.?;
    const bfalse = node.data.ter.bfalse.ntype.?;

    if (!btrue.deepEqual(bfalse.*, false)) {
        self.errctx = .{
            .position = node.position,
            .value = .{
                .DisjointTypes = .{
                    .t1 = btrue.*,
                    .t2 = bfalse.*,
                },
            },
        };
        return error.TypeMismatch;
    }

    node.ntype = btrue;
}

fn analyzeIntr(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    const intr = node.data.intr;
    var innerctx = TContext{};
    defer innerctx.deinit(self.allocator);

    innerctx.parent = currctx;

    for (intr.intermediates) |inter|
        try analyze(self, &innerctx, inter);

    try analyze(self, &innerctx, intr.application.?);
}

fn analyzeRange(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    const range = node.data.range;
    try analyze(self, currctx, range.start);
    try analyze(self, currctx, range.end);
    try analyze(self, currctx, range.epsilon);
    const rs = range.start.ntype.?;
    const re = range.end.ntype.?;
    const rp = range.epsilon.ntype.?;

    if (!rs.isValued())
        return error.RequiresValue;

    if (!re.isValued())
        return error.RequiresValue;

    if (!rp.isValued())
        return error.RequiresValue;

    if (rs.data == .integer and re.data == .integer and rp.data == .integer) {
        const istart = rs.data.integer.value;
        const iend = re.data.integer.value;
        //const ivalue = epsilon.ntype.?.data.integer.value;

        const trange = try self.talloc.create(Type);

        trange.* = .{
            .data = .{
                .integer = .{
                    .start = istart.?,
                    .end = iend.?,
                    .value = null,
                },
            },
        };

        node.ntype = trange;
        return;
    }

    if (rs.data == .decimal and re.data == .decimal and rp.data == .decimal) {
        const dstart = rs.data.decimal.value;
        const dend = re.data.decimal.value;

        const trange = try self.talloc.create(Type);

        trange.* = .{
            .data = .{
                .decimal = .{
                    .start = dstart.?,
                    .end = dend.?,
                    .value = null,
                },
            },
        };

        node.ntype = trange;
        return;
    }

    return error.TypeMismatch;
}

const debug = @import("debug.zig");

fn analyzeModule(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    // modules are always top level, we don't need to return useful nodes
    const mod = node.data.mod;

    if (mod.path[0] == '@') {
        const out = std.io.getStdErr().writer();
        if (std.mem.eql(u8, mod.path, "@echo")) {
            for (mod.nodes) |n| {
                if (n.data == .expr and std.mem.eql(u8, n.data.expr.name, "message")) {
                    if (n.data.expr.expr) |expr| {
                        debug.printNode(out, expr) catch {};
                        out.print("\n", .{}) catch {};
                    } else out.print("echoing a message\n", .{}) catch {};
                }
            }
        }
        return;
    }

    if (mod.direction) {
        for (mod.nodes) |mnode| {
            if (mnode.data.expr.expr == null) {
                self.errctx = .{
                    .position = mnode.position,
                    .value = .{ .EmptyImport = true },
                };
                return error.EmptyImport;
            }

            if (mnode.data.expr.expr.?.data != .ref) {
                self.errctx = .{
                    .position = mnode.position,
                    .value = .{ .EmptyImport = true },
                };
                return error.EmptyImport;
            }

            const name = mnode.data.expr.expr.?.data.ref;
            const pmod = self.modules.getTree(mod.path, name);
            if (pmod) |p| {
                try currctx.add(self.allocator, mnode.data.expr.name, p);
            } else {
                self.errctx = .{
                    .position = mnode.position,
                    .value = .{
                        .UndefinedExternSymbol = .{
                            .name = name,
                            .path = mod.path,
                        },
                    },
                };
                return error.UndefinedReference;
            }
        }
        return;
    }
    // todo: check from build if file tree matches the module path
    for (mod.nodes) |mnode| {
        const pname = mnode.data.expr.name;
        try analyze(self, currctx, mnode);
        const res = mnode.ntype.?;
        try self.modules.addTree(self.allocator, mod.path, pname, res);
    }
}

fn analyzeExpr(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    const expr = node.data.expr;
    var innerctx = TContext{};
    defer innerctx.deinit(self.allocator);
    innerctx.parent = currctx;

    var param_types = try self.talloc.alloc(*Type, expr.params.len);
    defer self.talloc.free(param_types);

    for (expr.params, 0..) |param, i| {
        const cast_idx = self.castIndex + @as(u32, @intCast(i));
        const param_type = try self.talloc.create(Type);
        param_type.* = Type.initCasting(cast_idx);
        param_type.paramIdx = @intCast(i + 1);
        param.ntype = param_type;
        param_types[i] = param_type;
        try innerctx.add(self.allocator, param.data.ref, param.ntype.?);
    }

    self.castIndex += @intCast(expr.params.len);

    if (expr.ntype) |type_node| {
        self.inDefinition = true;
        _ = try analyze(self, &innerctx, type_node);
        self.inDefinition = false;
        errdefer type_node.deinit(self.talloc);

        var annotated_type = type_node.ntype.?;
        for (expr.params, 0..) |param, i| {
            if (annotated_type.data != .function) break;
            const ann_param_type = try annotated_type.data.function.argument.deepCopy(self.talloc);
            ann_param_type.paramIdx = @intCast(i + 1);
            param.ntype = ann_param_type;
            param_types[i] = ann_param_type;
            annotated_type = annotated_type.data.function.ret;
        }
    }

    var return_type: *Type = undefined;
    if (expr.expr) |body_node| {
        _ = try analyze(self, &innerctx, body_node);
        if (body_node.ntype) |body_type| {
            return_type = body_type;
        } else {
            return_type = try self.talloc.create(Type);
            self.castIndex += 1;
            return_type.* = Type.initCasting(self.castIndex);
        }
    } else {
        return_type = try self.talloc.create(Type);
        return_type.* = Type.initCasting(self.castIndex);
        self.castIndex += 1;
    }

    var func_type = return_type;
    for (1..expr.params.len + 1) |idx| {
        const i = expr.params.len - idx;
        const ft = try self.talloc.create(Type);
        ft.* = .{
            .data = .{
                .function = .{
                    .argument = param_types[i],
                    .ret = func_type,
                    .body = null,
                },
            },
        };
        func_type = ft;
    }

    node.ntype = func_type;

    var it = self.substitutions.iterator();
    while (it.next()) |val| {
        instantiate(node, val.key_ptr, val.value_ptr.*);
    }
    // Step 6: Add to current context
    try currctx.add(self.allocator, expr.name, func_type);
}

fn analyzeSum(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    const sum = node.data.sum;

    var elements = std.ArrayList(*Type).init(self.talloc);

    for (sum) |el| {
        try self.analyze(currctx, el);
        try elements.append(el.ntype.?);
    }

    node.ntype = try self.talloc.create(Type);
    node.ntype.?.* = Type{
        .data = .{
            .aggregate = .{ .types = try elements.toOwnedSlice(), .isSum = true },
        },
    };
}

fn analyzeAggr(self: *Analyzer, currctx: *TContext, node: *Node) Error!void {
    const prod = node.data.aggr;

    var elements = std.ArrayList(*Type).init(self.talloc);

    for (prod.children) |el| {
        try self.analyze(currctx, el);
        try elements.append(el.ntype.?);
    }

    node.ntype = try self.talloc.create(Type);
    node.ntype.?.* = Type{
        .data = .{
            .aggregate = .{ .types = try elements.toOwnedSlice(), .isSum = false },
        },
    };
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

fn instantiateType(haysack: *Type, find: *Type, replace: *Type) void {
    switch (haysack.data) {
        .integer, .decimal, .boolean, .casting => if (haysack.deepEqual(find.*, true)) {
            haysack.* = replace.*;
        } else {},
        .pointer => |v| instantiateType(v, find, replace),
        .array => |v| {
            instantiateType(v.indexer, find, replace);

            for (v.value) |i|
                if (i) |t|
                    instantiateType(t, find, replace);
        },
        .aggregate => |v| {
            for (v.types) |i|
                instantiateType(i, find, replace);
        },
        .function => |v| {
            instantiateType(v.argument, find, replace);
            instantiateType(v.ret, find, replace);
        },
    }
}

fn instantiateContext(ctx: *TContext, find: *Type, replace: *Type) void {
    var memIt = ctx.members.valueIterator();

    while (memIt.next()) |val| {
        if (val.*.deepEqual(find.*, true))
            val.* = replace;
    }

    var childIt = ctx.children.valueIterator();

    while (childIt.next()) |val|
        instantiateContext(val, find, replace);
}

/// given a node tree, instantiate "find" with "replace"
fn instantiate(node: *Node, find: *Type, replace: *Type) void {
    instantiateType(node.ntype.?, find, replace);

    switch (node.data) {
        .int, .dec, .str, .ref, .mod, .type => {},
        .arr => |v| {
            for (v) |i|
                instantiate(i, find, replace);
        },
        .unr => |v| instantiate(v.val, find, replace),
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
        .match => |v| {
            instantiate(v.on, find, replace);

            for (v.cases) |case| {
                instantiate(case.tag, find, replace);
                instantiate(case.result, find, replace);
            }
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
