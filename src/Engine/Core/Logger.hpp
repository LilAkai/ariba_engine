#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>

#define RED_STRING    "\033[31m"
#define GREEN_STRING  "\033[32m"
#define YELLOW_STRING "\033[33m"
#define BLUE_STRING   "\033[34m"
#define RESET_STRING  "\033[0m"

struct Logger {
    static void Info(const std::string& message) {
        std::cout << BLUE_STRING
                  << "[INFO] "
                  << RESET_STRING;

        std::cout << message << std::endl;
    }

    static void Success(const std::string& message) {
        std::cout << GREEN_STRING
                  << "[SUCCESS] "
                  << RESET_STRING;

        std::cout << message << std::endl;
    }

    static void Warning(const std::string& message) {
        std::cout << YELLOW_STRING
                  << "[WARNING] "
                  << RESET_STRING;

        std::cout << message << std::endl;
    }

    static void Error(const std::string& message) {
        std::cout << RED_STRING
                  << "[ERROR] "
                  << RESET_STRING;

        std::cout << message << std::endl;
    }
};

#endif