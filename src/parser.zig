//! Translates the source code into an AST.

const std = @import("std");
const misc = @import("misc.zig");
const Lexer = @import("lexer.zig");
const Types = @import("types.zig");
const Node = @import("node.zig");

/// Errors thrown by the parser
pub const Error = error{
    /// Could not request a token
    LexerEnd,

    /// Parenthesis not closed
    UnclosedParen,

    /// Unexpected token
    UnexpectedToken,
} || Lexer.Error || error{OutOfMemory};

const Parser = @This();

/// The source string
code: misc.String,

/// Current position in the source code
pos: misc.Pos = .{},

/// Node allocator
alloc: std.mem.Allocator,

/// Error context
errctx: misc.ErrorContext = .{},

/// Lexer instance
lexer: Lexer,

/// Indicates if the parser is currently parsing a module
inModule: bool = false,

/// Merges two positions
fn newPosition(start: misc.Pos, end: misc.Pos) misc.Pos {
    return .{
        .index = start.index,
        .span = end.index - start.index + end.span,
    };
}

/// Get operator precedence
fn getPrecedence(val: Lexer.Tokens) i32 {
    return switch (val) {
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

/// Generate an unexpected token error
fn generateUnexpected(self: *Parser, expected: []const Lexer.Tokens, got: Lexer.Token) Error!void {
    self.errctx = .{
        .value = .{
            .UnexpectedToken = expected,
        },
        .position = got.pos,
    };
    return error.UnexpectedToken;
}

const IntrFlags = enum { noApp, normal, skipLcur };

fn parseIntr(self: *Parser, flags: IntrFlags) Error!*Node {
    const open = try self.lexer.consume();
    const start = open.pos;

    var intermediates: [16]*Node = undefined;
    var idx: usize = 0;

    errdefer for (intermediates, 0..) |intr, i| {
        if (i >= idx) break;
        intr.deinit(self.alloc);
    };

    const lcur = try self.lexer.consume();
    if (flags != .skipLcur and lcur.val != .Lcur)
        try self.generateUnexpected(&[_]Lexer.Tokens{.Lcur}, lcur);

    if (flags != .skipLcur)
        self.lexer = self.lexer.catchUp(lcur);

    for (&intermediates, 0..) |*int, i| {
        const rcur = try self.lexer.consume();
        if (rcur.val == .Rcur or i >= 16) break;

        int.* = try self.parseStatement();
        const semi = try self.lexer.consume();
        if (semi.val != .Semi)
            try self.generateUnexpected(&[_]Lexer.Tokens{ .Semi }, semi);
        self.lexer = self.lexer.catchUp(semi);
        idx = i;
    }

    const rcur = try self.lexer.consume();

    if (rcur.val != .Rcur)
        try self.generateUnexpected(&[_]Lexer.Tokens{.Rcur}, rcur);

    self.lexer = self.lexer.catchUp(rcur);

    const actualImm = try self.alloc.alloc(*Node, idx + 1);
    errdefer self.alloc.free(actualImm);
    @memcpy(actualImm, intermediates[0 .. idx + 1]);

    if (flags == IntrFlags.noApp or self.inModule) {
        const node = try self.alloc.create(Node);
        node.* = .{
            .position = newPosition(start, rcur.pos),
            .data = .{
                .intr = .{
                    .intermediates = actualImm,
                    .application = null,
                },
            },
        };

        return node;
    }

    const application = try self.parsePrimary();
    errdefer application.deinit(self.alloc);

    const node = try self.alloc.create(Node);
    node.* = .{
        .position = newPosition(start, application.position),
        .data = .{
            .intr = .{
                .intermediates = actualImm,
                .application = application,
            },
        },
    };
    return node;
}

fn parseModule(self: *Parser) Error!*Node {
    const module = try self.lexer.consume();
    const start = module.pos;

    self.lexer = self.lexer.catchUp(module);

    const direction = try self.lexer.consume();

    if (direction.val != .Rsh and direction.val != .Lsh)
        try self.generateUnexpected(&[_]Lexer.Tokens{ .Lsh, .Rsh }, direction);

    self.lexer = self.lexer.catchUp(direction);

    const name = try self.lexer.consume();

    if (name.val != .Str)
        try self.generateUnexpected(&[_]Lexer.Tokens{.{ .Str = "" }}, name);

    self.lexer = self.lexer.catchUp(name);
    self.inModule = true;

    const content = try self.parseIntr(.noApp);
    errdefer content.deinit(self.alloc);

    self.inModule = false;

    const node = try self.alloc.create(Node);
    node.* = .{
        .position = newPosition(start, content.position),
        .data = .{
            .mod = .{
                .path = name.val.Str,
                .nodes = content.data.intr.intermediates,
                .direction = direction.val == .Lsh,
            },
        },
        .ntype = undefined,
    };
    return node;
}

fn parsePrimary(self: *Parser) Error!*Node {
    const token = try self.lexer.consume();
    self.lexer = self.lexer.catchUp(token);
    return switch (token.val) {
        .Int => |v| try Node.initInt(self.alloc, token.pos, v),
        .Dec => |v| try Node.initDec(self.alloc, token.pos, v),
        .Str => |v| try Node.initStr(self.alloc, token.pos, v),
        .Ref => |v| try Node.initRef(self.alloc, token.pos, v),
        .Lpar => {
            const expr = try self.parseExpr();
            errdefer expr.deinit(self.alloc);

            const rpar = try self.lexer.consume();
            if (rpar.val != .Rpar) 
                try self.generateUnexpected(&[_]Lexer.Tokens{.Rpar}, rpar);

            self.lexer = self.lexer.catchUp(rpar);
            expr.position = newPosition(token.pos, rpar.pos);
            return expr;
        },
        .Lcur => try self.parseIntr(IntrFlags.skipLcur),
        else => {
            self.pos = token.pos;
            try self.generateUnexpected(&[_]Lexer.Tokens{
                .{ .Int = 0 },
                .{ .Dec = 0 },
                .{ .Ref = "" },
                .{ .Str = "" },
                .Lpar,
            }, token);
            unreachable;
        },
    };
}

fn isAtomic(val: Lexer.Tokens) bool {
    return switch (val) {
        .Int, .Dec, .Ref, .Str, .Lpar => true,
        else => false,
    };
}
const debug = @import("debug.zig");

fn parseCall(self: *Parser) Error!*Node {
    var caller = try self.parsePrimary();
    errdefer caller.deinit(self.alloc);

    while (true) {
        const token = try self.lexer.consume();
        if (!isAtomic(token.val)) {
            return caller;
        }

        const callee = try self.parsePrimary();
        errdefer callee.deinit(self.alloc);

        caller = try Node.initCall(self.alloc, token.pos, caller, callee);
        errdefer caller.deinit(self.alloc);
    }
}

fn parseUnary(self: *Parser) Error!*Node {
    const token = try self.lexer.consume();
    switch (token.val) {
        .Min, .Plus, .Til, .Bang, .Star, .Amp => {},
        else => return self.parseCall(),
    }

    self.lexer = self.lexer.catchUp(token);
    const operand = try self.parseUnary();
    errdefer operand.deinit(self.alloc);

    return Node.initUnr(self.alloc, token.pos, token.val, operand);
}

fn parseBinary(self: *Parser, precedence: i32) Error!*Node {
    var left = try self.parseUnary();
    errdefer left.deinit(self.alloc);

    while (true) {
        const token = try self.lexer.consume();
        const tokenPrecedence = getPrecedence(token.val);
        if (tokenPrecedence < precedence)
            return left;

        self.lexer = self.lexer.catchUp(token);
        const right = try self.parseBinary(tokenPrecedence + 1);
        errdefer right.deinit(self.alloc);

        left = try Node.initBin(self.alloc, token.pos, token.val, left, right);
        errdefer left.deinit(self.alloc);
    }
}

fn parseTernary(self: *Parser) Error!*Node {
    const condition = try self.parseBinary(1);
    errdefer condition.deinit(self.alloc);

    const question = try self.lexer.consume();
    if (question.val != .Ques) return condition;

    self.lexer = self.lexer.catchUp(question);

    const thenExpr = try self.parseBinary(1);
    errdefer thenExpr.deinit(self.alloc);

    const colon = try self.lexer.consume();
    if (colon.val != .Cln) {
        try self.generateUnexpected(&[_]Lexer.Tokens{.Cln}, colon);
    }

    self.lexer = self.lexer.catchUp(colon);

    const elseExpr = try self.parseTernary();
    errdefer elseExpr.deinit(self.alloc);

    return Node.initTer(self.alloc, newPosition(condition.position, elseExpr.position), condition, thenExpr, elseExpr);
}

inline fn parseExpr(self: *Parser) Error!*Node {
    return self.parseTernary();
}

fn parseRange(self: *Parser) Error!*Node {
    const lsq = try self.lexer.consume();
    if (lsq.val != .Lsqb) try self.generateUnexpected(&[_]Lexer.Tokens{.Lsqb}, lsq);

    self.lexer = self.lexer.catchUp(lsq);
    const start = try self.parseExpr();
    errdefer start.deinit(self.alloc);

    const semi = try self.lexer.consume();
    if (semi.val != .Semi) try self.generateUnexpected(&[_]Lexer.Tokens{.Semi}, semi);

    self.lexer = self.lexer.catchUp(semi);
    const end = try self.parseExpr();
    errdefer end.deinit(self.alloc);

    const semi2 = try self.lexer.consume();
    if (semi2.val != .Semi) try self.generateUnexpected(&[_]Lexer.Tokens{.Semi}, semi2);

    self.lexer = self.lexer.catchUp(semi2);
    const epsilon = try self.parseExpr();
    errdefer epsilon.deinit(self.alloc);

    const rsq = try self.lexer.consume();

    if (rsq.val != .Rsqb) try self.generateUnexpected(&[_]Lexer.Tokens{.Rsqb}, rsq);

    self.lexer = self.lexer.catchUp(rsq);

    const range = try self.alloc.create(Node);
    range.* = .{
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

fn parseTPrimary(self: *Parser) Error!*Node {
    const tok = try self.lexer.consume();
    return switch (tok.val) {
        .Lsqb => self.parseRange(),
        .Ref => self.parseProd(),
        .Star => {
            self.lexer = self.lexer.catchUp(tok);
            const val = try self.parseTPrimary();
            const ptr = try self.alloc.create(Node);
            errdefer self.alloc.destroy(ptr);
            ptr.* = .{
                .position = newPosition(tok.pos, val.position),
                .data = .{
                    .unr = .{ .op = .Star, .val = ptr },
                },
            };
            return ptr;
        },
        else => {
            try self.generateUnexpected(&[_]Lexer.Tokens{ .Lsqb, .{.Ref = ""}, .Star }, tok);

            // generateUnexpected **will** throw, this is just for compliance
            unreachable;
        },
    };
}

fn getTypePrecedence(tok: Lexer.Tokens) i3 {
    return switch (tok) {
        .Arrow => 1,
        .Hash => 2,
        else => -1,
    };
}

fn parseTBinary(self: *Parser, prec: i3) Error!*Node {
    var left = try self.parseTPrimary();
    errdefer left.deinit(self.alloc);

    while (true) {
        const token = try self.lexer.consume();
        const tokenPrecedence = getTypePrecedence(token.val);
        if (tokenPrecedence < prec)
            return left;

        self.lexer = self.lexer.catchUp(token);
        const next_prec: i3 = if (token.val == .Arrow) tokenPrecedence else tokenPrecedence + 1;

        const right = try self.parseTBinary(next_prec);
        errdefer right.deinit(self.alloc);

        left = try Node.initBin(self.alloc, token.pos, token.val, left, right);
        errdefer left.deinit(self.alloc);
    }
}

fn parseSum(self: *Parser) Error!*Node {
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
        if (token.val != .Bar)
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

fn parseProd(self: *Parser) Error!*Node {
    const ctor = try self.lexer.consume();
    const start = ctor.pos;
    if (ctor.val != .Ref) try self.generateUnexpected(&[_]Lexer.Tokens{.{.Ref = ""}}, ctor);

    self.lexer = self.lexer.catchUp(ctor);

    const open = try self.lexer.consume();
    if (open.val != .Lcur)
        return Node.initRef(self.alloc, ctor.pos, ctor.val.Ref);

    self.lexer = self.lexer.catchUp(open);

    var array = std.ArrayList(*Node).init(self.alloc);

    var closed = try self.lexer.consume();
    while (closed.val != .Rcur) : (closed = try self.lexer.consume()) {
        try array.append(try self.parseType());
    }

    self.lexer = self.lexer.catchUp(closed);
    const prod = try self.alloc.create(Node);
    prod.* = .{
        .position = newPosition(start, closed.pos),
        .data = .{
            .aggr = .{
                .name = ctor.val.Ref,
                .children = array.items,
            },
        },
    };

    return prod;
}

inline fn parseType(self: *Parser) Error!*Node {
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

    while (isAtomic((try self.lexer.consume()).val) or (try self.lexer.consume()).val == .Lpar) : (pidx += 1) {
        params[pidx] = try parsePrimary(self);
    }

    var exprType: ?*Node = null;
    var expr: ?*Node = null;
    const colon = try self.lexer.consume();
    if (colon.val == .Cln) {
        self.lexer = self.lexer.catchUp(colon);
        exprType = try self.parseType();
    }

    const equal = try self.lexer.consume();
    if (equal.val == .Equ) {
        self.lexer = self.lexer.catchUp(equal);
        expr = try self.parseExpr();
    }

    if (exprType == null and expr == null)
        return error.MalformedToken;

    const endPos = (expr orelse exprType).?.position;

    const pos = newPosition(start, endPos);
    const actParams = try self.alloc.alloc(*Node, pidx);
    errdefer self.alloc.free(actParams);

    @memcpy(actParams, params[0..pidx]);

    const semi = try self.lexer.consume();
    if (semi.val != .Semi) {
        try self.generateUnexpected(&[_]Lexer.Tokens{.Semi}, colon);
    }

    self.lexer = self.lexer.catchUp(semi);

    return Node.initExpr(self.alloc, pos, name.val.Ref, actParams, exprType, expr);
}

pub fn parseNode(self: *Parser) Error!*Node {
    const token = try self.lexer.consume();
    switch (token.val) {
        .Mod => return self.parseModule(),
        .Ref => return self.parseStatement(),
        else => {
            try self.generateUnexpected(&[_]Lexer.Tokens{ .Mod, .{.Ref = ""} }, token);
            unreachable;
        },
    }
}

pub fn parseNodes(self: *Parser) Error!std.ArrayListUnmanaged(*Node) {
    var nodes = std.ArrayListUnmanaged(*Node){};
    errdefer nodes.deinit(self.alloc);

    while (!self.lexer.end()) {
        const node = try self.parseNode();
        try nodes.append(self.alloc, node);
    }

    return nodes;
}

pub fn init(code: misc.String, alloc: std.mem.Allocator) Parser {
    return Parser{
        .code = code,
        .alloc = alloc,
        .lexer = Lexer.init(code),
    };
}

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;

test "Parser - Primary int" {
    const code = "test = 1;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();
    defer node.deinit(alloc);

    const pos = misc.Pos{};

    const expected = try Node.initExpr(alloc, pos, "test", &[_]*Node{}, null, try Node.initInt(alloc, pos, 1));
    defer expected.deinit(alloc);

    try expect(node.eql(expected));
}

test "Parser - Primary float" {
    const code = "test = 1.0;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();
    defer node.deinit(alloc);

    const pos = misc.Pos{};
    const expected = try Node.initExpr(alloc, pos, "test", &[_]*Node{}, null, try Node.initDec(alloc, pos, 1.0));
    defer expected.deinit(alloc);

    try expect(node.eql(expected));
}

test "Parser - Primary string" {
    const code = "test = \"test\";";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();
    defer node.deinit(alloc);

    const pos = misc.Pos{};
    const expected = try Node.initExpr(alloc, pos, "test", &[_]*Node{}, null, try Node.initStr(alloc, pos, "test"));
    defer expected.deinit(alloc);

    try expect(node.eql(expected));
}

test "Parser - Primary reference" {
    const code = "test = test;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();
    defer node.deinit(alloc);

    const pos = misc.Pos{};
    const expected = try Node.initExpr(alloc, pos, "test", &[_]*Node{}, null, try Node.initRef(alloc, pos, "test"));
    defer expected.deinit(alloc);

    try expect(node.eql(expected));
}

test "Parser - Unary" {
    const code = "test = -1;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parseNode();
    defer node.deinit(alloc);

    const pos = misc.Pos{};
    const expected = try Node.initExpr(alloc, pos, "test", &[_]*Node{}, null, try Node.initUnr(alloc, pos, .Min, try Node.initInt(alloc, pos, 1)));
    defer expected.deinit(alloc);

    try expect(node.eql(expected));
}

test "Parser - Binary" {
    const code = "test = 1 + 2;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parseNode();
    defer node.deinit(alloc);

    const pos = misc.Pos{};
    const expected = try Node.initExpr(alloc, pos, "test", &[_]*Node{}, null, try Node.initBin(alloc, pos, .Plus, try Node.initInt(alloc, pos, 1), try Node.initInt(alloc, pos, 2)));

    defer expected.deinit(alloc);
    try expect(node.eql(expected));
}

test "Parser - Ternary" {
    const code = "test = 1 ? 2 : 3;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parseNode();
    defer node.deinit(alloc);

    const pos = misc.Pos{};
    const expected = try Node.initExpr(alloc, pos, "test", &[_]*Node{}, null, try Node.initTer(alloc, pos, try Node.initInt(alloc, pos, 1), try Node.initInt(alloc, pos, 2), try Node.initInt(alloc, pos, 3)));
    defer expected.deinit(alloc);

    try expect(node.eql(expected));
}

test "Parser - Intr" {
    const code =
        \\test = {
        \\  va = 1;
        \\  vb = 2;
        \\} (va + vb);
    ;
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parseNode();
    defer node.deinit(alloc);
    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(0, node.data.expr.params.len);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .intr);
}

test "Parser - Params" {
    const code = "test a b c d e f g h = a;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parseNode();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(8, node.data.expr.params.len);
    try expect(node.data.expr.params[0].data == .ref);
    try expect(std.mem.eql(u8, node.data.expr.params[0].data.ref, "a"));
    try expect(node.data.expr.params[7].data == .ref);
    try expect(std.mem.eql(u8, node.data.expr.params[7].data.ref, "h"));
}
