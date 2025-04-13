const zenith = @import("zenith.zig");

const std = @import("std");
const expect = std.testing.expect;

const String = zenith.String;
const Pos = zenith.Pos;
const Token = zenith.Token;
const Tokens = zenith.Tokens;
const Lexer = zenith.Lexer;
const Type = zenith.Type;
const Parser = zenith.Parser;
const Context = zenith.Context;
const Analyzer = zenith.Analyzer;

test "token" {
    const t1 = Token{ .tid = Tokens.Tint, .val = .{ .int = 1 } };
    const t2 = Token{ .tid = Tokens.Tstr, .val = .{ .str = "str" } };
    const t3 = Token{ .tid = Tokens.Tint, .pos = Pos{ .index = 1, .span = 3 }, .val = .{ .int = 1 } };

    try expect(t1.same(t3));
    try expect(t1.sameId(Tokens.Tint));
    try expect(t2.notId(Tokens.Tand));
}

test "Lexer - Number" {
    var lexer = Lexer.init("0 1 0.0 1.0 1e2 1.3e2");
    const t1 = try lexer.consume();
    try expect(t1.tid == Tokens.Tint);
    try expect(t1.val.int == 0);

    lexer = lexer.catch_up(t1);
    const t2 = try lexer.consume();
    try expect(t2.tid == Tokens.Tint);
    try expect(t2.val.int == 1);

    lexer = lexer.catch_up(t2);
    const t3 = try lexer.consume();

    try expect(t3.tid == Tokens.Tdec);
    try expect(t3.val.dec == 0.0);

    lexer = lexer.catch_up(t3);
    const t4 = try lexer.consume();

    try expect(t4.tid == Tokens.Tdec);
    try expect(t4.val.dec == 1.0);

    lexer = lexer.catch_up(t4);
    const t5 = try lexer.consume();

    try expect(t5.tid == Tokens.Tdec);
    try expect(t5.val.dec == 100.0);

    lexer = lexer.catch_up(t5);
    const t6 = try lexer.consume();

    try expect(t6.tid == Tokens.Tdec);
    try expect(t6.val.dec == 130.0);
}

test "consume string" {
    var l = Lexer.init("\"hello\" \"\"");
    const t1 = try l.consume();
    try expect(t1.tid == Tokens.Tstr);
    try expect(std.mem.eql(u8, t1.val.str, "hello"));

    l = l.catch_up(t1);

    const t2 = try l.consume();
    try expect(t2.tid == Tokens.Tstr);
    try expect(std.mem.eql(u8, t2.val.str, ""));
}

test "consume identifier" {
    var l = Lexer.init("identifier");
    const token = try l.consume();
    try expect(token.tid == Tokens.Tref);
    try expect(std.mem.eql(u8, token.val.str, "identifier"));
}

test "consume operators" {
    var l = Lexer.init("+-*/");
    const plus = try l.consume();
    try expect(plus.tid == Tokens.Tplus);

    l = l.catch_up(plus);
    const minus = try l.consume();
    try expect(minus.tid == Tokens.Tmin);

    l = l.catch_up(minus);
    const star = try l.consume();
    try expect(star.tid == Tokens.Tstar);

    l = l.catch_up(star);
    const bar = try l.consume();
    try expect(bar.tid == Tokens.Tbar);
}

test "consume operators 2" {
    var l = Lexer.init("== != && || <= >= << >>");
    const eq = try l.consume();
    try expect(eq.tid == Tokens.Tcequ);

    l = l.catch_up(eq);
    const neq = try l.consume();
    try expect(neq.tid == Tokens.Tcneq);

    l = l.catch_up(neq);
    const tand = try l.consume();
    try expect(tand.tid == Tokens.Tand);

    l = l.catch_up(tand);
    const tor = try l.consume();
    try expect(tor.tid == Tokens.Tor);

    l = l.catch_up(tor);
    const le = try l.consume();
    try expect(le.tid == Tokens.Tcle);

    l = l.catch_up(le);
    const ge = try l.consume();
    try expect(ge.tid == Tokens.Tcge);

    l = l.catch_up(ge);
    const lsh = try l.consume();
    try expect(lsh.tid == Tokens.Tlsh);

    l = l.catch_up(lsh);
    const rsh = try l.consume();
    try expect(rsh.tid == Tokens.Trsh);
}

// types

test "Type operations" {
    const t1 = Type.initInt(2, 2, 2);
    const t2 = Type.initInt(5, 5, 5);

    const alloc = std.testing.allocator;

    try expect((try Type.synthUnary(alloc, Tokens.Tplus, t1)).deepEqual(t1, true));
    try expect((try Type.synthUnary(alloc, Tokens.Tmin, t1)).deepEqual(Type.initInt(-2, -2, -2), true));
    try expect((try Type.synthUnary(alloc, Tokens.Ttil, t1)).deepEqual(Type.initInt(-3, -3, -3), true));
    try expect((try Type.synthUnary(alloc, Tokens.Tamp, t1)).deepEqual(Type.makePtr(t1), true));

    try expect((try Type.binOperate(alloc, Tokens.Tplus, t1, t2)).deepEqual(Type.initInt(7, 7, 7), true));
    try expect((try Type.binOperate(alloc, Tokens.Tmin, t1, t2)).deepEqual(Type.initInt(-3, -3, -3), true));
    try expect((try Type.binOperate(alloc, Tokens.Tmin, t2, t1)).deepEqual(Type.initInt(3, 3, 3), true));
    try expect((try Type.binOperate(alloc, Tokens.Tmin, t2, t2)).deepEqual(Type.zeroInt, true));
}

test "type instatiantion" {
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

// parser

test "Parser - Primary int" {
    const code = "test = 1;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 0);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .int);
    try expect(node.data.expr.expr.?.data.int == 1);
}

test "Parser - Primary float" {
    const code = "test = 1.0;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 0);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .dec);
    try expect(node.data.expr.expr.?.data.dec == 1.0);
}

test "Parser - Primary string" {
    const code = "test = \"test\";";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 0);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .str);
    try expect(std.mem.eql(u8, node.data.expr.expr.?.data.str, "test"));
}

test "Parser - Primary reference" {
    const code = "test = test;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 0);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .ref);
    try expect(std.mem.eql(u8, node.data.expr.expr.?.data.ref, "test"));
}

test "Parser - Unary" {
    const code = "test = -1;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 0);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .unr);
    try expect(node.data.expr.expr.?.data.unr.op == Tokens.Tmin);
    try expect(node.data.expr.expr.?.data.unr.val.data == .int);
    try expect(node.data.expr.expr.?.data.unr.val.data.int == 1);
}

test "Parser - Binary" {
    const code = "test = 1 + 2;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 0);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .bin);
    try expect(node.data.expr.expr.?.data.bin.op == Tokens.Tplus);
    try expect(node.data.expr.expr.?.data.bin.lhs.data == .int);
    try expect(node.data.expr.expr.?.data.bin.lhs.data.int == 1);
    try expect(node.data.expr.expr.?.data.bin.rhs.data == .int);
    try expect(node.data.expr.expr.?.data.bin.rhs.data.int == 2);
}

test "Parser - Ternary" {
    const code = "test = 1 ? 2 : 3;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 0);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .ter);
    try expect(node.data.expr.expr.?.data.ter.cond.data == .int);
    try expect(node.data.expr.expr.?.data.ter.cond.data.int == 1);
    try expect(node.data.expr.expr.?.data.ter.btrue.data == .int);
    try expect(node.data.expr.expr.?.data.ter.btrue.data.int == 2);
    try expect(node.data.expr.expr.?.data.ter.bfalse.data == .int);
    try expect(node.data.expr.expr.?.data.ter.bfalse.data.int == 3);
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
    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 0);
    try expect(node.data.expr.expr != null);
    try expect(node.data.expr.expr.?.data == .intr);
    try expect(node.data.expr.expr.?.data.intr.intermediates.len == 2);
}

test "Parser - Params" {
    const code = "test a b c d e f g h = a;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);
    const node = try parser.parse_node();
    defer node.deinit(alloc);

    try expect(node.data == .expr);
    try expect(std.mem.eql(u8, node.data.expr.name, "test"));
    try expect(node.data.expr.params.len == 8);
    try expect(node.data.expr.params[0].data == .ref);
    try expect(std.mem.eql(u8, node.data.expr.params[0].data.ref, "a"));
    try expect(node.data.expr.params[7].data == .ref);
    try expect(std.mem.eql(u8, node.data.expr.params[7].data.ref, "h"));
}

test "Parser - Ranges" {}

// context

test "Context - is empty" {
    const alloc = std.testing.allocator;
    const ctx = try Context(u8).init(alloc);
    defer ctx.deinit(alloc);

    try expect(ctx.isEmpty());
    try ctx.add("test", 0);
    try expect(!ctx.isEmpty());
}

test "Context - get/add" {
    const alloc = std.testing.allocator;
    const ctx = try Context(u8).init(alloc);
    defer ctx.deinit(alloc);

    try ctx.add("test", 0);
    try expect(ctx.get("test") != null);
    try expect(ctx.get("test") == 0);
}

test "Context - getTree/addTree" {
    const alloc = std.testing.allocator;
    const ctx = try Context(u8).init(alloc);
    defer ctx.deinit(alloc);

    try ctx.addTree("a/b/c", "test", 0);
    try expect(ctx.getTree("a/b/c", "test") != null);
    try expect(ctx.getTree("a/b/c", "test") == 0);
}

test "Analyzer - Integer" {
    const code = "test : [1; 3; 2] = 2;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parse_node();
    
    var analyzer = try Analyzer.init(alloc);
    defer analyzer.deinit();

    const analyzed = try analyzer.analyze(analyzer.context, node);
    defer analyzed.deinit(alloc);

    try expect(analyzed.ntype != null);

    const tres = analyzed.ntype.?.data;

    try expect(tres == .integer);
    try expect(tres.integer.start == 1);
    try expect(tres.integer.end == 3);
    try expect(tres.integer.value == null);

    const eres = analyzed.data;
    try expect(eres == .expr);
    try expect(eres.expr.params.len == 0);
    try expect(eres.expr.expr != null);

    try expect(eres.expr.expr.?.data == .int);
    try expect(eres.expr.expr.?.data.int == 2);
    try expect(eres.expr.ntype != null);
    try expect(eres.expr.ntype.?.ntype != null);

}