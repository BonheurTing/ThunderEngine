#pragma once
#include <memory>
#include <string>

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual double evaluate() const = 0;
};

class NumberNode : public ASTNode {
    double value;
public:
    explicit NumberNode(double val) : value(val) {}
    double evaluate() const override { return value; }
};

class BinaryOpNode : public ASTNode {
    char op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
public:
    BinaryOpNode(char op, std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    double evaluate() const override {
        double l = left->evaluate();
        double r = right->evaluate();
        
        switch(op) {
            case '+': return l + r;
            case '-': return l - r;
            case '*': return l * r;
            case '/': return r != 0 ? l / r : throw std::runtime_error("Division by zero");
            default: throw std::runtime_error("Unknown operator");
        }
    }
};