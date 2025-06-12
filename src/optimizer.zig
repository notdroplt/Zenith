
const std = @import("std");

const Type = @import("types.zig");
const IR = @import("ir.zig");

const Optimzer = @This();

/// Performs both CSE and a premature DCO on the IR.
fn cse(ir: *IR, alloc: std.mem.Allocator) !void {
    var seen: std.AutoHashMapUnmanaged(IR.Instruction, IR.SNIR.InfiniteRegister) = .{};
    defer seen.deinit(alloc);

    var regMap: std.AutoHashMapUnmanaged(IR.SNIR.InfiniteRegister, IR.SNIR.InfiniteRegister) = .{};
    defer regMap.deinit(alloc);

    var nodeit = ir.nodes.iterator();
    while(nodeit.next()) |node| {
        const instruction = node.value_ptr.instructions.items;
        var i: usize = 0;
        for (instruction) |inst| {
            const key = IR.Instruction {
                .opcode = inst.opcode,
                .rd = 0,
                .r1 = regMap.get(inst.r1) orelse inst.r1,
                .r2 = regMap.get(inst.r2) orelse inst.r2,
            };

            if (key.opcode == IR.SNIR.Opcodes.mov_reg) {
                try regMap.put(alloc, inst.rd, key.r1);
                continue;
            }

            if (seen.get(key)) |res| {
                // we have seen this before, dco time
                try regMap.put(alloc, inst.rd, res);
            } else {
                instruction[i] = IR.Instruction {
                    .opcode = inst.opcode,
                    .rd = inst.rd,
                    .r1 = regMap.get(inst.r1) orelse inst.r1,
                    .r2 = regMap.get(inst.r2) orelse inst.r2,
                    .immediate = inst.immediate,
                };
                try seen.put(alloc, key, inst.rd);
                i += 1;
            }
        }
        node.value_ptr.instructions.items = instruction[0..i];
        node.value_ptr.instructions.shrinkAndFree(alloc, i);
    }
}

pub fn optimize_ir(ir: *IR, alloc: std.mem.Allocator) !void {
    //_ = ir;
    //_ = alloc;
    try cse(ir, alloc);
}