const std = @import("std");
const misc = @import("misc.zig");
const debug = @import("debug.zig");

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;

const Parser = @import("parser.zig");
const Analyzer = @import("analyzer.zig");
const IR = @import("ir.zig");
const Optimizer = @import("optimizer.zig");


/// Pipelines the entire compiling process, its the compiler main function
pub fn pipeline(name: misc.String, alloc: std.mem.Allocator) !u8 {
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

    const node = parser.parseNode() catch {
        try debug.printError(stdout, name, parser.code, parser.errctx);
        try bw.flush();
        return 254;
    };

    defer node.deinit(alloc);

    try debug.printNode(stdout, node);

    const typed = analyzer.runAnalysis(node) catch {
        try debug.printError(stdout, name, parser.code, analyzer.errctx);
        try bw.flush();
        return 253;
    };

    try bw.flush();

    var ir = IR.init(alloc, analyzer.context);
    defer ir.deinit();
    ir.fromNode(typed) catch {
        try debug.printError(stdout, name, parser.code, ir.errctx);
        try bw.flush();
        return 252;
    };

    try Optimizer.optimize_ir(&ir, alloc);
    try debug.printIR(&ir, stdout);

    try stdout.print("analysis successful\n", .{});
    try bw.flush();

    return 0;
}
