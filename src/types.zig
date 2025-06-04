const std = @import("std");

const Lexer = @import("lexer.zig");
const Node = @import("node.zig");
const Parser = @import("parser.zig");
const IR = @import("ir.zig");
const misc = @import("misc.zig");

const Type = @This();

/// Syntactic analysis errors
pub const Error = error{
    /// Out of memory (allocator compliant)
    OutOfMemory,

    /// Undefined operation between types
    UndefinedOperation,

    /// Possible division by zero
    PossibleDivisionByZero,

    /// Division by zero
    DivisionByZero,

    /// Invalid function composition
    InvalidComposition,

    /// Array size mismatch
    ArraySizeMismatch,

    /// Parameter mismatch
    ParameterMismatch,

    /// Unknown parameter given
    UnknownParameter,

    /// Disjoint types
    DisjointTypes,
};

/// Define a cast type
pub const Casting = union(enum) {
    /// Casting by name
    name: misc.String,

    /// Compiler generated casts
    index: u32,

    /// Check if two castings are equal
    pub fn eql(self: Casting, other: Casting) bool {
        if (std.meta.activeTag(self) != std.meta.activeTag(other))
            return false;

        return switch (self) {
            .name => std.mem.eql(u8, self.name, other.name),
            .index => self.index == other.index,
        };
    }
};

/// Substitution for
pub const Substitution = struct {
    /// All types matching this one
    from: Type,

    /// Substitute to this type
    to: Type,
};

/// Value inside the type
data: union(enum) {
    /// Boolean: can be true/false/valueless
    boolean: ?bool,

    /// Interval [start, end] subset of all integers, can have a value
    integer: struct {
        /// Lower bound
        start: i64,

        /// Upper bound
        end: i64,

        /// Type value, if any
        value: ?i64,
    },

    /// Interval [start, end] subset of all rationals, can have a value
    decimal: struct {
        /// Lower bound
        start: f64,

        /// Upper bound
        end: f64,

        /// Decimal value, if any
        value: ?f64,
    },

    /// Sized collection of elements
    array: struct {
        /// What is the type for each element on the array
        indexer: *Type,

        /// Array size
        size: i64,

        /// Array value, if any
        value: []?Type,
    },

    /// Collection of heterogeneous data, can be either from a sum
    /// or a product type
    aggregate: struct {
        /// All types inside the struct
        types: []Type,

        /// Offsets for all elements, increasing order when analyzed,
        /// but byte offsets after perfect alignment
        indexes: []i64,

        /// Values for the elements in the aggregate
        values: []?Type,
    },

    /// Transformer of values
    function: struct {
        /// Type for the function input
        argument: *Type,

        /// Type for the function output
        ret: *Type,

        /// Function body, if able to deduce
        body: ?*Node = null,
    },

    /// Type variable, allowing for polymorphism
    casting: Casting,
},

/// How many pointers deep is this type
pointerIndex: u8 = 0,

/// Apply arguments into this closure
partialArgs: ?[]const ?IR.Value = null,

/// 1 + param index, making retrieval easier
paramIdx: u8 = 0,

/// Value for Integer[0, 0]{0}
pub const zeroInt = Type.initInt(0, 0, 0);

/// Value for Integer[-1, -1]{-1}
pub const negOneInt = Type.initInt(-1, -1, -1);

/// Value for Rational[0, 0]{0}
pub const zeroDec = Type.initFloat(0, 0, 0);

/// Return if this type is a pointer
pub inline fn isPointer(self: Type) bool {
    return self.pointerIndex > 0;
}

/// Check if this type is a function
pub inline fn isFunction(self: Type) bool {
    return self.data == .function;
}

/// Check if this type is a casting
pub fn isCasting(self: Type) bool {
    return self.data == .casting;
}

/// Check if this type has had all its parameters fulfilled
pub fn isFullyApplied(self: Type) bool {
    return switch (self.data) {
        .function => |f| self.partialArgs != null and
            self.partialArgs.?.len == f.expectedArgs,
        else => false,
    };
}

/// How many arguments is this function still expecting
pub fn expectedArgs(self: Type) usize {
    if (self.data == .function)
        return 1 + self.data.function.body.?.ntype.?.expectedArgs();
    return 0;
}

/// Initialize an integer with a range
pub fn initInt(start: i64, end: i64, value: ?i64) Type {
    return Type{
        .data = .{
            .integer = .{
                .start = start,
                .end = end,
                .value = value,
            },
        },
    };
}

/// Initialize a float with a range
pub fn initFloat(start: f64, end: f64, value: ?f64) Type {
    return Type{
        .data = .{
            .decimal = .{
                .start = start,
                .end = end,
                .value = value,
            },
        },
    };
}

/// Initialize a boolean
pub fn initBool(value: ?bool) Type {
    return Type{
        .data = .{ .boolean = value },
    };
}

pub fn initIdxCasting(value: u32) Type {
    return Type{
        .data = .{
            .casting = .{ .index = value },
        },
    };
}

/// Check if this type has a value
pub fn isValued(self: Type) bool {
    return switch (self.data) {
        .boolean => |v| v != null,
        .integer => |v| v.value != null,
        .decimal => |v| v.value != null,
        .array => |v| {
            for (v.value) |value|
                if (value == null or !value.?.isValued())
                    return false;
            return true;
        },
        .aggregate => |v| {
            for (v.values) |value|
                if (value == null or !value.?.isValued())
                    return false;
            return true;
        },
        .function => |v| v.body != null,
        .casting => false,
    };
}

/// synthesize a type given another and an unary operation
pub fn synthUnary(op: Lexer.Tokens, v: Type) Error!Type {
    if (op == .Amp) {
        var nv = v;
        nv.pointerIndex += 1;
        return nv;
    }
    if (op == .Plus) return v;

    if (v.isPointer())
        return switch (op) {
            .Star => {
                var other = v;
                other.pointerIndex -= 1;
                return other;
            },
            else => error.UndefinedOperation,
        };
    return switch (v.data) {
        .integer => switch (op) {
            .Min => handleInt(op, Type.zeroInt, v),
            .Til => handleInt(.Min, Type.negOneInt, v),
            else => error.UndefinedOperation,
        },
        .decimal => switch (op) {
            .Plus => v,
            .Min => handleFloat(op, Type.zeroDec, v),
            else => error.UndefinedOperation,
        },
        .boolean => switch (op) {
            .Bang => {
                if (v.data.boolean) |vbool|
                    return initBool(!vbool);
                return initBool(null);
            },
            else => error.UndefinedOperation,
        },
        else => error.UndefinedOperation,
    };
}

pub fn checkUnary(op: Lexer.Tokens, v: Type, res: Type) !Type {
    return Type.join(try synthUnary(op, v), res);
}

pub fn callFunction(f: Type, args: Type) Error!*Type {
    // todo: add casting branch
    if (!f.isFunction())
        return error.UndefinedOperation;

    if (f.data.function.argument.deepEqual(args, false))
        return f.data.function.ret;

    return error.ParameterMismatch;
}

pub fn binOperate(
    alloc: std.mem.Allocator,
    op: Lexer.Tokens,
    a: Type,
    b: Type,
) Error!Type {
    if (op == .Arrow) {
        const left = try alloc.create(Type);
        errdefer alloc.destroy(left);

        left.* = a;

        const right = try alloc.create(Type);
        errdefer alloc.destroy(right);

        right.* = b;

        return Type{
            .pointerIndex = 0,
            .data = .{
                .function = .{
                    .body = null,
                    .argument = left,
                    .ret = right,
                },
            },
        };
    }

    if (std.meta.activeTag(a.data) != std.meta.activeTag(b.data))
        return error.UndefinedOperation;

    if (a.pointerIndex != b.pointerIndex)
        return error.UndefinedOperation;

    // if (a.pointerIndex > 0) {
    //     return switch (op) {
    //         .Cequ, .Cneq => binOperate(alloc, op, a, b),
    //         else => TypeError.UndefinedOperation,
    //     };
    // }

    if (a.isFunction() and b.isFunction() and op == .Star) {
        if (a.data.function.ret != b.data.function.argument)
            return error.InvalidComposition;

        return Type{
            .data = .{
                .function = .{
                    .argument = a.data.function.argument,
                    .ret = b.data.function.ret,
                    .body = null,
                },
            },
        };
    }

    return switch (a.data) {
        .boolean => |ba| {
            const bb = b.data.boolean;
            const res: ?bool = switch (op) {
                .Cequ => binopt(bool, ba, bb, misc.equ),
                .Cneq => binopt(bool, ba, bb, misc.neq),
                .And => binopt(bool, ba, bb, misc.land),
                .Or => binopt(bool, ba, bb, misc.land),
                else => null,
            };

            if (res) |r| {
                return initBool(r);
            }
            return error.UndefinedOperation;
        },
        .integer => try handleInt(op, a, b),
        .decimal => try handleFloat(op, a, b),
        .array => try handleArray(alloc, op, a, b),
        else => error.UndefinedOperation,
    };
}

fn minMax(comptime T: type, x1: T, x2: T, y1: T, y2: T, f: anytype) struct { T, T } {
    if (T == bool) {
        return .{ false, true };
    } else {
        const d1 = f(T, x1, y1);
        const d2 = f(T, x1, y2);
        const d3 = f(T, x2, y1);
        const d4 = f(T, x2, y2);

        const gMin = @min(@min(d1, d2), @min(d3, d4));
        const gMax = @max(@max(d1, d2), @max(d3, d4));
        return .{ gMin, gMax };
    }
}

fn binopt(comptime T: type, a: ?T, b: ?T, func: anytype) ?@TypeOf(func(T, a.?, b.?)) {
    if (a == null or b == null) return null;
    return func(T, a.?, b.?);
}

fn doMinMax(comptime T: type, func: anytype, ma: anytype, mb: anytype) Type {
    const mm = minMax(T, ma.start, ma.end, mb.start, mb.end, func);
    const val = binopt(T, ma.value, mb.value, func);
    if (T == i64) {
        return initInt(mm[0], mm[1], val);
    }
    return initFloat(mm[0], mm[1], val);
}

fn doBoolMinMax(comptime T: type, func: anytype, ma: anytype, mb: anytype) Type {
    return initBool(binopt(T, ma.value, mb.value, func));
}

fn handleInt(op: Lexer.Tokens, a: Type, b: Type) Error!Type {
    const ia = a.data.integer;
    const ib = b.data.integer;

    return switch (op) {
        .Plus => doMinMax(i64, misc.add, ia, ib),
        .Min => doMinMax(i64, misc.sub, ia, ib),
        .Star => doMinMax(i64, misc.mul, ia, ib),
        .Bar => {
            if (ib.value != null and ib.value.? == 0)
                return error.DivisionByZero;
            // todo: add "possible division by zero"
            const mm = minMax(i64, ia.start, ia.end, ib.start, ib.end, misc.div);
            return initInt(mm[0], mm[1], binopt(i64, ia.value, ib.value, misc.div));
        },
        .Per => {
            if (ib.value != null and ib.value.? == 0)
                return error.DivisionByZero;
            // todo: add "possible division by zero"
            return initInt(0, ia.end - 1, binopt(i64, ia.value, ib.value, misc.mod));
        },
        .Pipe => doMinMax(i64, misc.pipe, ia, ib),
        .Amp => doMinMax(i64, misc.amp, ia, ib),
        .Hat => doMinMax(i64, misc.hat, ia, ib),
        .Lsh => doMinMax(i64, misc.lsh, ia, ib),
        .Rsh => doMinMax(i64, misc.rsh, ia, ib),
        .Cequ => doBoolMinMax(i64, misc.equ, ia, ib),
        .Cneq => doBoolMinMax(i64, misc.neq, ia, ib),
        .Cge => doBoolMinMax(i64, misc.gte, ia, ib),
        .Cgt => doBoolMinMax(i64, misc.gt, ia, ib),
        .Cle => doBoolMinMax(i64, misc.lte, ia, ib),
        .Clt => doBoolMinMax(i64, misc.lt, ia, ib),
        else => error.UndefinedOperation,
    };
}

fn handleFloat(op: Lexer.Tokens, a: Type, b: Type) Error!Type {
    const fa = a.data.decimal;
    const fb = b.data.decimal;

    return switch (op) {
        .Plus => doMinMax(f64, misc.add, fa, fb),
        .Min => doMinMax(f64, misc.sub, fa, fb),
        .Star => doMinMax(f64, misc.mul, fa, fb),
        .Bar => {
            if (fb.value != null and fb.value.? == 0)
                return error.DivisionByZero;
            // todo: add "possible division by zero"
            const mm = minMax(f64, fa.start, fa.end, fb.start, fb.end, misc.div);
            return initFloat(mm[0], mm[1], binopt(f64, fa.value, fb.value, misc.div));
        },
        .Per => {
            if (fb.value != null and fb.value.? == 0) return error.DivisionByZero;
            // todo: add "possible division by zero"
            return initFloat(0, fb.end - 1, binopt(f64, fa.value, fb.value, misc.mod));
        },
        .Cequ => doBoolMinMax(f64, misc.equ, fa, fb),
        .Cneq => doBoolMinMax(f64, misc.neq, fa, fb),
        .Cge => doBoolMinMax(f64, misc.gte, fa, fb),
        .Cgt => doBoolMinMax(f64, misc.gt, fa, fb),
        .Cle => doBoolMinMax(f64, misc.lte, fa, fb),
        .Clt => doBoolMinMax(f64, misc.lt, fa, fb),
        else => error.UndefinedOperation,
    };
}

fn scalarArray(allocator: std.mem.Allocator, op: Lexer.Tokens, arr: Type, other: Type, reversed: bool) Error!Type {
    const arrData = arr.data.array;
    var results = try allocator.alloc(?Type, arrData.value.len);

    for (arrData.value, 0..) |elem, i| {
        if (elem == null) {
            results[i] = null;
            continue;
        }
        results[i] = try if (reversed) binOperate(allocator, op, other, elem.?) else binOperate(allocator, op, elem.?, other);
    }

    return Type{
        .pointerIndex = 0,
        .data = .{
            .array = .{
                .indexer = arrData.indexer,
                .size = arrData.size,
                .value = results,
            },
        },
    };
}

fn handleArray(alloc: std.mem.Allocator, op: Lexer.Tokens, a: Type, b: Type) Error!Type {
    if (a.data == .array and b.data == .array) {
        if (a.data.array.size != b.data.array.size) return error.ArraySizeMismatch;
        const arrA = a.data.array;
        const arrB = b.data.array;
        var results = try alloc.alloc(?Type, arrA.value.len);

        for (arrA.value, arrB.value, 0..) |aElem, bElem, i| {
            if (aElem == null or bElem == null) {
                results[i] = null;
                continue;
            }

            results[i] = try binOperate(alloc, op, aElem.?, bElem.?);
        }

        return Type{
            .data = .{
                .array = .{
                    .indexer = arrA.indexer,
                    .size = arrA.size,
                    .value = results,
                },
            },
        };
    }

    if (a.data == .array or b.data == .array)
        return scalarArray(alloc, op, a, b, b.data == .array);

    return error.UndefinedOperation;
}

pub fn deepEqual(t1: Type, t2: Type, checkValued: bool) bool {
    if (std.meta.activeTag(t1.data) != std.meta.activeTag(t2.data)) return false;
    if (t1.pointerIndex != t2.pointerIndex) return false;

    return switch (t1.data) {
        .integer => |v1| {
            const v2 = t2.data.integer;
            return v1.start == v2.start and v1.end == v2.end and (v1.value == v2.value or !checkValued);
        },
        .decimal => |v1| {
            const v2 = t2.data.decimal;
            return v1.start == v2.start and v1.end == v2.end and (v1.value == v2.value or !checkValued);
        },
        .boolean => t1.data.boolean == t2.data.boolean,
        .array => {
            if (t1.data.array.size != t2.data.array.size) return false;
            if (!deepEqual(t1.data.array.indexer.*, t2.data.array.indexer.*, checkValued)) return false;
            if (!checkValued) return true;

            for (t1.data.array.value, t2.data.array.value) |a, b| {
                if (a == null or b == null) return false;
                if (!deepEqual(a.?, b.?, checkValued)) return false;
            }
            return true;
        },
        .aggregate => |v| {
            if (v.types.len != t2.data.aggregate.types.len) return false;
            for (v.types, t2.data.aggregate.types) |a, b| {
                if (!deepEqual(a, b, checkValued)) return false;
            }
            return true;
        },
        .function => |v| {
            if (!deepEqual(v.argument.*, t2.data.function.argument.*, checkValued)) return false;
            return deepEqual(v.ret.*, t2.data.function.ret.*, checkValued);
        },
        // possibly false but we move
        .casting => |v| std.meta.activeTag(v) == std.meta.activeTag(t2.data.casting) and switch (v) {
            .index => v.index == t2.data.casting.index,
            .name => std.mem.eql(u8, v.name, t2.data.casting.name),
        },
    };
}

pub fn deepCopy(self: Type, alloc: std.mem.Allocator) !*Type {
    const t = try alloc.create(Type);
    errdefer alloc.destroy(t); 
    switch (self.data) {
        .integer => t.* = initInt(self.data.integer.start, self.data.integer.end, self.data.integer.value),
        .decimal => t.* = initFloat(self.data.decimal.start, self.data.decimal.end, self.data.decimal.value),
        .boolean => t.* = initBool(self.data.boolean),
        .array => |v| {
            var newValues = try alloc.alloc(?Type, v.value.len);
            for (v.value, 0..) |value, i| {
                if (value == null) continue;
                const res = try value.?.deepCopy(alloc);
                newValues[i] = res.*;
                alloc.destroy(res); // only the top
            }

            t.data = .{
                .array = .{
                    .size = v.size,
                    .indexer = try v.indexer.deepCopy(alloc),
                    .value = newValues,
                }
            };
        },
        .aggregate => |v| {
            const newTypes = try alloc.alloc(Type, v.types.len);
            errdefer alloc.free(newTypes);
            for (self.data.aggregate.types, 0..) |value, i| {
                const res = try value.deepCopy(alloc);
                newTypes[i] = res.*;
                alloc.destroy(res);
            }
            newType.data.aggregate.types = newTypes;
            break :blk newType;
        },
        .function => blk: {
            var newType = self;
            newType.data.function.argument = try self.data.function.argument.deepCopy(alloc);
            newType.data.function.ret = try self.data.function.ret.deepCopy(alloc);

            break :blk newType;
        },
        .casting => self,
    };

    t.paramIdx = self.paramIdx;
    return t;
}

pub fn instantiate(self: Type, instances: []const struct { misc.String, u32, Type }) Type {
    return switch (self.data) {
        .casting => {
            const cast = self.data.casting;
            for (instances) |instance| {
                if (switch (cast) {
                    .name => std.mem.eql(u8, cast.name, instance[0]),
                    .index => cast.index == instance[1],
                })
                    return instance[2];
            }
            return self;
        },
        .array => |v| {
            const result = v.indexer.instantiate(instances);
            if (!result.deepEqual(v.indexer.*, false))
                v.indexer.* = result;

            return self;
        },
        .aggregate => |v| {
            for (v.types) |value| {
                var val = value;
                val = val.instantiate(instances);
            }
            return self;
        },
        .function => |v| {
            v.argument.* = v.argument.instantiate(instances);
            v.ret.* = v.ret.instantiate(instances);
            return self;
        },
        else => self,
    };
}

pub fn join(t1: Type, t2: Type) Error!struct { []const Substitution, Type } {
    if (t1.pointerIndex != t2.pointerIndex) return error.DisjointTypes(t1, t2);

    return switch (t1.data) {
        .boolean => .{ .{}, t1 },
        .integer => |v1| {
            if (t2.data != .integer)
                return error.DisjointTypes;

            const v2 = t2.data.integer;

            if (v1.value != null and v2.value != null and v1.value != v2.value)
                return error.UndefinedOperation;

            const result = initInt(@max(v1.start, v2.start), @min(v1.end, v2.end), if (v1.value != null) v1 else v2);

            const rint = result.data.integer;

            if (rint.start > result.data.integer.end)
                return error.UndefinedOperation;

            if (rint.value) |rv|
                if (rint.start > rv or rv > rint.end)
                    return error.UndefinedOperation;

            return .{ .{}, result };
        },
        .decimal => |v1| {
            if (t2.data != .decimal)
                return error.UndefinedOperation;

            const v2 = t2.data.decimal;

            if (v1.value != null and v2.value != null and v1.value != v2.value)
                return error.UndefinedOperation;

            const result = initFloat(@max(v1.start, v2.start), @min(v1.end, v2.end), if (v1.value != null) v1 else v2);

            const rdec = result.data.decimal;

            if (rdec.start > rdec.end)
                return error.UndefinedOperation;

            if (rdec.value) |rv|
                if (rdec.start > rv or rv > rdec.end)
                    return error.UndefinedOperation;

            return .{ .{}, result };
        },
    };
}

pub fn meet(alloc: std.mem.Allocator, t1: Type, t2: Type) !?std.ArrayList(struct { misc.String, Type }) {
    const ret = std.ArrayList(struct { misc.String, Type });
    return switch (t1.data) {
        .casting => {
            if (std.mem.eql(u8, t1.data.casting, t2.data.casting)) {
                const list = try ret.init(alloc);
                try list.append(.{ t1.data.casting, t2 });
                return list;
            }
            return null;
        },
        .array => {
            if (t2.data.array.size != t1.data.array.size) return null;
            return t1.data.array.indexer.meet(alloc, t2.data.array.indexer);
        },
        .aggregate => {
            if (t2.data.aggregate.types.len != t1.data.aggregate.types.len) return null;
            var list = try ret.init(alloc);
            for (t1.data.aggregate.types, t2.data.aggregate.types) |a, b| {
                const met = try a.meet(alloc, b);
                if (met == null) return null;
                for (met) |m|
                    try list.append(m);
            }
            return list;
        },
        .function => {
            const argMeet = try t1.data.function.argument.meet(alloc, t2.data.function.argument);
            const retMeet = try t1.data.function.ret.meet(alloc, t2.data.function.ret);
            if (argMeet == null or retMeet == null) return null;
            var list = try ret.init(alloc);
            list.extend();
            for (argMeet.?) |a| {
                for (retMeet.?) |r| {
                    try list.append(a);
                    try list.append(r);
                }
            }
            return list;
        },
        else => null,
    };
}

// get a function parameter given index
pub fn getParameter(self: Type, index: u8) Error!*Type {
    var typ = &self;
    var idx: u8 = 0;
    while (typ.isFunction()) : (idx += 1) {
        if (idx == index) return typ.data.function.argument;
        typ = typ.data.function.ret;
    }

    return error.UnknownParameter;
}

const expect = std.testing.expect;

////////// Types test
test "Type operations" {
    const t1 = Type.initInt(2, 2, 2);
    const t2 = Type.initInt(5, 5, 5);

    const alloc = std.testing.allocator;

    try expect((try Type.synthUnary(.Plus, t1)).deepEqual(t1, true));
    try expect((try Type.synthUnary(.Min, t1)).deepEqual(Type.initInt(-2, -2, -2), true));
    try expect((try Type.synthUnary(.Til, t1)).deepEqual(Type.initInt(-3, -3, -3), true));

    try expect((try Type.binOperate(alloc, .Plus, t1, t2)).deepEqual(Type.initInt(7, 7, 7), true));
    try expect((try Type.binOperate(alloc, .Min, t1, t2)).deepEqual(Type.initInt(-3, -3, -3), true));
    try expect((try Type.binOperate(alloc, .Min, t2, t1)).deepEqual(Type.initInt(3, 3, 3), true));
    try expect((try Type.binOperate(alloc, .Min, t2, t2)).deepEqual(Type.zeroInt, true));
}

test "type instancing" {
    var b = Type{
        .pointerIndex = 0,
        .data = .{
            .casting = .{ .name = "b" },
        },
    };

    var a = Type{
        .pointerIndex = 0,
        .data = .{ .casting = .{ .name = "a" } },
    };

    const fab = Type{
        .pointerIndex = 0,
        .data = .{
            .function = .{
                .argument = &a,
                .ret = &b,
                .body = null,
            },
        },
    };

    const instance1 = [_]struct { misc.String, u32, Type }{
        .{ "a", 0, Type.initBool(true) },
    };

    const res1 = Type.instantiate(fab, &instance1);

    var b1 = Type.initBool(true);

    const exp1 = Type{
        .pointerIndex = 0,
        .data = .{
            .function = .{
                .argument = &b1,
                .ret = &b,
                .body = null,
            },
        },
    };

    try (expect(res1.deepEqual(exp1, true)));
}
