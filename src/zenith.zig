const std = @import("std");
const misc = @import("misc.zig");

pub const String = []const u8;

////////////////////////////////////////////////////////////////////////
// Lexer                                                              //
////////////////////////////////////////////////////////////////////////

/// Possible token types
pub const Tokens = enum {
    Tnone,
    Tint,
    Tdec,
    Tstr,
    Tref,
    Tmod,
    Tmatch,
    Tlpar,
    Trpar,
    Tlcur,
    Trcur,
    Tlsqb,
    Trsqb,
    Tequ,
    Tsemi,
    Tplus,
    Tmin,
    Tstar,
    Tbar,
    Tamp,
    Ttil,
    Tbang,
    Thash,
    Tpipe,
    That,
    Tper,
    Tlsh,
    Trsh,
    Tcequ,
    Tcneq,
    Tcgt,
    Tcge,
    Tclt,
    Tcle,
    Tand,
    Tor,
    Tques,
    Tcln,
    Tdot,
    Tarrow,
};

pub const LexerError = error{ MalformedNumber, MalformedIdentifier, MalformedToken, MalformedString, LexerEnd };

/// Assign a position to every token/node, a position is only a view
/// over the code string, so
pub const Pos = struct { index: usize = 0, span: usize = 0 };

pub const Token = struct {
    tid: Tokens,
    pos: Pos = Pos{},
    val: union { int: i64, dec: f64, str: String },

    pub fn init(tid: Tokens, pos: Pos) Token {
        return Token{ .tid = tid, .pos = pos, .val = .{ .int = 0 } };
    }

    pub fn same(self: Token, other: Token) bool {
        const id = self.tid == other.tid;

        return switch (self.tid) {
            Tokens.Tint, Tokens.Tref => id and self.val.int == other.val.int,
            Tokens.Tdec => id and self.val.dec == other.val.dec,
            Tokens.Tstr => id and std.mem.eql(u8, self.val.str, other.val.str),
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

pub const Lexer = struct {
    code: String,
    index: usize,
    last: ?Token,

    pub fn init(code: String) Lexer {
        return Lexer{ .code = code, .index = 0, .last = null };
    }

    pub fn end(self: Lexer) bool {
        return self.code.len <= self.index;
    }

    pub fn current(self: Lexer) LexerError!u8 {
        return if (self.end()) LexerError.LexerEnd else self.code[self.index];
    }

    pub fn char(self: Lexer, c: u8) bool {
        return if (self.end()) false else self.code[self.index] == c;
    }

    fn isAlpha(self: Lexer) bool {
        return if (self.end()) false else std.ascii.isAlphabetic(self.code[self.index]);
    }

    fn isNumeric(self: Lexer) bool {
        return if (self.end()) false else std.ascii.isDigit(self.code[self.index]);
    }

    fn isAlphanumeric(self: Lexer) bool {
        return if (self.end()) false else std.ascii.isAlphanumeric(self.code[self.index]);
    }

    fn walk(self: Lexer) Lexer {
        return Lexer{ .code = self.code, .index = self.index + @intFromBool(!self.end()), .last = self.last };
    }

    fn consumeWhitespace(self: Lexer) Lexer {
        var lex = self;
        while (lex.char(' ') or lex.char('\n') or lex.char('\t') or lex.char('\r'))
            lex = lex.walk();
        return lex;
    }

    fn skipComments(self: Lexer) Lexer {
        var str = self.code[self.index..];
        var lex = self;
        while (str.prefix("//")) : (str = lex.code[lex.index..]) {
            while (lex.char('\n'))
                lex = lex.walk();
            lex = lex.walk();
        }
        return lex.consumeWhitespace();
    }

    fn consumeString(self: Lexer) LexerError!Token {
        var lex = self.walk();
        const start = lex.index;
        while (!lex.char('"') and !lex.end()) 
            lex = lex.walk();
        
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
        
        lex = lex.walk();

        while (lex.isAlphanumeric() or lex.char('_') or lex.char('\'')) 
            lex = lex.walk();

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

        while (lex.isNumeric()) 
            lex = lex.walk();

        if (lex.char('.')) {
            curr = Tokens.Tdec;
            lex = lex.walk();
            while (lex.isNumeric()) 
                lex = lex.walk();
        }

        if (lex.char('e')) {
            curr = Tokens.Tdec;
            lex = lex.walk();
            if (lex.char('+') or lex.char('-'))
                lex = lex.walk();
        
            if (!std.ascii.isDigit(try lex.current()))
                return LexerError.MalformedNumber;

            while (lex.isNumeric())
                lex = lex.walk();
        }
        const rpos = Pos{ .index = start, .span = lex.index - start };

        const str = lex.code[start .. start + rpos.span];

        if (curr == Tokens.Tint)
            return try Lexer.parseInt(str, rpos);

        return try Lexer.parseDec(str, rpos);
    }

    fn parseInt(str: String, pos: Pos) LexerError!Token {
        var result: i64 = 0;
        var idx: usize = 0;

        while (idx < str.len) : (idx += 1)
            result = result * 10 + (str[idx] - '0');

        return Token{ .tid = Tokens.Tint, .pos = pos, .val = .{ .int = result } };
    }

    fn parseDec(str: String, pos: Pos) LexerError!Token {
        var result: f64 = 0;
        var exp: f64 = 0;
        var sign: f64 = 1;
        var idx: usize = 0;

        while (idx < str.len and str[idx] != '.' and std.ascii.toLower(str[idx]) != 'e') : (idx += 1) {
            const digit = @as(f64, @floatFromInt(str[idx] - '0'));
            const r1 = @mulAdd(f64, result, 10, digit);
            result = r1;
        }

        if (idx < str.len and str[idx] == '.') {
            idx += 1;
            const s = idx;
            while (idx < str.len and std.ascii.isDigit(str[idx])) : (idx += 1) {
                const digit = @as(f64, @floatFromInt(str[idx] - '0'));

                const expon: isize = @as(isize, @intCast(s - idx)) - 1;
                const res = std.math.pow(f64, 10, @floatFromInt(expon));
                result = @mulAdd(f64, res, digit, result);
            }
        }

        if (idx < str.len and std.ascii.toLower(str[idx]) == 'e') {
            idx += 1;
            if (idx != str.len and !std.ascii.isDigit(str[idx])) { // for + or -
                sign = if (str[idx] == '-') -1 else 1;
                idx += 1;
            }
            while (idx < str.len) : (idx += 1) {
                const digit = @as(f64, @floatFromInt(str[idx] - '0'));
                const r1 = @mulAdd(f64, exp, 10, digit);
                exp = r1;
            }
        }
        return Token{ .tid = Tokens.Tdec, .pos = pos, .val = .{ .dec = result * std.math.pow(f64, 10, sign * exp) } };
    }

    pub fn consume(self: *Lexer) LexerError!Token {
        if (self.last) |last| return last;

        var lex = self.consumeWhitespace();

        if (lex.isNumeric()) {
            self.last = try lex.consumeNumber();
            return self.last.?;
        }

        if (lex.char('"')) {
            self.last = try lex.consumeString();
            return self.last.?;
        }

        const str = lex.code[lex.index..];

        if (std.mem.startsWith(u8, str, "module")) {
            const tok = Token.init(Tokens.Tmod, Pos{ .index = lex.index, .span = 6 });
            self.last = tok;
            return tok;
        }

        if (std.mem.startsWith(u8, str, "match")) {
            const tok = Token.init(Tokens.Tmatch, Pos{ .index = self.index, .span = 5 });
            self.last = tok;
            return tok;
        }

        if (lex.isAlpha() or lex.char('_')) {
            self.last = try lex.consumeIdentifier();
            return self.last.?;
        }

        const pos = Pos{ .index = lex.index, .span = 1 };
        const curr = try lex.current();
        lex = lex.walk();

        const tok: ?Token = switch (curr) {
            '(' => Token.init(Tokens.Tlpar, pos),
            ')' => Token.init(Tokens.Trpar, pos),
            '{' => Token.init(Tokens.Tlcur, pos),
            '}' => Token.init(Tokens.Trcur, pos),
            '[' => Token.init(Tokens.Tlsqb, pos),
            ']' => Token.init(Tokens.Trsqb, pos),
            '=' => lex.generate('=', Tokens.Tequ, Tokens.Tcequ),
            '+' => Token.init(Tokens.Tplus, pos),
            '-' => lex.generate('>', Tokens.Tmin, Tokens.Tarrow),
            '*' => Token.init(Tokens.Tstar, pos),
            '/' => Token.init(Tokens.Tbar, pos),
            '&' => lex.generate('&', Tokens.Tamp, Tokens.Tand),
            '~' => Token.init(Tokens.Ttil, pos),
            '!' => lex.generate('=', Tokens.Tbang, Tokens.Tcneq),
            '#' => Token.init(Tokens.Thash, pos),
            '|' => lex.generate('|', Tokens.Tpipe, Tokens.Tor),
            '%' => Token.init(Tokens.Tper, pos),
            '<' => lex.doubleGenerate('=', '<', Tokens.Tcle, Tokens.Tlsh, Tokens.Tclt),
            '>' => lex.doubleGenerate('=', '>', Tokens.Tcge, Tokens.Trsh, Tokens.Tcgt),
            '?' => Token.init(Tokens.Tques, pos),
            ':' => Token.init(Tokens.Tcln, pos),
            ';' => Token.init(Tokens.Tsemi, pos),
            else => null,
        };

        self.last = tok;

        if (tok) |t| 
            return t;
        return LexerError.MalformedToken;
    }

    fn generate(self: Lexer, chr: u8, small: Tokens, big: Tokens) Token {
        const p1 = Pos{ .index = self.index - 1, .span = 1 };
        const p2 = Pos{ .index = self.index - 1, .span = 2 };

        if (self.char(chr)) 
            return Token.init(big, p2);
        return Token.init(small, p1);
    }

    fn doubleGenerate(self: Lexer, c1: u8, c2: u8, t1: Tokens, t2: Tokens, telse: Tokens) Token {
        const p1 = Pos{ .index = self.index - 1, .span = 1 };
        const p2 = Pos{ .index = self.index - 1, .span = 2 };

        if (self.char(c1)) return Token.init(t1, p2);        
        if (self.char(c2)) return Token.init(t2,  p2);
        return Token.init(telse, p1);
    }

    pub fn catch_up(self: Lexer, token: Token) Lexer {
        return Lexer{
            .code = self.code,
            .index = token.pos.index + token.pos.span,
            .last = null
        };
    }
};

////////////////////////////////////////////////////////////////////////
// Types                                                              //
////////////////////////////////////////////////////////////////////////

pub const TypeError = error{ OutOfMemory, // allocator
UndefinedOperation, PossibleDivisionByZero, DivisionByZero, InvalidComposition, ArraySizeMismatch, ParameterMismatch, UnknownParameter, DisjointTypes };

pub const Type = struct {
    data: union(enum) {
        boolean: ?bool,
        integer: struct { start: i64, end: i64, value: ?i64 },
        decimal: struct { start: f64, end: f64, value: ?f64 },
        array: struct { type_: *Type, size: i64, value: []?Type },
        aggregate: struct { types: []Type, indexes: []i64, values: []?Type },
        function: struct { argument: *Type, ret: *Type, body: ?*Node = null },
        casting: Casting,
    },
    pointerIndex: u8 = 0,

    pub const zeroInt = Type{ .data = .{ .integer = .{ .start = 0, .end = 0, .value = 0 } } };
    pub const negOneInt = Type{ .data = .{ .integer = .{ .start = -1, .end = -1, .value = -1 } } };
    pub const zeroDec = Type{ .data = .{ .decimal = .{ .start = 0, .end = 0, .value = 0 } } };

    pub fn isPointer(self: Type) bool {
        return self.pointerIndex > 0;
    }

    pub fn isFunction(self: Type) bool {
        return self.data == .function;
    }

    pub fn isCasting(self: Type) bool {
        return self.data == .casting;
    }

    pub fn initInt(start: i64, end: i64, value: ?i64) Type {
        return Type{ .data = .{ .integer = .{ .start = start, .end = end, .value = value } } };
    }

    pub fn initFloat(start: f64, end: f64, value: ?f64) Type {
        return Type{ .data = .{ .decimal = .{ .start = start, .end = end, .value = value } } };
    }

    pub fn initBool(value: ?bool) Type {
        return Type{ .data = .{ .boolean = value } };
    }

    pub fn deinit(self: Type, allocator: std.mem.Allocator) void {
        return switch (self.data) {
            .boolean => {},
            .integer => {},
            .decimal => {},
            .array => |v| {
                v.type_.deinit(allocator);
                for (v.value) |elem| 
                    if (elem != null) elem.?.deinit(allocator);
                
                allocator.free(v.value);
                allocator.destroy(v.type_);
            },
            .aggregate => |v| {
                for (v.values) |elem| 
                    if (elem != null) elem.?.deinit(allocator);
                
                allocator.free(v.values);
            },
            .function => |v| {
                v.argument.deinit(allocator);
                v.ret.deinit(allocator);
            },
            .casting => {},
        };
    }

    pub fn isTag(self: Type, tag: @TypeOf(std.meta.activeTag(self.data))) bool {
        return std.meta.activeTag(self.data) == tag;
    }

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

    pub fn makePtr(v: Type) Type {
        var other = v;
        other.pointerIndex += 1;
        return other;
    }

    // synthUnary : operator + inner type -> outer type

    pub fn synthUnary(alloc: std.mem.Allocator, op: Tokens, v: Type) !Type {
        _ = alloc;
        if (op == Tokens.Tamp) return makePtr(v);
        if (op == Tokens.Tplus) return v;

        if (v.isPointer()) 
            return switch (op) {
                .Tstar => {
                    var other = v;
                    other.pointerIndex -= 1;
                    return other;
                },
                else => TypeError.UndefinedOperation,
            };
        return switch (v.data) {
            .integer => switch (op) {
                .Tmin => handleInt(op, Type.zeroInt, v),
                .Ttil => handleInt(.Tmin, Type.negOneInt, v),
                else => TypeError.UndefinedOperation,
            },
            .decimal => switch (op) {
                .Tplus => v,
                .Tmin => handleFloat(op, Type.zeroDec, v),
                else => TypeError.UndefinedOperation,
            },
            .boolean => switch (op) {
                .Tbang => {
                    if (v.data.boolean) |vbool|
                        return initBool(!vbool);
                    return initBool(null);
                },
                else => TypeError.UndefinedOperation,
            },
            else => TypeError.UndefinedOperation,
        };
    }

    pub fn checkUnary(alloc: std.mem.Allocator, op: Tokens, v: Type, res: Type) !Type {
        const synth = try synthUnary(alloc, op, v);
        return Type.join(alloc, synth, res);
    }

    pub fn callFunction(f: Type, args: Type) TypeError!*Type {
        // todo: add casting branch
        if (!f.isFunction())
            return TypeError.UndefinedOperation;

        if (f.data.function.argument.deepEqual(args, false))
            return f.data.function.ret;

        return TypeError.ParameterMismatch;
    }

    pub fn binOperate(
        alloc: std.mem.Allocator,
        op: Tokens,
        a: Type,
        b: Type,
    ) TypeError!Type {
        if (std.meta.activeTag(a.data) != std.meta.activeTag(b.data)) 
            return TypeError.UndefinedOperation;
        
        if (a.pointerIndex != b.pointerIndex) 
            return TypeError.UndefinedOperation;
        
        if (a.pointerIndex > 0) {
            return switch (op) {
                .Tcequ, .Tcneq => binOperate(alloc, op, a, b),
                else => TypeError.UndefinedOperation,
            };
        }

        if (a.isFunction() and a.isFunction() and op == Tokens.Tdot) {
            if (a.data.function.ret != b.data.function.argument)
                return TypeError.InvalidComposition;

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
                    .Tcequ => binopt(bool, ba, bb, misc.equ),
                    .Tcneq => binopt(bool, ba, bb, misc.neq),
                    .Tand => binopt(bool, ba, bb, misc.land),
                    .Tor => binopt(bool, ba, bb, misc.land),
                    else => null,
                };

                if (res) |r| {
                    return initBool(r);
                } 
                return TypeError.UndefinedOperation;
            },
            .integer => try handleInt(op, a, b),
            .decimal => try handleFloat(op, a, b),
            .array => try handleArray(alloc, op, a, b),
            else => error.UndefinedOperation,
        };
    }

    fn minMax(comptime T: type, x1: T, x2: T, y1: T, y2: T, f: anytype) struct { T, T } {
        if (@typeInfo(T) == .Bool) {
            return .{ false, true };
        } else {
            const d1 = f(T, x1, y1);
            const d2 = f(T, x1, y2);
            const d3 = f(T, x2, y1);
            const d4 = f(T, x2, y2);

            const gmin = @min(@min(d1, d2), @min(d3, d4));
            const gmax = @max(@max(d1, d2), @max(d3, d4));
            return .{ gmin, gmax };
        }
    }

    fn binopt(comptime T: type, a: ?T, b: ?T, func: anytype) ?@TypeOf(func(T, a.?, b.?)) {
        if (a == null or b == null) return null;
        return func(T, a.?, b.?);
    }

    fn handleInt(op: Tokens, a: Type, b: Type) TypeError!Type {
        const ia = a.data.integer;
        const ib = b.data.integer;

        const closure = struct {
            fn doMinMax(func: anytype, ma: anytype, mb: anytype) Type {
                const minmax = minMax(i64, ma.start, ma.end, mb.start, mb.end, func);
                return initInt(minmax[0], minmax[1], binopt(i64, ma.value, mb.value, func));
            }
            fn doBoolMinmax(func: anytype, ma: anytype, mb: anytype) Type {
                return initBool(binopt(i64, ma.value, mb.value, func));
            }
        };

        return switch (op) {
            .Tplus => closure.doMinMax(misc.add, ia, ib),
            .Tmin => closure.doMinMax(misc.sub, ia, ib),
            .Tstar => closure.doMinMax(misc.mul, ia, ib),
            .Tbar => {
                if (ib.value != null and ib.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                const minmax = minMax(i64, ia.start, ia.end, ib.start, ib.end, misc.div);
                return initInt(minmax[0], minmax[1], binopt(i64, ia.value, ib.value, misc.div));
            },
            .Tper => {
                if (ib.value != null and ib.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                return initInt(0, ia.end - 1, binopt(i64, ia.value, ib.value, misc.mod));
            },
            .Tpipe => closure.doMinMax(misc.pipe, ia, ib),
            .Tamp => closure.doMinMax(misc.amp, ia, ib),
            .That => closure.doMinMax(misc.hat, ia, ib),
            .Tlsh => closure.doMinMax(misc.lsh, ia, ib),
            .Trsh => closure.doMinMax(misc.rsh, ia, ib),
            .Tcequ => closure.doBoolMinmax(misc.equ, ia, ib),
            .Tcneq => closure.doBoolMinmax(misc.neq, ia, ib),
            .Tcge => closure.doBoolMinmax(misc.gte, ia, ib),
            .Tcgt => closure.doBoolMinmax(misc.gt, ia, ib),
            .Tcle => closure.doBoolMinmax(misc.lte, ia, ib),
            .Tclt => closure.doBoolMinmax(misc.lt, ia, ib),
            else => error.UndefinedOperation,
        };
    }

    fn handleFloat(op: Tokens, a: Type, b: Type) TypeError!Type {
        const fa = a.data.decimal;
        const fb = b.data.decimal;
        const closure = struct {
            fn doMinMax(func: anytype, ma: anytype, mb: anytype) Type {
                const minmax = minMax(f64, ma.start, ma.end, mb.start, mb.end, func);
                return initFloat(minmax[0], minmax[1], binopt(f64, ma.value, mb.end, func));
            }
            fn doBoolMinmax(func: anytype, ma: anytype, mb: anytype) Type {
                return initBool(binopt(f64, ma.value, mb.value, func));
            }
        };

        return switch (op) {
            .Tplus => closure.doMinMax(misc.add, fa, fb),
            .Tmin => closure.doMinMax(misc.sub, fa, fb),
            .Tstar => closure.doMinMax(misc.mul, fa, fb),
            .Tbar => {
                if (fb.value != null and fb.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                const minmax = minMax(f64, fa.start, fa.end, fb.start, fb.end, misc.div);
                return initFloat(minmax[0], minmax[1], binopt(f64, fa.value, fb.value, misc.div));
            },
            .Tper => {
                if (fb.value != null and fb.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                return initFloat(0, fb.end - 1, binopt(f64, fa.value, fb.value, misc.mod));
            },
            .Tcequ => closure.doBoolMinmax(misc.equ, fa, fb),
            .Tcneq => closure.doBoolMinmax(misc.neq, fa, fb),
            .Tcge => closure.doBoolMinmax(misc.gte, fa, fb),
            .Tcgt => closure.doBoolMinmax(misc.gt, fa, fb),
            .Tcle => closure.doBoolMinmax(misc.lte, fa, fb),
            .Tclt => closure.doBoolMinmax(misc.lt, fa, fb),
            else => error.UndefinedOperation,
        };
    }

    fn scalarArray(allocator: std.mem.Allocator, op: Tokens, arr: Type, other: Type, reversed: bool) TypeError!Type {
        const arr_data = arr.data.array;
        var results = try allocator.alloc(?Type, arr_data.value.len);

        for (arr_data.value, 0..) |elem, i| {
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
                    .type_ = arr_data.type_,
                    .size = arr_data.size,
                    .value = results,
                },
            },
        };
    }

    fn handleArray(
        allocator: std.mem.Allocator,
        op: Tokens,
        a: Type,
        b: Type,
    ) TypeError!Type {
        if (a.isTag(.array) and b.isTag(.array)) {
            if (a.data.array.size != b.data.array.size) return TypeError.ArraySizeMismatch;
            const arr_a = a.data.array;
            const arr_b = b.data.array;
            var results = try allocator.alloc(?Type, arr_a.value.len);

            for (arr_a.value, arr_b.value, 0..) |a_elem, b_elem, i| {
                if (a_elem == null or b_elem == null) {
                    results[i] = null;
                    continue;
                }

                results[i] = try binOperate(allocator, op, a_elem.?, b_elem.?);
            }

            return Type{
                .data = .{
                    .array = .{
                        .type_ = arr_a.type_,
                        .size = arr_a.size,
                        .value = results,
                    },
                },
            };
        }

        if (a.isTag(.array) or b.isTag(.array))
            return scalarArray(allocator, op, a, b, b.isTag(.array));

        return TypeError.UndefinedOperation;
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
                if (!deepEqual(t1.data.array.type_.*, t2.data.array.type_.*, checkValued)) return false;
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
        t.* = switch (self.data) {
            .integer => initInt(self.data.integer.start, self.data.integer.end, self.data.integer.value),
            .decimal => initFloat(self.data.decimal.start, self.data.decimal.end, self.data.decimal.value),
            .boolean => initBool(self.data.boolean),
            .array => blk: {
                var new_type = self;
                new_type.data.array.type_ = try self.data.array.type_.deepCopy(alloc);
                var new_values = try alloc.alloc(?Type, self.data.array.value.len);
                for (self.data.array.value, 0..) |value, i| {
                    if (value == null) continue; 
                    const res = try value.?.deepCopy(alloc);
                    new_values[i] = res.*;
                    alloc.destroy(res); // only the top
                }
                new_type.data.array.value = new_values;
                break :blk new_type;
            },
            .aggregate => blk: {
                var new_type = self;
                const new_types = try alloc.alloc(Type, self.data.aggregate.types.len);
                for (self.data.aggregate.types,0..) |value, i| {
                    const res = try value.deepCopy(alloc);
                    new_types[i] = res.*;
                    alloc.destroy(res); 
                }
                new_type.data.aggregate.types = new_types;
                break :blk new_type;
            },
            .function => blk: {
                var new_type = self;
                new_type.data.function.argument = try self.data.function.argument.deepCopy(alloc);
                new_type.data.function.ret = try self.data.function.ret.deepCopy(alloc);

                break :blk new_type;
            },
            .casting => self,
        };

        return t;
    }

    pub fn instantiate(self: Type, instances: []const struct { String, u32, Type }) Type {
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
                const result = v.type_.instantiate(instances);
                if (!result.deepEqual(v.type_.*, false))
                    v.type_.* = result;

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

    pub fn join(alloc: std.mem.Allocator, t1: Type, t2: Type) !struct { []const Substitution, Type} {
        _ = alloc;

        if (t1.pointerIndex != t2.pointerIndex) return TypeError.DisjointTypes(t1, t2);

        return switch (t1.data) {
            .boolean => .{ .{}, t1}, 
            .integer => |v1| {
                if (t2.data != .integer)
                    return TypeError.DisjointTypes;
                
                const v2 = t2.data.integer;

                if (v1.value != null and v2.value != null and v1.value != v2.value)
                    return TypeError.UndefinedOperation;
                

                const result = initInt(@max(v1.start, v2.start), @min(v1.end, v2.end), if (v1.value != null) v1 else v2);

                const rint = result.data.integer;

                if (rint.start > result.data.integer.end) 
                    return TypeError.UndefinedOperation;
                

                if (rint.value) |rv| 
                    if (rint.start > rv or rv > rint.end) 
                        return TypeError.UndefinedOperation;

                return .{ .{}, result };
            },
            .decimal => |v1| {
                if (t2.data != .decimal)
                    return TypeError.UndefinedOperation;
                
                const v2 = t2.data.decimal;

                if (v1.value != null and v2.value != null and v1.value != v2.value)
                    return TypeError.UndefinedOperation;
                

                const result = initFloat(@max(v1.start, v2.start), @min(v1.end, v2.end), if (v1.value != null) v1 else v2);

                const rdec = result.data.decimal;

                if (rdec.start > rdec.end) 
                    return TypeError.UndefinedOperation;
                
                if (rdec.value) |rv|
                    if (rdec.start > rv or rv > rdec.end) 
                        return TypeError.UndefinedOperation;

                return .{ .{}, result };
            }
        };
    }

    pub fn meet(alloc: std.mem.Allocator, t1: Type, t2: Type) !?std.ArrayList(struct { String, Type }) {
        return switch (t1.data) {
            .casting => {
                if (std.mem.eql(u8, t1.data.casting, t2.data.casting)) {
                    const list = try std.ArrayList(struct { String, Type }).init(alloc);
                    try list.append(.{ t1.data.casting, t2 });
                    return list;
                }
                return null;
            },
            .array => {
                if (t2.data.array.size != t1.data.array.size) return null;
                return t1.data.array.type_.meet(alloc, t2.data.array.type_);
            },
            .aggregate => {
                if (t2.data.aggregate.types.len != t1.data.aggregate.types.len) return null;
                var list = try std.ArrayList(struct { String, Type }).init(alloc);
                for (t1.data.aggregate.types, t2.data.aggregate.types) |a, b| {
                    const met = try a.meet(alloc, b);
                    if (met == null) return null;
                    for (met) |m| 
                        try list.append(m);
                    
                }
                return list;
            },
            .function => {
                const arg_meet = try t1.data.function.argument.meet(alloc, t2.data.function.argument);
                const ret_meet = try t1.data.function.ret.meet(alloc, t2.data.function.ret);
                if (arg_meet == null or ret_meet == null) return null;
                var list = try std.ArrayList(struct { String, Type }).init(alloc);
                list.extend();
                for (arg_meet.?) |a| {
                    for (ret_meet.?) |r| {
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
    pub fn getParameter(self: Type, index: u8) !*Type {
        var typ = &self;
        var idx: u8 = 0;
        while (typ.isFunction()) : (idx += 1) {
            if (idx == index) return typ.data.function.argument;
            typ = typ.data.function.ret;
        }

        return TypeError.UnknownParameter;
    }

    pub fn isMeetable(self: Type, other: Type) bool {
        _ = self;
        _ = other;

        // todo: fast meet check types
        return true;
    }
};


////////////////////////////////////////////////////////////////////////
// Parser                                                             //
////////////////////////////////////////////////////////////////////////

pub const Node = struct {
    position: Pos = Pos{},
    ntype: ?*Type = null,
    data: union(enum) {
        none: void,
        type: void,
        int: i64,
        dec: f64,
        str: String,
        ref: String,
        unr: struct { val: *Node, op: Tokens },
        bin: struct { lhs: *Node, rhs: *Node, op: Tokens },
        ter: struct { cond: *Node, btrue: *Node, bfalse: *Node },
        call: struct { caller: *Node, callee: *Node },
        expr: struct { name: String, params: []*Node, ntype: ?*Node, expr: ?*Node },
        mod: struct { path: String, pairs: std.StringHashMap(*Node), direction: bool },
        range: struct { start: *Node, end: *Node, epsilon: *Node },
        aggr: struct { name: String, children: []*Node },
        sum: []*Node,
        intr: struct { intermediates: []*Node, application: *Node },
    },

    pub fn deinit(self: *Node, alloc: std.mem.Allocator) void {
        if (self.ntype) |ntype| {
            ntype.deinit(alloc);
            alloc.destroy(ntype);
        }

        switch (self.data) {
            .none => {},
            .type => {},
            .int => {},
            .dec => {},
            .str => {},
            .ref => {},
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
                for (v.params) |param| param.deinit(alloc);
                alloc.free(v.params);

                if (v.ntype) |nt| nt.deinit(alloc);

                if (v.expr) |expr| expr.deinit(alloc);
            },
            .mod => |v| {
                var it = v.pairs.iterator();
                while (it.next()) |pair| 
                    pair.value_ptr.*.deinit(alloc);

                @constCast(&v.pairs).*.deinit();
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
                for (v.intermediates) |inter|
                    inter.deinit(alloc);
                alloc.free(v.intermediates);
                v.application.deinit(alloc);
            },
        }

        alloc.destroy(self);
    }
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
    code: String,
    pos: Pos = Pos{},
    alloc: std.mem.Allocator,
    lexer: Lexer,

    fn new_position(start: Pos, end: Pos) Pos {
        return .{
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

    fn create_str_node(self: Parser, position: Pos, value: String) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .str = value },
        };
        return node;
    }

    fn create_ref_node(self: Parser, position: Pos, value: String) !*Node {
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

    fn create_expr_node(self: Parser, position: Pos, name: String, params: []*Node, type_: ?*Node, expr: ?*Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .expr = .{ .name = name, .params = params, .ntype = type_, .expr = expr } },
        };
        return node;
    }

    fn get_precedence(tid: Tokens) i32 {
        return switch (tid) {
            .Tor, .Tand => 1,
            .Tcequ, .Tcneq, .Tclt, .Tcle, .Tcgt, .Tcge => 2,
            .Tplus, .Tmin => 3,
            .Tstar, .Tbar, .Tper => 4,
            .Tlsh, .Trsh => 5,
            .Tpipe, .That, .Tamp => 6,
            .Thash, .Tdot => 7,
            else => -1,
        };
    }

    const IntrFlags = enum { no_app, normal, skip_lcur };

    fn parse_intr(self: *Parser, flags: IntrFlags) ParserError!*Node {
        const open = try self.lexer.consume();
        const start = open.pos;
        
        var intermediates: [16]*Node = undefined;
        var idx: usize = 0;

        const lcur = try self.lexer.consume();
        if (flags != .skip_lcur and lcur.notId(.Tlcur)){
            return ParserError.UnexpectedToken;
        }

        while (!(try self.lexer.consume()).sameId(Tokens.Trcur) and idx < 16) : (idx += 1) {
            intermediates[idx] = try parse_statement(self);
            const semi = try self.lexer.consume();
            if (semi.notId(Tokens.Tsemi))
                return ParserError.UnexpectedToken;
            self.lexer = self.lexer.catch_up(semi);
        }

        const rcur = try self.lexer.consume();

        if (rcur.notId(Tokens.Trcur))
            return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(rcur);

        // we use less memory if we reallocate for every idx < 15
        const actual_imm = try self.alloc.alloc(*Node, idx);

        for (0..idx) |i|
            actual_imm[i] = intermediates[i];
        

        if (flags == IntrFlags.no_app) {
            const node = try self.alloc.create(Node);
            node.* = Node{ 
                .position = new_position(start, rcur.pos),
                .data = .{ 
                    .intr = .{ 
                        .intermediates = actual_imm,
                        .application = undefined
                    }
                }
            };

            return node;
        }

        const application = try self.parse_primary();

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

        if (direction.tid != Tokens.Tlsh and direction.tid != Tokens.Trsh)
            return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(direction);

        const name = try self.lexer.consume();

        if (name.tid != Tokens.Tstr)
            return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(name);

        const lcur = try self.lexer.consume();
        if (lcur.tid != Tokens.Tlcur)
            return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(lcur);

        var pairs = std.StringHashMap(*Node).init(self.alloc);
        errdefer pairs.deinit();

        var open = try self.lexer.consume();
        while (open.notId(Tokens.Trcur)) : (open = try self.lexer.consume()) {
            const key = try self.lexer.consume();
            if (key.tid != Tokens.Tstr and key.tid != Tokens.Tref)
                return ParserError.UnexpectedToken;

            self.lexer = self.lexer.catch_up(key);

            const colon = try self.lexer.consume();
            if (colon.tid != Tokens.Tcln)
                return ParserError.UnexpectedToken;

            self.lexer = self.lexer.catch_up(colon);

            const value = try self.parse_expr();
            errdefer value.deinit(self.alloc);

            try pairs.put(key.val.str, value);

            const semi = try self.lexer.consume();
            if (semi.tid != Tokens.Trcur and semi.tid != Tokens.Tsemi)
                return ParserError.UnexpectedToken;

            self.lexer = self.lexer.catch_up(semi);
        }

        if (open.tid != Tokens.Trcur)
            return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(open);

        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = new_position(start, open.pos),
            .data = .{
                .mod = .{
                    .path = name.val.str,
                    .pairs = pairs,
                    .direction = direction.tid == Tokens.Tlsh,
                },
            },
        };
        return node;
    }

    fn parse_primary(self: *Parser) ParserError!*Node {
        const token = try self.lexer.consume();
        self.lexer = self.lexer.catch_up(token);
        return switch (token.tid) {
            Tokens.Tint => try self.create_int_node(token.pos, token.val.int),
            Tokens.Tdec => try self.create_dec_node(token.pos, token.val.dec),
            Tokens.Tstr => try self.create_str_node(token.pos, token.val.str),
            Tokens.Tref => try self.create_ref_node(token.pos, token.val.str),
            Tokens.Tlpar => {
                const expr = try self.parse_expr();
                errdefer expr.deinit(self.alloc);

                const rpar = try self.lexer.consume();
                if (rpar.tid != Tokens.Trpar)
                    return ParserError.UnclosedParen;

                self.lexer = self.lexer.catch_up(rpar);
                expr.position = new_position(token.pos, rpar.pos);
                return expr;
            },
            Tokens.Tlcur => try self.parse_intr(IntrFlags.skip_lcur),
            else => {
                self.pos = token.pos;
                return ParserError.UnexpectedToken;
            },
        };
    }

    fn is_atomic(tid: Tokens) bool {
        return switch (tid) {
            .Tint, .Tdec, .Tref, .Tstr, .Tlpar => true,
            else => false,
        };
    }

    fn parse_call(self: *Parser) ParserError!*Node {
        var caller = try self.parse_primary();
        errdefer caller.deinit(self.alloc);

        while (true) {
            const token = try self.lexer.consume();
            if (!is_atomic(token.tid) or token.tid == Tokens.Tsemi) 
                return caller;

            const callee = try self.parse_primary();
            errdefer callee.deinit(self.alloc);

            caller = try self.create_call_node(token.pos, caller, callee);
            errdefer caller.deinit(self.alloc);
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

        return try self.create_unr_node(token.pos, token.tid, operand);
    }

    fn parse_binary(self: *Parser, precedence: i32) ParserError!*Node {
        var left = try self.parse_unary();
        errdefer left.deinit(self.alloc);

        while (true) {
            const token = try self.lexer.consume();
            const token_precedence = get_precedence(token.tid);
            if (token_precedence < precedence) 
                return left;

            self.lexer = self.lexer.catch_up(token);
            const right = try self.parse_binary(token_precedence + 1);
            errdefer right.deinit(self.alloc);

            left = try self.create_bin_node(token.pos, token.tid, left, right);
            errdefer left.deinit(self.alloc);
        }
    }

    fn parse_ternary(self: *Parser) ParserError!*Node {
        const condition = try self.parse_binary(1);
        errdefer condition.deinit(self.alloc);

        const question = try self.lexer.consume();
        if (question.tid != Tokens.Tques) return condition;
        
        self.lexer = self.lexer.catch_up(question);

        const then_expr = try self.parse_binary(1);
        errdefer then_expr.deinit(self.alloc);

        const colon = try self.lexer.consume();
        if (colon.tid != Tokens.Tcln) 
            return ParserError.UnexpectedColon;
        
        self.lexer = self.lexer.catch_up(colon);

        const else_expr = try self.parse_ternary();
        errdefer else_expr.deinit(self.alloc);

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
        const lsq = try self.lexer.consume();
        if (lsq.tid != Tokens.Tlsqb) return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(lsq);
        const start = try self.parse_expr();
        errdefer start.deinit(self.alloc);

        const semi = try self.lexer.consume();
        if (semi.tid != Tokens.Tsemi) return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(semi);
        const end = try self.parse_expr();
        errdefer end.deinit(self.alloc);

        const semi2 = try self.lexer.consume();
        if (semi2.tid != Tokens.Tsemi) return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(semi2);
        const epsilon = try self.parse_expr();
        errdefer epsilon.deinit(self.alloc);
        
        const rsq = try self.lexer.consume();

        if (rsq.tid != Tokens.Trsqb) return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(rsq);

        const range = try self.alloc.create(Node);
        range.* = Node{
            .position = new_position(lsq.pos, rsq.pos),
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

    fn parse_tprimary(self: *Parser) ParserError!*Node {
        const tok = try self.lexer.consume();
        return switch (tok.tid) {
            Tokens.Tlsqb => self.parse_range(),
            Tokens.Tref => self.parse_prod(),
            Tokens.Tstar => |v| {
                self.lexer = self.lexer.catch_up(tok);
                const val = try self.parse_tprimary();
                const ptr = try self.alloc.create(Node);
                errdefer self.alloc.destroy(ptr);
                ptr.* = Node{ .position = new_position(tok.pos, val.position), .data = .{ .unr = .{ .op = v, .val = ptr } } };
                return ptr;
            },

            else => ParserError.MalformedToken,
        };
    }

    fn get_tprecedence(tok: Tokens) i3 {
        return switch (tok) {
            Tokens.Tarrow => 1,
            Tokens.Thash => 2,
            else => -1,
        };
    }

    fn parse_tbinary(self: *Parser, prec: i3) ParserError!*Node {
        var left = try self.parse_tprimary();
        errdefer left.deinit(self.alloc);

        while (true) {
            const token = try self.lexer.consume();
            const token_precedence = get_tprecedence(token.tid);
            if (token_precedence < prec) 
                return left;            

            self.lexer = self.lexer.catch_up(token);
            const right = try self.parse_tbinary(token_precedence + 1);
            errdefer right.deinit(self.alloc);

            left = try self.create_bin_node(token.pos, token.tid, left, right);
            errdefer left.deinit(self.alloc);
        }
    }

    fn parse_sum(self: *Parser) ParserError!*Node {
        const member = try self.parse_tbinary(1);
        errdefer member.deinit(self.alloc);

        var array = std.ArrayList(*Node).init(self.alloc);
        errdefer {
            for (array.items) |item|
                item.deinit(self.alloc);
            array.deinit();
        }

        try array.append(member);

        while (true) {
            const token = try self.lexer.consume();
            if (token.notId(Tokens.Tbar))
                break;

            self.lexer = self.lexer.catch_up(token);
            try array.append(try self.parse_tbinary(0));
        }

        if (array.items.len == 1) {
            array.deinit();
            return member;
        }

        const sum = try self.alloc.create(Node);
        sum.* = .{
            .data = .{ .sum = array.items },
            .position = new_position(member.position, array.getLast().position),
        };

        return sum;
    }

    fn parse_prod(self: *Parser) ParserError!*Node {
        const ctor = try self.lexer.consume();
        const start = ctor.pos;
        if (ctor.tid != Tokens.Tref) return ParserError.UnexpectedToken;

        self.lexer = self.lexer.catch_up(ctor);

        const open = try self.lexer.consume();
        if (open.tid != Tokens.Tlcur)
            return try self.create_ref_node(ctor.pos, ctor.val.str);

        self.lexer = self.lexer.catch_up(open);

        var array = std.ArrayList(*Node).init(self.alloc);

        var closed = try self.lexer.consume();
        while (closed.notId(Tokens.Trcur)) {
            try array.append(try self.parse_type());
        }

        self.lexer = self.lexer.catch_up(closed);
        const prod = try self.alloc.create(Node);
        prod.* = Node{ .position = new_position(start, closed.pos), .data = .{ .aggr = .{ .name = ctor.val.str, .children = array.items } } };

        return prod;
    }

    fn parse_type(self: *Parser) ParserError!*Node {
        return parse_sum(self);
    }

    fn parse_statement(self: *Parser) !*Node {
        const name = try self.lexer.consume();
        if (name.tid != Tokens.Tref) {
            self.pos = name.pos;
            return ParserError.UnexpectedStartOfDeclaration;
        }

        const start = name.pos;
        self.lexer = self.lexer.catch_up(name);

        var params: [8]*Node = undefined;
        var pidx: usize = 0;
        while (is_atomic((try self.lexer.consume()).tid) or (try self.lexer.consume()).sameId(Tokens.Tlpar)) : (pidx += 1) {
            params[pidx] = try parse_primary(self);
        }

        var type_: ?*Node = null;
        var expr: ?*Node = null;
        const colon = try self.lexer.consume();
        if (colon.tid == Tokens.Tcln) {
            self.lexer = self.lexer.catch_up(colon);
            type_ = try self.parse_type();
        }

        const equal = try self.lexer.consume();
        if (equal.tid == Tokens.Tequ) {
            self.lexer = self.lexer.catch_up(equal);
            expr = try self.parse_expr();
        }

        if (type_ == null and expr == null) 
            return ParserError.MalformedToken;

        const end_pos = (expr orelse type_).?.position;

        const pos = new_position(start, end_pos);
        const act_params = try self.alloc.alloc(*Node, pidx);
        errdefer {
            for (params[0..pidx]) |param|
                param.deinit(self.alloc);
            self.alloc.free(act_params);
        }

        @memcpy(act_params, params[0..pidx]);

        return try self.create_expr_node(pos, name.val.str, act_params, type_, expr);
    }

    pub fn parse_node(self: *Parser) ParserError!*Node {
        const token = try self.lexer.consume();

        return switch (token.tid) {
            .Tmod => self.parse_module(),
            .Tref => self.parse_statement(),
            else => {
                self.pos = token.pos;
                return error.UnexpectedStartOfDeclaration;
            },
        };
    }

    pub fn init(code: String, alloc: std.mem.Allocator) Parser {
        return Parser{
            .code = code,
            .alloc = alloc,
            .lexer = Lexer.init(code),
        };
    }
};

////////////////////////////////////////////////////////////////////////
// Semantic Analyzer                                                  //
////////////////////////////////////////////////////////////////////////

const AnalyzerOnlyError = error{ UndefinedReference, NonBooleanDecision, RequiresValue, TypeMismatch, ImpossibleUnification };

pub const AnalyzerError = ParserError || TypeError || AnalyzerOnlyError;

pub fn Context(comptime T: type) type {
    return struct {
        const ctxt = @This();

        parent: ?*ctxt = null,
        alloc: std.mem.Allocator,
        members: std.StringHashMap(T),
        children: std.StringHashMap(*ctxt),


        pub fn init(alloc: std.mem.Allocator) !*ctxt {
            const ctx = try alloc.create(ctxt);
            ctx.* = .{
                .parent = null,
                .alloc = alloc,
                .members = std.StringHashMap(T).init(alloc),
                .children = std.StringHashMap(*ctxt).init(alloc),
            };
            return ctx;
        }

        pub fn initTree(alloc: std.mem.Allocator, path: String) !*ctxt {
            const ctx = try ctxt.init(alloc);
            const parts = std.mem.split(u8, path, "/");
            var current_ctx = ctx;

            while (parts.next()) |part| {
                var sub_ctx = current_ctx.children.get(part);
                if (sub_ctx == null) {
                    sub_ctx = try ctxt.init(alloc);
                    sub_ctx.parent = current_ctx.?;
                    try current_ctx.children.put(part, sub_ctx);
                }
                current_ctx = sub_ctx.?;
            }

            return ctx;
        }

        pub fn getTree(self: *ctxt, path: String, name: String) ?T {
            var parts = std.mem.split(u8, path, "/");
            var current_ctx = self;

            while (parts.next()) |part| {
                const sub_ctx = current_ctx.children.get(part);
                if (sub_ctx == null) {
                    return null;
                }
                current_ctx = sub_ctx.?;
            }

            return current_ctx.get(name);
        }

        pub fn addTree(self: *ctxt, path: String, name: String, value: T) !void {
            var parts = std.mem.split(u8, path, "/");
            var current_ctx = self;

            while (parts.next()) |part| {
                var sub_ctx = current_ctx.children.get(part);
                if (sub_ctx == null) {
                    sub_ctx = try ctxt.init(self.alloc);
                    sub_ctx.?.parent = current_ctx;
                    try current_ctx.children.put(part, sub_ctx.?);
                }
                current_ctx = sub_ctx.?;
            }

            try current_ctx.add(name, value);
        }

        pub fn deinit(self: *ctxt, alloc: std.mem.Allocator) void {
            var it = self.members.iterator();
            if (std.meta.hasMethod(T, "deinit")) {
                while (it.next()) |pair| {
                    pair.value_ptr.*.deinit(alloc);
                    alloc.destroy(pair.value_ptr.*);
                }
            }

            var it2 = self.children.iterator();
            while (it2.next()) |pair| {
                pair.value_ptr.*.deinit(alloc);
            }

            @constCast(&self.members).*.deinit();
            @constCast(&self.children).*.deinit();
            alloc.destroy(self);
        }

        pub fn get(self: ctxt, name: String) ?T {
            return self.members.get(name) orelse if (self.parent) |p| p.get(name) else null;
        }

        pub fn add(self: *ctxt, name: String, value: T) !void {
            return try self.members.put(name, value);
        }

        pub fn isEmpty(self: ctxt) bool {
            return self.members.count() == 0 and (self.parent == null or self.parent.?.isEmpty());
        }
    };
}



const Casting = union(enum) {
    name: String,
    index: u32,

    pub fn eql(self: Casting, other: Casting) bool {
        if (std.meta.activeTag(self) != std.meta.activeTag(other))
            return false;

        return switch (self) {
            .name => std.mem.eql(u8, self.name, other.name),
            .index => self.index == other.index,
        };
    }
};

const Substitution = struct {
    from: Type,
    to: Type,
};

const InferenceContext = struct {
    alloc: std.mem.Allocator,
    subs: std.ArrayList(Substitution),
    counter: u32 = 0,

    pub fn init(alloc: std.mem.Allocator) InferenceContext {
        return .{
            .alloc = alloc,
            .subs = std.ArrayList(Substitution).init(alloc),
        };
    }

    pub fn freshTypeVar(self: *InferenceContext) !Type {
        const index = self.counter;
        self.counter += 1;
        
        const type_var = try self.alloc.create(Type);
        type_var.* = Type{
            .pointerIndex = 0,
            .data = .{ .casting = .{ .index = index } },
        };
        return type_var;
    }

    pub fn applySubstitution(self: *InferenceContext, t: Type) !Type {
        var current = t;
        for (self.subs.items) |sub| 
            switch (current.data) {
                .casting => |c| {
                    if (sub.from.eql(c))
                        current = try sub.to.deepCopy(self.alloc);
                }
            };
        
        return current;
    }

    pub fn unify(self: *InferenceContext, t1: Type, t2: Type) !void {
        const a = try self.applySubstitution(t1);
        const b = try self.applySubstitution(t2);

        if (a.deepEqual(b, false)) return;

        if (a.data == .casting) {
            try self.substitutions.append(.{ .from = a.data.casting, .to = b });
            return;
        }

        if (b.data == .casting) {
            try self.substitutions.append(.{ .from = b.data.casting, .to = a });
            return;
        }
        
        if (std.meta.activeTag(a.data) != std.meta.activeTag(b.data)) {
            return AnalyzerError.TypeMismatch;
        }

        switch (a.data) {
            .function => |fa| {
                const fb = b.data.function;
                try self.unify(fa.argument.*, fb.argument.*);
                try self.unify(fa.ret.*, fb.ret.*);
            },
            .array => |aa| {
                const ab = b.data.array;
                if (aa.size != ab.size) return AnalyzerError.TypeMismatch;
                try self.unify(aa.type_.*, ab.type_.*);

                for (aa.value, ab.value) |va, vb| 
                    if (va) |na| 
                        if (vb) |nb| 
                            self.unify(na, nb);
                
            },
            .aggregate => |aa| {
                const ab = b.data.aggregate;
                if (aa.indexes.len != ab.indexes.len) return AnalyzerError.TypeMismatch;

                for (aa.types, ab.types) |va, vb|
                    self.unify(va, vb);

                for (aa.values, ab.values) |va, vb| 
                    if (va) |na| 
                        if (vb) |nb| 
                            self.unify(na, nb);
            },
            else => return AnalyzerError.TypeMismatch,
        }
    }
};

// Bidirectional typing system? 

// todo: verify that types match, or meet in case of casting for every
// operation
pub const Analyzer = struct {
    const TContext = Context(*Type);
    allocator: std.mem.Allocator,

    context: *TContext,
    modules: *TContext,
    inferences: InferenceContext,
    castIndex: u32,

    pub fn init(allocator: std.mem.Allocator) !Analyzer {
        return Analyzer{ 
            .allocator = allocator,
            .context = try TContext.init(allocator),
            .modules = try TContext.init(allocator),
            .inferences = InferenceContext.init(allocator),
            .castIndex = 0
        };
    }

    pub fn getContext(self: *Analyzer, path: String) ?*TContext {
        return self.context.get(path);
    }

    pub fn getModules(self: *Analyzer, path: String) ?*TContext {
        return self.modules.get(path);
    }

    pub fn deinit(self: *Analyzer) void {
        self.context.deinit(self.allocator);
        self.modules.deinit(self.allocator);
    }

    pub fn runAnalysis(self: *Analyzer, node: *Node) !*Node {
        return try self.analyze(self.context, node);
    }

    fn analyze(self: *Analyzer, currctx: *TContext, node: *Node) !*Node {
        switch (node.data) {
            .none => return node,
            .type => return node,
            .int => return self.analyzeInt(currctx, node),
            .dec => return self.analyzeDec(currctx, node),
            .str => return self.analyzeStr(currctx, node),
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

    fn analyzeInt(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        _ = currctx;
        node.ntype = try self.allocator.create(Type);
        node.ntype.?.* = Type.initInt(64, 64, node.data.int);
        return node;
    }

    fn analyzeDec(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        _ = currctx;
        node.ntype = try self.allocator.create(Type);
        node.ntype.?.* = Type.initFloat(64, 64, node.data.dec);
        return node;
    }

    fn analyzeStr(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        _ = currctx;
        var arr = try self.allocator.alloc(?Type, node.data.str.len);
        errdefer self.allocator.free(arr);

        var type_ = try self.allocator.create(Type);
        errdefer type_.deinit(self.allocator);

        type_.* = Type.initInt(0, 255, null);

        for (node.data.str, 0..) |c, i| {
            arr[i] = Type.initInt(c, c, c);
        }

        var arrtype = try self.allocator.create(Type);
        errdefer arrtype.deinit(self.allocator);

        arrtype.* = Type{ .pointerIndex = 0, .data = .{ .array = .{
            .size = @intCast(node.data.str.len),
            .type_ = type_,
            .value = arr,
        } } };

        node.ntype = arrtype;
        return node;
    }

    fn analyzeRef(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const type_ = currctx.get(node.data.ref);
        if (type_ == null) {
            return AnalyzerError.UndefinedReference;
        }

        node.ntype = try type_.?.deepCopy(self.allocator);
        return node;
    }

    fn analyzeUnr(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        node.ntype = try self.allocator.create(Type);
        errdefer node.ntype.?.deinit(self.allocator);

        const rest = try analyze(self, currctx, node.data.unr.val);

        var unt = try self.allocator.create(Type);
        errdefer unt.deinit(self.allocator);

        unt.* = try Type.synthUnary(self.allocator, node.data.unr.op, rest.ntype.?.*);
        node.ntype = unt;
        return node;
    }

    fn analyzeBin(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const lhs = try analyze(self, currctx, node.data.bin.lhs);
        const rhs = try analyze(self, currctx, node.data.bin.rhs);

        node.ntype = try self.allocator.create(Type);
        errdefer node.ntype.?.deinit(self.allocator);

        node.ntype.?.* = try Type.binOperate(self.allocator, node.data.bin.op, lhs.ntype.?.*, rhs.ntype.?.*);

        return node;
    }

    fn analyzeCall(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const caller = try analyze(self, currctx, node.data.call.caller);
        const callee = try analyze(self, currctx, node.data.call.callee);

        node.ntype = try self.allocator.create(Type);
        errdefer node.ntype.?.deinit(self.allocator);

        node.ntype = try Type.callFunction(caller.ntype.?.*, callee.ntype.?.*);
        return node;
    }

    fn analyzeTernary(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const cond = try analyze(self, currctx, node.data.ter.cond);
        if (cond.ntype == null or cond.ntype.?.isTag(.boolean)) {
            return AnalyzerError.NonBooleanDecision;
        }

        const btrue = try analyze(self, currctx, node.data.ter.btrue);
        const bfalse = try analyze(self, currctx, node.data.ter.bfalse);

        if (btrue.ntype.?.deepEqual(bfalse.ntype.?.*, false)) {
            return AnalyzerError.TypeMismatch;
        }

        node.ntype = btrue.ntype;
        return node;
    }

    fn analyzeIntr(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const intr = node.data.intr;
        var innerctx = try TContext.init(self.allocator);
        defer innerctx.deinit(self.allocator);

        innerctx.parent = currctx;

        for (intr.intermediates) |inter| {
            const i = inter;
            i.* = (try analyze(self, innerctx, inter)).*;
        }

        intr.application.* = (try analyze(self, innerctx, node.data.intr.application)).*;
        return node;
    }

    fn analyzeRange(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const range = node.data.range;
        const start = try analyze(self, currctx, range.start);
        const end = try analyze(self, currctx, range.end);
        const epsilon = try analyze(self, currctx, range.epsilon);

        if (!start.ntype.?.isValued())
            return AnalyzerError.RequiresValue;

        if (!end.ntype.?.isValued())
            return AnalyzerError.RequiresValue;

        if (!epsilon.ntype.?.isValued())
            return AnalyzerError.RequiresValue;

        if (start.ntype.?.isTag(.integer) and end.ntype.?.isTag(.integer) and epsilon.ntype.?.isTag(.integer)) {
            const istart = start.ntype.?.data.integer.value;
            const iend = end.ntype.?.data.integer.value;
            //const ivalue = epsilon.ntype.?.data.integer.value;

            const trange = try self.allocator.create(Type);
            errdefer trange.deinit(self.allocator);

            trange.* = Type{ .pointerIndex = 0, .data = .{ .integer = .{
                .start = istart.?,
                .end = iend.?,
                .value = null,
            } } };

            node.ntype = trange;
            return node;
        }

        if (start.ntype.?.isTag(.decimal) and end.ntype.?.isTag(.decimal) and epsilon.ntype.?.isTag(.decimal)) {
            const dstart = start.ntype.?.data.decimal.value;
            const dend = end.ntype.?.data.decimal.value;
            
            const trange = try self.allocator.create(Type);
            errdefer trange.deinit(self.allocator);

            trange.* = Type{ .pointerIndex = 0, .data = .{ .decimal = .{
                .start = dstart.?,
                .end = dend.?,
                .value = null,
            } } };

            node.ntype = trange;
            return node;
        }

        return AnalyzerError.TypeMismatch;
    }

    fn analyzeModule(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        // modules are guaranteed top level, we dont need to return useful nodes

        const mod = node.data.mod;

        if (mod.path[0] == '@') {
            // todo: compile interface modules
            return node;
        }

        if (mod.direction) {
            var it = mod.pairs.iterator();
            while (it.next()) |pair| {
                const pname = pair.key_ptr.*;
                const pmod = self.modules.getTree(mod.path, pname);
                if (pmod) |p| {
                    try currctx.add(pname, p);
                } else 
                    return AnalyzerError.UndefinedReference;

            }

            return node;
        } 
        // todo: check from build if a virtual file tree matches the module path
        var it = mod.pairs.iterator();
        while (it.next()) |pair| {
            const pname = pair.key_ptr.*;
            const mnode = try analyze(self, currctx, pair.value_ptr.*);
            const res = try mnode.ntype.?.deepCopy(self.allocator);
            errdefer res.deinit(self.allocator);

            try self.modules.addTree(mod.path, pname, res);

        } 
        return node;

    }

    fn analyzeExpr(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const expr = node.data.expr;
        var innerctx = try TContext.init(self.allocator);
        defer innerctx.deinit(self.allocator);

        innerctx.parent = currctx;

        var freturn = try self.allocator.create(Type);
        freturn.* = Type{ .pointerIndex = 0, .data = .{ .casting = .{ .index = @intCast(self.castIndex + expr.params.len + 1) } } };
        for (0..expr.params.len) |i| {
            const idx = expr.params.len - i;
            const arg = expr.params[idx];
            const argT = try self.allocator.create(Type);
            errdefer argT.deinit(self.allocator);

            argT.* = .{ .pointerIndex = 0, .data = .{ .casting = .{
                .index = @intCast(idx),
            } } };

            arg.ntype = argT;
            errdefer arg.ntype.?.deinit(self.allocator);

            try innerctx.add(arg.data.str, try argT.deepCopy(self.allocator));

            const ft = try self.allocator.create(Type);
            errdefer ft.deinit(self.allocator);

            ft.* = .{
                .pointerIndex = 0,
                .data = .{ 
                    .function = .{
                        .argument = argT,
                        .ret = freturn,
                        .body = null
                    }
                }
            };

            freturn = ft;
        }    

        self.castIndex += @intCast(expr.params.len + 1);     

        if (expr.ntype) |res| {
            res.* = (try analyze(self, innerctx, expr.ntype.?)).*;
            errdefer res.deinit(self.allocator);
            node.ntype = try res.ntype.?.deepCopy(self.allocator);
            errdefer node.ntype.?.deinit(self.allocator);

            var param_val = res.ntype.?;
            for (expr.params) |n| {
                if (!param_val.isFunction()) break;
                n.ntype = try param_val.data.function.argument.deepCopy(self.allocator);
                errdefer n.ntype.?.deinit(self.allocator);
            }

        } 

        for (expr.params, 0..) |param, pidx|
            if (param.data == .ref) {
                const pname = param.data.ref;
                const ptype = try node.ntype.?.getParameter(@intCast(pidx));
                const res = try ptype.deepCopy(self.allocator);
                errdefer res.deinit(self.allocator);
                try innerctx.add(pname, res);
            };

        if (expr.expr != null) {
            expr.expr.?.ntype = (try analyze(self, innerctx, expr.expr.?)).ntype;
        }

        if (expr.ntype != null and expr.expr != null) {
            if (expr.ntype.?.ntype.?.deepEqual(expr.expr.?.ntype.?.*, false))
                return AnalyzerError.TypeMismatch;

            if (!expr.ntype.?.ntype.?.isMeetable(expr.expr.?.ntype.?.*)) 
                return AnalyzerError.TypeMismatch;
        }

        try currctx.add(expr.name, freturn);

        return node;
    }
};



test "Analyzer - Float" {}

const SNIR = @import("targets/supernova.zig");

pub const BlockOffset = usize;

pub const Instruction = struct {
    opcode: SNIR.Instruction,
    r1: SNIR.InfiniteRegister,
    r2: SNIR.InfiniteRegister,
    rs: SNIR.InfiniteRegister,
    immediate: u64,
};

pub const Value = struct {
    vtype: *Type,
    value: union(enum) {
        constant: void,
        param: u3,
        instruction: Instruction,
    }
};

pub const Block = struct {
    label: u64,
    contents: std.ArrayListUnmanaged(Value),
    predecessors: std.ArrayListUnmanaged(BlockOffset),
    successors: std.ArrayListUnmanaged(BlockOffset),
};