#pragma once
#ifdef TESTING // NSLog's output is prettier
void NSLog(CFStringRef, ...);
#define Log(format, stuff...) NSLog(CFSTR(format), ##stuff)
#else
void CFLog(int32_t level, CFStringRef format, ...);
#define Log(format, stuff...) CFLog(3, CFSTR(format), ##stuff)
#endif
#define A(x) if(!(x)) { Log("Assertion failed (%s:%d): %s", __FILE__, __LINE__, #x); exit(1); }
#define AZERO(x) do { int _x = x; if(0 != _x) { Log("Assertion failed (%s:%d): 0 == (%s) (but it was %d)", __FILE__, __LINE__, #x, _x); exit(1); } } while(0)
