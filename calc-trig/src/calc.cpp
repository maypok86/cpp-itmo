#include "calc.h"

#include <cctype>   // for std::isspace
#include <cmath>    // various math functions
#include <iostream> // for error reporting via std::cerr
#include <vector>

namespace {

const std::size_t max_decimal_digits = 10;
const double eps = 1e-15;

enum class Op
{
    ERR,
    SET,
    ADD,
    SUB,
    MUL,
    DIV,
    REM,
    NEG,
    POW,
    SQRT,
    SIN,
    COS,
    TAN,
    CTN,
    ASIN,
    ACOS,
    ATAN,
    ACTN,
    RAD,
    DEG
};

std::size_t arity(const Op op)
{
    switch (op) {
        // error
    case Op::ERR: return -1;
    case Op::RAD: return 0;
    case Op::DEG:
        return 0;
        // unary
    case Op::NEG: return 1;
    case Op::SQRT: return 1;
    case Op::SIN: return 1;
    case Op::COS: return 1;
    case Op::TAN: return 1;
    case Op::CTN: return 1;
    case Op::ASIN: return 1;
    case Op::ACOS: return 1;
    case Op::ATAN: return 1;
    case Op::ACTN:
        return 1;
        // binary
    case Op::SET: return 2;
    case Op::ADD: return 2;
    case Op::SUB: return 2;
    case Op::MUL: return 2;
    case Op::DIV: return 2;
    case Op::REM: return 2;
    case Op::POW: return 2;
    default: return 0;
    }
}

bool is_digit(const char symbol)
{
    return symbol >= '0' && symbol <= '9';
}

Op parse_string_op(const std::string & line, std::size_t & i)
{
    const auto test = [&i, &line](const std::string & s) {
        for (std::size_t back = 0; back < s.size(); ++back) {
            std::size_t index = i + back;
            if (index >= line.size() || line[index] != s[back]) {
                return false;
            }
        }
        i += s.size();
        return true;
    };
    const std::vector<std::pair<std::string, Op>> operations{
            {"SQRT", Op::SQRT},
            {"SIN", Op::SIN},
            {"COS", Op::COS},
            {"TAN", Op::TAN},
            {"CTN", Op::CTN},
            {"ASIN", Op::ASIN},
            {"ACOS", Op::ACOS},
            {"ATAN", Op::ATAN},
            {"ACTN", Op::ACTN},
            {"DEG", Op::DEG},
            {"RAD", Op::RAD}};
    for (const auto & operation : operations) {
        if (test(operation.first)) {
            return operation.second;
        }
    }
    return Op::ERR;
}

Op parse_op(const std::string & line, std::size_t & i)
{
    if (is_digit(line[i])) {
        return Op::SET;
    }
    switch (line[i++]) {
    case '+':
        return Op::ADD;
    case '-':
        return Op::SUB;
    case '*':
        return Op::MUL;
    case '/':
        return Op::DIV;
    case '%':
        return Op::REM;
    case '_':
        return Op::NEG;
    case '^':
        return Op::POW;
    default:
        --i;
        Op op = parse_string_op(line, i);
        if (op == Op::ERR) {
            std::cerr << "Unknown operation " << line << std::endl;
        }
        return op;
    }
}

std::size_t skip_ws(const std::string & line, std::size_t i)
{
    while (i < line.size() && std::isspace(line[i])) {
        ++i;
    }
    return i;
}

double parse_arg(const std::string & line, std::size_t & i)
{
    double res = 0;
    std::size_t count = 0;
    bool good = true;
    bool integer = true;
    double fraction = 1;
    while (good && i < line.size() && count < max_decimal_digits) {
        switch (line[i]) {
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
            if (integer) {
                res *= 10;
                res += line[i] - '0';
            }
            else {
                fraction /= 10;
                res += (line[i] - '0') * fraction;
            }
            ++i;
            ++count;
            break;
        case '.':
            integer = false;
            ++i;
            break;
        default:
            good = false;
            break;
        }
    }
    if (!good) {
        std::cerr << "Argument parsing error at " << i << ": '" << line.substr(i) << "'" << std::endl;
    }
    else if (i < line.size()) {
        std::cerr << "Argument isn't fully parsed, suffix left: '" << line.substr(i) << "'" << std::endl;
    }
    return res;
}

// тригонометрические функции всегда принимают аргумент в радианах
// эта функция преобразует градусы в радины, если сейчас вычисления происходят в градусах
double cast_trig_arg(const double number, const bool rad_on)
{
    if (!rad_on) {
        return M_PI * number / 180;
    }
    return number;
}

// тригонометрические аркфункции всегда возвращают результат в радианах
// эта функция преобразует радианы в градусы, если сейчас вычисления происходят в градусах
double cast_arc_trig_result(const double number, const bool rad_on)
{
    if (!rad_on) {
        return 180 * number / M_PI;
    }
    return number;
}

double unary(const double current, const Op op, const bool rad_on)
{
    switch (op) {
    case Op::NEG:
        return -current;
    case Op::SIN:
        return std::sin(cast_trig_arg(current, rad_on));
    case Op::COS:
        return std::cos(cast_trig_arg(current, rad_on));
    case Op::TAN:
        return std::tan(cast_trig_arg(current, rad_on));
    case Op::CTN: {
        const double t = std::tan(cast_trig_arg(current, rad_on));
        if (std::fabs(t) < eps) {
            if (t < 0) {
                return -INFINITY;
            }
            return INFINITY;
        }
        return 1 / t;
    }
    case Op::ASIN:
        return cast_arc_trig_result(std::asin(current), rad_on);
    case Op::ACOS:
        return cast_arc_trig_result(std::acos(current), rad_on);
    case Op::ATAN:
        return cast_arc_trig_result(std::atan(current), rad_on);
    case Op::ACTN:
        return cast_arc_trig_result(M_PI_2 - std::atan(current), rad_on);
    case Op::SQRT:
        if (current >= 0) {
            return std::sqrt(current);
        }
        std::cerr << "Bad argument for SQRT: " << current << std::endl;
        [[fallthrough]];
    default:
        return current;
    }
}

double binary(const Op op, const double left, const double right)
{
    switch (op) {
    case Op::SET:
        return right;
    case Op::ADD:
        return left + right;
    case Op::SUB:
        return left - right;
    case Op::MUL:
        return left * right;
    case Op::DIV:
        if (right != 0) {
            return left / right;
        }
        else {
            std::cerr << "Bad right argument for division: " << right << std::endl;
            return left;
        }
    case Op::REM:
        if (right != 0) {
            return std::fmod(left, right);
        }
        else {
            std::cerr << "Bad right argument for remainder: " << right << std::endl;
            return left;
        }
    case Op::POW:
        return std::pow(left, right);
    default:
        return left;
    }
}

double nullary(const double current, const Op op, bool & rad_on)
{
    if (op == Op::RAD) {
        rad_on = true;
    }
    else if (op == Op::DEG) {
        rad_on = false;
    }
    return current;
}

} // anonymous namespace

double process_line(const double current, bool & rad_on, const std::string & line)
{
    std::size_t i = 0;
    const auto op = parse_op(line, i);
    switch (arity(op)) {
    case 2: {
        i = skip_ws(line, i);
        const auto old_i = i;
        const auto arg = parse_arg(line, i);
        if (i == old_i) {
            std::cerr << "No argument for a binary operation" << std::endl;
            break;
        }
        else if (i < line.size()) {
            break;
        }
        return binary(op, current, arg);
    }
    case 1: {
        if (i < line.size()) {
            std::cerr << "Unexpected suffix for a unary operation: '" << line.substr(i) << "'" << std::endl;
            break;
        }
        return unary(current, op, rad_on);
    }
    case 0: {
        // передаётся current, так как в теории он может потребоваться какой-то нульарной функции
        return nullary(current, op, rad_on);
    }
    default: break;
    }
    return current;
}
