const std = @import("std");
const misc = @import("misc.zig");
const ErrorContext = misc.ErrorContext;

const Lexer = @This();

/// Possible token types
pub const Tokens = union(enum) {
    /// Integer tokens
    Int: i64,

    /// Decimal Tokens
    Dec: f64,

    /// String Tokens
    Str: misc.String,

    /// Reference Tokens
    Ref: misc.String,

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
pub const Error = error{
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

/// Lexer Token
pub const Token = struct {
    /// Token value
    val: Tokens,

    /// Position in the code
    pos: misc.Pos = .{},

    /// init a token 
    pub fn init(tid: Tokens, pos: misc.Pos) Token {
        return Token{ .val = tid, .pos = pos };
    }
};

/// Code string
code: misc.String,

/// Index of current character
index: usize = 0,

/// Last token consumed, multiple calls of consume() without a
/// call to catchUp() return early
last: ?Token = null,

/// Context for error printing
errctx: ErrorContext = .{ .value = .NoContext },

/// Initialize a lexer
pub fn init(code: misc.String) Lexer {
    return Lexer{ .code = code };
}

/// Check if end without erroring out
inline fn end(self: Lexer) bool {
    return self.code.len <= self.index;
}

/// Get current character, errors out on end
fn current(self: Lexer) Error!u8 {
    return if (self.end()) error.LexerEnd else self.code[self.index];
}

/// Checks if the current character against a constant errorless
fn char(self: Lexer, c: u8) bool {
    return if (self.end()) false else self.code[self.index] == c;
}

/// Performs isAlpha without erroring out
fn isAlpha(self: Lexer) bool {
    return if (self.end()) false else std.ascii.isAlphabetic(self.code[self.index]);
}

/// Performs isDigit without erroring out
fn isNumeric(self: Lexer) bool {
    return if (self.end()) false else std.ascii.isDigit(self.code[self.index]);
}

/// Performs isAlphanumeric without erroring out
fn isAlphanumeric(self: Lexer) bool {
    return if (self.end()) false else std.ascii.isAlphanumeric(self.code[self.index]);
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
    while (!lex.end()) {
        if (lex.code[lex.index] != ' '
        and lex.code[lex.index] != '\n'
        and lex.code[lex.index] != '\r'
        and lex.code[lex.index] != '\t')
            break;
        lex = lex.walk();
    }
    return lex;
}

/// Skip comments
fn skipComments(self: Lexer) Lexer {
    // it should work for multiple sequential comments
    var str = self.code[self.index..];
    var lex = self;
    while (str[0] == '/' and str[1] == '/') : (str = lex.code[lex.index..]) {
        while (lex.code[lex.index] != '\n')
            lex = lex.walk();
        lex = lex.consumeWhitespace().walk();
    }
    return lex.consumeWhitespace();
}

/// Consume a string, errors only if it is incomplete
fn consumeString(self: Lexer) Error!Token {
    var lex = self.walk();
    const start = lex.index;
    while (!lex.char('"') and !lex.end())
        lex = lex.walk();

    if (lex.end()) {
        lex.errctx = ErrorContext{
            .position = misc.Pos{ .index = start, .span = lex.index - start },
            .value = .MalformedToken,
        };
        return error.MalformedString;
    }
    lex = lex.walk();

    return Token{
        .val = .{ .Str = self.code[start .. lex.index - 1] },
        .pos = misc.Pos{
            .index = start - 1,
            .span = lex.index - start + 1,
        },
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
        .pos = misc.Pos{
            .index = start,
            .span = lex.index - start,
        },
        .val = .{
            .Ref = lex.code[start..lex.index],
        },
    };
}

/// Consume an integer or a rational
fn consumeNumber(self: Lexer) Error!Token {
    var isInt = true;
    const start = self.index;
    var lex = self;

    while (lex.isNumeric())
        lex = lex.walk();

    if (lex.char('.')) {
        isInt = false;
        lex = lex.walk();
        while (lex.isNumeric())
            lex = lex.walk();
    }

    if (lex.char('e')) {
        isInt = false;
        lex = lex.walk();
        if (lex.char('+') or lex.char('-'))
            lex = lex.walk();

        if (!lex.isNumeric())
            return error.MalformedNumber;

        while (lex.isNumeric())
            lex = lex.walk();
    }
    const rpos = misc.Pos{ .index = start, .span = lex.index - start };

    const str = lex.code[start .. start + rpos.span];

    if (isInt)
        return Lexer.parseInt(str, rpos);

    return Lexer.parseDec(str, rpos);
}

/// Parse an int string into an integer token
fn parseInt(str: misc.String, pos: misc.Pos) Token {
    var result: i64 = 0;

    for (0..str.len) |idx|
        result = result * 10 + (str[idx] - '0');

    return Token{
        .pos = pos,
        .val = .{ .Int = result },
    };
}

fn parseDec(str: misc.String, pos: misc.Pos) Token {
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
        .pos = pos,
        .val = .{ .Dec = result * std.math.pow(f64, 10, sign * exp) },
    };
}

/// Check if the current token is one of the given tokens
/// Returns the index of the first token that matches, or null if none match
/// This cannot fail, just returns null
pub fn is_any(self: *Lexer, tokens: []const Tokens, skip: bool) ?Token {
    const token = self.consume() catch return null;

    if (skip) self.* = self.catchUp(token);

    for (tokens) |t| {
        if (token.value == t) return token;
    }
    return null;
}

pub fn skipToken(self: *Lexer) Error!void {
    const token = try self.consume();
    self.catchUp(token);
}

pub fn consume(self: *Lexer) Error!Token {
    if (self.last) |last| return last;

    var lex = self.skipComments();

    if (lex.isNumeric()) {
        self.last = lex.consumeNumber() catch {
            self.errctx = ErrorContext{
                .position = misc.Pos{
                    .index = lex.index,
                    .span = 1,
                },
                .value = .MalformedToken,
            };
            return error.MalformedToken;
        };
        return self.last.?;
    }

    if (lex.char('"')) {
        self.last = lex.consumeString() catch {
            self.errctx = ErrorContext{
                .position = misc.Pos{
                    .index = lex.index,
                    .span = 1,
                },
                .value = .MalformedToken,
            };
            return error.MalformedString;
        };
        return self.last.?;
    }

    const str = lex.code[lex.index..];

    if (std.mem.startsWith(u8, str, "module")) {
        const tok = Token.init(.Mod, misc.Pos{ .index = lex.index, .span = 6 });
        self.last = tok;
        return tok;
    }

    if (std.mem.startsWith(u8, str, "match")) {
        const tok = Token.init(.Match, misc.Pos{ .index = self.index, .span = 5 });
        self.last = tok;
        return tok;
    }

    if (lex.isAlpha() or lex.char('_')) {
        self.last = lex.consumeIdentifier();
        return self.last.?;
    }

    const pos = misc.Pos{ .index = lex.index, .span = 1 };
    const curr = lex.current() catch {
        self.errctx = ErrorContext{ .position = pos, .value = .MalformedToken };
        return error.MalformedToken;
    };

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
        '^' => Token.init(.Hat, pos),
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
        '.' => Token.init(.Dot, pos),
        else => null,
    };

    self.last = tok;

    if (tok) |t|
        return t;

    self.errctx = ErrorContext{ .position = pos, .value = .MalformedToken, };

    return error.MalformedToken;
}

/// generate either token `small`, or `big` if next char is `chr`
fn generate(self: Lexer, chr: u8, small: Tokens, big: Tokens) Token {
    const p1 = misc.Pos{ .index = self.index, .span = 1 };
    const p2 = misc.Pos{ .index = self.index, .span = 2 };

    if (self.walk().char(chr))
        return Token.init(big, p2);
    return Token.init(small, p1);
}

/// generate `t1` if next char is `c1`, else `t2` if `c2`, `telse` otherwise
fn doubleGenerate(self: Lexer, c1: u8, c2: u8, t1: Tokens, t2: Tokens, telse: Tokens) Token {
    const p1 = misc.Pos{ .index = self.index, .span = 1 };
    const p2 = misc.Pos{ .index = self.index, .span = 2 };

    if (self.walk().char(c1)) return Token.init(t1, p2);
    if (self.walk().char(c2)) return Token.init(t2, p2);
    return Token.init(telse, p1);
}

/// After consuming a token, go to the next position
pub fn catchUp(self: Lexer, token: Token) Lexer {
    return Lexer{
        .code = self.code,
        .index = token.pos.index + token.pos.span,
    };
}

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
////////// lexer tests

test Lexer {
    var lexer = Lexer.init(
        \\0 1 0.0 1.0 1e2 1.3e2 
        \\"hello" "" 
        \\identifier a b'
        \\(){}[]=;+-*/&~!#|^%<<>>==!=><>=<=&&||?:.->
        \\module match 
    );

    const tokens = [_]Token{
        Token{ .val = .{ .Int = 0 } },
        Token{ .val = .{ .Int = 1 } },
        Token{ .val = .{ .Dec = 0.0 } },
        Token{ .val = .{ .Dec = 1.0 } },
        Token{ .val = .{ .Dec = 100 } },
        Token{ .val = .{ .Dec = 130 } },
        Token{ .val = .{ .Str = "hello" } },
        Token{ .val = .{ .Str = "" } },
        Token{ .val = .{ .Ref = "identifier" } },
        Token{ .val = .{ .Ref = "a" } },
        Token{ .val = .{ .Ref = "b'" } },
        Token{ .val = .Lpar },
        Token{ .val = .Rpar },
        Token{ .val = .Lcur },
        Token{ .val = .Rcur },
        Token{ .val = .Lsqb },
        Token{ .val = .Rsqb },
        Token{ .val = .Equ },
        Token{ .val = .Semi },
        Token{ .val = .Plus },
        Token{ .val = .Min },
        Token{ .val = .Star },
        Token{ .val = .Bar },
        Token{ .val = .Amp },
        Token{ .val = .Til },
        Token{ .val = .Bang },
        Token{ .val = .Hash },
        Token{ .val = .Pipe },
        Token{ .val = .Hat },
        Token{ .val = .Per },
        Token{ .val = .Lsh },
        Token{ .val = .Rsh },
        Token{ .val = .Cequ },
        Token{ .val = .Cneq },
        Token{ .val = .Cgt },
        Token{ .val = .Clt },
        Token{ .val = .Cge },
        Token{ .val = .Cle },
        Token{ .val = .And },
        Token{ .val = .Or },
        Token{ .val = .Ques },
        Token{ .val = .Cln },
        Token{ .val = .Dot },
        Token{ .val = .Arrow },
        Token{ .val = .Mod },
        Token{ .val = .Match },
    };

    for (tokens) |token| {
        const tok = try lexer.consume();
        try expectEqual(std.meta.activeTag(token.val), std.meta.activeTag(tok.val));
        switch (token.val) {
            .Int => |v| try expectEqual(v, tok.val.Int),
            .Dec => |v| try expectEqual(v, tok.val.Dec),
            .Str => |v| try expect(std.mem.eql(u8, v, tok.val.Str)),
            .Ref => |v| try expect(std.mem.eql(u8, v, tok.val.Ref)),
            else => {}
        }
        lexer = lexer.catchUp(tok);
    }
}
