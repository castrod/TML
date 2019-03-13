#include <cassert>
typedef int64_t int_t;
typedef wchar_t* wstr;
typedef std::vector<int_t> term;
typedef std::vector<term> matrix;// set of relational terms
typedef std::vector<bool> bools;
typedef std::vector<bools> vbools;
std::wostream& operator<<(std::wostream& os, const matrix& m);
//#define DEBUG
#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif
#define er(x)	perror(x), exit(0)
#define msb(x) ((sizeof(unsigned long long)<<3) - \
	__builtin_clzll((unsigned long long)(x)))