#pragma once
#include <stdexcept>
#include <string>

class CodeFabException : public std::runtime_error {
public:
    CodeFabException(std::string message, int line, int column)
        : std::runtime_error(std::move(message)), line(line), column(column) {}
    int line;
    int column;
};

class TokenizeError : public CodeFabException { using CodeFabException::CodeFabException; };
class ParseError    : public CodeFabException { using CodeFabException::CodeFabException; };
class CheckError    : public CodeFabException { using CodeFabException::CodeFabException; };
class ExecuteError  : public CodeFabException { using CodeFabException::CodeFabException; };
