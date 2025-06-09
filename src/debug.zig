const std = @import("std");
const misc = @import("misc.zig");
const Lexer = @import("lexer.zig");
const Node = @import("node.zig");
const Type = @import("types.zig");
const IR = @import("ir.zig");
const SuperNova = @import("Supernova/supernova.zig");

pub const PrettyPosition = struct {
    code: []const u8,
    startLine: u32,
    endLine: u32,
    startColumn: u16,
    endColumn: u16,
};

pub fn getPrettyPosition(code: []const u8, pos: misc.Pos) PrettyPosition {
    var startLine: u32 = 1;
    var endLine: u32 = 1;
    var startColumn: u16 = 1;
    var endColumn: u16 = 1;

    var lastLineStart: usize = 0;
    var nextLineEnd: usize = 0;

    for (code, 0..) |c, i| {
        if (c == '\n') {
            if (i < pos.index) {
                lastLineStart = i + 1;
                startLine += 1;
            } else if (i >= pos.index and i < pos.index + pos.span) {
                endLine += 1;
                nextLineEnd = i + 1;
            }
        }
    }

    if (nextLineEnd == 0) nextLineEnd = code.len;

    startColumn = @intCast(pos.index - lastLineStart + 1);
    endColumn = @intCast(pos.index + pos.span - lastLineStart + 1);

    return PrettyPosition{
        .code = code,
        .startLine = startLine,
        .endLine = endLine,
        .startColumn = startColumn,
        .endColumn = endColumn,
    };
}

fn printContext(writer: anytype, ctx: misc.ErrorContext) !void {
    switch (ctx.value) {
        .UnexpectedToken => |err| {
            try writer.print("Expected token \"{s}\" here",
                .{ printToken(err) });
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
        .UnknownParameter => |names| {
            try writer.print("Unknown parameter \"{s}\" on function \"{s}\"", .{names.name, names.func});
        },
        else => {
            try writer.print("error : {s}\n", .{@tagName(ctx.value)});
        }
    }

    _ = try writer.write("\n");
}

pub fn printError(writer: anytype, filename: []const u8, code: []const u8, ctx: misc.ErrorContext) !void {
    const pp = getPrettyPosition(code, ctx.position);
    try writer.print("\x1b[2m{s}:{}:{}:\x1b[0m ", .{ filename, pp.startLine, pp.startColumn });

    try printContext(writer, ctx);

    // Print code context with line numbers and caret
    const max_lines: usize = 3; // lines before and after
    var line_start: usize = 0;
    var i: usize = 0;

    // Find the start of the first line to print
    const start_line = if (pp.startLine > max_lines) pp.startLine - max_lines else 1;
    var current_line: usize = 1;
    var start_idx: usize = 0;
    while (i < code.len and current_line < start_line) : (i += 1) {
        if (code[i] == '\n') {
            current_line += 1;
            start_idx = i + 1;
        }
    }

    // Print lines from start_line to endLine + max_lines
    const end_line = pp.endLine + max_lines;
    current_line = start_line;
    i = start_idx;
    while (i < code.len and current_line <= end_line) {
        line_start = i;
        // Find end of line
        var line_end = i;
        while (line_end < code.len and code[line_end] != '\n') : (line_end += 1) {}
        const line_slice = code[line_start..line_end];

        // Print line number and code
        try writer.print("{d: >4} | {s}\n", .{current_line, line_slice});

        // Print caret line if this is the error line
        if (current_line == pp.startLine) {
            // Calculate caret position and length
            const caret_start = pp.startColumn;
            const caret_end = if (pp.endLine == pp.startLine) pp.endColumn else @as(u16, @intCast(line_slice.len + 1));
            // Print spaces up to caret, then carets
            try writer.print("     | ", .{});
            var c: usize = 1;
            while (c < caret_start) : (c += 1) try writer.print(" ", .{});
            var caret_len = caret_end - caret_start;
            if (caret_len == 0) caret_len = 1;
            var k: usize = 0;
            while (k < caret_len) : (k += 1) try writer.print("^", .{});
            try writer.print("\n", .{});
        }
        // Print caret for multi-line errors
        if (current_line > pp.startLine and current_line <= pp.endLine) {
            try writer.print("     | ", .{});
            const caret_len = if (current_line == pp.endLine) pp.endColumn - 1 else line_slice.len;
            var k: usize = 0;
            while (k < caret_len) : (k += 1) try writer.print("^", .{});
            try writer.print("\n", .{});
        }

        // Move to next line
        i = line_end + 1;
        current_line += 1;
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
    switch (inst.opcode) {
        .mov => try writer.print("| mov r{} <- {}\n", .{inst.rd, inst.immediate}),
        .blk_jmp => try writer.print("| jmp ({s}) r{} r{} -> BlockID({})\n", .{
            @tagName(@as(SuperNova.BlockJumpCondition, @enumFromInt(inst.r1))),
            inst.r2, inst.rd,
            inst.immediate
            }),
        .mov_reg =>try writer.print("| mov r{} <- r{}\n", .{inst.rd, inst.r1}),
        .nop => try writer.print("| nop\n", .{}),
        else => switch (typeByOpcode(@intFromEnum(inst.opcode))) {
            .R => try writer.print("| {s} r{} <- r{}, r{}\n", .{@tagName(inst.opcode), inst.rd, inst.r1, inst.r2}),
            .S => try writer.print("| {s} r{} <- r{}, {}\n", .{@tagName(inst.opcode), inst.rd, inst.r1, inst.immediate}),
            .L => try writer.print("| {s} r{}, {}\n", .{@tagName(inst.opcode), inst.r1, inst.immediate}),
        }
    }
}

pub fn printIR(ir: *IR, writer: anytype) !void {
    try writer.print("IR graph edge count: {}\n", .{ir.edges.size});
    var edgeIt = ir.edges.iterator();
    while (edgeIt.next()) |value| {
        try writer.print("| blockID({}) -> blockID({}) \n", .{value.key_ptr.from, value.key_ptr.to});
    }

    try writer.print("IR graph blocks: {}\n", .{ir.nodes.count()});
    var nodeIt = ir.nodes.iterator();
    while(nodeIt.next()) |value| {
        try writer.print("blockID({}):\n", .{ value.key_ptr.* });
        for (value.value_ptr.instructions.items) |instr| {
            try printInstruction(instr, writer);
        }
    }
}

pub fn printToken(tok: Lexer.Tokens) []const u8 {
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
        .Arrow => "->",
        else => @tagName(tok),
    };
}

pub fn printNode(writer: anytype, node: *const Node) !void {
    try switch (node.data) {
        .type => writer.print("<type>", .{}),
        .int => |v| writer.print("{}", .{v}),
        .dec => |v| writer.print("{}", .{v}),
        .ref => |v| writer.print("{s}", .{v}),
        .str => |v| writer.print("\"{s}\"", .{v}),
        .unr => |v| {
            try writer.print("{s}", .{ printToken(v.op) });
            try printNode(writer, v.val);
        },
        .bin => |v| {
            _ = try writer.write("(");
            try printNode(writer, v.lhs);
            try writer.print("{s}", .{ printToken(v.op) });
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
            try writer.print("{s}", .{v.name});
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
                _ = try writer.write(" = ");
                try printNode(writer, expr);
            }
            _ = try writer.write(";\n");
        },
        .range => |v| {
            _ = try writer.write("[");
            try printNode(writer, v.start);
            _ = try writer.write("..");
            try printNode(writer, v.end);
            _ = try writer.write(", ");
            try printNode(writer, v.epsilon);
            _ = try writer.write("]");
        },
        else => {},
    };
}

pub fn printType(writer: anytype, t: Type) !void {
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