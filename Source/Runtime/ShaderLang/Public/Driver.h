#pragma once

#include <iostream>
#include "Scanner.h"
#include "../Generated/parser.hpp"

namespace Marker
{
    class Driver
    {
    private:
        Parser _parser;
        Scanner _scanner;
    public:
        Driver();

        int parse();

        virtual ~Driver();
    };
}


