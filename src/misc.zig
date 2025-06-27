
const Lexer = @import("lexer.zig");
const Type = @import("types.zig");

pub const String = []const u8;

/// Assign a position to every token/node, a position is only a view
/// over the code string
pub const Pos = struct {
    /// Index for the first character in the code string
    index: usize = 0,

    /// How far does the symbol span
    span: usize = 0,
};

/// Give more details about any errors that occurs during compilation,
/// that was thrown by the compiler
pub const ErrorContext = struct {
    /// Context values
    value: union(enum) {
        /// This token was not recognized by the lexer
        MalformedToken: void,

        /// The parser expected a token that was not found
        MalformedExpression: void,

        /// A token that was not expected shows up
        UnexpectedToken: []const Lexer.Tokens,

        /// An operation between one or two types that was not defined
        UndefinedOperation: struct {
            /// Left hand side of the operation
            lhs: ?*Type,

            /// Operation token
            token: Lexer.Tokens,

            /// Right hand side
            rhs: *Type,
        },

        NonBooleanDecision: struct {
            /// Non boolean type
            condtype: Type,
        },

        /// Two types that can not be joined together
        DisjointTypes: struct {
            /// Type one
            t1: Type,

            /// Type two
            t2: Type,
        },

        UnknownParameter: struct {
            /// Parameter name
            name: String,

            /// Function name
            func: String,
        },

        UnknownReference: struct {
            /// Parameter name
            name: String
        },

        /// The IR generator could not load the following type into a register
        InvalidLoad: *Type,

        /// The IR generator could not store the following type into a register
        InvalidStore: *Type,

        /// Tried to call a non function
        InvalidCall,

        /// Tried to reassign a constant
        ConstantReassignment,

        /// Tried to divide by zero
        DivisionByZero,

        /// Empty module import/export
        EmptyImport: bool,

        /// Could not resolve the current symbol
        UndefinedExternSymbol: struct {
            path: String,
            name: String
        },

        /// Base case
        NoContext,
    } = .NoContext,

    /// Error position
    position: Pos = .{},
};

//-- functions for operations

pub fn add(comptime T: type, a: T, b: T) @TypeOf(a + b) { return a + b; }
pub fn sub(comptime T: type, a: T, b: T) @TypeOf(a - b) { return a - b; }
pub fn mul(comptime T: type, a: T, b: T) @TypeOf(a * b) { return a * b; }
pub fn div(comptime T: type, a: T, b: T) T { return @divTrunc(a, b); }
pub fn mod(comptime T: type, a: T, b: T) T { return @rem(a, b); }
pub fn pipe(comptime T: type, a: T, b: T) T { return a | b; }
pub fn amp(comptime T: type, a: T, b: T) T { return a & b; }
pub fn hat(comptime T: type, a: T, b: T) T { return a ^ b; }
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