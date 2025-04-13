pub const Instruction = union(enum) {
    RInstruction: u64,
    SInstruction: u64,
    LInstruction: u64,
};

pub const Register = u4;

// Allow better naming for registers before allocation
pub const InfiniteRegister = usize;

pub const Opcodes = enum(u8) {
    andr = 0x00,
    andi = 0x01,
    xorr = 0x02,
    xori = 0x03,
    orr = 0x04,
    ori = 0x05,
    not = 0x06,
    cnt = 0x07,
    llsr = 0x08,
    llsi = 0x09,
    lrsr = 0x0A,
    lrsi = 0x0B,

    addr = 0x10,
    addi = 0x11,
    subr = 0x12,
    subi = 0x13,
    umulr = 0x14,
    umuli = 0x15,
    smulr = 0x16,
    smuli = 0x17,
    udivr = 0x18,
    udivi = 0x19,
    sdivr = 0x1A,
    sdivi = 0x1B,
    call = 0x1C,
    push = 0x1D,
    retn = 0x1E,
    pull = 0x1F,

    ld_byte = 0x20,
    ld_half = 0x21,
    ld_word = 0x22,
    ld_dwrd = 0x23,
    st_byte = 0x24,
    st_half = 0x25,
    st_word = 0x26,
    st_dwrd = 0x27,
    jal = 0x28,
    jalr = 0x29,
    je = 0x2A,
    jne = 0x2B,
    jgu = 0x2C,
    jgs = 0x2D,
    jleu = 0x2E,
    jles = 0x2F,

    setgur = 0x30,
    setgui = 0x31,
    setgsr = 0x32,
    setgsi = 0x33,
    setleur = 0x34,
    setleui = 0x35,
    setlesr = 0x36,
    setlesi = 0x37,
    lui = 0x38,
    auipc = 0x39,
    pcall = 0x3A,
    pret = 0x3B,
    bout = 0x3C,
    out = 0x3D,
    bin = 0x3E,
    in = 0x3F,

    flt_ldu = 0x40,
    flt_lds = 0x41,
    flt_stu = 0x42,
    flt_sts = 0x43,
    flt_add = 0x44,
    flt_sub = 0x45,
    flt_mul = 0x46,
    flt_div = 0x47,
    flt_ceq = 0x48,
    flt_cne = 0x49,
    flt_cgt = 0x4A,
    flt_cle = 0x4B,
    flt_rou = 0x4C,
    flt_flr = 0x4D,
    flt_cei = 0x4E,
    flt_trn = 0x4F,
};

pub fn initRInstruction(op: Opcodes, r1: Register, r2: Register, rd: Register) Instruction {
    return op | r1 << 8 | r2 << 12 | rd << 16 | u64(0);
}

pub fn initSInstruction(op: Opcodes, r1: Register, rd: Register, imm: u48) Instruction {
    return op | r1 << 8 | rd << 12 | imm << 16 | u64(0);
}

pub fn initLInstruction(op: Opcodes, r1: Register, imm: u52) Instruction {
    return op | r1 << 8 | imm << 12 | u64(0);
}
