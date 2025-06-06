
const std = @import("std");

const Type = @import("types.zig");
const IR = @import("ir.zig");

const Optimzer = @This();

fn cse(ir: *IR, alloc: std.mem.Allocator) !void {
    var seen: std.AutoHashMapUnmanaged(IR.Instruction, IR.SNIR.InfiniteRegister) = .{};
    defer seen.deinit(alloc);

    var regMap: std.AutoHashMapUnmanaged(IR.SNIR.InfiniteRegister, IR.SNIR.InfiniteRegister) = .{};
    defer regMap.deinit(alloc);

    var nodeit = ir.nodes.iterator();
    while(nodeit.next()) |node| {
        const instruction = node.value_ptr.instructions.items;
        for (instruction, 0..) |inst, i| {
            const key = IR.Instruction {
                .opcode = inst.opcode,
                .rd = 0,
                .r1 = regMap.get(inst.r1) orelse inst.r1,
                .r2 = regMap.get(inst.r2) orelse inst.r2,
            };

            if (seen.get(key)) |res| {
                instruction[i] = IR.Instruction {
                    .opcode = IR.SNIR.Opcodes.mov_reg,
                    .rd = inst.rd,
                    .r1 = res,
                };

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
            }
        }
    }
}

pub fn optimize_ir(ir: *IR, alloc: std.mem.Allocator) !void {
    _ = ir;
    _ = alloc;
//   try cse(ir, alloc);
}