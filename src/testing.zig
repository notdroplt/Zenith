const std = @import("std");
const zenith = @import("zenith.zig");

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;

const String = zenith.String;
const Pos = zenith.Pos;
const Token = zenith.Token;
const Tokens = zenith.Tokens;
const Lexer = zenith.Lexer;
const Type = zenith.Type;
const Parser = zenith.Parser;
const Context = zenith.Context;
const Analyzer = zenith.Analyzer;

// types

// parser

test "Parser - Primary int" {
    const code = "test = 1;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();
    defer node.deinit(alloc);

    const pos = Pos{};

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
    try expectEqual(0, ctx.get("test"));
}

test "Context - getTree/addTree" {
    const alloc = std.testing.allocator;
    const ctx = try Context(u8).init(alloc);
    defer ctx.deinit(alloc);

    try ctx.addTree("a/b/c", "test", 0);
    try expect(ctx.getTree("a/b/c", "test") != null);
    try expectEqual(ctx.getTree("a/b/c", "test"), 0);
}

test "Analyzer - Integer" {
    const code = "test : [1; 3; 2] = 2;";
    const alloc = std.testing.allocator;
    var parser = Parser.init(code, alloc);

    const node = try parser.parseNode();

    var analyzer = try Analyzer.init(alloc);
    defer analyzer.deinit();

    const analyzed = try analyzer.runAnalysis(node);
    defer analyzed.deinit(alloc);

    try expect(analyzed.ntype != null);

    const tres = analyzed.ntype.?.data;

    try expect(tres == .integer);
    try expectEqual(1, tres.integer.start);
    try expectEqual(3, tres.integer.end);
    try expectEqual(null, tres.integer.value);

    const eres = analyzed.data;
    try expect(eres == .expr);
    try expectEqual(0, eres.expr.params.len);
    try expect(eres.expr.expr != null);

    try expect(eres.expr.expr.?.data == .int);
    try expectEqual(2, eres.expr.expr.?.data.int);
    try expect(eres.expr.ntype != null);
    try expect(eres.expr.ntype.?.ntype != null);
}
