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
errctx: misc.ErrorContext,

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

/// Integer node factory
fn createIntNode(self: Parser, position: misc.Pos, value: i64) !*Node {
    const node = try self.alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .int = value },
    };
    return node;
}

/// Decimal node factory
fn createDecNode(self: Parser, position: misc.Pos, value: f64) !*Node {
    const node = try self.alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .dec = value },
    };
    return node;
}

/// String node factory
fn createStrNode(self: Parser, position: misc.Pos, value: misc.String) !*Node {
    const node = try self.alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .str = value },
    };
    return node;
}

/// Reference node factory
fn createRefNode(self: Parser, position: misc.Pos, value: misc.String) !*Node {
    const node = try self.alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{ .ref = value },
    };
    return node;
}

/// Unary node factory
fn createUnrNode(self: Parser, position: misc.Pos, operator: Lexer.Tokens, value: *Node) !*Node {
    const node = try self.alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{
            .unr = .{
                .val = value,
                .op = operator,
            },
        },
    };
    return node;
}

/// Binary node factory
fn createBinNode(self: Parser, position: misc.Pos, operator: Lexer.Tokens, lhs: *Node, rhs: *Node) !*Node {
    const node = try self.alloc.create(Node);
    node.* = .{
        .position = position,
        .data = .{
            .bin = .{
                .lhs = lhs,
                .rhs = rhs,
                .op = operator,
            },
        },
    };
    return node;
}

/// Call node factory
fn createCallNode(self: Parser, position: misc.Pos, caller: *Node, callee: *Node) !*Node {
    const node = try self.alloc.create(Node);
    node.* = Node{
        .position = position,
        .data = .{ .call = .{ .caller = caller, .callee = callee } },
    };
    return node;
}

/// Expression node factory
pub fn createExprNode(self: Parser, position: misc.Pos, name: misc.String, params: []*Node, exprType: ?*Node, expr: ?*Node) !*Node {
    const node = try self.alloc.create(Node);
    node.* = Node{
        .position = position,
        .data = .{ .expr = .{ .name = name, .params = params, .ntype = exprType, .expr = expr } },
    };
    return node;
}

/// Get operator precedence
fn getPrecedence(tid: Lexer.Tokens) i32 {
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

/// Generate an unexpected token error
fn generateUnexpected(self: *Parser, expected: Lexer.Tokens, got: Lexer.Token) Error!void {
    self.errctx = .{
        .value = .{
            .UnexpectedToken = .{
                .expected = expected,
                .token = got,
            },
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
        node.* = Node{
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
    node.* = Node{
        .position = newPosition(start, application.position),
        .data = .{ .intr = .{
            .intermediates = actualImm,
            .application = application,
        } },
    };
    return node;
}

fn parseModule(self: *Parser) Error!*Node {
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

fn parsePrimary(self: *Parser) Error!*Node {
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
                return error.UnclosedParen;

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

fn isAtomic(tid: Lexer.Tokens) bool {
    return switch (tid) {
        .Int, .Dec, .Ref, .Str, .Lpar => true,
        else => false,
    };
}

fn parseCall(self: *Parser) Error!*Node {
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

fn parseUnary(self: *Parser) Error!*Node {
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

fn parseBinary(self: *Parser, precedence: i32) Error!*Node {
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

fn parseTernary(self: *Parser) Error!*Node {
    const condition = try self.parseBinary(1);
    errdefer condition.deinit(self.alloc);

    const question = try self.lexer.consume();
    if (question.tid != .Ques) return condition;

    self.lexer = self.lexer.catchUp(question);

    const thenExpr = try self.parseBinary(1);
    errdefer thenExpr.deinit(self.alloc);

    const colon = try self.lexer.consume();
    if (colon.tid != .Cln) {
        self.errctx = .{
            .value = .{ .UnexpectedToken = .{
                .expected = .Cln,
                .token = colon,
            } },
            .position = colon.pos,
        };
    }

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

inline fn parseExpr(self: *Parser) Error!*Node {
    return self.parseTernary();
}

fn parseRange(self: *Parser) Error!*Node {
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

fn parseTPrimary(self: *Parser) Error!*Node {
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
        else => {
            try self.generateUnexpected(Lexer.Tokens.Lsqb, tok);

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
        const tokenPrecedence = getTypePrecedence(token.tid);
        if (tokenPrecedence < prec)
            return left;

        self.lexer = self.lexer.catchUp(token);
        const next_prec: i3 = if (token.tid == .Arrow) tokenPrecedence else tokenPrecedence + 1;

        const right = try self.parseTBinary(next_prec);
        errdefer right.deinit(self.alloc);

        left = try self.createBinNode(token.pos, token.tid, left, right);
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

fn parseProd(self: *Parser) Error!*Node {
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
        return error.MalformedToken;

    const endPos = (expr orelse exprType).?.position;

    const pos = newPosition(start, endPos);
    const actParams = try self.alloc.alloc(*Node, pidx);
    errdefer self.alloc.free(actParams);

    @memcpy(actParams, params[0..pidx]);
    return self.createExprNode(pos, name.val.str, actParams, exprType, expr);
}

pub fn parseNode(self: *Parser) Error!*Node {
    const token = try self.lexer.consume();
    switch (token.tid) {
        .Mod => return self.parseModule(),
        .Ref => return self.parseStatement(),
        else => {
            self.pos = token.pos;
            return error.UnexpectedToken;
        },
    }
}

pub fn init(code: misc.String, alloc: std.mem.Allocator) Parser {
    return Parser{
        .errctx = .{ .value = .NoContext },
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

    const expected = try parser.createExprNode(pos, "test", .{}, null, try parser.createIntNode(1, 1, 1));
    defer expected.deinit();

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(0, node.data.expr.params.len);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .int);
    try expectEqual(1, node.data.expr.expr.?.data.int);
}

test "Parser - Primary float" {
    const code = "test = 1.0;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(0, node.data.expr.params.len);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .dec);
    try expectEqual(1.0, node.data.expr.expr.?.data.dec);
}

test "Parser - Primary string" {
    const code = "test = \"test\";";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(0, node.data.expr.params.len);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .str);
    try expect(std.mem.eql(u8, node.data.expr.expr.?.data.str, "test"));
}

test "Parser - Primary reference" {
    const code = "test = test;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(0, node.data.expr.params.len);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .ref);
    try expect(std.mem.eql(u8, node.data.expr.expr.?.data.ref, "test"));
}

test "Parser - Unary" {
    const code = "test = -1;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parseNode();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(0, node.data.expr.params.len);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .unr);
    try expectEqual(.Min, node.data.expr.expr.?.data.unr.op);
    try expect(node.data.expr.expr.?.data.unr.val.data == .int);
    try expectEqual(1, node.data.expr.expr.?.data.unr.val.data.int);
}

test "Parser - Binary" {
    const code = "test = 1 + 2;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parseNode();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(0, node.data.expr.params.len);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .bin);
    try expectEqual(.Plus, node.data.expr.expr.?.data.bin.op);
    try expect(node.data.expr.expr.?.data.bin.lhs.data == .int);
    try expectEqual(1, node.data.expr.expr.?.data.bin.lhs.data.int);
    try expect(node.data.expr.expr.?.data.bin.rhs.data == .int);
    try expectEqual(2, node.data.expr.expr.?.data.bin.rhs.data.int);
}

test "Parser - Ternary" {
    const code = "test = 1 ? 2 : 3;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parseNode();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expectEqual(0, node.data.expr.params.len);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .ter);
    try expect(node.data.expr.expr.?.data.ter.cond.data == .int);
    try expectEqual(1, node.data.expr.expr.?.data.ter.cond.data.int);
    try expect(node.data.expr.expr.?.data.ter.btrue.data == .int);
    try expectEqual(2, node.data.expr.expr.?.data.ter.btrue.data.int);
    try expect(node.data.expr.expr.?.data.ter.bfalse.data == .int);
    try expectEqual(3, node.data.expr.expr.?.data.ter.bfalse.data.int);
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
    try expectEqual(2, node.data.expr.expr.?.data.intr.intermediates.len);
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
