
#include "../Public/Driver.h"
#include <iostream>

using namespace Marker;
using namespace std;

Driver::Driver() : _scanner(*this), _parser(_scanner, *this) {

}

Driver::~Driver() {

}

int Driver::parse() {
  _scanner.switch_streams(cin, cout);
  return _parser.parse();
}
