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

pub const Flags = enum(usize) {
    Materializable = 0,
    Constructor = 1,
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

    /// Point to another type
    pointer: *Type,

    /// Sized collection of elements
    array: struct {
        /// What is the type for each element on the array
        indexer: *Type,

        /// Array size
        size: u64,

        /// Array value, if any
        value: []?*Type,
    },

    /// Collection of heterogeneous data, can be either from a sum
    /// or a product type
    aggregate: struct {
        /// All types inside the struct
        types: []*Type,

        /// if this is a product type, the constructor is not null
        constructor: ?misc.String,
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

/// Apply arguments into this closure
partialArgs: ?[]?IR.Value = null, // stupidly cursed way to make a unique_ptr

/// sets flags for this type:
/// bit 0: is this materializable
/// bit 1: is this a constructor (only when `.data == .function`)
flags: std.bit_set.IntegerBitSet(2) = .{ .mask = 0 },

/// 1 + param index, making retrieval easier
paramIdx: u8 = 0,

/// Size of this type
tsize: i32 = 0,

/// Padding of this type
tpadding: i32 = 0,

/// How many arguments is this function still expecting
pub fn expectedArgs(self: Type) usize {
    return switch (self.data) {
        .function => |v| 1 + v.body.?.ntype.?.expectedArgs(),
        else => 0,
    };
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
        .pointer => |v| v.isValued(),
        .array => |v| {
            for (v.value) |value|
                if (value == null or !value.?.isValued())
                    return false;
            return true;
        },
        .aggregate => |v| {
            for (v.types) |value|
                if (!value.isValued())
                    return false;
            return true;
        },
        .function => |v| v.body != null,
        .casting => false,
    };
}

pub fn devalue(self: *Type) void {
    switch (self.data) {
        .boolean => self.data.boolean = null,
        .integer => self.data.integer.value = null,
        .decimal => self.data.decimal.value = null,
        .pointer => |v| v.devalue(),
        .array => |v| {
            self.data.array.indexer.devalue();
            for (v.value) |t| {
                if (t) |i| i.devalue();
            }
        },
        .aggregate => |v| {
            for (v.types) |value|
                value.devalue();
        },
        .function => |v| {
            v.argument.devalue();
            v.ret.devalue();
        },
        .casting => {},
    }
}

pub fn isTemplated(self: Type) bool {
    return switch (self.data) {
        .casting => true,
        .function => |f| f.argument.isTemplated() and f.ret.isTemplated(),
        .pointer => |p| p.isTemplated(),
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

    return switch (t1.data) {
        .integer => |v1| {
            const v2 = t2.data.integer;
            return v1.start == v2.start and v1.end == v2.end and (!checkValued or v1.value == v2.value);
        },
        .decimal => |v1| {
            const v2 = t2.data.decimal;
            return v1.start == v2.start and v1.end == v2.end and (!checkValued or v1.value == v2.value);
        },
        .pointer => t1.data.pointer.deepEqual(t2.data.pointer.*, checkValued),
        .boolean => t1.data.boolean == t2.data.boolean,
        .array => {
            if (t1.data.array.size != t2.data.array.size) return false;
            if (!deepEqual(t1.data.array.indexer.*, t2.data.array.indexer.*, checkValued)) return false;
            if (!checkValued) return true;

            for (t1.data.array.value, t2.data.array.value) |a, b| {
                if (a == null or b == null) return false;
                if (!deepEqual(a.?.*, b.?.*, checkValued)) return false;
            }
            return true;
        },
        .aggregate => |v| {
            if (v.types.len != t2.data.aggregate.types.len) return false;
            for (v.types, t2.data.aggregate.types) |a, b| {
                if (!deepEqual(a.*, b.*, checkValued)) return false;
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
        .integer => |v| t.* = initInt(v.start, v.end, v.value),
        .decimal => |v| t.* = initFloat(v.start, v.end, v.value),
        .boolean => |v| t.* = initBool(v),
        .pointer => |v| t.data.pointer = try v.deepCopy(alloc),
        .array => |v| {
            var newValues = try alloc.alloc(?*Type, v.value.len);
            for (v.value, 0..) |value, i| {
                if (value == null) continue;
                const res = try value.?.deepCopy(alloc);
                newValues[i] = res;
            }

            t.data = .{
                .array = .{
                    .size = v.size,
                    .indexer = try v.indexer.deepCopy(alloc),
                    .value = newValues,
                },
            };
        },
        .aggregate => |v| {
            const newTypes = try alloc.alloc(*Type, v.types.len);
            errdefer alloc.free(newTypes);
            for (v.types, newTypes) |value, *el| {
                const res = try value.deepCopy(alloc);
                el.* = res;
            }
            t.data = .{
                .aggregate = .{
                    .types = newTypes,
                    .constructor = v.constructor,
                },
            };
        },
        .function => |v| t.data = .{
            .function = .{
                .argument = try v.argument.deepCopy(alloc),
                .ret = try v.ret.deepCopy(alloc),
                .body = v.body,
            },
        },
        .casting => t.* = self,
    }

    t.paramIdx = self.paramIdx;
    t.tpadding = self.tpadding;
    t.tsize = self.tsize;
    t.flags = self.flags;
    return t;
}

pub const SetResult = union(enum) {
    /// A ⊇ B
    Left,

    /// A ⊆ B
    Right,

    /// A ∩ B is not equal to either A or B, but also not empty
    New: Type,

    // A ∩ B = {}
    Impossible,
};

/// Does A ∪ B, if possible
pub fn join(a: Type, b: Type) SetResult {
    const ad = a.data;
    const bd = b.data;

    if (b.data == .casting) return .Left;
    if (a.data == .casting) return .Right;

    if (std.meta.activeTag(a.data) != std.meta.activeTag(bd))
        return .Impossible;

    if (a.deepEqual(b, true))
        return SetResult.Left;

    switch (ad) {
        .boolean => return SetResult{ .New = Type.initBool(null) },
        .integer => |ia| {
            const ib = bd.integer;
            if (ia.start <= ib.start and ia.end >= ib.end)
                return SetResult.Left;

            if (ib.start <= ia.start and ib.end >= ia.end)
                return SetResult.Right;

            return SetResult{
                .New = Type.initInt(@min(ia.start, ib.start), @max(ia.start, ib.start), ia.value orelse ib.value),
            };
        },
        .decimal => |da| {
            const db = bd.decimal;
            if (da.start <= db.start and da.end >= db.end)
                return SetResult.Left;

            if (db.start <= da.start and db.end >= da.end)
                return SetResult.Right;

            return SetResult{
                .New = Type.initFloat(@min(da.start, db.start), @max(da.start, db.start), da.value orelse db.value),
            };
        },
        .array => |aa| {
            const ab = bd.array;
            if (!deepEqual(aa.indexer.*, ab.indexer.*, false)) {
                return SetResult.Impossible;
            }

            if (aa.size >= ab.size) return SetResult.Left;
            return SetResult.Right;
        },
        .casting => return SetResult.Left,
        // wrong but yeah :sob:
        else => return SetResult.Impossible,
    }
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
    return list.toOwnedSlice();
}

fn hashImpl(self: Type, hasher: *std.hash.Wyhash) void {
    var h = hasher;
    h.update(&[_]u8{ @intFromEnum(self.data), self.paramIdx });

    switch (self.data) {
        .boolean => |v| {
            if (v) |b| h.update(&[_]u8{if (b) 1 else 0});
        },
        .integer => |v| {
            h.update(std.mem.asBytes(&v.start));
            h.update(std.mem.asBytes(&v.end));
            if (v.value) |val| h.update(std.mem.asBytes(&val));
        },
        .decimal => |v| {
            h.update(std.mem.asBytes(&v.start));
            h.update(std.mem.asBytes(&v.end));
            if (v.value) |val| h.update(std.mem.asBytes(&val));
        },
        .pointer => |v| return hashImpl(v.*, h),
        .array => |v| {
            hashImpl(v.indexer.*, h);
            h.update(std.mem.asBytes(&v.size));
            for (v.value) |elem| {
                if (elem) |e| hashImpl(e.*, h);
            }
        },
        .aggregate => |v| {
            for (v.types) |t| hashImpl(t.*, h);
            h.update(&[_]u8{if (v.constructor != null) 1 else 0});
            if (v.constructor) |ctor|
                h.update(ctor);
        },
        .function => |v| {
            hashImpl(v.argument.*, h);
            hashImpl(v.ret.*, h);
        },
        .casting => |v| {
            h.update(std.mem.asBytes(&v));
        },
    }
}

pub fn hash(self: Type) u64 {
    var hasher = std.hash.Wyhash.init(0);
    self.hashImpl(&hasher);
    return hasher.final();
}

pub fn serialize(self: Type, writer: anytype) !void {
    try writer.writeByte(@intFromEnum(self.data));
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
    const paramIdx = try reader.readByte();

    var t: Type = undefined;
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
            const s = try reader.readInt(i64, .little);
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
            t.data = .{ .array = .{ .indexer = indexer, .size = s, .value = value } };
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

pub fn size(self: Type) isize {
    if (self.tsize != 0) return self.tsize;

    var s = 0;
    switch (self.data) {
        .boolean => s = 1,
        .function => s = 8, // function pointers only (?)
        .casting => s = -1,
        .pointer => s = 8,
        .aggregate => |v| if (v.isSum) {
            for (v.types) |t| {
                const p = t.padding();
                const rem = @mod(s, p);
                s += t.size() + if (rem == 0) 0 else p - rem;
            }
        } else {
            for (v.types) |t| s = @max(s, t.size());
        },
        .array => |v| s = v.indexer.size() * v.size,
        .integer => |v| {
            const items = @as(u64, @intCast(v.end - v.end)) + 1;
            const bits = std.math.log2_int_ceil(u64, items);
            s = @divFloor(bits + 7, 8);
        },
        .decimal => |v|
        // TODO: f128 and f256 (?)
        s = if (v.end >= std.math.floatMax(f32) or v.start <= -std.math.floatMax(f32))
            8
        else
            4,
    }

    self.tsize = s;
    return s;
}

pub fn padding(self: Type) isize {
    if (self.tpadding != 0) return self.tpadding;

    var p = 0;
    switch (self.data) {
        .boolean => p = 1,
        .function => p = 8,
        .casting => p = -1,
        .pointer => p = 8,
        .aggregate => |v| for (v.types) |t| {
            p = @max(p, t.padding());
        },
        .array => |v| p = v.indexer.padding(),
        // only for scalar integers
        else => p = self.size(),
    }
    self.tpadding = p;
    return p;
}

/// try to avoid padding inside a type
pub fn perfectType(self: Type) void {
    if (self.isTemplated() // templates will float to the top
    or self.data != .aggregate or self.data.aggregate.isSum)
        return; // almost everyone is perfect

    const agg = self.data.aggregate;
    std.mem.sort(Type, agg.types, {}, struct {
        fn lessThan(_: void, a: Type, b: Type) bool {
            return a.padding() > b.padding();
        }
    }.lessThan);
}

pub const TypeContext = struct {
    pub fn hash(_: TypeContext, t: Type) u64 {
        return t.hash();
    }
    pub fn eql(_: TypeContext, a: Type, b: Type) bool {
        return a.deepEqual(b, false); // should this be true
    }
};
