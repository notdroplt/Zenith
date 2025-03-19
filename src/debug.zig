const std = @import("std");
const zenith = @import("./zenith.zig");

pub fn print_token(writer: anytype, tok: zenith.Tokens) !void {
    _ = switch (tok) {
        zenith.Tokens.Tmod => try writer.write("module"),
        zenith.Tokens.Tmatch => try writer.write("match"),
        zenith.Tokens.Tlpar => try writer.write("("),
        zenith.Tokens.Trpar => try writer.write(")"),
        zenith.Tokens.Tlcur => try writer.write("{"),
        zenith.Tokens.Trcur => try writer.write("}"),
        zenith.Tokens.Tlsqb => try writer.write("["),
        zenith.Tokens.Trsqb => try writer.write("]"),
        zenith.Tokens.Tequ => try writer.write("="),
        zenith.Tokens.Tsemi => try writer.write(";"),
        zenith.Tokens.Tplus => try writer.write("+"),
        zenith.Tokens.Tmin => try writer.write("-"),
        zenith.Tokens.Tstar => try writer.write("*"),
        zenith.Tokens.Tbar => try writer.write("/"),
        zenith.Tokens.Tamp => try writer.write("&"),
        zenith.Tokens.Ttil => try writer.write("~"),
        zenith.Tokens.Tbang => try writer.write("!"),
        zenith.Tokens.Thash => try writer.write("#"),
        zenith.Tokens.Tpipe => try writer.write("|"),
        zenith.Tokens.That => try writer.write("^"),
        zenith.Tokens.Tper => try writer.write("%"),
        zenith.Tokens.Tlsh => try writer.write("<<"),
        zenith.Tokens.Trsh => try writer.write(">>"),
        zenith.Tokens.Tcequ => try writer.write("=="),
        zenith.Tokens.Tcneq => try writer.write("!="),
        zenith.Tokens.Tcgt => try writer.write(">"),
        zenith.Tokens.Tcge => try writer.write(">="),
        zenith.Tokens.Tclt => try writer.write("<"),
        zenith.Tokens.Tcle => try writer.write("<="),
        zenith.Tokens.Tand => try writer.write("&&"),
        zenith.Tokens.Tor => try writer.write("||"),
        zenith.Tokens.Tques => try writer.write("?"),
        zenith.Tokens.Tcln => try writer.write(":"),
        zenith.Tokens.Tdot => try writer.write("."),
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
        else => {}
    };
}
