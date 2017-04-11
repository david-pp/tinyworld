#ifndef _COMMON_TINTYWORLD_H
#define _COMMON_TINTYWORLD_H

typedef unsigned int uint;

#ifdef _MSC_VER

typedef __int8  int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

#else

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#endif

#define TINY_NAMESPACE_BEGIN // namespace tiny {
#define TINY_NAMESPACE_END   // }


#include <functional>
#include <iomanip>

//
// 利用构造函数执行一些初始化代码的技巧
//
struct RunOnceHelper {
    RunOnceHelper(std::function<void()> func) {
        func();
    }
};

#define RUN_ONCE(tagname) \
        void reg_func_##tagname(); \
        Register reg_obj_##structname(reg_func_##structname); \
        void reg_func_##tagname()


//
// 输出二进制(仿照`hexdump -C`命令)
//
inline void hexdump(const std::string &hex, std::ostream &os = std::cout) {
    size_t line = 0;
    size_t left = hex.size();

    while (left) {
        size_t linesize = (left >= 16 ? 16 : left);
        std::string linestr = hex.substr(line * 16, linesize);

        os << std::hex << std::setw(8) << std::setfill('0') << line * 16 << " ";
        for (size_t i = 0; i < linestr.size(); ++i) {
            if (i % 8 == 0)
                os << " ";

            int v = (uint8_t) linestr[i];
            os << std::hex << std::setw(2) << std::setfill('0') << v << " ";
        }

        os << " " << "|";
        for (size_t i = 0; i < linestr.size(); ++i) {
            if (std::isprint(linestr[i]))
                os << linestr[i];
            else
                os << '.';
        }
        os << "|" << std::endl;

        left -= linesize;
        line++;
    }
}

#endif // _COMMON_TINTYWORLD_H