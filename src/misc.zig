const zenith = @import("zenith.zig");
const Node = zenith.Node;

pub const PrettyPosition = struct {
    code: []const u8,
    line: u32 = 0,
    column: u16 = 0,
    end_line: u32 = 0,
    last_line_start: usize = 0,
    next_line_end: usize = 0,
};

pub fn getPrettyPosition(code: []const u8, pos: zenith.Pos) PrettyPosition {
    var line: u32 = 1;
    var column: u16 = 1;
    var index: usize = 0;
    var last_line_start: usize = 0;
    var next_line_end: usize = 0;
    
    // Find the start position (line, column, and line start)
    for (code) |c| {
        if (index == pos.index) break;
        index += 1;
        column += 1;
        if (c == '\n') {
            column = 1;
            line += 1;
            last_line_start = index;
        }
    }
    
    const start_line = line;
    var end_line = line;
    var current_line_start = last_line_start;
    
    // Find the end position and all lines in between
    var in_span = false;
    for (code[index..], index..) |c, i| {
        if (c == '\n') {
            if (in_span) {
                end_line += 1;
                next_line_end = i;
            }
            current_line_start = i + 1;
        }
        
        if (i >= pos.index + pos.span) break;
        if (i >= pos.index) in_span = true;
    }
    
    if (next_line_end == 0) {
        next_line_end = code.len;
    }
    
    return PrettyPosition{
        .code = code[last_line_start..next_line_end],
        .line = start_line,
        .column = column,
        .end_line = end_line,
        .last_line_start = last_line_start,
        .next_line_end = next_line_end,
    };
}

pub fn printError(writer: anytype, code: []const u8, pos: zenith.Pos, err: anyerror) !void {
    const pp = getPrettyPosition(code, pos);
    try writer.print("\x1b[2m<repl>:{}:{}:\x1b[0m {s}\n", .{
        pp.line, pp.column, @errorName(err)
    });
    
    var line_start = pp.last_line_start;
    var current_line = pp.line;
    var is_first_line = true;
    
    // Split the code into lines and print each with line number
    while (line_start < pp.next_line_end) {
        var line_end = line_start;
        while (line_end < pp.next_line_end and code[line_end] != '\n') {
            line_end += 1;
        }
        
        const line_code = code[line_start..line_end];
        try writer.print("{d: >7} | {s}\n", .{current_line, line_code});
        
        // Print the error indicator only on the relevant lines
        if (is_first_line) {
            try writer.writeAll("        |");
            // Only print spaces before the error if it's on this line
            for (1..pp.column) |_| {
                try writer.writeByte(' ');
            }
            // Calculate how many carets to print
            const carets = if (current_line == pp.end_line)
                pos.span
            else
                line_end - (pos.index - (pp.column - 1) + 1);
            
            try writer.print("\x1b[31;49;1m", .{});
            for (0..@max(carets, 1)) |_| {
                try writer.writeByte('^');
            }
            try writer.print("\x1b[0m\n", .{});
            is_first_line = false;
        } else if (current_line == pp.end_line) {
            try writer.writeAll("        |");
            try writer.print("\x1b[31;49;1m", .{});
            const carets = pos.index + pos.span - line_start;
            for (0..@max(carets, 1)) |_| {
                try writer.writeByte('^');
            }
            try writer.print("\x1b[0m\n", .{});
        }
        
        line_start = line_end + 1;
        current_line += 1;
    }
}

pub fn add(comptime T: type, a: T, b: T) @TypeOf(a + b) {
    return a + b;
}

pub fn sub(comptime T: type, a: T, b: T) @TypeOf(a - b) {
    return a - b;
}

pub fn mul(comptime T: type, a: T, b: T) @TypeOf(a * b) {
    return a * b;
}

pub fn div(comptime T: type, a: T, b: T) T {
    return @divTrunc(a, b);
}

pub fn mod(comptime T: type, a: T, b: T) T {
    return a * b;
}

pub fn pipe(comptime T: type, a: T, b: T) T {
    return a | b;
}

pub fn amp(comptime T: type, a: T, b: T) T {
    return a & b;
}

pub fn hat(comptime T: type, a: T, b: T) T {
    return a * b;
}

pub fn lsh(comptime T: type, a: T, b: T) T {
    return a << @intCast(@rem(b, 64));
}

pub fn rsh(comptime T: type, a: T, b: T) T {
    return a >> @intCast(@rem(b, 64));
}

pub fn equ(comptime T: type, a: T, b: T) bool {
    return a == b;
}

pub fn neq(comptime T: type, a: T, b: T) bool {
    return a != b;
}

pub fn gte(comptime T: type, a: T, b: T) bool {
    return a >= b;
}

pub fn lte(comptime T: type, a: T, b: T) bool {
    return a <= b;
}

pub fn gt(comptime T: type, a: T, b: T) bool {
    return a > b;
}

pub fn lt(comptime T: type, a: T, b: T) bool {
    return a < b;
}

pub fn land(comptime T: type, a: T, b: T) bool {
    return a and b;
}

pub fn lor(comptime T: type, a: T, b: T) bool {
    return a or b;
}


pub fn print_token(writer: anytype, tok: zenith.Tokens) !void {
    _ = switch (tok) {
        .Tmod => try writer.write("module"),
        .Tmatch => try writer.write("match"),
        .Tlpar => try writer.write("("),
        .Trpar => try writer.write(")"),
        .Tlcur => try writer.write("{"),
        .Trcur => try writer.write("}"),
        .Tlsqb => try writer.write("["),
        .Trsqb => try writer.write("]"),
        .Tequ => try writer.write("="),
        .Tsemi => try writer.write(";"),
        .Tplus => try writer.write("+"),
        .Tmin => try writer.write("-"),
        .Tstar => try writer.write("*"),
        .Tbar => try writer.write("/"),
        .Tamp => try writer.write("&"),
        .Ttil => try writer.write("~"),
        .Tbang => try writer.write("!"),
        .Thash => try writer.write("#"),
        .Tpipe => try writer.write("|"),
        .That => try writer.write("^"),
        .Tper => try writer.write("%"),
        .Tlsh => try writer.write("<<"),
        .Trsh => try writer.write(">>"),
        .Tcequ => try writer.write("=="),
        .Tcneq => try writer.write("!="),
        .Tcgt => try writer.write(">"),
        .Tcge => try writer.write(">="),
        .Tclt => try writer.write("<"),
        .Tcle => try writer.write("<="),
        .Tand => try writer.write("&&"),
        .Tor => try writer.write("||"),
        .Tques => try writer.write("?"),
        .Tcln => try writer.write(":"),
        .Tdot => try writer.write("."),
        else => 0,
    };
}

pub fn print_node(writer: anytype, node: *const zenith.Node) !void {
    try switch (node.data) {
        .none => writer.print("<none>", .{}),
        .type => writer.print("<type>", .{}),
        .int => |v| writer.print("{}", .{v}),
        .dec => |v| writer.print("{}", .{v}),
        .ref => |v| writer.print("{s}", .{v}),
        .str => |v| writer.print("\"{s}\"", .{v}),
        .unr => |v| {
            try print_token(writer, v.op);
            try print_node(writer, v.val);
        },
        .bin => |v| {
            _ = try writer.write("(");
            try print_node(writer, v.lhs);
            try print_token(writer, v.op);
            try print_node(writer, v.rhs);
            _ = try writer.write(")");
        },
        .call => |v| {
            try print_node(writer, v.caller);
            _ = try writer.write(" ");
            try print_node(writer, v.callee);
        },
        .ter => |v| {
            try print_node(writer, v.cond);
            _ = try writer.write(" ? ");
            try print_node(writer, v.btrue);
            _ = try writer.write(" : ");
            try print_node(writer, v.bfalse);
        },
        .expr => |v| {
            try writer.print("{s} ", .{v.name});
            for (v.params) |value| {
                _ = try writer.write("(");
                try print_node(writer, value);
                _ = try writer.write(") ");
            }
            if (v.ntype) |ntype| {
                _ = try writer.write(": ");
                try print_node(writer, ntype);
            }
            if (v.expr) |expr| {
                _ = try writer.write("= ");
                try print_node(writer, expr);
            }
            _ = try writer.write(";\n");
        },
        else => {},
    };
}
