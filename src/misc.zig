pub const PrettyPosition = struct {
    code: []const u8,
    line: u32 = 0,
    column: u16 = 0,
};

pub const PrettierError = error {
    UnachievablePosition,
};

pub fn getPrettyPosition(code: []const u8, pos: anytype) PrettyPosition {
    // find start 
    var line: u32 = 1;
    var column: u16 = 1;
    var index: usize = 0;
    var last_line: usize = 0;
    var next_line: usize = 0;
    for (code) |c| {
        index += 1;
        column += 1;
        if (c == '\n') {
            column = 1;
            line += 1;
            last_line = index;
        }

        if (index == pos.index) break;
    }

    for (code[index..]) |c| {
        if (c == '\n') {
            next_line = index; 
            if (index == pos.index + pos.span) break;
        }
        index += 1;
    }

    if (next_line == 0) {
        next_line = index;
    }

    return PrettyPosition {
        .code = code[last_line..next_line],
        .column = column,
        .line = line
    };
}


pub fn printError(writer: anytype, code: []const u8, pos: anytype, err: anyerror) !void {
    const pp = getPrettyPosition(code, pos);
    try writer.print(
        "\x1b[2m<repl>:{}:{}:\x1b[0m {s}\n" ++
        "{d: >7} | {s}\n" ++
        "        |"
        , .{ pp.line, pp.column, @errorName(err), pp.line, pp.code });
    for (0..pp.column) |_| {
        try writer.print(" ", .{});
    }
    try writer.print("\x1b[31;49;1m", .{});
    for (0..pos.span) |_| {
        try writer.print("^", .{});
    }

    try writer.print("\x1b[0m\n", .{});
}


pub fn add(a: anytype, b: @TypeOf(a)) @TypeOf(a + b) {
    return a + b;
}

pub fn sub(a: anytype, b: @TypeOf(a)) @TypeOf(a - b) {
    return a - b;
}

pub fn mul(a: anytype, b: @TypeOf(a)) @TypeOf(a * b) {
    return a * b;
}

pub fn div(a: anytype, b: @TypeOf(a)) @TypeOf(a / b) {
    return a / b;
}

pub fn mod(a: anytype, b: @TypeOf(a)) @TypeOf(a % b) {
    return a * b;
}

pub fn pipe(a: anytype, b: @TypeOf(a)) @TypeOf(a | b) {
    return a | b;
}

pub fn amp(a: anytype, b: @TypeOf(a)) @TypeOf(a & b) {
    return a & b;
}

pub fn hat(a: anytype, b: @TypeOf(a)) @TypeOf(a ^ b) {
    return a * b;
}

pub fn lsh(a: anytype, b: @TypeOf(a)) @TypeOf(a << b) {
    return a << b;
}

pub fn rsh(a: anytype, b: @TypeOf(a)) @TypeOf(a >> b) {
    return a >> b;
}

pub fn equ(a: anytype, b: @TypeOf(a)) bool {
    return a == b;
}

pub fn neq(a: anytype, b: @TypeOf(a)) bool {
    return a != b;
}

pub fn gte(a: anytype, b: @TypeOf(a)) bool {
    return a >= b;
}

pub fn lte(a: anytype, b: @TypeOf(a)) bool {
    return a <= b;
}

pub fn gt(a: anytype, b: @TypeOf(a)) bool {
    return a > b;
}

pub fn lt(a: anytype, b: @TypeOf(a)) bool {
    return a < b;
}
