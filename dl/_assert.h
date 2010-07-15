#pragma once

#define _assert(x) do { if(!(x)) { Fatal(__LINE__, "Assertion failed (%s:%d): %s", __FILE__, __LINE__, #x); } } while(0)
#define _assertZero(x) do { int _x = x; if(_x) { Fatal(__LINE__, "Assertion failed (%s:%d): 0 == (%s) (but it was %d)", __FILE__, __LINE__, #x, _x); } } while(0)

#define _assertWithCode(x, n) do { if(!(x)) { Fatal((n), "Assertion failed (%s:%d): %s", __FILE__, __LINE__, #x); } } while(0)
#define _assertZeroWithCode(x, n) do { int _x = x; if(_x) { Fatal((n), "Assertion failed (%s:%d): 0 == (%s) (but it was %d)", __FILE__, __LINE__, #x, _x); } } while(0)
