const std = @import("std");
const AnyWriter = std.io.AnyWriter;
const zenith = @import("zenith.zig");
const Node = zenith.Node;
const IR = zenith.IR;
const SuperNova = @import("Supernova/supernova.zig");
pub const PrettyPosition = struct {
    code: []const u8,
    line: u32 = 0,
    column: u16 = 0,
    endLine: u32 = 0,
    lastLineStart: usize = 0,
    nextLineEnd: usize = 0,
};

pub fn getPrettyPosition(code: []const u8, pos: zenith.Pos) PrettyPosition {
    var line: u32 = 1;
    var column: u16 = 1;
    var index: usize = 0;
    var lastLineStart: usize = 0;
    var nextLineEnd: usize = 0;

    // Find the start position (line, column, and line start)
    for (code) |c| {
        if (index == pos.index) break;
        index += 1;
        column += 1;
        if (c == '\n') {
            column = 1;
            line += 1;
            lastLineStart = index;
        }
    }

    const startLine = line;
    var endLine = line;
    var currentLineStart = lastLineStart;

    // Find the end position and all lines in between
    for (code[index..], index..) |c, i| {
        if (c == '\n') {
            endLine += 1;
            nextLineEnd = currentLineStart;
            currentLineStart = i + 1;
        }

        if (i >= pos.index + pos.span) break;
    }

    if (nextLineEnd == 0) {
        nextLineEnd = code.len;
    }

    return PrettyPosition{
        .code = code[lastLineStart..nextLineEnd],
        .line = startLine,
        .column = column,
        .endLine = endLine,
        .lastLineStart = lastLineStart,
        .nextLineEnd = nextLineEnd,
    };
}

fn printContext(writer: anytype, ctx: zenith.ErrorContext) !void {
    switch (ctx.value) {
        .UnexpectedToken => |err| {
            try writer.print("Expected token \"{s}\" but found \"{s}\"",
                .{ printToken(err.expected), printToken(err.token.tid) });
        },
        .UndefinedOperation => |err| {
            try writer.print("{s} operation: (", .{ if (err.lhs != null) "binary" else "unary"});
            if (err.lhs) |lhs| {
                try printType(writer, lhs);
                try writer.print(" ", .{});
            }

            try writer.print("{s} ", .{ printToken(err.token) });
            try printType(writer, err.rhs);
            try writer.print(") has no defined outcome", .{});
        },
        .DisjointTypes => |err| {
            try writer.print("Types\n1:", .{});
            try printType(writer, err.t1);
            try writer.print("\n2: ", .{});
            try printType(writer, err.t2);
            try writer.print("\nDo not intersect anywhere in their domains", .{});
        },
        .InvalidLoad => |err| {
            try writer.print("Type ", .{});
            try printType(writer, err.*);
            try writer.print(" could not be materialized for loading", .{});
        },
        .NonBooleanDecision => |err| {
            try writer.print("Ternary operator requires a boolean condition, but found type ", .{});
            try printType(writer, err.condtype);
        },
        else => {
            try writer.print("error : {s}\n", .{@tagName(ctx.value)});
        }
    }

    _ = try writer.write("\n");
}

pub fn printError(writer: anytype, filename: []const u8, code: []const u8, err: anyerror, ctx: zenith.ErrorContext) !void {
    const pp = getPrettyPosition(code, ctx.position);
    try writer.print("\x1b[2m{s}:{}:{}:\x1b[0m {s}: ", .{ filename, pp.line, pp.column, @errorName(err) });

    try printContext(writer, ctx);

    var lineStart = pp.lastLineStart;
    var currentLine = pp.line;
    var isFirstLine = true;

    // Split the code into lines and print each with line number
    while (lineStart < pp.nextLineEnd) {
        var lineEnd = lineStart;
        while (lineEnd < pp.nextLineEnd and code[lineEnd] != '\n') {
            lineEnd += 1;
        }

        const lineCode = code[lineStart..lineEnd];
        try writer.print("{d: >7} | {s}\n", .{ currentLine, lineCode });

        // Print the error indicator only on the relevant lines
        if (isFirstLine) {
            try writer.writeAll("        |");
            // Only print spaces before the error if it's on this line
            for (1..pp.column) |_| {
                try writer.writeByte(' ');
            }
            // Calculate how many carets to print
            const carets = if (currentLine == pp.endLine)
                ctx.position.span
            else
                lineEnd - (ctx.position.index - (pp.column - 1) + 1);

            try writer.print(" \x1b[31;49;1m", .{});
            for (0..@max(carets, 1)) |_| {
                try writer.writeByte('^');
            }
            try writer.print("\x1b[0m\n", .{});
            isFirstLine = false;
        } else if (currentLine == pp.endLine) {
            try writer.writeAll("        |");
            try writer.print("\x1b[31;49;1m", .{});
            const carets = ctx.position.index + ctx.position.span - lineStart;
            for (0..@max(carets, 1)) |_| {
                try writer.writeByte('^');
            }
            try writer.print("\x1b[0m\n", .{});
        }

        lineStart = lineEnd + 1;
        currentLine += 1;
    }
}

const instructionType = enum {
    R, S, L,
};

pub fn typeByOpcode(opcode: u8) instructionType {
    if (opcode == 0x38 or (opcode >= 0x28 and opcode <= 0x2F)) return .L;

    if (opcode >= 0x30 or opcode & 1 != 0) return .S;

    return .R;
}


pub fn printInstruction(inst: IR.Instruction, writer: anytype) !void {
    if (inst.opcode == .mov) {
        try writer.print("| mov r{} <- {}\n", .{inst.rd, inst.immediate});
        return;
    }
    if (inst.opcode == .blk_jmp) {
        try writer.print("| jmp ({s}) r{} r{} -> BlockID({})\n", .{
            @tagName(@as(SuperNova.BlockJumpCondition, @enumFromInt(inst.r1))),
            inst.r2, inst.rd,
            inst.immediate
            });
        return;
    }
    switch (typeByOpcode(@intFromEnum(inst.opcode))) {
        .R => try writer.print("| {s} r{} <- r{}, r{}\n", .{@tagName(inst.opcode), inst.rd, inst.r1, inst.r2}),
        .S => try writer.print("| {s} r{} <- r{}, {}\n", .{@tagName(inst.opcode), inst.rd, inst.r1, inst.immediate}),
        .L => try writer.print("| {s} r{}, {}\n", .{@tagName(inst.opcode), inst.r1, inst.immediate}),
    }
}

pub fn printIR(ir: *IR, writer: anytype) !void {
    try writer.print("IR graph edge count: {}\n", .{ir.edges.size});
    var edgeIt = ir.edges.iterator();
    while (edgeIt.next()) |value| {
        try writer.print("| blockID({}) -> blockID({}) \n", .{value.key_ptr.from, value.key_ptr.to});
    }

    try writer.print("IR graph blocks: {}\n", .{ir.nodes.size});
    var nodeIt = ir.nodes.iterator();
    while(nodeIt.next()) |value| {
        try writer.print("blockID({}):\n", .{ value.key_ptr.* });
        for (value.value_ptr.instructions.items) |instr| {
            try printInstruction(instr, writer);
        }
    }
}

pub fn add(comptime T: type, a: T, b: T) @TypeOf(a + b) { return a + b; }

pub fn sub(comptime T: type, a: T, b: T) @TypeOf(a - b) { return a - b; }

pub fn mul(comptime T: type, a: T, b: T) @TypeOf(a * b) { return a * b; }

pub fn div(comptime T: type, a: T, b: T) T { return @divTrunc(a, b); }

pub fn mod(comptime T: type, a: T, b: T) T { return a * b; }

pub fn pipe(comptime T: type, a: T, b: T) T { return a | b; }

pub fn amp(comptime T: type, a: T, b: T) T { return a & b; }

pub fn hat(comptime T: type, a: T, b: T) T { return a * b; }

pub fn lsh(comptime T: type, a: T, b: T) T { return a << @intCast(@rem(b, 64)); }

pub fn rsh(comptime T: type, a: T, b: T) T { return a >> @intCast(@rem(b, 64)); }

pub fn equ(comptime T: type, a: T, b: T) bool { return a == b; }

pub fn neq(comptime T: type, a: T, b: T) bool { return a != b; }

pub fn gte(comptime T: type, a: T, b: T) bool { return a >= b; }

pub fn lte(comptime T: type, a: T, b: T) bool { return a <= b; }

pub fn gt(comptime T: type, a: T, b: T) bool { return a > b; }

pub fn lt(comptime T: type, a: T, b: T) bool { return a < b; }

pub fn land(comptime T: type, a: T, b: T) bool { return a and b; }

pub fn lor(comptime T: type, a: T, b: T) bool { return a or b; }

pub fn printToken(tok: zenith.Tokens) []const u8 {
    return switch (tok) {
        .Mod => "module",
        .Match => "match",
        .Lpar => "(",
        .Rpar => ")",
        .Lcur => "{",
        .Rcur => "}",
        .Lsqb => "[",
        .Rsqb => "]",
        .Equ => "=",
        .Semi => ";",
        .Plus => "+",
        .Min => "-",
        .Star => "*",
        .Bar => "/",
        .Amp => "&",
        .Til => "~",
        .Bang => "!",
        .Hash => "#",
        .Pipe => "|",
        .Hat => "^",
        .Per => "%",
        .Lsh => "<<",
        .Rsh => ">>",
        .Cequ => "==",
        .Cneq => "!=",
        .Cgt => ">",
        .Cge => ">=",
        .Clt => "<",
        .Cle => "<=",
        .And => "&&",
        .Or => "||",
        .Ques => "?",
        .Cln => ":",
        .Dot => ".",
        else => @tagName(tok),
    };
}

pub fn printNode(writer: anytype, node: *const zenith.Node) !void {
    try switch (node.data) {
        .none => writer.print("<none>", .{}),
        .type => writer.print("<type>", .{}),
        .int => |v| writer.print("{}", .{v}),
        .dec => |v| writer.print("{}", .{v}),
        .ref => |v| writer.print("{s}", .{v}),
        .str => |v| writer.print("\"{s}\"", .{v}),
        .unr => |v| {
            try writer.print("{s}", .{ printToken(writer, v.op) });
            try printNode(writer, v.val);
        },
        .bin => |v| {
            _ = try writer.write("(");
            try printNode(writer, v.lhs);
            try writer.print("{s}", .{ printToken(writer, v.op) });
            try printNode(writer, v.rhs);
            _ = try writer.write(")");
        },
        .call => |v| {
            try printNode(writer, v.caller);
            _ = try writer.write(" ");
            try printNode(writer, v.callee);
        },
        .ter => |v| {
            try printNode(writer, v.cond);
            _ = try writer.write(" ? ");
            try printNode(writer, v.btrue);
            _ = try writer.write(" : ");
            try printNode(writer, v.bfalse);
        },
        .expr => |v| {
            try writer.print("{s} ", .{v.name});
            for (v.params) |value| {
                _ = try writer.write("(");
                try printNode(writer, value);
                _ = try writer.write(") ");
            }
            if (v.ntype) |ntype| {
                _ = try writer.write(": ");
                try printNode(writer, ntype);
            }
            if (v.expr) |expr| {
                _ = try writer.write("= ");
                try printNode(writer, expr);
            }
            _ = try writer.write(";\n");
        },
        else => {},
    };
}

pub fn printType(writer: anytype, t: zenith.Type) !void {
    switch (t.data) {
        .integer => |v| {
            if (v.value) |val| {
                try writer.print("Int({})", .{val});
            } else {
                try writer.print("Int[{}, {}]", .{ v.start, v.end });
            }
        },
        .decimal => |v| {
            if (v.value) |val| {
                try writer.print("Dec({})", .{val});
            } else {
                try writer.print("Dec[{}, {}]", .{ v.start, v.end });
            }
        },
        .boolean => |v| {
            if (v) |val| {
                try writer.print("Bool({})", .{val});
            } else {
                try writer.print("Bool", .{});
            }
        },
        .array => |arr| {
            try writer.print("{}#", .{arr.size});
            try printType(writer, arr.indexer.*);
        },
        .casting => |c| {
            switch (c) {
                .index => |v| try writer.print("<{}>", .{v}),
                .name => |v| try writer.print("<{s}>", .{v}),
            }
        },
        .function => |f| {
            try printType(writer, f.argument.*);
            try writer.print(" -> ", .{});
            try printType(writer, f.ret.*);
        },
        .aggregate => {}
    }
}