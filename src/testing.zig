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


// context



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
