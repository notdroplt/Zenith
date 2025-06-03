
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

        /// A token that was not expected shows up
        UnexpectedToken: struct {
            /// Token received
            token: Lexer.Token,

            /// Token expected
            expected: Lexer.Tokens,
        },

        /// An operation between one or two types that was not defined
        UndefinedOperation: struct {
            /// Left hand side of the operation
            lhs: ?Type,

            /// Operation token
            token: Lexer.Tokens,

            /// Right hand side
            rhs: Type,
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

        /// The IR generator could not load the following type into a register
        InvalidLoad: *Type,

        /// The IR generator could not store the following type into a register
        InvalidStore: *Type,

        /// Base case
        NoContext: void,
    },

    /// Error position
    position: Pos = .{},
};