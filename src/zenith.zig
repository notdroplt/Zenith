const std = @import("std");
const misc = @import("misc.zig");

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;

pub const String = []const u8;

////////////////////////////////////////////////////////////////////////
// Lexer - approx 500 lines                                           //
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
/// Error Contexts
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
// Types - approx 700 lines                                           //
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
// Parser - approx 800 lines                                          //
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// Semantic Analyzer - approx 600 lines                               //
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
/// Intermediate Representation - approx 600 lines                    //
////////////////////////////////////////////////////////////////////////


///////////

pub const Optimizations = struct {
    pub const AST = struct {
        

    };
};

const Parser = @import("parser.zig");
const Analyzer = @import("analyzer.zig");
const IR = @import("ir.zig");
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

    var arena = std.heap.ArenaAllocator.init(alloc);
    const aalloc = arena.allocator();
    defer arena.deinit();

    defer file.close();

    const codeLength = (try file.stat()).size;

    const buffer = try alloc.alloc(u8, codeLength);
    defer alloc.free(buffer);

    const result = try file.reader().readAll(buffer);

    if (result != codeLength) return error.EndOfStream;

    var parser = Parser.init(buffer, alloc);
    var analyzer = try Analyzer.init(alloc, aalloc);
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
