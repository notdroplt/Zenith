const std = @import("std");
const misc = @import("misc.zig");
const Context = @import("context.zig").Context;
const Type = @import("types.zig");
const Node = @import("node.zig");
const Parser = @import("parser.zig");

pub const IRError = error{
    OutOfMemory,
    InvalidLoad,
    InvalidStore,
    InvalidBlock,
    InvalidOperation,
};

// all jumps are relative to the start of the block

const IR = @This();

pub const SNIR = @import("Supernova/supernova.zig");
const BlockOffset = usize;

pub const Instruction = struct {
    opcode: SNIR.Opcodes,
    r1: SNIR.InfiniteRegister = 0,
    r2: SNIR.InfiniteRegister = 0,
    rd: SNIR.InfiniteRegister = 0,
    immediate: u64 = 0,
};

// register 0 is always 0,
// registers 1 - 8 are function parameters, register 4 returns
// registers 9 - 12 are software stack based
// registers 13 onwards are scratch

/// Possible value inside a block
pub const Value = struct {
    /// Value type
    vtype: *Type = undefined, // owning

    /// Which block does this come from
    from: BlockOffset = 0,

    /// Optional partial application arguments
    args: ?*[]Value = null,

    /// Meta value
    value: union(enum) {
        /// Constant (uses vtype)
        constant: void,

        /// Instruction value
        instruction: Instruction,

        /// Register return
        register: SNIR.InfiniteRegister,

        /// Partial applications (wip)
        partial: struct {
            base: *Value,
        },
    } = undefined,

    /// Deinit a type
    pub fn deinit(self: Value, alloc: std.mem.Allocator) void {
        switch (self.value) {
            .partial => |v| {
                v.base.deinit(alloc);
                alloc.destroy(v.base);
            },
            else => {},
        }
    }
};

/// "Atomic" IR block
pub const Block = struct {
    /// Instructions in this block
    instructions: std.ArrayListUnmanaged(Instruction) = .{},

    /// Result of the block
    result: Value = .{},
};

/// Phi node
const PhiSource = struct { predecessor: BlockOffset, registers: SNIR.InfiniteRegister };

//// Edge between two blocks
const EdgeValue = struct {
    /// From block offset
    from: BlockOffset,

    /// To block offset
    to: BlockOffset,
};

/// Current edges inside this graph
edges: std.AutoHashMapUnmanaged(EdgeValue, void) = .{},

/// All IR graph blocks
nodes: std.AutoArrayHashMapUnmanaged(BlockOffset, Block) = .{},

// is this used
phiSources: std.AutoArrayHashMapUnmanaged(u64, std.ArrayListUnmanaged(PhiSource)) = .{},

/// Current block counter, unique incremental id
blockCounter: BlockOffset = 0,

/// Current "allocated" register, real allocation happens later
regIndex: SNIR.InfiniteRegister = 8,

/// Memory allocator
alloc: std.mem.Allocator,

/// Error context
errctx: misc.ErrorContext = .{ .value = .NoContext },

/// Variable context
context: Context(*Type) = undefined,

/// Instantiate a new IR context
pub fn init(alloc: std.mem.Allocator, context: Context(*Type)) IR {
    return .{ .alloc = alloc, .context = context };
}

/// Deinitialize the IR context and free all resources
pub fn deinit(self: *IR) void {
    self.edges.deinit(self.alloc);
    var nit = self.nodes.iterator();
    while (nit.next()) |node| {
        node.value_ptr.*.instructions.deinit(self.alloc);
    }
    self.nodes.deinit(self.alloc);
    self.phiSources.deinit(self.alloc);
}

pub fn fromNode(self: *IR, node: *Node) !void {
    self.blockCounter += 256; // "global" ish blocks
    const bid = try self.newBlock();
    const val = try self.blockNode(bid, node);
    _ = try self.appendIfConstant(val.from, val);
}

pub fn newBlock(self: *IR) !BlockOffset {
    try self.nodes.put(self.alloc, self.blockCounter, .{});
    self.blockCounter += 1;
    return self.blockCounter - 1;
}

pub fn newEdge(self: *IR, from: BlockOffset, to: BlockOffset) !void {
    try self.edges.put(self.alloc, .{ .from = from, .to = to }, {});
}

pub fn getBlock(self: *IR, offset: BlockOffset) ?Block {
    return self.nodes.get(offset);
}

pub fn addInstruction(self: *IR, blockID: BlockOffset, instruction: Instruction) !void {
    const block = self.nodes.getEntry(blockID);
    if (block) |b| {
        try b.value_ptr.instructions.append(self.alloc, instruction);
    } else return IRError.InvalidBlock;
}

pub fn deinitBlock(self: *IR, blockID: BlockOffset) void {
    const block = self.nodes.get(blockID);
    if (block) |b| b.result.deinit(self.alloc);
}

fn addPhi(self: *IR, merge: BlockOffset, dest: SNIR.InfiniteRegister, sources: []PhiSource) !void {
    const phi_inst = Instruction{
        .opcode = SNIR.Instruction.phi,
        .rd = dest,
        .immediate = self.phi_sources.count(),
    };

    const block = self.nodes.getPtr(merge).?;
    try block.append(self.alloc, phi_inst);

    try self.phiSources.put(self.alloc, phi_inst.immediate, sources);
}

fn allocateRegister(self: *IR) SNIR.InfiniteRegister {
    const idx = self.regIndex;
    self.regIndex += 1;
    return idx;
}

fn calculateLoadSize(self: *IR, value: *Type) IRError!SNIR.Opcodes {
    if (value.data == .decimal)
        return SNIR.Opcodes.ld_dwrd;

    if (value.data != .integer) {
        self.errctx = .{
            .value = .{
                .InvalidLoad = value,
            },
        };
        return IRError.InvalidLoad;
    }

    const int = value.data.integer;
    if ((int.start >= 0 and int.end <= (1 << 8) - 1) or (int.start >= -(1 << 7) and int.end <= (1 << 7) - 1))
        return SNIR.Opcodes.ld_byte;

    if ((int.start >= 0 and int.end <= (1 << 16) - 1) or (int.start >= -(1 << 15) and int.end <= (1 << 15) - 1))
        return SNIR.Opcodes.ld_half;

    if ((int.start >= 0 and int.end <= (1 << 32) - 1) or (int.start >= -(1 << 31) and int.end <= (1 << 31) - 1))
        return SNIR.Opcodes.ld_word;

    return SNIR.Opcodes.ld_dwrd;
}

fn calculateStoreSize(value: *Type) IRError!SNIR.Opcodes {
    if (value.data == .decimal)
        return SNIR.Opcodes.st_dwrd;

    if (value.data != .integer)
        return IRError.InvalidStore;

    const int = value.data.integer;
    if ((int.start >= 0 and int.end <= (1 << 8) - 1) or (int.start >= -(1 << 7) and int.end <= (1 << 7) - 1))
        return SNIR.Opcodes.st_byte;

    if ((int.start >= 0 and int.end <= (1 << 16) - 1) or (int.start >= -(1 << 15) and int.end <= (1 << 15) - 1))
        return SNIR.Opcodes.st_half;

    if ((int.start >= 0 and int.end <= (1 << 32) - 1) or (int.start >= -(1 << 31) and int.end <= (1 << 31) - 1))
        return SNIR.Opcodes.st_word;

    return SNIR.Opcodes.st_dwrd;
}

fn blockNode(self: *IR, currblock: BlockOffset, node: *Node) IRError!Value {
    switch (node.data) {
        .int => return self.blockInteger(currblock, node),
        .dec => return self.blockDecimal(currblock, node),
        // .str => return self.blockString(currblock, node),
        .ref => return self.blockReference(currblock, node),
        .unr => return self.blockUnary(currblock, node),
        .bin => return self.blockBinary(currblock, node),
        .call => return self.blockCall(currblock, node),
        .ter => return self.blockTernary(currblock, node),
        .expr => return self.blockExpr(currblock, node),
        else => return IRError.InvalidBlock,
    }
}

fn appendIfConstant(self: *IR, block: BlockOffset, value: Value) IRError!SNIR.InfiniteRegister {
    const v = value.value;
    return switch (v) {
        .constant => {
            const reg = self.allocateRegister();
            if (value.vtype.data == .decimal) {
                try self.addInstruction(block, .{
                    .opcode = SNIR.Opcodes.mov,
                    .rd = reg,
                    .r1 = 0,
                    .immediate = @bitCast(value.vtype.data.decimal.value.?),
                });    
            } else {
                try self.addInstruction(block, .{
                    .opcode = SNIR.Opcodes.mov,
                    .rd = reg,
                    .r1 = 0,
                    .immediate = @bitCast(value.vtype.data.integer.value.?),
                });
            }
            return reg;
        },
        .instruction => {
            try self.addInstruction(block, v.instruction);
            return v.instruction.rd;
        },
        .register => v.register,
        .partial => unreachable,
    };
}

fn constantFromType(self: *IR, block: BlockOffset, node: *Node) Value {
    const nodeType = node.ntype.?;
    _ = self;
    return .{
        .vtype = nodeType,
        .from = block,
        .value = .constant,
    };
}

fn blockInteger(self: *IR, block: BlockOffset, node: *Node) Value {
    return self.constantFromType(block, node);
}

fn blockDecimal(self: *IR, block: BlockOffset, node: *Node) Value {
    return self.constantFromType(block, node);
}

fn blockReference(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
    const refType = node.ntype.?;

    if (refType.isValued())
        return self.constantFromType(block, node);

    if (refType.paramIdx > 0) {
        return .{
            .vtype = refType,
            .from = block,
            .value = .{
                .instruction = .{
                    .opcode = SNIR.Opcodes.mov_reg,
                    .rd = self.allocateRegister(),
                    .r1 = refType.paramIdx,
                },
            },
        };
    }

    return .{
        .vtype = refType,
        .from = block,
        .value = .{
            .instruction = .{
                .opcode = self.calculateLoadSize(refType) catch |err| {
                    self.errctx.position = node.position;
                    return err;
                },
                .rd = self.allocateRegister(),
                .r1 = 0,
                .immediate = 0, // todo: come back here
            },
        },
    };
}

fn blockUnary(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
    if (node.ntype.?.isValued())
        return self.constantFromType(block, node);

    const unary = node.data.unr;
    const unrtype = node.ntype;

    const inner = try self.blockNode(block, unary.val);

    if (unary.op == .Plus)
        return inner; // unary plus does NOTHING so we ball

    const result = try self.appendIfConstant(inner.from, inner);

    const instruction = try switch (unary.op) {
        .Bang, .Til => Instruction{
            .opcode = .not,
            .r1 = result,
            .rd = self.allocateRegister(),
        },
        .Min => Instruction{
            .opcode = .subr,
            .rd = self.allocateRegister(),
            .r1 = 0,
            .r2 = result,
        },
        .Star => Instruction{
            .opcode = self.calculateLoadSize(unrtype.?) catch |err| {
                self.errctx.position = node.position;
                return err;
            },
            .rd = self.allocateRegister(),
            .r1 = result,
        },
        .Amp => Instruction{
            .opcode = calculateStoreSize(unrtype.?) catch |err| {
                self.errctx.position = node.position;
                return err;
            },
            .rd = self.allocateRegister(),
            .r1 = result,
        },
        else => IRError.InvalidBlock,
    };

    return .{
        .vtype = unrtype.?,
        .from = inner.from,
        .value = .{ .instruction = instruction },
    };
}

fn getBinaryOp(self: *IR, from: BlockOffset, node: *Node, lreg: SNIR.InfiniteRegister, rreg: SNIR.InfiniteRegister) !SNIR.Opcodes {
    const binary = node.data.bin;

    if (node.ntype.?.data == .casting) {
        self.errctx = .{
            .position = node.position,
            .value = .{ .InvalidLoad = node.ntype.? }
        };
        return error.InvalidLoad;
    }

    const isSigned =
        (binary.lhs.ntype == null or binary.rhs.ntype == null) or
        (binary.lhs.ntype.?.data.integer.start < 0 and binary.rhs.ntype.?.data.integer.start < 0);
    
    return switch (binary.op) {
        .Plus => .addr,
        .Min => .subr,
        .Star => .umulr,
        .Bar => .udivr,
        .Per => unreachable, // fix modulo someday ?
        .Amp => .andr,
        .Pipe => .orr,
        .Hat => .xorr,
        .Lsh => .llsr,
        .Rsh => .lrsr,
        .Cequ => {
            try self.addInstruction(from, .{
                .opcode = SNIR.Opcodes.xorr,
                .rd = self.allocateRegister(),
                .r1 = lreg,
                .r2 = rreg,
            });

            return .setleur;
        },
        .Cneq => .xorr,
        .Cgt, .Clt => if (isSigned) .setgsr else .setgur,
        .Cge, .Cle => if (isSigned) .setlesr else .setleur,
        else => return IRError.InvalidOperation,
    };
}

fn blockBinary(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
    if (node.ntype.?.isValued())
        return self.constantFromType(block, node);

    const binary = node.data.bin;
    const bintype = node.ntype;
    const swap = binary.op == .Cge or binary.op == .Clt;

    const left = try self.blockNode(block, binary.lhs);
    const lreg = try self.appendIfConstant(left.from, left);

    const right = try self.blockNode(left.from, binary.rhs);
    const rreg = try self.appendIfConstant(right.from, right);

    // todo: add floating point support

    const instrOp = try self.getBinaryOp(right.from, node, lreg, rreg);

    return .{
        .vtype = bintype.?,
        .from = right.from,
        .value = .{
            .instruction = .{
                .opcode = instrOp,
                .rd = self.allocateRegister(),
                .r1 = if (swap) rreg else lreg,
                .r2 = if (swap) lreg else rreg,
            },
        },
    };
}

fn blockTernary(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
    if (node.ntype.?.isValued())
        return self.constantFromType(block, node);

    const ternary = node.data.ter;
    const ternarytype = node.ntype;

    const cond = try blockNode(self, block, ternary.cond);

    if (cond.value == .constant) {
        return blockNode(self, cond.from, if (cond.vtype.data.boolean.?) ternary.btrue else ternary.bfalse);
    }

    const condreg = try self.appendIfConstant(cond.from, cond);
    const trueblock = try self.newBlock();
    errdefer self.deinitBlock(trueblock);

    const btrue = try self.blockNode(trueblock, ternary.btrue);
    const treg = try self.appendIfConstant(trueblock, btrue);

    const falseblock = try self.newBlock();
    errdefer self.deinitBlock(falseblock);

    const bfalse = try self.blockNode(falseblock, ternary.bfalse);
    const freg = try self.appendIfConstant(falseblock, bfalse);

    if (treg != freg) {
        try self.addInstruction(falseblock, .{
            .opcode = SNIR.Opcodes.mov_reg,
            .rd = treg,
            .r1 = freg,
        });
    }

    //     condition block
    //        /        \
    // true block    false block
    //        \        /
    //        merge block

    try self.newEdge(cond.from, trueblock);
    try self.newEdge(cond.from, falseblock);

    const mergeblock = try self.newBlock();
    errdefer self.deinitBlock(mergeblock);

    try self.newEdge(btrue.from, mergeblock);
    try self.newEdge(bfalse.from, mergeblock);

    // jump on false
    try self.addInstruction(cond.from, .{
        .opcode = SNIR.Opcodes.blk_jmp,
        .r1 = @intFromEnum(SNIR.BlockJumpCondition.Equal),
        .r2 = condreg,
        .rd = 0,
        .immediate = @truncate(falseblock),
    });

    try self.addInstruction(cond.from, .{
        .opcode = SNIR.Opcodes.blk_jmp,
        .r1 = @intFromEnum(SNIR.BlockJumpCondition.Unconditional),
        .r2 = 0,
        .immediate = @truncate(trueblock),
    });

    return .{
        .vtype = ternarytype.?,
        .from = mergeblock,
        .value = .{
            .instruction = .{
                .opcode = SNIR.Opcodes.blk_jmp,
                .rd = @truncate(mergeblock),
                .r1 = @intFromEnum(SNIR.BlockJumpCondition.Unconditional),
                .r2 = 0,
            },
        },
    };
}

fn blockIntermediate(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
    const intrNode = node.data.intr;

    if (intrNode.intermediates.len == 0)
        return self.blockNode(block, intrNode.application);

    // dominant predecessor
    // |
    // |- intermediate 1 -\
    // |- intermediate 2 -|
    // |-      ...       -|
    // \- intermediate n -|
    //                    \- application block

    const post = try self.newBlock();

    for (intrNode.intermediates) |intermediate| {
        const intermediateValue = try self.blockNode(block, intermediate);
        self.newEdge(intermediateValue.from, post);
    }

    if (intrNode.application) |application|
        return try self.blockNode(post, application);

    // all intermediates need to have an application
    // the ones which dont usually are merged into other nodes
    unreachable;
}

fn blockCall(self: *IR, block: BlockOffset, node: *Node) !Value {
    var callee = try self.blockNode(block, node.data.call.callee);
    var caller = try self.blockNode(callee.from, node.data.call.caller);

    // Handle partial application
    const expected_args = callee.vtype.expectedArgs();

    if (expected_args > 1) {
        if (caller.vtype.partialArgs) |pa| {
            for (0..pa.len) |i| {
                if (pa[i]) |v| {
                    @constCast(&v).* = caller;
                    break;
                }
            }
        } else {
            caller.vtype.partialArgs = try self.alloc.alloc(?Value, expected_args);
            @constCast(&caller.vtype.partialArgs.?[0]).* = callee;
        }
        return callee;
    }

    // Handle full application
    const instr = Instruction{
        .opcode = .call,
        .rd = self.allocateRegister(),
        .r1 = caller.value.instruction.rd,
        .r2 = callee.value.instruction.rd,
    };

    try self.addInstruction(caller.from, instr);

    return .{
        .vtype = callee.vtype.data.function.ret,
        .value = .{ .instruction = instr },
    };
}

fn blockExpr(self: *IR, block: BlockOffset, node: *Node) IRError!Value {
    // top level expression

    const initialBlock = block;

    if (block == 0) {
        // refresh registers
        self.regIndex = 8;
    }

    return self.blockNode(initialBlock, node.data.expr.expr.?);
}
