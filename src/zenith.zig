const std = @import("std");
const misc = @import("./misc.zig");
////////////////////////////////////////////////////////////////////////
// Lexer
////////////////////////////////////////////////////////////////////////

pub const Tokens = enum { Tnone, Tint, Tdec, Tstr, Tref, Tmod, Tmatch, Tlpar, Trpar, Tlcur, Trcur, Tlsqb, Trsqb, Tequ, Tsemi, Tplus, Tmin, Tstar, Tbar, Tamp, Ttil, Tbang, Thash, Tpipe, That, Tper, Tlsh, Trsh, Tcequ, Tcneq, Tcgt, Tcge, Tclt, Tcle, Tand, Tor, Tques, Tcln, Tdot };

pub const LexerError = error{ MalformedNumber, MalformedIdentifier, MalformedToken, MalformedString, LexerEnd };

pub const Pos = struct {
    index: usize = 0,
    span: usize = 0,

    pub fn new() Pos {
        return Pos{
            .index = 0,
            .span = 0,
        };
    }
};

pub const Token = struct {
    tid: Tokens,
    pos: Pos = Pos{},
    val: union { int: i64, dec: f64, str: []const u8 },

    pub fn init(tid: Tokens, pos: Pos) Token {
        return Token{ .tid = tid, .pos = pos, .val = .{ .int = 0 } };
    }

    pub fn same(self: Token, other: Token) bool {
        const id = self.tid == other.tid;

        return switch (self.tid) {
            Tokens.Tint => id and self.val.int == other.val.int,
            Tokens.Tdec => id and self.val.dec == other.val.dec,
            Tokens.Tstr, Tokens.Tref => id and std.mem.eql(u8, self.val.str, other.val.str),
            else => id,
        };
    }

    pub fn sameId(self: Token, other: Tokens) bool {
        return self.tid == other;
    }

    pub fn notId(self: Token, other: Tokens) bool {
        return self.tid != other;
    }
};

test {
    const t1 = Token{ .tid = Tokens.Tint, .pos = Pos.new(), .val = .{ .int = 1 } };
    const t2 = Token{ .tid = Tokens.Tstr, .pos = Pos.new(), .val = .{ .str = "str" } };
    const t3 = Token{ .tid = Tokens.Tint, .pos = Pos{ .index = 1, .span = 3 }, .val = .{ .int = 1 } };

    try std.testing.expect(t1.same(t3));
    try std.testing.expect(t1.sameId(Tokens.Tint));
    try std.testing.expect(t2.notId(Tokens.Tand));
}

pub const Lexer = struct {
    code: []const u8,
    index: usize,

    pub fn init(code: []const u8) Lexer {
        return Lexer{ .code = code, .index = 0 };
    }

    fn end(self: Lexer) bool {
        return self.code.len <= self.index;
    }

    pub fn current(self: Lexer) LexerError!u8 {
        return if (self.end()) LexerError.LexerEnd else self.code.ptr[self.index];
    }

    fn walk(self: Lexer) Lexer {
        return Lexer{ .code = self.code, .index = self.index + @intFromBool(!self.end()) };
    }

    fn run(self: Lexer, n: usize) Lexer {
        const new_index = @min(self.index + n, self.code.len);

        return Lexer{ .code = self.code, .index = new_index };
    }

    fn consumeWhitespace(self: Lexer) LexerError!Lexer {
        var lex = self;
        while (std.ascii.isWhitespace(try lex.current()))
            lex = lex.walk();
        return lex;
    }

    fn skipComments(self: Lexer) LexerError!Lexer {
        var str = self.code[self.index..];
        var lex = self;
        while (str.prefix("//")) : (str = lex.code[lex.index..]) {
            while (lex.current() != '\n' and !lex.end())
                lex = lex.walk();
            lex = lex.walk();
        }
        return lex.consumeWhitespace();
    }

    fn consumeString(self: Lexer) LexerError!Token {
        var lex = self.walk();
        const start = lex.index;
        while (try lex.current() != '"' and !lex.end()) {
            lex = lex.walk();
        }
        if (lex.end())
            return LexerError.MalformedString;
        lex = lex.walk();

        return Token{
            .tid = Tokens.Tstr,
            .pos = Pos{ .index = start - 1, .span = lex.index - start + 1 },
            .val = .{ .str = self.code[start .. lex.index - 1] },
        };
    }

    fn consumeIdentifier(self: Lexer) LexerError!Token {
        const start = self.index;
        var lex = self;
        if (!std.ascii.isAlphabetic(try lex.current()) and try lex.current() != '_') {
            return LexerError.MalformedIdentifier;
        }
        lex = lex.walk();
        while ((std.ascii.isAlphanumeric(try lex.current()) or try lex.current() == '_' or try lex.current() == '\'') and !lex.end()) {
            lex = lex.walk();
        }

        return Token{
            .tid = Tokens.Tref,
            .pos = Pos{ .index = start, .span = lex.index - start },
            .val = .{ .str = lex.code[start..lex.index] },
        };
    }

    fn consumeNumber(self: Lexer) LexerError!Token {
        var curr = Tokens.Tint;
        const start = self.index;
        var lex = self;

        while (std.ascii.isDigit(try lex.current()) and !lex.end()) {
            lex = lex.walk();
        }

        if (try lex.current() == '.') {
            curr = Tokens.Tdec;
            lex = lex.walk();
            while (std.ascii.isDigit(try lex.current()) and !lex.end()) {
                lex = lex.walk();
            }
        }

        if (try lex.current() == 'e') {
            lex = lex.walk();
            if (try lex.current() == '+' or try lex.current() == '-') {
                curr = if (try lex.current() == '-') Tokens.Tdec else curr;
                lex = lex.walk();
            }
            if (!std.ascii.isDigit(try lex.current()))
                return LexerError.MalformedNumber;

            while (std.ascii.isDigit(try lex.current()))
                lex = lex.walk();
        }

        var result = Token.init(curr, Pos{ .index = start, .span = lex.index - start });

        const str = lex.code[start .. start + result.pos.span];

        if (curr == Tokens.Tint) {
            result.val.int = try Lexer.parse_int(str);
        } else {
            result.val.dec = try Lexer.parse_dec(str);
        }

        return result;
    }

    fn parse_int(str: []const u8) LexerError!i64 {
        var result: i64 = 0;
        var exp: i64 = 0;
        var idx: usize = 0;

        while (idx < str.len) : (idx += 1) {
            result = result * 10 + (str[idx] - '0');
        }

        if (idx < str.len and std.ascii.toLower(str[idx]) == 'e') {
            idx += 1;
            if (idx < str.len and (str[idx] == '+')) {
                idx += 1;
            }
            while (idx < str.len) : (idx += 1) {
                exp = exp * 10 + (str[idx] - '0');
            }
        }
        return result * std.math.pow(i64, 10, exp);
    }

    fn parse_dec(str: []const u8) LexerError!f64 {
        var result: f64 = 0;
        var exp: f64 = 0;
        var sign: f64 = 1;
        var idx: usize = 0;

        const base10: f64 = 10;

        while (idx < str.len) : (idx += 1) {
            const digit = @as(f64, @floatFromInt(str[idx] - '0'));
            const r1 = @mulAdd(f64, result, 10, digit);
            result = r1;
        }

        if (idx != str.len and std.ascii.toLower(str[idx]) == 'e') {
            idx += 1;
            if (idx != str.len and ((str[idx] == '+') or (str[idx] == '-'))) {
                sign = if (str[idx] == '-') -1 else 1;
                idx += 1;
            }
            while (idx < str.len) : (idx += 1) {
                const digit = @as(f64, @floatFromInt(str[idx] - '0'));
                const r1 = @mulAdd(f64, exp, 10, digit);
                exp = r1;
            }
        }
        return result * std.math.pow(f64, base10, sign * exp);
    }

    pub fn consume(self: Lexer) LexerError!Token {
        var lex = try self.consumeWhitespace();

        if (std.ascii.isDigit(try lex.current()))
            return lex.consumeNumber();

        if (try lex.current() == '"')
            return lex.consumeString();

        if (std.ascii.isAlphabetic(try lex.current()) or try lex.current() == '_')
            return lex.consumeIdentifier();

        const str = lex.code[lex.index..];

        if (std.mem.startsWith(u8, str, "module"))
            return Token.init(Tokens.Tmod, Pos{ .index = lex.index, .span = 6 });

        if (std.mem.startsWith(u8, str, "match"))
            return Token.init(Tokens.Tmatch, Pos{ .index = self.index, .span = 5 });

        const pos = Pos{ .index = lex.index, .span = 1 };
        const curr = try lex.current();
        lex = lex.walk();

        return switch (curr) {
            '(' => Token.init(Tokens.Tlpar, pos),
            ')' => Token.init(Tokens.Trpar, pos),
            '{' => Token.init(Tokens.Tlcur, pos),
            '}' => Token.init(Tokens.Trcur, pos),
            '[' => Token.init(Tokens.Tlsqb, pos),
            ']' => Token.init(Tokens.Trsqb, pos),
            '=' => lex.generate_eq(),
            '+' => Token.init(Tokens.Tplus, pos),
            '-' => Token.init(Tokens.Tmin, pos),
            '*' => Token.init(Tokens.Tstar, pos),
            '/' => Token.init(Tokens.Tbar, pos),
            '&' => lex.generate_amp(),
            '~' => Token.init(Tokens.Ttil, pos),
            '!' => lex.generate_bang(),
            '#' => Token.init(Tokens.Thash, pos),
            '|' => lex.generate_pipe(),
            '%' => Token.init(Tokens.Tper, pos),
            '<' => lex.generate_lt(),
            '>' => lex.generate_gt(),
            '?' => Token.init(Tokens.Tques, pos),
            ':' => Token.init(Tokens.Tcln, pos),
            ';' => Token.init(Tokens.Tsemi, pos),
            else => return LexerError.MalformedToken,
        };
    }

    fn generate_eq(self: Lexer) LexerError!Token {
        var pos = Pos{ .index = self.index - 1, .span = 1 };
        if (try self.current() == '=') {
            pos.span += 1;
            return Token.init(Tokens.Tcequ, pos);
        }
        return Token.init(Tokens.Tequ, pos);
    }

    fn generate_amp(self: Lexer) LexerError!Token {
        var pos = Pos{ .index = self.index - 1, .span = 1 };
        if (try self.current() == '&') {
            pos.span += 1;
            return Token.init(Tokens.Tand, pos);
        }
        return Token.init(Tokens.Tamp, pos);
    }

    fn generate_bang(self: Lexer) LexerError!Token {
        var pos = Pos{ .index = self.index - 1, .span = 1 };
        if (try self.current() == '=') {
            pos.span += 1;
            return Token.init(Tokens.Tcneq, pos);
        } else {
            return Token.init(Tokens.Tbang, pos);
        }
    }

    fn generate_pipe(self: Lexer) LexerError!Token {
        var pos = Pos{ .index = self.index - 1, .span = 1 };
        if (try self.current() == '|') {
            pos.span += 1;
            return Token.init(Tokens.Tor, pos);
        }
        return Token.init(Tokens.Tpipe, pos);
    }

    fn generate_lt(self: Lexer) LexerError!Token {
        var pos = Pos{ .index = self.index - 1, .span = 1 };
        if (try self.current() == '=') {
            pos.index += 1;
            return Token.init(Tokens.Tcle, pos);
        }
        if (try self.current() == '<') {
            pos.index += 1;
            return Token.init(Tokens.Tlsh, pos);
        }
        return Token.init(Tokens.Tclt, pos);
    }

    fn generate_gt(self: Lexer) LexerError!Token {
        var pos = Pos{ .index = self.index - 1, .span = 1 };
        if (try self.current() == '=') {
            pos.index += 1;
            return Token.init(Tokens.Tcge, pos);
        }
        if (try self.current() == '>') {
            pos.index += 1;
            return Token.init(Tokens.Trsh, pos);
        }
        return Token.init(Tokens.Tcgt, pos);
    }

    pub fn catch_up(self: Lexer, token: Token) Lexer {
        const str_jmp: usize = if (token.tid == Tokens.Tstr) 2 else 0;
        return Lexer{
            .code = self.code,
            .index = token.pos.index + token.pos.span + str_jmp,
        };
    }

    pub fn is_finished(self: Lexer) bool {
        return self.end();
    }
};

test {
    var l1 = Lexer.init("1");
    try std.testing.expect(!l1.is_finished());
    try std.testing.expect((try l1.current()) == '1');
    l1 = l1.walk();

    try std.testing.expect(l1.is_finished());
    try std.testing.expectError(LexerError.LexerEnd, l1.current());

    var l2 = Lexer.init("123");
    try std.testing.expect(!l2.is_finished());
    try std.testing.expect((try l2.current()) == '1');
    l2 = l2.run(2);
    try std.testing.expect(!l2.is_finished());
    try std.testing.expect(l2.index == 2);
    try std.testing.expect((try l2.current()) == '3');
    l2 = l2.run(2);
    try std.testing.expect(l2.is_finished());
    try std.testing.expectError(LexerError.LexerEnd, l2.current());
}

////////////////////////////////////////////////////////////////////////
// types
//
// 1. get ssa
// 2. put everything as a template
// 3. hm-type all
// 4. ???
// 5. profit
//
////////////////////////////////////////////////////////////////////////

//pub const Types = enum { none, boolean, integer, decimal, pointer, array, aggregate, casting };

pub const TypeError = error{
    UndefinedOperation,
    PossibleDivisionByZero,
    DivisionByZero,
};

pub const Type = union(enum) {
    boolean: ?bool,
    integer: struct { start: i64, end: i64, value: ?i64 },
    decimal: struct { start: f64, end: f64, value: ?f64 },
    pointer: struct { type_: *Type },
    array: struct { type_: *Type, size: i64, value: []?Type },
    aggregate: struct { types: []Type, indexes: []i64, values: []?Type },
    function: struct { argument: *Type, ret: *Type, body: ?*Node },
    casting: []u8,

    const zeroInt = Type{ .integer = .{ .start = 0, .end = 0, .value = 0}};
    const oneInt = Type{ .integer = .{ .start = 1, .end = 1, .value = 1}};
    const zeroDec = Type{ .decimal = .{ .start = 0, .end = 0, .value = 0}};

    pub fn isScalar(self: Type) bool {
        return @intFromEnum(self) <= @intFromEnum(Type.pointer);
    }

    pub fn initInt(start: i64, end: i64, value: ?i64) Type {
        return Type{ .integer = .{ .start = start, .end = end, .value = value } };
    }

    pub fn initFloat(start: f64, end: f64, value: ?f64) Type {
        return Type{ .decimal = .{ .start = start, .end = end, .value = value } };
    }

    pub fn isTag(self: Type, tag: @TypeOf(std.meta.activeTag(self))) bool {
        return std.meta.activeTag(self) == tag;
    }

    pub fn argCount(self: Type) u8 {
        var typ = self;
        var idx: u8 = 0;
        while (typ.isTag(.function)) : (idx += 1) {
            typ = typ.function.ret;
        }

        return idx;
    }

    pub fn makePtr(alloc: std.mem.Allocator, v: Type) !Type {
        const t = try alloc.create(Type);
        t.* = v;
        return Type { .pointer = .{ .type_ = t}};
    }

    pub fn unrOperate(alloc: std.mem.Allocator, op: Tokens, v: Type) !Type {
        return switch (v) {
            .integer => try unInt(alloc, op, v),
            .decimal => try unDec(alloc, op, v),
            .boolean => try unBool(alloc, op, v)
            
        };
    }

    fn unInt(alloc: std.mem.Allocator, op: Tokens, v: Type) !Type {
        return switch (op) {
            Tokens.Tplus => v,
            Tokens.Tmin => handleInt(op, Type.zeroInt, v),
            Tokens.Ttil => handleInt(Tokens.Tmin, Type.oneInt, v), // ~x = 1 - x (for the type system that is absolutely fine)
            Tokens.Tamp => makePtr(alloc, v),
            else => TypeError.UndefinedOperation
        };
    }

    fn unDec(alloc: std.mem.Allocator, op: Tokens, v: Type) !Type {
        return switch (op) {
            Tokens.Tplus => v,
            Tokens.Tmin => handleFloat(op, Type.zeroDec, v),
            Tokens.Tamp => makePtr(alloc, v),
            else => TypeError.UndefinedOperation
        };
    }

    fn unBool(alloc: std.mem.Allocator, op: Tokens, v: Type) !Type {
        return switch (op) {
            Tokens.Tamp => makePtr(alloc, v),
            Tokens.Tbang => Type { .boolean = !v.boolean orelse null },
            else => TypeError.UndefinedOperation
        };
    }

    pub fn binOperate(
        allocator: std.mem.Allocator,
        op: Tokens,
        a: Type,
        b: Type,
    ) !Type {
        if (a.isTag(.array) or b.isTag(.array))
            return handleArray(allocator, op, a, b);

        if (a.isTag(.function) and a.isTag(.function) and op == Tokens.Tstar)
            return composeFunction(a, b);

        if (std.meta.activeTag(a) != std.meta.activeTag(b)) {
            return TypeError.UndefinedOperation;
        }

        return switch (a) {
            .integer => |ia| try handleInt(op, ia, b),
            .decimal => |da| try handleFloat(op, da, b),
            .boolean => |ba| try handleBool(op, ba, b),
            .pointer => |pa| try handlePointer(op, pa, b),
            else => error.UnsupportedOperation,
        };
    }

    fn composeFunction(a: Type, b: Type) TypeError!Type {
        if (a.function.ret != b.function.argument) {
            return TypeError.InvalidComposition;
        }

        return Type{
            .function = .{
                .argument = b.function.argument,
                .ret = a.function.ret,
                .body = null, // todo: compose if possible
            },
        };
    }

    fn minMax(x1: anytype, x2: @TypeOf(x1), y1: @TypeOf(x1), y2: @TypeOf(x1), f: fn (@TypeOf(x1), @TypeOf(x1)) @TypeOf(x1)) struct { @TypeOf(x1), @TypeOf(x1) } {
        const d1 = f(x1, y1);
        const d2 = f(x1, y2);
        const d3 = f(x2, y1);
        const d4 = f(x2, y2);

        const gmin = @min(@min(d1, d2), @min(d3, d4));
        const gmax = @max(@max(d1, d2), @max(d3, d4));
        return .{ gmin, gmax };
    }

    fn binopt(comptime T: type, a: ?T, b: ?T, func: fn (T, T) T) ?T {
        if (a != null and b != null)
            return func(a, b);
        return null;
    }

    fn handleInt(op: Tokens, a: Type, b: Type) !Type {
        const ia = a.integer;
        const ib = b.integer;

        const closure = struct {
            fn doMinMax(func: fn (i64, i64) i64) Type {
                const minmax = minMax(ia.start, ia.end, ib.start, ib.end);
                return initInt(minmax[0], minmax[1], binopt(i64, ia.value, ib.end, func));
            }
        };

        return switch (op) {
            Tokens.Tplus => closure.doMinMax(misc.add),
            Tokens.Tmin => closure.doMinMax(misc.sub),
            Tokens.Tstar => closure.doMinMax(misc.mul),
            Tokens.Tbar => {
                if (ib.value != null and ib.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                const minmax = minMax(ia.start, ia.end, ib.start, ib.end, misc.div);
                return initInt(minmax[0], minmax[1], binopt(i64, ia.value, ib.value, misc.div));
            },
            Tokens.Tper => {
                if (ib.value != null and ib.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                return initInt(0, ib.value - 1, binopt(i64, ia.value, ib.value, misc.mod));
            },
            Tokens.Tpipe => closure.doMinMax(misc.pipe),
            Tokens.Tamp => closure.doMinMax(misc.amp),
            Tokens.That => closure.doMinMax(misc.hat),
            Tokens.Tlsh => closure.doMinMax(misc.lsh),
            Tokens.Trsh => closure.doMinMax(misc.rsh),
            Tokens.Tcequ => closure.doMinMax(misc.equ),
            Tokens.Tcneq => closure.doMinMax(misc.neq),
            Tokens.Tcge => closure.doMinMax(misc.gte),
            Tokens.Tcgt => closure.doMinMax(misc.gt),
            Tokens.Tcle => closure.doMinMax(misc.lte),
            Tokens.Tclt => closure.doMinMax(misc.lt),
            else => error.UnsupportedOperation,
        };
    }

    fn handleFloat(op: Tokens, a: Type, b: Type) !Type {
        const fa = a.decimal;
        const fb = b.decimal;
        const closure = struct {
            fn doMinMax(func: fn (f64, f64) f64) Type {
                const minmax = minMax(fa.start, fa.end, fb.start, fb.end);
                return initFloat(minmax[0], minmax[1], binopt(f64, fa.value, fb.end, func));
            }
        };

        return switch (op) {
            Tokens.Tplus => closure.doMinMax(misc.add),
            Tokens.Tmin => closure.doMinMax(misc.sub),
            Tokens.Tstar => closure.doMinMax(misc.mul),
            Tokens.Tbar => {
                if (fb.value != null and fb.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                const minmax = minMax(fa.start, fa.end, fb.start, fb.end, misc.div);
                return initFloat(minmax[0], minmax[1], binopt(f64, fa.value, fb.value, misc.div));
            },
            Tokens.Tper => {
                if (fb.value != null and fb.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                return initFloat(0, fb.value - 1, binopt(f64, fa.value, fb.value, misc.mod));
            },
            Tokens.Tcequ => closure.doMinMax(misc.equ),
            Tokens.Tcneq => closure.doMinMax(misc.neq),
            Tokens.Tcge => closure.doMinMax(misc.gte),
            Tokens.Tcgt => closure.doMinMax(misc.gt),
            Tokens.Tcle => closure.doMinMax(misc.lte),
            Tokens.Tclt => closure.doMinMax(misc.lt),
            else => error.UnsupportedOperation,
        };
    }

    fn handleBool(op: Tokens, a: Type, b: Type) TypeError!Type {
        const ba = a.boolean;
        const bb = b.boolean;

        return switch (op) {
            Tokens.Tcequ => binopt(bool, ba, bb, misc.equ),
            Tokens.Tcneq => binopt(bool, ba, bb, misc.neq),
            Tokens.Tand => binopt(bool, ba, bb, misc.land),
            Tokens.Tor => binopt(bool, ba, bb, misc.land),
            else => TypeError.UndefinedOperation,
        };
    }

    fn handlePointer(op: Tokens, a: Type, b: Type) TypeError!Type {
        const pa = a.boolean;
        const pb = b.boolean;

        return switch (op) {
            Tokens.Tcequ => binopt(bool, pa, pb, misc.equ),
            Tokens.Tcneq => binopt(bool, pa, pb, misc.neq),
            else => TypeError.UndefinedOperation,
        };
    }

    fn scalarArray(allocator: std.mem.Allocator, op: Tokens, arr: Type, other: Type, reversed: bool) !Type {
        const arr_data = arr.array;
        var results = try allocator.alloc(?Type, arr_data.value.len);

        for (arr_data.value, 0..) |elem, i| {
            if (elem == null) {
                results[i] = null;
                continue;
            }
            results[i] = try if (reversed) binOperate(allocator, op, other, elem.?) else binOperate(allocator, op, elem.?, other);
        }

        return Type{
            .array = .{
                .type_ = arr_data.type_,
                .size = arr_data.size,
                .value = results,
            },
        };
    }

    fn handleArray(
        allocator: std.mem.Allocator,
        op: Tokens,
        a: Type,
        b: Type,
    ) !Type {
        if (a.isTag(.array) and b.isTag(.array)) {
            if (a.array.size != b.array.size) return error.ArraySizeMismatch;
            const arr_a = a.array;
            const arr_b = b.array;
            var results = try allocator.alloc(?Type, arr_a.value.len);

            for (arr_a.value, arr_b.value, 0..) |a_elem, b_elem, i| {
                if (a_elem == null or b_elem == null) {
                    results[i] = null;
                    continue;
                }

                results[i] = try binOperate(allocator, op, a_elem.?, b_elem.?);
            }

            return Type{
                .array = .{
                    .type_ = arr_a.type_,
                    .size = arr_a.size,
                    .value = results,
                },
            };
        }

        if (a.isTag(.array))
            return scalarArray(allocator, op, a, b, false);

        if (b.isTag(.array))
            return scalarArray(allocator, op, b, a, true);

        return error.InvalidOperands;
    }
};

////////////////////////////////////////////////////////////////////////
// parser
////////////////////////////////////////////////////////////////////////

pub const Nodes = enum {
    none,
    type,
    int,
    dec,
    str,
    ref,
    unr,
    bin,
    ter,
    call,
    expr,
    mod,
    range,
    aggr,
    sum,
    intr,
};

const UnaryNode = struct {
    val: *Node,
    op: Tokens,
};

const BinaryNode = struct { lhs: *Node, rhs: *Node, op: Tokens };

const TernaryNode = struct {
    cond: *Node,
    btrue: *Node,
    bfalse: *Node,
};

const CallNode = struct {
    caller: *Node,
    callee: *Node,
};

const ExprNode = struct {
    name: []const u8,
    params: []*Node,
    ntype: ?*Node,
    expr: ?*Node,
};

const ModuleNode = struct {
    path: []const u8,
    pairs: std.StringHashMap(*Node),
    direction: bool,
};

const RangeNode = struct {
    start: *Node,
    end: *Node,
    epsilon: *Node,
};

const AggregateNode = struct {
    name: []const u8,
    children: []*Node,
};

const SumNode = struct { children: []*Node };

const IntermediateNode = struct {
    intermediates: []*Node,
    application: *Node,
};

pub const Node = struct {
    position: Pos,
    ntype: ?Type = null,
    data: union(Nodes) {
        none: void,
        type: void,
        int: i64,
        dec: f64,
        str: []const u8,
        ref: []const u8,
        unr: UnaryNode,
        bin: BinaryNode,
        ter: TernaryNode,
        call: CallNode,
        expr: ExprNode,
        mod: ModuleNode,
        range: RangeNode,
        aggr: AggregateNode,
        sum: SumNode,
        intr: IntermediateNode,
    },
};

const ParserOnlyError = error{
    LexerEnd,
    Overflow,
    UnclosedParen,
    UnexpectedToken,
    UnexpectedStartOfDeclaration,
    unexpected_eof,
    UnexpectedColon,
    overflow,
    underflow,
};

pub const ParserError = LexerError || ParserOnlyError || error{OutOfMemory};

pub const Parser = struct {
    code: []const u8,
    pos: Pos,
    alloc: std.mem.Allocator,
    lexer: Lexer,

    fn new_position(start: Pos, end: Pos) Pos {
        return Pos{
            .index = start.index,
            .span = end.index - start.index + end.span,
        };
    }

    fn create_int_node(self: Parser, position: Pos, value: i64) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .int = value },
        };
        return node;
    }

    fn create_dec_node(self: Parser, position: Pos, value: f64) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .dec = value },
        };
        return node;
    }

    fn create_str_node(self: Parser, position: Pos, value: []const u8) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .str = value },
        };
        return node;
    }

    fn create_ref_node(self: Parser, position: Pos, value: []const u8) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .ref = value },
        };
        return node;
    }

    fn create_unr_node(self: Parser, position: Pos, operator: Tokens, value: *Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .unr = .{ .val = value, .op = operator } },
        };
        return node;
    }

    fn create_bin_node(self: Parser, position: Pos, operator: Tokens, lhs: *Node, rhs: *Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .bin = .{ .lhs = lhs, .rhs = rhs, .op = operator } },
        };
        return node;
    }

    fn create_call_node(self: Parser, position: Pos, caller: *Node, callee: *Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .call = .{ .caller = caller, .callee = callee } },
        };
        return node;
    }

    fn create_expr_node(self: Parser, position: Pos, name: []const u8, params: []*Node, type_: ?*Node, expr: ?*Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .expr = .{ .name = name, .params = params, .ntype = type_, .expr = expr } },
        };
        return node;
    }

    fn get_precedence(tid: Tokens) i32 {
        return switch (tid) {
            Tokens.Tor, Tokens.Tand => 1,
            Tokens.Tcequ, Tokens.Tcneq, Tokens.Tclt, Tokens.Tcle, Tokens.Tcgt, Tokens.Tcge => 2,
            Tokens.Tplus, Tokens.Tmin => 3,
            Tokens.Tstar, Tokens.Tbar, Tokens.Tper => 4,
            Tokens.Tlsh, Tokens.Trsh => 5,
            Tokens.Tpipe, Tokens.That, Tokens.Tamp => 6,
            Tokens.Thash, Tokens.Tdot => 7,
            else => -1,
        };
    }

    const IntrFlags = enum {
        no_app,
        normal,
    };

    fn parse_intr(self: *Parser, flags: IntrFlags) ParserError!*Node {
        const open = try self.lexer.consume();
        const start = open.pos;
        if (open.notId(Tokens.Tlcur)) {
            return ParserError.unexpected_tok;
        }

        self.lexer = self.lexer.catch_up(open);

        var intermediates: [16]Node = {};
        var idx = 0;

        while (!(try self.lexer.consume()).sameId(Tokens.Trcur) and idx < 16) : (idx += 1) {
            intermediates[idx] = try parse_expr(self);
            const semi = try self.lexer.consume();
            if (semi.notId(Tokens.Tsemi)) {
                return ParserError.expecting_semi;
            }
            self.lexer = self.lexer.catch_up(semi);
        }

        const rcur = try self.lexer.consume();

        if (rcur.notId(Tokens.Trcur)) {
            return ParserError.expecting_rcur;
        }

        // we use less memory if we reallocate for every idx < 15
        const actual_imm = try self.alloc.alloc(Node, idx);

        if (flags == IntrFlags.no_app) {
            const node = try self.alloc.create(Node);
            node.* = Node{ .type = null, .position = new_position(start, rcur.pos), .data = .{ .intr = .{ .application = actual_imm } } };

            return node;
        }

        const application = try self.parse_expr();

        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = new_position(start, application.position),
            .data = .{ .intr = .{
                .intermediates = actual_imm,
                .application = application,
            } },
        };
        return node;
    }

    fn parse_module(self: *Parser) ParserError!*Node {
        const module = try self.lexer.consume();
        const start = module.pos;

        self.lexer = self.lexer.catch_up(module);

        const direction = try self.lexer.consume();

        if (direction.tid != Tokens.Tlsh and direction.tid != Tokens.Trsh) {
            self.pos = direction.pos;
            return error.UnexpectedToken;
        }

        self.lexer = self.lexer.catch_up(direction);

        const name = try self.lexer.consume();

        if (name.tid != Tokens.Tstr) {
            self.pos = name.pos;
            return error.UnexpectedToken;
        }

        self.lexer = self.lexer.catch_up(name);

        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = new_position(start, name.pos),
            .data = .{
                .mod = .{
                    .path = name.val.str,
                    .pairs = std.StringHashMap(*Node).init(self.alloc),
                    .direction = direction.tid == Tokens.Tlsh,
                },
            },
        };
        return node;
    }

    fn parse_primary(self: *Parser) ParserError!*Node {
        const start = self.pos;
        const token = try self.lexer.consume();
        self.lexer = self.lexer.catch_up(token);

        return switch (token.tid) {
            Tokens.Tint => try self.create_int_node(token.pos, token.val.int),
            Tokens.Tdec => try self.create_dec_node(token.pos, token.val.dec),
            Tokens.Tstr => try self.create_str_node(token.pos, token.val.str),
            Tokens.Tref => try self.create_ref_node(token.pos, token.val.str),
            Tokens.Tlpar => {
                const expr = try self.parse_expr();
                const rpar = try self.lexer.consume();
                if (rpar.tid != Tokens.Trpar) {
                    return ParserError.UnclosedParen;
                }
                expr.position = new_position(start, self.pos);
                return expr;
            },
            else => {
                self.pos = token.pos;
                return ParserError.UnexpectedToken;
            },
        };
    }

    fn is_atomic(tid: Tokens) bool {
        return (@intFromEnum(tid) >= @intFromEnum(Tokens.Tint) and
            @intFromEnum(tid) <= @intFromEnum(Tokens.Tref)) or tid == Tokens.Tlpar;
    }

    fn parse_call(self: *Parser) ParserError!*Node {
        var caller = try self.parse_primary();

        while (true) {
            const token = try self.lexer.consume();
            if (!is_atomic(token.tid)) {
                return caller;
            }

            self.lexer = self.lexer.catch_up(token);

            const callee = try self.parse_expr();

            caller = try self.create_call_node(token.pos, caller, callee);
        }
    }

    fn parse_unary(self: *Parser) ParserError!*Node {
        const token = try self.lexer.consume();

        switch (token.tid) {
            Tokens.Tmin, Tokens.Tplus, Tokens.Ttil, Tokens.Tbang, Tokens.Tstar, Tokens.Tamp => {},
            else => return self.parse_call(),
        }

        self.lexer = self.lexer.catch_up(token);
        const operand = try self.parse_unary();

        const node = try self.create_unr_node(token.pos, token.tid, operand);

        return node;
    }

    fn parse_binary(self: *Parser, precedence: i32) ParserError!*Node {
        var left = try self.parse_unary();

        while (true) {
            const token = try self.lexer.consume();
            const token_precedence = get_precedence(token.tid);
            if (token_precedence < precedence) {
                return left;
            }

            self.lexer = self.lexer.catch_up(token);
            const right = try self.parse_binary(token_precedence + 1);
            left = try self.create_bin_node(token.pos, token.tid, left, right);
        }
    }

    fn parse_ternary(self: *Parser) ParserError!*Node {
        const condition = try self.parse_binary(1);

        const question = try self.lexer.consume();
        if (question.tid != Tokens.Tques) {
            return condition;
        }

        self.lexer = self.lexer.catch_up(question);

        const then_expr = try self.parse_binary(1);

        const colon = try self.lexer.consume();
        if (colon.tid != Tokens.Tcln) {
            return ParserError.UnexpectedColon;
        }

        self.lexer = self.lexer.catch_up(colon);

        const else_expr = try self.parse_ternary();

        const ternary = try self.alloc.create(Node);
        ternary.* = Node{
            .position = new_position(condition.position, else_expr.position),
            .data = .{
                .ter = .{
                    .cond = condition,
                    .btrue = then_expr,
                    .bfalse = else_expr,
                },
            },
        };
        return ternary;
    }

    fn parse_expr(self: *Parser) ParserError!*Node {
        return self.parse_ternary();
    }

    fn parse_range(self: *Parser) ParserError!*Node {
        const lsq = self.lexer.consume();
        if (lsq.tid != Tokens.Tlsqb) {
            self.state = ParserError.error_uxd_tok;
            return error.UnexpectedToken;
        }

        self.lexer = self.lexer.catch_up(lsq);
        const start = try self.parse_expr();
        if (start == null) {
            return error.AllocError;
        }

        const semi = self.lexer.consume();
        if (semi.tid != Tokens.Tsemi) {
            self.state = ParserError.error_uxd_tok;
            return error.UnexpectedToken;
        }

        self.lexer = self.lexer.catch_up(lsq);
        const end = try self.parse_expr();
        if (end == null) {
            return error.AllocError;
        }

        const semi2 = self.lexer.consume();
        if (semi2.tid != Tokens.Tsemi) {
            self.state = ParserError.error_uxd_tok;
            return error.UnexpectedToken;
        }

        self.lexer = self.lexer.catch_up(lsq);
        const epsilon = try self.parse_expr();
        if (epsilon == null) {
            return error.AllocError;
        }

        const rsq = self.lexer.consume();
        if (rsq.tid != Tokens.Trsqb) {
            self.state = ParserError.error_uxd_tok;
            return error.UnexpectedToken;
        }

        self.lexer = self.lexer.catch_up(lsq);

        const range = try self.alloc.create(Node);
        range.* = Node{
            .position = new_position(lsq.pos, rsq.pos),
            .type = null,
            .nid = Nodes.NRange,
            .data = .{
                .range = .{
                    .start = start,
                    .end = end,
                    .epsilon = epsilon,
                },
            },
        };
        return range;
    }

    fn parse_prod(self: *Parser) ParserError!*Node {
        const ctor = self.lexer.consume();
        if (ctor.tid != Tokens.Tref) {
            self.state = ParserError.error_uxd_tok;
            return error.UnexpectedToken;
        }

        self.lexer = self.lexer.catch_up(ctor);

        const open = self.lexer.consume();
        if (open.tid != Tokens.Tlcur) {
            const node = try self.create_ref_node(ctor.pos, ctor.str);
            if (node == null) {
                self.state = ParserError.error_alloc;
                return error.AllocError;
            }
            return node;
        }

        const intr = try self.parse_intr(.intr_no_app);
        const children = intr.data.intr.exprs;
        const child_count = intr.data.intr.size;

        intr.data.aggr.children = children;
        intr.data.aggr.size = child_count;
        intr.data.aggr.name = ctor.str;
        intr.nid = Nodes.NAggr;
        intr.position = new_position(ctor.pos, intr.position);
        return intr;
    }

    fn parse_statement(self: *Parser) !*Node {
        const name = try self.lexer.consume();
        if (name.tid != Tokens.Tref) {
            self.pos = name.pos;
            return error.UnexpectedStartOfDeclaration;
        }

        const start = name.pos;
        self.lexer = self.lexer.catch_up(name);

        var params: [8]*Node = undefined;
        var pidx: usize = 0;
        // hard cap on 8 parameters per function
        while (is_atomic((try self.lexer.consume()).tid) or (try self.lexer.consume()).sameId(Tokens.Tlpar)) : (pidx += 1) {
            params[pidx] = try parse_primary(self);
        }

        var type_: ?*Node = null;
        var expr: ?*Node = null;
        const colon = try self.lexer.consume();
        if (colon.tid == Tokens.Tcln) {
            self.lexer = self.lexer.catch_up(colon);
            type_ = try self.parse_expr();
        }

        const equal = try self.lexer.consume();
        if (equal.tid == Tokens.Tequ) {
            self.lexer = self.lexer.catch_up(equal);
            expr = try self.parse_expr();
        }

        const pos = new_position(start, self.pos);
        const act_params = try self.alloc.alloc(*Node, pidx);

        @memcpy(act_params, params[0..pidx]);

        return try self.create_expr_node(pos, name.val.str, act_params, type_, expr);
    }

    pub fn parse_node(self: *Parser) ParserError!*Node {
        const token = try self.lexer.consume();

        return switch (token.tid) {
            Tokens.Tmod => self.parse_module(),
            Tokens.Tref => self.parse_statement(),
            else => {
                self.pos = token.pos;
                return error.UnexpectedStartOfDeclaration;
            },
        };
    }

    pub fn init(code: []const u8, alloc: std.mem.Allocator) Parser {
        return Parser{
            .code = code,
            .pos = Pos.new(),
            .alloc = alloc,
            .lexer = Lexer.init(code),
        };
    }
};

////////////////////////////////////////////////////////////////////////
// Optimizations
////////////////////////////////////////////////////////////////////////

// 1. ssa (because its functional we a-normalize)
// 2. scp (basically dco - cse)
// 3. beta applications (if seen useful)
// 4. monomorphization (no runtime reflection)

fn opWeight(node: *Node) usize {
    switch (node.data) {
        .int, .dec => 1,
        .str => 2,
        .ref => 4,
        .unr => |v| 1 + opWeight(v.val),
        .bin => |v| 3 + opWeight(v.lhs) + opWeight(v.rhs),
        else => 0,
    }
}

////////////////////////////////////////////////////////////////////////
// Interpreter
////////////////////////////////////////////////////////////////////////

pub const Context = struct {
    parent: ?*Context = null,
    members: std.StringHashMap(Type),

    pub fn init(alloc: std.mem.Allocator) Context {
        return .{
            .parent = null,
            .members = std.StringHashMap(Type).init(alloc),
        };
    }

    pub fn get(self: Context, name: []const u8) ?Type {
        return self.members.get(name) orelse if (self.parent) |p| p.get(name) else null;
    }
};

pub const Interpreter = struct {};
