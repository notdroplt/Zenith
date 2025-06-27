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
pub const Casting = u32;

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
pub inline fn initCasting(value: u32) Type {
    return .{
        .data = .{
            .casting = value,
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
        .function => |f| f.argument.isTemplated() and f.ret.isTemplated(),
        .array => |a| a.indexer.isTemplated(),
        .aggregate => |a| {
            for (a.types) |t|
                if (t.isTemplated())
                    return true;
            return false;
        },
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
        .casting => |v| v == t2.data.casting,
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

pub fn getCastings(self: Type, alloc: std.mem.Allocator) ![]const Casting {
    var list = std.ArrayList(Casting).init(alloc);
    defer list.deinit();

    const function = struct {
        fn collect(t: *const Type, l: *std.ArrayList(Casting)) !void {
            switch (t.data) {
                .casting => |c| {
                    for (l.items) |existing|
                        if (c.eql(existing)) return;

                    try l.append(c);
                },
                .array => |a| try collect(a.indexer, l),
                .aggregate => |a| for (a.types) |ts| try collect(&ts, l),
                .function => |f| {
                    try collect(f.argument, l);
                    try collect(f.ret, l);
                },
                else => {},
            }
        }
    }.collect;

    try function(&self, &list);
    return try list.toOwnedSlice();
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
            hasher.update(std.mem.asBytes(&v));
        },
    }
    return hasher.final();
}

pub fn serialize(self: Type, writer: anytype) !void {
    try writer.writeByte(@intFromEnum(self.data));
    try writer.writeByte(self.pointerIndex);
    try writer.writeByte(self.paramIdx);

    switch (self.data) {
        .boolean => |v| {
            try writer.writeByte(if (v == null) 2 else if (v.?) 1 else 0);
        },
        .integer => |v| {
            try writer.writeInt(i64, v.start, .little);
            try writer.writeInt(i64, v.end, .little);
            try writer.writeByte(if (v.value == null) 0 else 1);
            if (v.value) |val| try writer.writeInt(i64, val, .little);
        },
        .decimal => |v| {
            try writer.writeFloat(f64, v.start, .little);
            try writer.writeFloat(f64, v.end, .little);
            try writer.writeByte(if (v.value == null) 0 else 1);
            if (v.value) |val| try writer.writeFloat(f64, val, .little);
        },
        .array => |v| {
            try v.indexer.serialize(writer);
            try writer.writeInt(i64, v.size, .little);
            try writer.writeInt(u32, @intCast(v.value.len), .little);
            for (v.value) |elem| {
                try writer.writeByte(if (elem == null) 0 else 1);
                if (elem) |e| try e.serialize(writer);
            }
        },
        .aggregate => |v| {
            try writer.writeInt(u32, @intCast(v.types.len), .little);
            for (v.types) |t| try t.serialize(writer);
            try writer.writeInt(u32, @intCast(v.indexes.len), .little);
            for (v.indexes) |idx| try writer.writeInt(i64, idx, .little);
            try writer.writeInt(u32, @intCast(v.values.len), .little);
            for (v.values) |val| {
                try writer.writeByte(if (val == null) 0 else 1);
                if (val) |v2| try v2.serialize(writer);
            }
        },
        .function => |v| {
            try v.argument.serialize(writer);
            try v.ret.serialize(writer);
        },
        .casting => |v| {
            switch (v) {
                .name => |s| {
                    try writer.writeInt(u32, @intCast(s.len), .little);
                    try writer.writeAll(s);
                },
                .index => |idx| {
                    try writer.writeInt(u32, idx, .little);
                },
            }
        },
    }
}

pub fn deserialize(reader: anytype, allocator: std.mem.Allocator) !Type {
    const tag = @as(@TypeOf(Type.data), try reader.readByte());
    const pointerIndex = try reader.readByte();
    const paramIdx = try reader.readByte();

    var t: Type = undefined;
    t.pointerIndex = pointerIndex;
    t.paramIdx = paramIdx;

    switch (@as(Type.data, @enumFromInt(tag))) {
        .boolean => {
            const b = try reader.readByte();
            t.data = .{ .boolean = if (b == 2) null else b == 1 };
        },
        .integer => {
            const start = try reader.readInt(i64, .little);
            const end = try reader.readInt(i64, .little);
            const has_value = try reader.readByte();
            const value = if (has_value == 1) try reader.readInt(i64, .little) else null;
            t.data = .{ .integer = .{ .start = start, .end = end, .value = value } };
        },
        .decimal => {
            const start = try reader.readFloat(f64, .little);
            const end = try reader.readFloat(f64, .little);
            const has_value = try reader.readByte();
            const value = if (has_value == 1) try reader.readFloat(f64, .little) else null;
            t.data = .{ .decimal = .{ .start = start, .end = end, .value = value } };
        },
        .array => {
            const indexer = try allocator.create(Type);
            indexer.* = try deserialize(reader, allocator);
            const size = try reader.readInt(i64, .little);
            const len = try reader.readInt(u32, .little);
            const value = try allocator.alloc(?Type, len);
            for (value) |*elem| {
                const has_elem = try reader.readByte();
                if (has_elem == 1) {
                    const e = try deserialize(reader, allocator);
                    elem.* = e;
                } else {
                    elem.* = null;
                }
            }
            t.data = .{ .array = .{ .indexer = indexer, .size = size, .value = value } };
        },
        .aggregate => {
            const types_len = try reader.readInt(u32, .little);
            const types = try allocator.alloc(Type, types_len);
            for (types) |*tt| tt.* = try deserialize(reader, allocator);
            const idx_len = try reader.readInt(u32, .little);
            const indexes = try allocator.alloc(i64, idx_len);
            for (indexes) |*idx| idx.* = try reader.readInt(i64, .little);
            const values_len = try reader.readInt(u32, .little);
            const values = try allocator.alloc(?Type, values_len);
            for (values) |*val| {
                const has_val = try reader.readByte();
                if (has_val == 1) {
                    const v2 = try deserialize(reader, allocator);
                    val.* = v2;
                } else {
                    val.* = null;
                }
            }
            t.data = .{ .aggregate = .{ .types = types, .indexes = indexes, .values = values } };
        },
        .function => {
            const arg = try allocator.create(Type);
            arg.* = try deserialize(reader, allocator);
            const ret = try allocator.create(Type);
            ret.* = try deserialize(reader, allocator);
            t.data = .{ .function = .{ .argument = arg, .ret = ret, .body = null } };
        },
        .casting => {
            const tag2 = try reader.readByte();
            if (tag2 == 0) {
                const len = try reader.readInt(u32, .little);
                const buf = try allocator.alloc(u8, len);
                _ = try reader.readAll(buf);
                t.data = .{ .casting = .{ .name = buf } };
            } else {
                const idx = try reader.readInt(u32, .little);
                t.data = .{ .casting = .{ .index = idx } };
            }
        },
    }
    return t;
}

pub const TypeContext = struct {
    pub fn hash(_: TypeContext, t: Type) u64 {
        return t.hash();
    }
    pub fn eql(_: TypeContext, a: Type, b: Type) bool {
        return a.deepEqual(b, false); // should this be true
    }
};
