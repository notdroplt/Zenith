const std = @import("std");
const misc = @import("misc.zig");

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;

pub const String = []const u8;

////////////////////////////////////////////////////////////////////////
// Lexer - approx 500 lines                                           //
////////////////////////////////////////////////////////////////////////

/// Possible token types
pub const Tokens = enum {
    /// Integer tokens
    Int,

    /// Decimal Tokens
    Dec,

    /// String Tokens
    Str,

    /// Reference Tokens
    Ref,

    /// "module"
    Mod,

    /// "match"
    Match,

    /// "("
    Lpar,

    /// ")"
    Rpar,

    /// "{"
    Lcur,

    /// "}"
    Rcur,

    /// "["
    Lsqb,

    /// "]"
    Rsqb,

    /// "="
    Equ,

    /// ";"
    Semi,

    /// "+"
    Plus,

    /// "-"
    Min,

    /// "*"
    Star,

    /// "/"
    Bar,

    /// "&"
    Amp,

    /// "~"
    Til,

    /// "!"
    Bang,

    /// "#"
    Hash,

    /// "|"
    Pipe,

    /// "^"
    Hat,

    /// "%"
    Per,

    /// "<<"
    Lsh,

    /// ">>"
    Rsh,

    /// "=="
    Cequ,

    /// "!="
    Cneq,

    /// ">"
    Cgt,

    /// ">="
    Cge,

    /// "<"
    Clt,

    /// "<="
    Cle,

    /// "&&"
    And,

    /// "||"
    Or,

    /// "?"
    Ques,

    /// ":"
    Cln,

    /// "."
    Dot,

    /// "->"
    Arrow,
};

/// Lexer errors
pub const LexerError = error{
    /// Number format is invalid
    MalformedNumber,

    /// Identifier format is invalid
    MalformedIdentifier,

    /// Token was not recognized
    MalformedToken,

    /// String format is invalid
    MalformedString,

    /// Lexer reached the end of the code
    LexerEnd,
};

/// Assign a position to every token/node, a position is only a view
/// over the code string
pub const Pos = struct {
    /// Index for the first character in the code string
    index: usize = 0,

    /// How far does the symbol span
    span: usize = 0,
};

/// Lexer Token
pub const Token = struct {
    /// Token id
    tid: Tokens,

    /// Position in the code
    pos: Pos = Pos{},

    /// Token value (if applicable)
    val: union(enum) { int: i64, dec: f64, str: String, empty: void } = .empty,

    /// init a token without value
    pub fn init(tid: Tokens, pos: Pos) Token {
        return Token{ .tid = tid, .pos = pos, .val = .empty };
    }
};

/// Tokenizer for a string
pub const Lexer = struct {
    /// Code string
    code: String,

    /// Index of current character
    index: usize = 0,

    /// Last token consumed, multiple calls of consume() without a
    /// call to catchUp() return early
    last: ?Token = null,

    /// Context for error printing
    errctx: ErrorContext = .{ .value = .NoContext },

    /// Initialize a lexer
    pub fn init(code: String) Lexer {
        return Lexer{ .code = code };
    }

    /// Check if end without erroring out
    fn end(self: Lexer) bool {
        return self.code.len <= self.index;
    }

    /// Get current character, errors out on end
    fn current(self: Lexer) LexerError!u8 {
        return if (self.end()) LexerError.LexerEnd else self.code[self.index];
    }

    /// Checks if the current character against a constant errorless
    fn char(self: Lexer, c: u8) bool {
        return if (self.end()) false else self.code[self.index] == c;
    }

    /// Performs isAlpha without erroring out
    fn isAlpha(self: Lexer) bool {
        if (self.end())
            return false;
        return std.ascii.isAlphabetic(self.code[self.index]);
    }

    /// Performs isDigit without erroring out
    fn isNumeric(self: Lexer) bool {
        if (self.end())
            return false;
        return std.ascii.isDigit(self.code[self.index]);
    }

    /// Performs isAlphanumeric without erroring out
    fn isAlphanumeric(self: Lexer) bool {
        if (self.end())
            return false;
        return std.ascii.isAlphanumeric(self.code[self.index]);
    }

    /// Walks into the next character, or don't if end
    fn walk(self: Lexer) Lexer {
        return Lexer{
            .code = self.code,
            .index = self.index + @intFromBool(!self.end()),
            .last = self.last,
        };
    }

    /// Walk until a non whitespace character
    fn consumeWhitespace(self: Lexer) Lexer {
        var lex = self;
        while (lex.char(' ') or lex.char('\n') or lex.char('\t') or lex.char('\r'))
            lex = lex.walk();
        return lex;
    }

    /// Skip comments
    fn skipComments(self: Lexer) Lexer {
        // it should work for multiple sequential comments
        var str = self.code[self.index..];
        var lex = self;
        while (str[0] == '/' and str[1] == '/') : (str = lex.code[lex.index..]) {
            while (lex.char('\n'))
                lex = lex.walk();
            lex = lex.consumeWhitespace().walk();
        }
        return lex.consumeWhitespace();
    }

    /// Consume a string, errors only if it is incomplete
    fn consumeString(self: Lexer) LexerError!Token {
        var lex = self.walk();
        const start = lex.index;
        while (!lex.char('"') and !lex.end())
            lex = lex.walk();

        if (lex.end()) {
            lex.errctx = ErrorContext{ .position = Pos{ .index = start, .span = lex.index - start }, .value = .MalformedToken };
            return LexerError.MalformedString;
        }
        lex = lex.walk();

        return Token{
            .tid = .Str,
            .pos = Pos{ .index = start - 1, .span = lex.index - start + 1 },
            .val = .{ .str = self.code[start .. lex.index - 1] },
        };
    }

    /// Consume an identifier
    fn consumeIdentifier(self: Lexer) Token {
        const start = self.index;
        var lex = self;

        lex = lex.walk();

        while (lex.isAlphanumeric() or lex.char('_') or lex.char('\''))
            lex = lex.walk();

        return Token{
            .tid = .Ref,
            .pos = Pos{ .index = start, .span = lex.index - start },
            .val = .{ .str = lex.code[start..lex.index] },
        };
    }

    /// Consume an integer or a rational
    fn consumeNumber(self: Lexer) LexerError!Token {
        var curr = Tokens.Int;
        const start = self.index;
        var lex = self;

        while (lex.isNumeric())
            lex = lex.walk();

        if (lex.char('.')) {
            curr = .Dec;
            lex = lex.walk();
            while (lex.isNumeric())
                lex = lex.walk();
        }

        if (lex.char('e')) {
            curr = .Dec;
            lex = lex.walk();
            if (lex.char('+') or lex.char('-'))
                lex = lex.walk();

            if (!lex.isNumeric())
                return LexerError.MalformedNumber;

            while (lex.isNumeric())
                lex = lex.walk();
        }
        const rpos = Pos{ .index = start, .span = lex.index - start };

        const str = lex.code[start .. start + rpos.span];

        if (curr == .Int)
            return Lexer.parseInt(str, rpos);

        return Lexer.parseDec(str, rpos);
    }

    /// Parse an int string into an integer token
    fn parseInt(str: String, pos: Pos) Token {
        var result: i64 = 0;

        for (0..str.len) |idx|
            result = result * 10 + (str[idx] - '0');

        return Token{
            .tid = .Int,
            .pos = pos,
            .val = .{ .int = result },
        };
    }

    fn parseDec(str: String, pos: Pos) Token {
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
            if (idx != str.len and !std.ascii.isDigit(str[idx])) {
                sign = if (str[idx] == '-') -1 else 1;
                idx += 1;
            }
            while (idx < str.len) : (idx += 1) {
                const digit = @as(f64, @floatFromInt(str[idx] - '0'));
                const r1 = @mulAdd(f64, exp, 10, digit);
                exp = r1;
            }
        }
        return Token{
            .tid = .Dec,
            .pos = pos,
            .val = .{ .dec = result * std.math.pow(f64, 10, sign * exp) },
        };
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
            const tok = Token.init(.Mod, Pos{ .index = lex.index, .span = 6 });
            self.last = tok;
            return tok;
        }

        if (std.mem.startsWith(u8, str, "match")) {
            const tok = Token.init(.Match, Pos{ .index = self.index, .span = 5 });
            self.last = tok;
            return tok;
        }

        if (lex.isAlpha() or lex.char('_')) {
            self.last = lex.consumeIdentifier();
            return self.last.?;
        }

        const pos = Pos{ .index = lex.index, .span = 1 };
        const curr = try lex.current();

        const tok: ?Token = switch (curr) {
            '(' => Token.init(.Lpar, pos),
            ')' => Token.init(.Rpar, pos),
            '{' => Token.init(.Lcur, pos),
            '}' => Token.init(.Rcur, pos),
            '[' => Token.init(.Lsqb, pos),
            ']' => Token.init(.Rsqb, pos),
            '=' => lex.generate('=', .Equ, .Cequ),
            '+' => Token.init(.Plus, pos),
            '-' => lex.generate('>', .Min, .Arrow),
            '*' => Token.init(.Star, pos),
            '/' => Token.init(.Bar, pos),
            '&' => lex.generate('&', .Amp, .And),
            '~' => Token.init(.Til, pos),
            '!' => lex.generate('=', .Bang, .Cneq),
            '#' => Token.init(.Hash, pos),
            '|' => lex.generate('|', .Pipe, .Or),
            '%' => Token.init(.Per, pos),
            '<' => lex.doubleGenerate('=', '<', .Cle, .Lsh, .Clt),
            '>' => lex.doubleGenerate('=', '>', .Cge, .Rsh, .Cgt),
            '?' => Token.init(.Ques, pos),
            ':' => Token.init(.Cln, pos),
            ';' => Token.init(.Semi, pos),
            else => null,
        };

        self.last = tok;

        if (tok) |t|
            return t;

        self.errctx = ErrorContext{ .position = pos, .value = .MalformedToken };

        return LexerError.MalformedToken;
    }

    /// generate either token `small`, or `big` if next char is `chr`
    fn generate(self: Lexer, chr: u8, small: Tokens, big: Tokens) Token {
        const p1 = Pos{ .index = self.index, .span = 1 };
        const p2 = Pos{ .index = self.index, .span = 2 };

        if (self.walk().char(chr))
            return Token.init(big, p2);
        return Token.init(small, p1);
    }

    /// generate `t1` if next char is `c1`, else `t2` if `c2`, `telse` otherwise
    fn doubleGenerate(self: Lexer, c1: u8, c2: u8, t1: Tokens, t2: Tokens, telse: Tokens) Token {
        const p1 = Pos{ .index = self.index, .span = 1 };
        const p2 = Pos{ .index = self.index, .span = 2 };

        if (self.walk().char(c1)) return Token.init(t1, p2);
        if (self.walk().char(c2)) return Token.init(t2, p2);
        return Token.init(telse, p1);
    }

    /// After consuming a token, go to the next position
    pub fn catchUp(self: Lexer, token: Token) Lexer {
        return Lexer{
            .code = self.code,
            .index = token.pos.index + token.pos.span,
            .last = null,
        };
    }
};

////////// lexer tests
test Lexer {
    var lexer = Lexer.init(
        \\0 1 0.0 1.0 1e2 1.3e2 
        \\"hello" "" 
        \\identifier a b
        \\+-*/ == != && || <= >= << >>
    );

    const tokens = [_]Token{
        Token{ .tid = .Int, .val = .{ .int = 0 } },
        Token{ .tid = .Int, .val = .{ .int = 1 } },
        Token{ .tid = .Dec, .val = .{ .dec = 0.0 } },
        Token{ .tid = .Dec, .val = .{ .dec = 1.0 } },
        Token{ .tid = .Dec, .val = .{ .dec = 100 } },
        Token{ .tid = .Dec, .val = .{ .dec = 130 } },
        Token{ .tid = .Str, .val = .{ .str = "hello" } },
        Token{ .tid = .Str, .val = .{ .str = "" } },
        Token{ .tid = .Ref, .val = .{ .str = "identifier" } },
        Token{ .tid = .Ref, .val = .{ .str = "a" } },
        Token{ .tid = .Ref, .val = .{ .str = "b" } },
        Token{ .tid = .Plus },
        Token{ .tid = .Min },
        Token{ .tid = .Star },
        Token{ .tid = .Bar },
        Token{ .tid = .Cequ },
        Token{ .tid = .Cneq },
        Token{ .tid = .And },
        Token{ .tid = .Or },
        Token{ .tid = .Cle },
        Token{ .tid = .Cge },
        Token{ .tid = .Lsh },
        Token{ .tid = .Rsh },
    };

    for (tokens) |token| {
        const tok = try lexer.consume();
        try expectEqual(token.tid, tok.tid);
        if (token.val == .str) {
            try expect(std.mem.eql(u8, token.val.str, tok.val.str));
        } else try expectEqual(token.val, tok.val);
        lexer = lexer.catchUp(tok);
    }
}

////////////////////////////////////////////////////////////////////////
/// Error Contexts
////////////////////////////////////////////////////////////////////////

/// Give more details about any errors that occurs during compilation,
/// that was thrown by the compiler
pub const ErrorContext = struct {
    /// Context values
    value: union(enum) {
        /// This token was not recognized by the lexer
        MalformedToken: void,

        /// A token that was not expected shows up
        UnexpectedToken: struct {
            /// Token received
            token: Token,

            /// Token expected
            expected: Tokens,
        },

        /// An operation between one or two types that was not defined
        UndefinedOperation: struct {
            /// Left hand side of the operation
            lhs: ?Type,

            /// Operation token
            token: Tokens,

            /// Right hand side
            rhs: Type,
        },

        NonBooleanDecision: struct {
            /// Non boolean type
            condtype: Type,
        },

        /// Two types that can not be joined together
        DisjointTypes: struct {
            /// Type one
            t1: Type,

            /// Type two
            t2: Type,
        },

        /// The IR generator could not load the following type into a register
        InvalidLoad: *Type,

        /// The IR generator could not store the following type into a register
        InvalidStore: *Type,

        /// Base case
        NoContext: void,
    },

    /// Error position
    position: Pos = .{},
};

////////////////////////////////////////////////////////////////////////
// Types - approx 700 lines                                           //
////////////////////////////////////////////////////////////////////////

/// Syntactic analysis errors
pub const TypeError = error{
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

/// Operation types
pub const Type = struct {
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
    isParam: bool = false,

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
        return Type{ .data = .{ .boolean = value } };
    }

    pub fn initIdxCasting(value: u32) Type {
        return Type{ .data = .{ .casting = .{ .index = value } } };
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
    pub fn synthUnary(op: Tokens, v: Type) !Type {
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
                else => TypeError.UndefinedOperation,
            };
        return switch (v.data) {
            .integer => switch (op) {
                .Min => handleInt(op, Type.zeroInt, v),
                .Til => handleInt(.Min, Type.negOneInt, v),
                else => TypeError.UndefinedOperation,
            },
            .decimal => switch (op) {
                .Plus => v,
                .Min => handleFloat(op, Type.zeroDec, v),
                else => TypeError.UndefinedOperation,
            },
            .boolean => switch (op) {
                .Bang => {
                    if (v.data.boolean) |vbool|
                        return initBool(!vbool);
                    return initBool(null);
                },
                else => TypeError.UndefinedOperation,
            },
            else => TypeError.UndefinedOperation,
        };
    }

    pub fn checkUnary(op: Tokens, v: Type, res: Type) !Type {
        return Type.join(try synthUnary(op, v), res);
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
                    .function = .{ .body = null, .argument = left, .ret = right },
                },
            };
        }

        if (std.meta.activeTag(a.data) != std.meta.activeTag(b.data))
            return TypeError.UndefinedOperation;

        if (a.pointerIndex != b.pointerIndex)
            return TypeError.UndefinedOperation;

        if (a.pointerIndex > 0) {
            return switch (op) {
                .Cequ, .Cneq => binOperate(alloc, op, a, b),
                else => TypeError.UndefinedOperation,
            };
        }

        if (a.isFunction() and a.isFunction() and op == .Star) {
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
                    .Cequ => binopt(bool, ba, bb, misc.equ),
                    .Cneq => binopt(bool, ba, bb, misc.neq),
                    .And => binopt(bool, ba, bb, misc.land),
                    .Or => binopt(bool, ba, bb, misc.land),
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
            else => TypeError.UndefinedOperation,
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

    fn handleInt(op: Tokens, a: Type, b: Type) TypeError!Type {
        const ia = a.data.integer;
        const ib = b.data.integer;

        return switch (op) {
            .Plus => doMinMax(i64, misc.add, ia, ib),
            .Min => doMinMax(i64, misc.sub, ia, ib),
            .Star => doMinMax(i64, misc.mul, ia, ib),
            .Bar => {
                if (ib.value != null and ib.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                const mm = minMax(i64, ia.start, ia.end, ib.start, ib.end, misc.div);
                return initInt(mm[0], mm[1], binopt(i64, ia.value, ib.value, misc.div));
            },
            .Per => {
                if (ib.value != null and ib.value.? == 0) return TypeError.DivisionByZero;
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
            else => TypeError.UndefinedOperation,
        };
    }

    fn handleFloat(op: Tokens, a: Type, b: Type) TypeError!Type {
        const fa = a.data.decimal;
        const fb = b.data.decimal;

        return switch (op) {
            .Plus => doMinMax(f64, misc.add, fa, fb),
            .Min => doMinMax(f64, misc.sub, fa, fb),
            .Star => doMinMax(f64, misc.mul, fa, fb),
            .Bar => {
                if (fb.value != null and fb.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                const mm = minMax(f64, fa.start, fa.end, fb.start, fb.end, misc.div);
                return initFloat(mm[0], mm[1], binopt(f64, fa.value, fb.value, misc.div));
            },
            .Per => {
                if (fb.value != null and fb.value.? == 0) return TypeError.DivisionByZero;
                // todo: add "possible division by zero"
                return initFloat(0, fb.end - 1, binopt(f64, fa.value, fb.value, misc.mod));
            },
            .Cequ => doBoolMinMax(f64, misc.equ, fa, fb),
            .Cneq => doBoolMinMax(f64, misc.neq, fa, fb),
            .Cge => doBoolMinMax(f64, misc.gte, fa, fb),
            .Cgt => doBoolMinMax(f64, misc.gt, fa, fb),
            .Cle => doBoolMinMax(f64, misc.lte, fa, fb),
            .Clt => doBoolMinMax(f64, misc.lt, fa, fb),
            else => TypeError.UndefinedOperation,
        };
    }

    fn scalarArray(allocator: std.mem.Allocator, op: Tokens, arr: Type, other: Type, reversed: bool) TypeError!Type {
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

    fn handleArray(alloc: std.mem.Allocator, op: Tokens, a: Type, b: Type) TypeError!Type {
        if (a.data == .array and b.data == .array) {
            if (a.data.array.size != b.data.array.size) return TypeError.ArraySizeMismatch;
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
        t.* = switch (self.data) {
            .integer => initInt(self.data.integer.start, self.data.integer.end, self.data.integer.value),
            .decimal => initFloat(self.data.decimal.start, self.data.decimal.end, self.data.decimal.value),
            .boolean => initBool(self.data.boolean),
            .array => blk: {
                var newType = self;
                newType.data.array.indexer = try self.data.array.indexer.deepCopy(alloc);
                var newValues = try alloc.alloc(?Type, self.data.array.value.len);
                for (self.data.array.value, 0..) |value, i| {
                    if (value == null) continue;
                    const res = try value.?.deepCopy(alloc);
                    newValues[i] = res.*;
                    alloc.destroy(res); // only the top
                }
                newType.data.array.value = newValues;
                break :blk newType;
            },
            .aggregate => blk: {
                var newType = self;
                const newTypes = try alloc.alloc(Type, self.data.aggregate.types.len);
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

    pub fn join(t1: Type, t2: Type) !struct { []const Substitution, Type } {
        if (t1.pointerIndex != t2.pointerIndex) return TypeError.DisjointTypes(t1, t2);

        return switch (t1.data) {
            .boolean => .{ .{}, t1 },
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
            },
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
                return t1.data.array.indexer.meet(alloc, t2.data.array.indexer);
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
                const argMeet = try t1.data.function.argument.meet(alloc, t2.data.function.argument);
                const retMeet = try t1.data.function.ret.meet(alloc, t2.data.function.ret);
                if (argMeet == null or retMeet == null) return null;
                var list = try std.ArrayList(struct { String, Type }).init(alloc);
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
    pub fn getParameter(self: Type, index: u8) !*Type {
        var typ = &self;
        var idx: u8 = 0;
        while (typ.isFunction()) : (idx += 1) {
            if (idx == index) return typ.data.function.argument;
            typ = typ.data.function.ret;
        }

        return TypeError.UnknownParameter;
    }
};

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

    const instance1 = [_]struct { String, u32, Type }{
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

////////////////////////////////////////////////////////////////////////
// Parser - approx 800 lines                                          //
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
        mod: struct { path: String, nodes: []*Node, direction: bool },
        range: struct { start: *Node, end: *Node, epsilon: *Node },
        aggr: struct { name: String, children: []*Node },
        sum: []*Node,
        intr: struct { intermediates: []*Node, application: ?*Node },
    },

    pub fn eql(self: Node, other: Node) bool {
        if (std.meta.activeTag(self) != std.meta.activeTag(other))
            return false;

        const od = other.data;

        switch (self.data) {
            .none => true,
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

    pub fn deinit(self: *Node, alloc: std.mem.Allocator) void {
        switch (self.data) {
            .none => {},
            .type => {},
            .int => {},
            .dec => {},
            .str => {},
            .ref => {},
            .unr => |v| v.val.deinit(alloc),
            .bin => |v| {
                std.debug.print("op = {s}\n", .{@tagName(v.op)});
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
};

const ParserOnlyError = error{
    LexerEnd,
    Overflow,
    UnclosedParen,
    UnexpectedToken,
    UnexpectedStartOfDeclaration,
    unexpectedEof,
    UnexpectedColon,
    overflow,
    underflow,
};

pub const ParserError = LexerError || ParserOnlyError || error{OutOfMemory};

pub const Parser = struct {
    code: String,
    pos: Pos = Pos{},
    alloc: std.mem.Allocator,
    errctx: ErrorContext,
    lexer: Lexer,
    inModule: bool = false,

    fn newPosition(start: Pos, end: Pos) Pos {
        return .{
            .index = start.index,
            .span = end.index - start.index + end.span,
        };
    }

    fn createIntNode(self: Parser, position: Pos, value: i64) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .int = value },
        };
        return node;
    }

    fn createDecNode(self: Parser, position: Pos, value: f64) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .dec = value },
        };
        return node;
    }

    fn createStrNode(self: Parser, position: Pos, value: String) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .str = value },
        };
        return node;
    }

    fn createRefNode(self: Parser, position: Pos, value: String) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .ref = value },
        };
        return node;
    }

    fn createUnrNode(self: Parser, position: Pos, operator: Tokens, value: *Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .unr = .{ .val = value, .op = operator } },
        };
        return node;
    }

    fn createBinNode(self: Parser, position: Pos, operator: Tokens, lhs: *Node, rhs: *Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .bin = .{ .lhs = lhs, .rhs = rhs, .op = operator } },
        };
        return node;
    }

    fn createCallNode(self: Parser, position: Pos, caller: *Node, callee: *Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .call = .{ .caller = caller, .callee = callee } },
        };
        return node;
    }

    pub fn createExprNode(self: Parser, position: Pos, name: String, params: []*Node, exprType: ?*Node, expr: ?*Node) !*Node {
        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = position,
            .data = .{ .expr = .{ .name = name, .params = params, .ntype = exprType, .expr = expr } },
        };
        return node;
    }

    fn getPrecedence(tid: Tokens) i32 {
        return switch (tid) {
            .Or, .And => 1,
            .Cequ, .Cneq, .Clt, .Cle, .Cgt, .Cge => 2,
            .Plus, .Min => 3,
            .Star, .Bar, .Per => 4,
            .Lsh, .Rsh => 5,
            .Pipe, .Hat, .Amp => 6,
            .Hash, .Dot => 7,
            else => -1,
        };
    }

    fn generateUnexpected(self: *Parser, expected: Tokens, got: Token) ParserError!void {
        self.errctx = ErrorContext{
            .value = .{
                .UnexpectedToken = .{
                    .expected = expected,
                    .token = got,
                },
            },
            .position = got.pos,
        };
        return ParserError.UnexpectedToken;
    }

    const IntrFlags = enum { noApp, normal, skipLcur };

    fn parseIntr(self: *Parser, flags: IntrFlags) ParserError!*Node {
        const open = try self.lexer.consume();
        const start = open.pos;

        var intermediates: [16]*Node = undefined;
        var idx: usize = 0;
        errdefer for (0..idx) |i| {
            intermediates[i].deinit(self.alloc);
        };

        const lcur = try self.lexer.consume();
        if (flags != .skipLcur and lcur.tid != .Lcur)
            try self.generateUnexpected(.Lcur, lcur);

        self.lexer = self.lexer.catchUp(lcur);

        while ((try self.lexer.consume()).tid != .Rcur and idx < 16) : (idx += 1) {
            intermediates[idx] = try parseStatement(self);

            const semi = try self.lexer.consume();
            if (semi.tid != .Semi)
                try self.generateUnexpected(.Semi, semi);
            self.lexer = self.lexer.catchUp(semi);
        }

        const rcur = try self.lexer.consume();

        if (rcur.tid != .Rcur)
            try self.generateUnexpected(.Rcur, rcur);

        self.lexer = self.lexer.catchUp(rcur);

        // we use less memory if we reallocate for every idx < 15
        const actualImm = try self.alloc.alloc(*Node, idx);

        for (0..idx) |i|
            actualImm[i] = intermediates[i];

        if (flags == IntrFlags.noApp or self.inModule) {
            const node = try self.alloc.create(Node);
            node.* = Node{ .position = newPosition(start, rcur.pos), .data = .{ .intr = .{ .intermediates = actualImm, .application = null } } };

            return node;
        }

        const application = try self.parsePrimary();
        errdefer application.deinit(self.alloc);

        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = newPosition(start, application.position),
            .data = .{ .intr = .{
                .intermediates = actualImm,
                .application = application,
            } },
        };
        return node;
    }

    fn parseModule(self: *Parser) ParserError!*Node {
        const module = try self.lexer.consume();
        const start = module.pos;

        self.lexer = self.lexer.catchUp(module);

        const direction = try self.lexer.consume();

        if (direction.tid != .Rsh and direction.tid != .Lsh) {
            try self.generateUnexpected(.Rsh, direction);
        }

        self.lexer = self.lexer.catchUp(direction);

        const name = try self.lexer.consume();

        if (name.tid != .Str) {
            try self.generateUnexpected(.Str, name);
        }

        self.lexer = self.lexer.catchUp(name);
        self.inModule = true;

        const content = try self.parseIntr(.noApp);
        errdefer content.deinit(self.alloc);

        self.inModule = false;

        const node = try self.alloc.create(Node);
        node.* = Node{
            .position = newPosition(start, content.position),
            .data = .{
                .mod = .{
                    .path = name.val.str,
                    .nodes = content.data.intr.intermediates,
                    .direction = direction.tid == .Lsh,
                },
            },
        };
        return node;
    }

    fn parsePrimary(self: *Parser) ParserError!*Node {
        const token = try self.lexer.consume();
        self.lexer = self.lexer.catchUp(token);
        return switch (token.tid) {
            .Int => try self.createIntNode(token.pos, token.val.int),
            .Dec => try self.createDecNode(token.pos, token.val.dec),
            .Str => try self.createStrNode(token.pos, token.val.str),
            .Ref => try self.createRefNode(token.pos, token.val.str),
            .Lpar => {
                const expr = try self.parseExpr();
                errdefer expr.deinit(self.alloc);

                const rpar = try self.lexer.consume();
                if (rpar.tid != .Rpar)
                    return ParserError.UnclosedParen;

                self.lexer = self.lexer.catchUp(rpar);
                expr.position = newPosition(token.pos, rpar.pos);
                return expr;
            },
            .Lcur => try self.parseIntr(IntrFlags.skipLcur),
            else => {
                self.pos = token.pos;
                try self.generateUnexpected(.Lpar, token);
                unreachable;
            },
        };
    }

    fn isAtomic(tid: Tokens) bool {
        return switch (tid) {
            .Int, .Dec, .Ref, .Str, .Lpar => true,
            else => false,
        };
    }

    fn parseCall(self: *Parser) ParserError!*Node {
        var caller = try self.parsePrimary();
        errdefer caller.deinit(self.alloc);

        while (true) {
            const token = try self.lexer.consume();
            if (!isAtomic(token.tid) or token.tid == .Semi)
                return caller;

            const callee = try self.parsePrimary();
            errdefer callee.deinit(self.alloc);

            caller = try self.createCallNode(token.pos, caller, callee);
            errdefer caller.deinit(self.alloc);
        }
    }

    fn parseUnary(self: *Parser) ParserError!*Node {
        const token = try self.lexer.consume();
        switch (token.tid) {
            .Min, .Plus, .Til, .Bang, .Star, .Amp => {},
            else => return self.parseCall(),
        }

        self.lexer = self.lexer.catchUp(token);
        const operand = try self.parseUnary();
        errdefer operand.deinit(self.alloc);

        return self.createUnrNode(token.pos, token.tid, operand);
    }

    fn parseBinary(self: *Parser, precedence: i32) ParserError!*Node {
        var left = try self.parseUnary();
        errdefer left.deinit(self.alloc);

        while (true) {
            const token = try self.lexer.consume();
            const tokenPrecedence = getPrecedence(token.tid);
            if (tokenPrecedence < precedence)
                return left;

            self.lexer = self.lexer.catchUp(token);
            const right = try self.parseBinary(tokenPrecedence + 1);
            errdefer right.deinit(self.alloc);

            left = try self.createBinNode(token.pos, token.tid, left, right);
            errdefer left.deinit(self.alloc);
        }
    }

    fn parseTernary(self: *Parser) ParserError!*Node {
        const condition = try self.parseBinary(1);
        errdefer condition.deinit(self.alloc);

        const question = try self.lexer.consume();
        if (question.tid != .Ques) return condition;

        self.lexer = self.lexer.catchUp(question);

        const thenExpr = try self.parseBinary(1);
        errdefer thenExpr.deinit(self.alloc);

        const colon = try self.lexer.consume();
        if (colon.tid != .Cln)
            return ParserError.UnexpectedColon;

        self.lexer = self.lexer.catchUp(colon);

        const elseExpr = try self.parseTernary();
        errdefer elseExpr.deinit(self.alloc);

        const ternary = try self.alloc.create(Node);
        ternary.* = Node{
            .position = newPosition(condition.position, elseExpr.position),
            .data = .{
                .ter = .{
                    .cond = condition,
                    .btrue = thenExpr,
                    .bfalse = elseExpr,
                },
            },
        };
        return ternary;
    }

    fn parseExpr(self: *Parser) ParserError!*Node {
        return self.parseTernary();
    }

    fn parseRange(self: *Parser) ParserError!*Node {
        const lsq = try self.lexer.consume();
        if (lsq.tid != .Lsqb) try self.generateUnexpected(.Lsqb, lsq);

        self.lexer = self.lexer.catchUp(lsq);
        const start = try self.parseExpr();
        errdefer start.deinit(self.alloc);

        const semi = try self.lexer.consume();
        if (semi.tid != .Semi) try self.generateUnexpected(.Semi, semi);

        self.lexer = self.lexer.catchUp(semi);
        const end = try self.parseExpr();
        errdefer end.deinit(self.alloc);

        const semi2 = try self.lexer.consume();
        if (semi2.tid != .Semi) try self.generateUnexpected(.Semi, semi2);

        self.lexer = self.lexer.catchUp(semi2);
        const epsilon = try self.parseExpr();
        errdefer epsilon.deinit(self.alloc);

        const rsq = try self.lexer.consume();

        if (rsq.tid != .Rsqb) try self.generateUnexpected(.Rsqb, rsq);

        self.lexer = self.lexer.catchUp(rsq);

        const range = try self.alloc.create(Node);
        range.* = Node{
            .position = newPosition(lsq.pos, rsq.pos),
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

    fn parseTPrimary(self: *Parser) ParserError!*Node {
        const tok = try self.lexer.consume();
        return switch (tok.tid) {
            .Lsqb => self.parseRange(),
            .Ref => self.parseProd(),
            .Star => |v| {
                self.lexer = self.lexer.catchUp(tok);
                const val = try self.parseTPrimary();
                const ptr = try self.alloc.create(Node);
                errdefer self.alloc.destroy(ptr);
                ptr.* = Node{ .position = newPosition(tok.pos, val.position), .data = .{ .unr = .{ .op = v, .val = ptr } } };
                return ptr;
            },
            else => ParserError.MalformedToken,
        };
    }

    fn getTypePrecedence(tok: Tokens) i3 {
        return switch (tok) {
            .Arrow => 1,
            .Hash => 2,
            else => -1,
        };
    }

    fn parseTBinary(self: *Parser, prec: i3) ParserError!*Node {
        var left = try self.parseTPrimary();
        errdefer left.deinit(self.alloc);

        while (true) {
            const token = try self.lexer.consume();
            const tokenPrecedence = getTypePrecedence(token.tid);
            if (tokenPrecedence < prec)
                return left;

            self.lexer = self.lexer.catchUp(token);
            const right = try self.parseTBinary(tokenPrecedence + 1);
            errdefer right.deinit(self.alloc);

            left = try self.createBinNode(token.pos, token.tid, left, right);
            errdefer left.deinit(self.alloc);
        }
    }

    fn parseSum(self: *Parser) ParserError!*Node {
        const member = try self.parseTBinary(1);
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
            if (token.tid != .Bar)
                break;

            self.lexer = self.lexer.catchUp(token);
            try array.append(try self.parseTBinary(0));
        }

        if (array.items.len == 1) {
            array.deinit();
            return member;
        }

        const sum = try self.alloc.create(Node);
        sum.* = .{
            .data = .{ .sum = array.items },
            .position = newPosition(member.position, array.getLast().position),
        };

        return sum;
    }

    fn parseProd(self: *Parser) ParserError!*Node {
        const ctor = try self.lexer.consume();
        const start = ctor.pos;
        if (ctor.tid != .Ref) try self.generateUnexpected(.Ref, ctor);

        self.lexer = self.lexer.catchUp(ctor);

        const open = try self.lexer.consume();
        if (open.tid != .Lcur)
            return self.createRefNode(ctor.pos, ctor.val.str);

        self.lexer = self.lexer.catchUp(open);

        var array = std.ArrayList(*Node).init(self.alloc);

        var closed = try self.lexer.consume();
        while (closed.tid != .Rcur) : (closed = try self.lexer.consume()) {
            try array.append(try self.parseType());
        }

        self.lexer = self.lexer.catchUp(closed);
        const prod = try self.alloc.create(Node);
        prod.* = Node{ .position = newPosition(start, closed.pos), .data = .{ .aggr = .{ .name = ctor.val.str, .children = array.items } } };

        return prod;
    }

    fn parseType(self: *Parser) ParserError!*Node {
        return parseSum(self);
    }

    fn parseStatement(self: *Parser) !*Node {
        const name = try self.lexer.consume();
        const start = name.pos;
        self.lexer = self.lexer.catchUp(name);

        var params: [8]*Node = undefined;
        var pidx: usize = 0;

        errdefer for (0..pidx) |i| {
            params[i].deinit(self.alloc);
        };

        while (isAtomic((try self.lexer.consume()).tid) or (try self.lexer.consume()).tid == .Lpar) : (pidx += 1) {
            params[pidx] = try parsePrimary(self);
        }

        var exprType: ?*Node = null;
        var expr: ?*Node = null;
        const colon = try self.lexer.consume();
        if (colon.tid == .Cln) {
            self.lexer = self.lexer.catchUp(colon);
            exprType = try self.parseType();
        }

        const equal = try self.lexer.consume();
        if (equal.tid == .Equ) {
            self.lexer = self.lexer.catchUp(equal);
            expr = try self.parseExpr();
        }

        if (exprType == null and expr == null)
            return ParserError.MalformedToken;

        const endPos = (expr orelse exprType).?.position;

        const pos = newPosition(start, endPos);
        const actParams = try self.alloc.alloc(*Node, pidx);
        errdefer self.alloc.free(actParams);

        @memcpy(actParams, params[0..pidx]);
        return self.createExprNode(pos, name.val.str, actParams, exprType, expr);
    }

    pub fn parseNode(self: *Parser) ParserError!*Node {
        const token = try self.lexer.consume();
        switch (token.tid) {
            .Mod => return self.parseModule(),
            .Ref => return self.parseStatement(),
            else => {
                self.pos = token.pos;
                return error.UnexpectedStartOfDeclaration;
            },
        }
    }

    pub fn init(code: String, alloc: std.mem.Allocator) Parser {
        return Parser{
            .errctx = .{ .value = .NoContext },
            .code = code,
            .alloc = alloc,
            .lexer = Lexer.init(code),
        };
    }
};

////////////////////////////////////////////////////////////////////////
// Semantic Analyzer - approx 600 lines                               //
////////////////////////////////////////////////////////////////////////

/// Possible errors thrown by the analyzer only
pub const AnalyzerOnlyError = error{
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
};

pub const AnalyzerError = ParserError || TypeError || AnalyzerOnlyError;

pub fn Context(comptime T: type) type {
    return struct {
        const ctxt = @This();

        parent: ?*ctxt = null,
        alloc: std.mem.Allocator,
        members: std.StringHashMap(T),
        children: std.StringHashMap(ctxt),

        pub fn init(alloc: std.mem.Allocator) ctxt {
            return ctxt{
                .alloc = alloc,
                .members = std.StringHashMap(T).init(alloc),
                .children = std.StringHashMap(ctxt).init(alloc),
            };
        }

        pub fn initTree(alloc: std.mem.Allocator, path: String) !*ctxt {
            const ctx = ctxt.init(alloc);
            const parts = std.mem.split(u8, path, "/");
            var currentCtx = ctx;

            while (parts.next()) |part| {
                var subCtx = currentCtx.children.get(part);
                if (subCtx == null) {
                    subCtx = ctxt.init(alloc);
                    subCtx.parent = currentCtx.?;
                    try currentCtx.children.put(part, subCtx);
                }
                currentCtx = subCtx.?;
            }

            return ctx;
        }

        pub fn getTree(self: ctxt, path: String, name: String) ?T {
            var parts = std.mem.split(u8, path, "/");
            var currentCtx = self;

            while (parts.next()) |part| {
                const subCtx = currentCtx.children.get(part);
                if (subCtx == null) return null;
                currentCtx = subCtx.?;
            }

            return currentCtx.get(name);
        }

        pub fn addTree(self: *ctxt, path: String, name: String, value: T) !void {
            var parts = std.mem.split(u8, path, "/");
            var currentCtx = self;

            while (parts.next()) |part| {
                var subCtx = currentCtx.children.get(part);
                if (subCtx == null) {
                    subCtx = ctxt.init(self.alloc);
                    subCtx.?.parent = currentCtx;
                    try currentCtx.children.put(part, subCtx.?);
                }
                currentCtx = &subCtx.?;
            }

            try currentCtx.add(name, value);
        }

        pub fn deinit(self: *ctxt, alloc: std.mem.Allocator) void {
            var it2 = self.children.iterator();
            while (it2.next()) |pair| {
                pair.value_ptr.*.deinit(alloc);
            }

            self.members.deinit();
            self.children.deinit();
        }

        pub fn get(self: ctxt, name: String) ?T {
            return self.members.get(name) orelse if (self.parent) |p| p.get(name) else null;
        }

        pub fn add(self: *ctxt, name: String, value: T) !void {
            return self.members.put(name, value);
        }

        pub fn isEmpty(self: ctxt) bool {
            return self.members.count() == 0;
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

// Bidirectional typing system?

// todo: verify that types match, or meet in case of casting for every
// operation
pub const Analyzer = struct {
    const TContext = Context(*Type);
    allocator: std.mem.Allocator,
    /// Use an arena for types to allow shallow copying
    talloc: std.mem.Allocator,

    aalloc: std.heap.ArenaAllocator,
    context: TContext,
    modules: TContext,
    errctx: ErrorContext = .{ .value = .NoContext },
    castIndex: u32,

    pub fn init(allocator: std.mem.Allocator) !Analyzer {
        //var arena = std.heap.ArenaAllocator.init(allocator);
        return Analyzer{
            .allocator = allocator,
            .aalloc = undefined, //arena,
            .talloc = allocator,
            .context = TContext.init(allocator),
            .modules = TContext.init(allocator),
            .castIndex = 0,
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
        self.aalloc.deinit();
    }

    pub fn runAnalysis(self: *Analyzer, node: *Node) !*Node {
        return self.analyze(&self.context, node);
    }

    fn analyze(self: *Analyzer, currctx: *TContext, node: *Node) !*Node {
        switch (node.data) {
            .none => return node,
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

    fn analyzeInt(self: *Analyzer, node: *Node) AnalyzerError!*Node {
        node.ntype = try self.talloc.create(Type);
        node.ntype.?.* = Type.initInt(node.data.int, node.data.int, node.data.int);
        return node;
    }

    fn analyzeDec(self: *Analyzer, node: *Node) AnalyzerError!*Node {
        node.ntype = try self.talloc.create(Type);
        node.ntype.?.* = Type.initFloat(node.data.dec, node.data.dec, node.data.dec);
        return node;
    }

    fn analyzeStr(self: *Analyzer, node: *Node) AnalyzerError!*Node {
        var arr = try self.talloc.alloc(?Type, node.data.str.len);
        errdefer self.talloc.free(arr);

        const strType = try self.talloc.create(Type);

        strType.* = Type.initInt(0, 255, null);

        for (node.data.str, 0..) |c, i| {
            arr[i] = Type.initInt(c, c, c);
        }

        var arrtype = try self.talloc.create(Type);
        errdefer arrtype.deinit(self.talloc);

        arrtype.* = Type{ .pointerIndex = 0, .data = .{ .array = .{
            .size = @intCast(node.data.str.len),
            .indexer = strType,
            .value = arr,
        } } };

        node.ntype = arrtype;
        return node;
    }

    fn analyzeRef(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const refType = currctx.get(node.data.ref);
        if (refType) |refT| {
            node.ntype = try refT.deepCopy(self.talloc);
            return node;
        }
        return AnalyzerError.UndefinedReference;
    }

    fn analyzeUnr(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
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

    fn analyzeBin(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const lhs = try analyze(self, currctx, node.data.bin.lhs);
        const rhs = try analyze(self, currctx, node.data.bin.rhs);

        node.ntype = try self.talloc.create(Type);

        node.ntype.?.* = Type.binOperate(self.talloc, node.data.bin.op, lhs.ntype.?.*, rhs.ntype.?.*) catch |err| {
            self.errctx.position = node.position;
            return err;
        };

        return node;
    }

    fn analyzeCall(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const caller = try analyze(self, currctx, node.data.call.caller);
        const callee = try analyze(self, currctx, node.data.call.callee);

        node.ntype = try self.talloc.create(Type);
        node.ntype = try Type.callFunction(caller.ntype.?.*, callee.ntype.?.*);
        return node;
    }

    fn analyzeTernary(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        const cond = try analyze(self, currctx, node.data.ter.cond);
        if (cond.ntype == null or cond.ntype.?.data != .boolean) {
            self.errctx = ErrorContext{ .position = cond.position, .value = .{ .NonBooleanDecision = .{ .condtype = cond.ntype.?.* } } };
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
        var innerctx = TContext.init(self.allocator);
        defer innerctx.deinit(self.allocator);

        innerctx.parent = currctx;

        for (intr.intermediates) |inter| {
            inter.* = (try analyze(self, &innerctx, inter)).*;
        }

        intr.application.?.* = (try analyze(self, &innerctx, intr.application.?)).*;
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

        if (start.ntype.?.data == .integer and end.ntype.?.data == .integer and epsilon.ntype.?.data == .integer) {
            const istart = start.ntype.?.data.integer.value;
            const iend = end.ntype.?.data.integer.value;
            //const ivalue = epsilon.ntype.?.data.integer.value;

            const trange = try self.talloc.create(Type);
            errdefer trange.deinit(self.talloc);

            trange.* = Type{ .pointerIndex = 0, .data = .{ .integer = .{
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
                } else return AnalyzerError.UndefinedReference;
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

    fn analyzeExpr(self: *Analyzer, currctx: *TContext, node: *Node) AnalyzerError!*Node {
        var expr = node.data.expr;
        var innerctx = TContext.init(self.allocator);
        defer innerctx.deinit(self.allocator);

        innerctx.parent = currctx;

        var freturn = try self.talloc.create(Type);
        freturn.* = Type{
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
            };

            arg.ntype = try argT.deepCopy(self.talloc);

            try innerctx.add(arg.data.ref, arg.ntype.?);

            const ft = try self.talloc.create(Type);
            errdefer ft.deinit(self.talloc);

            ft.* = .{
                .pointerIndex = 0,
                .data = .{
                    .function = .{ .argument = argT, .ret = freturn, .body = null },
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
            for (expr.params) |n| {
                if (!paramVal.isFunction()) break;
                n.ntype = try paramVal.data.function.argument.deepCopy(self.talloc);
                errdefer n.ntype.?.deinit(self.talloc);
            }
        }

        for (expr.params, 0..) |param, pidx|
            if (param.data == .ref) {
                const pname = param.data.ref;
                var ptype: *Type = undefined;
                if (node.ntype) |nt| {
                    ptype = try nt.getParameter(@intCast(pidx));
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
                return AnalyzerError.TypeMismatch;
        }

        try currctx.add(expr.name, freturn);

        return node;
    }
};

////////////////////////////////////////////////////////////////////////
/// Intermediate Representation - approx 600 lines                    //
////////////////////////////////////////////////////////////////////////
pub const IRError = error{ OutOfMemory, InvalidLoad, InvalidStore, InvalidBlock, InvalidOperation };

// all jumps are relative to the start of the block
pub const IR = struct {
    const SNIR = @import("Supernova/supernova.zig");
    const BlockOffset = usize;

    pub const Instruction = struct {
        opcode: SNIR.Opcodes,
        r1: SNIR.InfiniteRegister = 0,
        r2: SNIR.InfiniteRegister = 0,
        rd: SNIR.InfiniteRegister = 0,
        immediate: u64 = 0,
    };

    pub const VContext = Context(SNIR.InfiniteRegister);

    // register 0 is always 0,
    // registers 1 - 8 are function parameters, register 4 returns
    // registers 9 - 12 are software stack based
    // registers 13 onwards are scratch

    pub const Value = struct {
        vtype: *Type = undefined, // owning
        from: BlockOffset = 0,
        args: ?*[]Value = null,
        value: union(enum) {
            constant: void,
            instruction: Instruction,
            register: SNIR.InfiniteRegister,
            partial: struct {
                base: *Value,
            },
        } = undefined,

        pub fn deinit(self: Value, alloc: std.mem.Allocator) void {
            switch (self.value) {
                .partial => |v| {
                    v.base.deinit(alloc);
                    alloc.destroy(v.base);
                },
                else => {},
            }
        }
    };

    pub const Block = struct {
        instructions: std.ArrayListUnmanaged(Instruction) = .{},
        result: Value = .{},
    };

    pub const PhiSource = struct { predecessor: BlockOffset, registers: SNIR.InfiniteRegister };

    const EdgeValue = struct { from: BlockOffset, to: BlockOffset };
    edges: std.AutoHashMapUnmanaged(EdgeValue, void) = .{},
    nodes: std.AutoHashMapUnmanaged(BlockOffset, Block) = .{},
    phiSources: std.AutoArrayHashMapUnmanaged(u64, std.ArrayListUnmanaged(PhiSource)) = .{},

    blockCounter: BlockOffset = 0,
    regIndex: SNIR.InfiniteRegister = 8, // skip param registers
    alloc: std.mem.Allocator,
    errctx: ErrorContext = .{ .value = .NoContext },
    pub fn init(alloc: std.mem.Allocator) IR {
        return IR{ .alloc = alloc };
    }

    pub fn deinit(self: *IR) void {
        self.edges.deinit(self.alloc);
        var nit = self.nodes.iterator();
        while (nit.next()) |node| {
            node.value_ptr.*.instructions.deinit(self.alloc);
        }
        self.nodes.deinit(self.alloc);
        self.phiSources.deinit(self.alloc);
        //var it = self.nodes.valueIterator();
        // value.deinit(self.alloc); // we *might need to delete partials
    }

    pub fn fromNode(self: *IR, node: *Node) !void {
        const bid = try self.newBlock();
        const val = try self.blockNode(bid, node);
        _ = try self.appendIfConstant(val.from, val);
    }

    pub fn newBlock(self: *IR) !BlockOffset {
        try self.nodes.put(self.alloc, self.blockCounter, .{});
        self.blockCounter += 1;
        return self.blockCounter - 1;
    }

    pub fn newEdge(self: *IR, from: BlockOffset, to: BlockOffset) !void {
        try self.edges.put(self.alloc, .{ .from = from, .to = to }, {});
    }

    pub fn getBlock(self: *IR, offset: BlockOffset) ?Block {
        return self.nodes.get(offset);
    }

    pub fn addInstruction(self: *IR, blockID: BlockOffset, instruction: Instruction) !void {
        const block = self.nodes.getEntry(blockID);
        if (block) |b| {
            try b.value_ptr.instructions.append(self.alloc, instruction);
        } else return IRError.InvalidBlock;
    }

    pub fn deinitBlock(self: *IR, blockID: BlockOffset) void {
        const block = self.nodes.get(blockID);
        if (block) |b| {
            b.result.deinit(self.alloc);
            _ = self.nodes.remove(blockID);
        }
    }

    fn addPhi(self: *IR, merge: BlockOffset, dest: SNIR.InfiniteRegister, sources: []PhiSource) !void {
        const phi_inst = Instruction{
            .opcode = SNIR.Instruction.phi,
            .rd = dest,
            .immediate = self.phi_sources.count(),
        };

        const block = self.nodes.getPtr(merge).?;
        try block.*.append(self.alloc, phi_inst);

        try self.phiSources.put(self.alloc, phi_inst.immediate, sources);
    }

    fn allocateRegister(self: *IR) SNIR.InfiniteRegister {
        const idx = self.regIndex;
        self.regIndex += 1;
        return idx;
    }

    fn calculateLoadSize(self: *IR, value: *Type) IRError!SNIR.Opcodes {
        if (value.data == .decimal)
            return SNIR.Opcodes.ld_dwrd;

        if (value.data != .integer) {
            self.errctx = ErrorContext{
                .value = .{
                    .InvalidLoad = value,
                },
            };
            return IRError.InvalidLoad;
        }

        const int = value.data.integer;
        if ((int.start >= 0 and int.end <= (1 << 8) - 1) or (int.start >= -(1 << 7) and int.end <= (1 << 7) - 1))
            return SNIR.Opcodes.ld_byte;

        if ((int.start >= 0 and int.end <= (1 << 16) - 1) or (int.start >= -(1 << 15) and int.end <= (1 << 15) - 1))
            return SNIR.Opcodes.ld_half;

        if ((int.start >= 0 and int.end <= (1 << 32) - 1) or (int.start >= -(1 << 31) and int.end <= (1 << 31) - 1))
            return SNIR.Opcodes.ld_word;

        return SNIR.Opcodes.ld_dwrd;
    }

    fn calculateStoreSize(value: *Type) IRError!SNIR.Opcodes {
        if (value.data == .decimal)
            return SNIR.Opcodes.st_dwrd;

        if (value.data != .integer)
            return IRError.InvalidStore;

        const int = value.data.integer;
        if ((int.start >= 0 and int.end <= (1 << 8) - 1) or (int.start >= -(1 << 7) and int.end <= (1 << 7) - 1))
            return SNIR.Opcodes.st_byte;

        if ((int.start >= 0 and int.end <= (1 << 16) - 1) or (int.start >= -(1 << 15) and int.end <= (1 << 15) - 1))
            return SNIR.Opcodes.st_half;

        if ((int.start >= 0 and int.end <= (1 << 32) - 1) or (int.start >= -(1 << 31) and int.end <= (1 << 31) - 1))
            return SNIR.Opcodes.st_word;

        return SNIR.Opcodes.st_dwrd;
    }

    fn blockNode(self: *IR, currblock: BlockOffset, node: *Node) IRError!Value {
        switch (node.data) {
            .int => return self.blockInteger(currblock, node),
            .dec => return self.blockDecimal(currblock, node),
            // .str => return self.blockString(currblock, node),
            .ref => return self.blockReference(currblock, node),
            .unr => return self.blockUnary(currblock, node),
            .bin => return self.blockBinary(currblock, node),
            .call => return self.blockCall(currblock, node),
            .ter => return self.blockTernary(currblock, node),
            .expr => return self.blockExpr(currblock, node),
            else => return IRError.InvalidBlock,
        }
    }

    fn appendIfConstant(self: *IR, block: BlockOffset, value: Value) IRError!SNIR.InfiniteRegister {
        const v = value.value;
        return switch (v) {
            .constant => {
                const reg = self.allocateRegister();
                try self.addInstruction(block, Instruction{
                    .opcode = SNIR.Opcodes.mov,
                    .rd = reg,
                    .r1 = 0,
                    .immediate = @bitCast(value.vtype.data.integer.value.?),
                });
                return reg;
            },
            .instruction => {
                try self.addInstruction(block, v.instruction);
                return v.instruction.rd;
            },
            .register => v.register,
            .partial => unreachable,
        };
    }

    fn constantFromType(self: *IR, block: BlockOffset, node: *Node) Value {
        const nodeType = node.ntype.?;
        node.deinit(self.alloc);

        return Value{ .vtype = nodeType, .from = block, .value = .constant };
    }

    fn blockInteger(self: *IR, block: BlockOffset, node: *Node) Value {
        return self.constantFromType(block, node);
    }

    fn blockDecimal(self: *IR, block: BlockOffset, node: *Node) Value {
        return self.constantFromType(block, node);
    }

    fn blockReference(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
        if (node.ntype.?.isValued())
            return self.constantFromType(block, node);

        const refType = node.ntype.?;
        const instruction = Instruction{
            .opcode = self.calculateLoadSize(refType) catch |err| {
                self.errctx.position = node.position;
                return err;
            },
            .rd = self.allocateRegister(),
            .r1 = 0,
            .immediate = 0, // todo: come back here
        };

        return Value{
            .vtype = refType,
            .from = block,
            .value = .{ .instruction = instruction },
        };
    }

    fn blockUnary(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
        if (node.ntype.?.isValued())
            return self.constantFromType(block, node);

        const unary = node.data.unr;
        const unrtype = node.ntype;

        const inner = try self.blockNode(block, unary.val);

        if (unary.op == .Plus)
            return inner; // unary plus does NOTHING so we ball

        const result = try self.appendIfConstant(inner.from, inner);

        const instruction = try switch (unary.op) {
            .Bang, .Til => Instruction{
                .opcode = .not,
                .r1 = result,
                .rd = self.allocateRegister(),
            },
            .Min => Instruction{
                .opcode = .subr,
                .rd = self.allocateRegister(),
                .r1 = 0,
                .r2 = result,
            },
            .Star => Instruction{
                .opcode = self.calculateLoadSize(unrtype.?) catch |err| {
                    self.errctx.position = node.position;
                    return err;
                },
                .rd = self.allocateRegister(),
                .r1 = result,
            },
            .Amp => Instruction{
                .opcode = calculateStoreSize(unrtype.?) catch |err| {
                    self.errctx.position = node.position;
                    return err;
                },
                .rd = self.allocateRegister(),
                .r1 = result,
            },
            else => IRError.InvalidBlock,
        };

        return Value{
            .vtype = unrtype.?,
            .from = inner.from,
            .value = .{ .instruction = instruction },
        };
    }

    fn getBinaryOp(self: *IR, from: BlockOffset, node: *Node, lreg: SNIR.InfiniteRegister, rreg: SNIR.InfiniteRegister) !SNIR.Opcodes {
        const binary = node.data.bin;
        const isSigned =
            (binary.lhs.ntype == null or binary.rhs.ntype == null) or
            (binary.lhs.ntype.?.data.integer.start < 0 and binary.rhs.ntype.?.data.integer.start < 0);
        return switch (binary.op) {
            .Plus => .addr,
            .Min => .subr,
            .Star => .umulr,
            .Bar => .udivr,
            .Per => unreachable, // fix modulo someday ?
            .Amp => .andr,
            .Pipe => .orr,
            .Hat => .xorr,
            .Lsh => .llsr,
            .Rsh => .lrsr,
            .Cequ => {
                try self.addInstruction(from, Instruction{
                    .opcode = SNIR.Opcodes.xorr,
                    .rd = self.allocateRegister(),
                    .r1 = lreg,
                    .r2 = rreg,
                });

                return .setleur;
            },
            .Cneq => .xorr,
            .Cgt, .Clt => if (isSigned) .setgsr else .setgur,
            .Cge, .Cle => if (isSigned) .setlesr else .setleur,
            else => return IRError.InvalidOperation,
        };
    }

    fn blockBinary(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
        if (node.ntype.?.isValued())
            return self.constantFromType(block, node);

        const binary = node.data.bin;
        const bintype = node.ntype;
        const swap = binary.op == .Cge or binary.op == .Clt;

        const left = try self.blockNode(block, binary.lhs);
        const lreg = try self.appendIfConstant(left.from, left);

        const right = try self.blockNode(left.from, binary.rhs);
        const rreg = try self.appendIfConstant(right.from, right);

        // todo: add floating point support

        const instrOp = try self.getBinaryOp(right.from, node, lreg, rreg);

        return Value{
            .vtype = bintype.?,
            .from = right.from,
            .value = .{
                .instruction = Instruction{
                    .opcode = instrOp,
                    .rd = self.allocateRegister(),
                    .r1 = if (swap) rreg else lreg,
                    .r2 = if (swap) lreg else rreg,
                },
            },
        };
    }

    fn blockTernary(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
        if (node.ntype.?.isValued())
            return self.constantFromType(block, node);

        const ternary = node.data.ter;
        const ternarytype = node.ntype;
        node.ntype = null;

        const cond = try blockNode(self, block, ternary.cond);

        if (cond.value == .constant) {
            return blockNode(self, cond.from, if (cond.vtype.data.boolean.?) ternary.btrue else ternary.bfalse);
        }

        const trueblock = try self.newBlock();
        errdefer self.deinitBlock(trueblock);

        const btrue = try self.blockNode(trueblock, ternary.btrue);

        const falseblock = try self.newBlock();
        errdefer self.deinitBlock(falseblock);

        const bfalse = try self.blockNode(falseblock, ternary.bfalse);

        if (bfalse.value.instruction.rd != btrue.value.instruction.rd) {
            try self.addInstruction(trueblock, Instruction{
                .opcode = SNIR.Opcodes.orr, // best move instruction ever
                .rd = btrue.value.instruction.rd,
                .r1 = bfalse.value.instruction.rd,
            });
        }

        //     condition block
        //        /        \
        // true block    false block
        //        \        /
        //        merge block

        try self.newEdge(cond.from, trueblock);
        try self.newEdge(cond.from, falseblock);

        const mergeblock = try self.newBlock();
        errdefer self.deinitBlock(mergeblock);

        try self.newEdge(btrue.from, mergeblock);
        try self.newEdge(bfalse.from, mergeblock);

        // jump on false
        try self.addInstruction(cond.from, Instruction{
            .opcode = SNIR.Opcodes.blk_jmp,
            .r1 = @intFromEnum(SNIR.BlockJumpCondition.Equal),
            .r2 = btrue.value.instruction.rd,
            .rd = 0,
            .immediate = @truncate(falseblock),
        });

        try self.addInstruction(cond.from, Instruction{
            .opcode = SNIR.Opcodes.blk_jmp,
            .r1 = @intFromEnum(SNIR.BlockJumpCondition.Unconditional),
            .r2 = 0,
            .immediate = @truncate(trueblock),
        });

        return Value{
            .vtype = ternarytype.?,
            .from = mergeblock,
            .value = .{
                .instruction = Instruction{
                    .opcode = SNIR.Opcodes.blk_jmp,
                    .rd = @truncate(mergeblock),
                    .r1 = @intFromEnum(SNIR.BlockJumpCondition.Unconditional),
                    .r2 = 0,
                },
            },
        };
    }

    fn blockIntermediate(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
        const intrNode = node.data.intr;

        if (intrNode.intermediates.len == 0)
            return self.blockNode(block, intrNode.application);

        // dominant predecessor
        // |
        // |- intermediate 1 -\
        // |- intermediate 2 -|
        // |-      ...       -|
        // \- intermediate n -|
        //                    \- application block

        const post = try self.newBlock();

        for (intrNode.intermediates) |intermediate| {
            const intermediateValue = try self.blockNode(block, intermediate);
            self.newEdge(intermediateValue.from, post);
        }

        if (intrNode.application) |application|
            return try self.blockNode(post, application);

        // all intermediates need to have an application
        // the ones which dont usually are merged into other nodes
        unreachable;
    }

    fn blockCall(self: *IR, block: BlockOffset, node: *Node) !Value {
        var callee = try self.blockNode(block, node.data.call.callee);
        var caller = try self.blockNode(callee.from, node.data.call.caller);

        // Handle partial application
        const expected_args = callee.vtype.expectedArgs();

        if (expected_args > 1) {
            if (caller.vtype.partialArgs) |pa| {
                for (0..pa.len) |i| {
                    if (pa[i]) |v| {
                        @constCast(&v).* = caller;
                        break;
                    }
                }
            } else {
                caller.vtype.partialArgs = try self.alloc.alloc(?Value, expected_args);
                @constCast(&caller.vtype.partialArgs.?[0]).* = callee;
            }
            return callee;
        }

        // Handle full application
        const instr = Instruction{
            .opcode = .call,
            .rd = self.allocateRegister(),
            .r1 = caller.value.instruction.rd,
            .r2 = callee.value.instruction.rd,
        };

        try self.addInstruction(caller.from, instr);

        return Value{
            .vtype = callee.vtype.data.function.ret,
            .value = .{ .instruction = instr },
        };
    }

    fn blockExpr(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
        // top level expression

        const initialBlock = block;

        if (block == 0) {
            // refresh registers
            self.regIndex = 8;
        }

        return self.blockNode(initialBlock, node.data.expr.expr.?);
    }
};

///////////

pub const Optimizations = struct {};

/// Pipelines the entire compiling process, its the compiler main function
pub fn pipeline(name: String, alloc: std.mem.Allocator) !u8 {
    const stdout_file = std.io.getStdOut().writer();
    var bw = std.io.bufferedWriter(stdout_file);
    var stdout = bw.writer();

    var file = std.fs.cwd().openFile(name, .{}) catch |err| {
        try stdout.print("Could not open file \"{s}\": {s}\n", .{ name, @errorName(err) });
        try bw.flush();
        return 255;
    };

    defer file.close();

    const codeLength = (try file.stat()).size;

    const buffer = try alloc.alloc(u8, codeLength);
    defer alloc.free(buffer);

    const result = try file.reader().readAll(buffer);

    if (result != codeLength) return error.EndOfStream;

    var parser = Parser.init(buffer, alloc);
    var analyzer = try Analyzer.init(alloc);
    defer analyzer.deinit();
    var ir = IR.init(alloc);
    defer ir.deinit();

    const node = parser.parseNode() catch |err| {
        try misc.printError(stdout, name, parser.code, err, parser.errctx);
        try bw.flush();
        return 254;
    };

    defer node.deinit(alloc);

    const typed = analyzer.runAnalysis(node) catch |err| {
        try misc.printError(stdout, name, parser.code, err, analyzer.errctx);
        try bw.flush();
        return 253;
    };

    try bw.flush();

    ir.fromNode(typed) catch |err| {
        try misc.printError(stdout, name, parser.code, err, ir.errctx);
        try bw.flush();
        return 252;
    };

    try misc.printIR(&ir, stdout);

    try stdout.print("analysis successful\n", .{});
    try bw.flush();

    return 0;
}
