#include "parser.hpp"

using Parsing::Lexer;

char Lexer::next()
{
    auto max_size = this->_content.size();
    if (this->_pos.index >= max_size)
    {
        this->_char = '\0';
        return '\0';
    }

    this->_char = this->_content.at(this->_pos.index);

    ++this->_pos.index;
    ++this->_pos.column;


    if (this->_char == '\n')
    {
        this->_pos.column = 1;
        this->_pos.last_line_pos = this->_pos.index;
        ++this->_pos.line;
    }

    return this->_char;
}

std::optional<Token> Lexer::string()
{
    size_t start = this->_pos.index + 1;

    do
        this->next();
    while (this->_char != '"');
    this->next();
    if (!this->_char)
        return std::nullopt;
    return Token(this->_content.substr(start, this->_pos.index - start - 1));
}

uint64_t Lexer::strtoi(std::string_view str)
{
    uint64_t value = 0;

    for (unsigned i = 0; i < str.size(); ++i)
    {
        value = value * 10 + (str[str.size() - i] - '0');
    }

    return value;
}

double Lexer::strtod(std::string_view str)
{
    double value = 0.0;
    size_t i = 0;
    double div = 10;

    for (; str[i] != '.'; ++i)
    {
        value = value * 10.0 + str[i] - '0';
    }

    ++i;

    for (; i < str.size(); ++i)
    {
        value += (double)(str[i] - '0') / div;
        div *= 10;
    }

    return value;
}

std::optional<Token> Lexer::number()
{
    bool dot = false;
    size_t start = this->_pos.index, count = 0;

    do
    {
        if (this->_char == '.' && dot)
        {
            return std::nullopt;
        }
        if (this->_char == '.')
        {
            dot = true;
        }
        count++;
    } while (this->next() && (isdigit(this->_char) || this->_char == '.'));

    auto substr = this->_content.substr(start, count);

    return dot ? Token(Lexer::strtod(substr)) : Token(Lexer::strtoi(substr));
}

std::optional<Token> Lexer::token(const enum TokenTypes val)
{
    this->next();
    return Token(val);
}

std::optional<Token> Lexer::identifier()
{
    Token tok = TT_Identifier;
    uint64_t offset = 0;
    uint64_t start = this->_pos.index;
    int i = 0;

    static std::string_view keywords[] = {"var", "function", "as", "do", "switch", "default", "if", "then",
                                          "else", "end", "return", "include", "type", "define", "is", ""};
    static std::string_view domains[] = {"byte", "hword", "word", "dword", "char", "short", "int", "long"};

    do
    {
        this->next();
    } while (isalnum(this->_char) || this->_char == '_');

    offset = this->_pos.index - start;

    tok.val = this->_content.substr(start, offset);

    const auto view = tok.get<std::string_view>();

    for (; i < KW_Unknown; ++i)
    {
        if (view == keywords[i])
        {
            tok.type = TT_Keyword;
            tok.val = static_cast<enum KeywordTypes>(i);
            return tok;
        }
    }

    for (i = 0; i < DM_unknown; ++i)
    {
        if (view == domains[i])
        {
            tok.type = TT_Domain;
            tok.val = static_cast<enum KeywordTypes>(i);
        }
    }

    return tok;
}

std::optional<Token> Lexer::compare(char c, enum TokenTypes type)
{
    Token tok = type;
    this->next();

    if (this->_char == '=')
    {
        tok.type = static_cast<enum TokenTypes>(type + 1);
        this->next();
    }
    else if (this->_char == c)
    {
        tok.type = static_cast<enum TokenTypes>(type + 2);
        this->next();
    }

    return tok;
}

std::optional<Token> Lexer::equal()
{
    Token tok = TT_Equal;
    this->next();

    if (this->_char == '=')
    {
        tok = TT_CompareEqual;
        this->next();
    }
    else if (this->_char == '>')
    {
        tok = TT_Arrow;
        this->next();
    }

    return tok;
}

std::optional<Token> Lexer::donot()
{
    Token tok = TT_Not;

    if (this->_char == '=')
    {
        tok = TT_NotEqual;
        this->next();
    }

    return tok;
}

std::optional<Token> Lexer::repeat(char c, enum TokenTypes type)
{
    Token tok = type;

    if (this->_char == c)
    {
        tok = static_cast<enum TokenTypes>(type + 1);
        this->next();
    }

    return tok;
}

std::optional<Token> Lexer::next_token()
{
    if (isalpha(this->_char) || this->_char == '_' ) {
        return this->identifier();
    }

    switch (this->_char)
    {
    case '\0':
        return std::nullopt;
    case '\n':
    case '\t':
    case '\v':
    case '\r':
    case ' ':
        this->next();
        return this->next_token();
    case '"':
        return this->string();
    case '#':
        while (this->_char && this->_char != '\n')
        {
            this->next();
        }

        return this->next_token();
    case ';':
        return this->token(TT_Semicolon);
    case '+':
        return this->repeat('+', TT_Plus);
    case '-':
        return this->repeat('-', TT_Minus);
    case '/':
        return this->token(TT_Divide);
    case '*':
        return this->token(TT_Multiply);
    case '=':
        return this->equal();
    case '!':
        return this->donot();
    case '?':
        return this->token(TT_Question);
    case ':':
        return this->token(TT_Colon);
    case '>':
        return this->compare('<', TT_GreaterThan);
    case '<':
        return this->compare('>', TT_LessThan);
    case '(':
        return this->token(TT_LeftParentesis);
    case ')':
        return this->token(TT_RightParentesis);
    case '[':
        return this->token(TT_LeftSquareBracket);
    case ']':
        return this->token(TT_RightSquareBracket);
    case '{':
        return this->token(TT_LeftSquareBracket);
    case '}':
        return this->token(TT_RightSquareBracket);
    case ',':
        return this->token(TT_Comma);
    case '.':
        return this->token(TT_Dot);
    case '@':
        return this->token(TT_At);
    case '|':
        return this->repeat('|', TT_BitwiseOr);
    case '&':
        return this->repeat('&', TT_BitwiseAnd);
    case '^':
        return this->token(TT_BitWiseXor);
    case '~':
        return this->token(TT_Tilda);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return this->number();
    default:
        __builtin_unreachable();
    }
}