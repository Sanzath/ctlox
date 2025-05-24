#include <ctlox/v2/scanner.hpp>

namespace test_scanner::v2 {

using namespace std::string_view_literals;

constexpr bool perform_tests() {
    constexpr auto source = R"(
// this source contains every token type (even a comment!)
(){},.-+;/*
! != = == > >= < <= foo "str" 14.5 114 nil false true
"multiline
string"
and class else fun for if or print return super this var while
)"sv;
    const auto tokens = ctlox::v2::scanner(source).scan_tokens();

    auto expect = [](bool expectation_met) {
        if (!expectation_met) {
            throw std::logic_error("test failed");
        }
    };

    expect(tokens.size() == 41);
    expect(tokens[0].type_ == ctlox::token_type::left_paren);
    expect(tokens[1].type_ == ctlox::token_type::right_paren);
    expect(tokens[2].type_ == ctlox::token_type::left_brace);
    expect(tokens[3].type_ == ctlox::token_type::right_brace);
    expect(tokens[4].type_ == ctlox::token_type::comma);
    expect(tokens[5].type_ == ctlox::token_type::dot);
    expect(tokens[6].type_ == ctlox::token_type::minus);
    expect(tokens[7].type_ == ctlox::token_type::plus);
    expect(tokens[8].type_ == ctlox::token_type::semicolon);
    expect(tokens[9].type_ == ctlox::token_type::slash);
    expect(tokens[10].type_ == ctlox::token_type::star);
    expect(tokens[11].type_ == ctlox::token_type::bang);
    expect(tokens[12].type_ == ctlox::token_type::bang_equal);
    expect(tokens[13].type_ == ctlox::token_type::equal);
    expect(tokens[14].type_ == ctlox::token_type::equal_equal);
    expect(tokens[15].type_ == ctlox::token_type::greater);
    expect(tokens[16].type_ == ctlox::token_type::greater_equal);
    expect(tokens[17].type_ == ctlox::token_type::less);
    expect(tokens[18].type_ == ctlox::token_type::less_equal);

    expect(tokens[19].type_ == ctlox::token_type::identifier);
    expect(tokens[19].lexeme_ == "foo"sv);

    expect(tokens[20].type_ == ctlox::token_type::string);
    expect(tokens[20].lexeme_ == "\"str\""sv);
    expect(tokens[20].literal_ == ctlox::v2::literal_t("str"sv));

    expect(tokens[21].type_ == ctlox::token_type::number);
    expect(tokens[21].lexeme_ == "14.5"sv);
    expect(tokens[21].literal_ == ctlox::v2::literal_t(14.5));

    expect(tokens[22].type_ == ctlox::token_type::number);
    expect(tokens[22].lexeme_ == "114"sv);
    expect(tokens[22].literal_ == ctlox::v2::literal_t(114.0));

    expect(tokens[23].type_ == ctlox::token_type::_nil);
    expect(tokens[23].literal_ == ctlox::v2::literal_t(ctlox::v2::nil));

    expect(tokens[24].type_ == ctlox::token_type::_false);
    expect(tokens[24].literal_ == ctlox::v2::literal_t(false));

    expect(tokens[25].type_ == ctlox::token_type::_true);
    expect(tokens[25].literal_ == ctlox::v2::literal_t(true));

    expect(tokens[26].type_ == ctlox::token_type::string);
    expect(tokens[26].literal_ == ctlox::v2::literal_t("multiline\nstring"sv));

    expect(tokens[27].type_ == ctlox::token_type::_and);
    expect(tokens[28].type_ == ctlox::token_type::_class);
    expect(tokens[29].type_ == ctlox::token_type::_else);
    expect(tokens[30].type_ == ctlox::token_type::_fun);
    expect(tokens[31].type_ == ctlox::token_type::_for);
    expect(tokens[32].type_ == ctlox::token_type::_if);
    expect(tokens[33].type_ == ctlox::token_type::_or);
    expect(tokens[34].type_ == ctlox::token_type::_print);
    expect(tokens[35].type_ == ctlox::token_type::_return);
    expect(tokens[36].type_ == ctlox::token_type::_super);
    expect(tokens[37].type_ == ctlox::token_type::_this);
    expect(tokens[38].type_ == ctlox::token_type::_var);
    expect(tokens[39].type_ == ctlox::token_type::_while);

    expect(tokens[40].type_ == ctlox::token_type::eof);

    return true;
}

static_assert(perform_tests());
}  // namespace test_scanner::v2