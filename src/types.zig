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

/// Value inside the type
data: union(enum) {
    /// Boolean: can be true/false/valueless (trool  lol)
    boolean: ?bool,

    /// Interval [start, end] subset of all integers, can have a value
    integer: struct {
        /// Range start, inclusive
        start: i64,

        /// Range end, inclusive
        end: i64,

        /// Current value, if any
        value: ?i64,
    },

    /// Interval [start, end] subset of all rationals, can have a value
    decimal: struct {
        /// Range start, inclusive
        start: f64,

        /// Range end, inclusive
        end: f64,

        /// Current value, if any
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
partialArgs: ?[]?IR.Value = null,

/// 1 + param index, making retrieval easier
paramIdx: u8 = 0,

/// How many arguments is this function still expecting
pub fn expectedArgs(self: Type) usize {
    if (self.data == .function)
        return 1 + self.data.function.body.?.ntype.?.expectedArgs();
    return 0;
}

/// Initialize an integer with a range
pub inline fn initInt(start: i64, end: i64, value: ?i64) Type {
    return .{
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
pub inline fn initFloat(start: f64, end: f64, value: ?f64) Type {
    return .{
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
pub inline fn initBool(value: ?bool) Type {
    return .{
        .data = .{ .boolean = value },
    };
}

/// Initalize a casting by index
pub inline fn initIdxCasting(value: u32) Type {
    return .{
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

pub fn isTemplated(self: Type) bool {
    return switch (self.data) {
        .casting => true,
        .function => |f| f.argument.isTemplated() or f.ret.isTemplated(),
        .array => |a| a.indexer.isTemplated(),
        .aggregate => |a| for (a.types) |t| if (t.isTemplated()) return true,
        else => false,
    };
}

pub fn deepEqual(t1: Type, t2: Type, checkValued: bool) bool {
    if (std.meta.activeTag(t1.data) != std.meta.activeTag(t2.data)) return false;
    if (t1.pointerIndex != t2.pointerIndex) return false;

    return switch (t1.data) {
        .integer => |v1| {
            const v2 = t2.data.integer;
            return v1.start == v2.start and v1.end == v2.end and (!checkValued or v1.value == v2.value);
        },
        .decimal => |v1| {
            const v2 = t2.data.decimal;
            return v1.start == v2.start and v1.end == v2.end and (!checkValued or v1.value == v2.value);
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
        .casting => |v| v.eql(t2.data.casting),
    };
}

pub fn deepCopy(self: Type, alloc: std.mem.Allocator) !*Type {
    const t = try alloc.create(Type);
    errdefer alloc.destroy(t);
    switch (self.data) {
        .integer => |v| {
            t.* = initInt(v.start, v.end, v.value);
        },
        .decimal => |v| {
            t.* = initFloat(v.start, v.end, v.value);
        },
        .boolean => |v| {
            t.* = initBool(v);
        },
        .array => |v| {
            var newValues = try alloc.alloc(?Type, v.value.len);
            for (v.value, 0..) |value, i| {
                if (value == null) continue;
                const res = try value.?.deepCopy(alloc);
                newValues[i] = res.*;
                alloc.destroy(res); // only the top
            }

            t.data = .{ .array = .{
                .size = v.size,
                .indexer = try v.indexer.deepCopy(alloc),
                .value = newValues,
            } };
        },
        .aggregate => |v| {
            const newTypes = try alloc.alloc(Type, v.types.len);
            errdefer alloc.free(newTypes);
            for (v.types, 0..) |value, i| {
                const res = try value.deepCopy(alloc);
                newTypes[i] = res.*;
                alloc.destroy(res);
            }
            t.data.aggregate.types = newTypes;
        },
        .function => |v| {
            t.data = .{
                .function = .{
                    .argument = try v.argument.deepCopy(alloc),
                    .ret = try v.ret.deepCopy(alloc),
                    .body = v.body,
                },
            };
        },
        .casting => {
            t.* = self;
        },
    }

    t.paramIdx = self.paramIdx;
    return t;
}

pub fn getParameter(self: Type, index: u8) Error!*Type {
    var typ = &self;
    var idx: u8 = 0;
    while (typ.data == .function) : (idx += 1) {
        if (idx == index) return typ.data.function.argument;
        typ = typ.data.function.ret;
    }

    return error.UnknownParameter;
}

pub fn hash(self: Type) u64 {
    var hasher = std.hash.Wyhash.init(0);

    hasher.update(&[_]u8{@intFromEnum(self.data)});
    hasher.update(&[_]u8{self.pointerIndex});
    hasher.update(&[_]u8{self.paramIdx});

    switch (self.data) {
        .boolean => |v| {
            if (v) |b| hasher.update(&[_]u8{if (b) 1 else 0});
        },
        .integer => |v| {
            hasher.update(std.mem.asBytes(&v.start));
            hasher.update(std.mem.asBytes(&v.end));
            if (v.value) |val| hasher.update(std.mem.asBytes(&val));
        },
        .decimal => |v| {
            hasher.update(std.mem.asBytes(&v.start));
            hasher.update(std.mem.asBytes(&v.end));
            if (v.value) |val| hasher.update(std.mem.asBytes(&val));
        },
        .array => |v| {
            hasher.update(std.mem.asBytes(&v.indexer.hash()));
            hasher.update(std.mem.asBytes(&v.size));
            for (v.value) |elem| {
                if (elem) |e| hasher.update(std.mem.asBytes(&e.hash()));
            }
        },
        .aggregate => |v| {
            for (v.types) |t| hasher.update(std.mem.asBytes(&t.hash()));
            for (v.indexes) |idx| hasher.update(std.mem.asBytes(&idx));
            for (v.values) |val| {
                if (val) |v2| hasher.update(std.mem.asBytes(&v2.hash()));
            }
        },
        .function => |v| {
            hasher.update(std.mem.asBytes(&v.argument.hash()));
            hasher.update(std.mem.asBytes(&v.ret.hash()));
        },
        .casting => |v| {
            switch (v) {
                .name => |s| hasher.update(s),
                .index => |idx| hasher.update(std.mem.asBytes(&idx)),
            }
        },
        
    }
    return hasher.final();
}

pub const TypeContext = struct {
  pub fn hash(_: TypeContext, t: Type) u64 {
    return t.hash();
  }
  pub fn eql(_: TypeContext, a: Type, b: Type) bool {
    return a.deepEqual(b, false); // should this be true
  }
};