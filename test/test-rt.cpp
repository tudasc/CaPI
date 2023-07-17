//
// Created by sebastian on 15.10.21.
//

#include "test-rt.h"

#include <stdio.h>
#include <string>
#include <cstdint>

namespace {
    int call_depth{-1};
    const char* exec_path{nullptr};
}

void init_verifier_rt(int argc, char** argv) {
    exec_path = argv[0];
}

std::string resolve_function_name(const void* fptr) {
    char command[64];
    int n = sprintf(command, "nm %s | grep %lx | awk '{ printf \"%%s\", $3 }'", exec_path, reinterpret_cast<std::uintptr_t>(fptr));

    FILE *output = popen(command, "r");
    char result[32];
    if (!fgets(result, sizeof(result), output)){
        return "unknown";
    }
    pclose(output);

    return result;
}

extern "C" {

#define print_fn_info(fn, caller) \
        do {\
            printf("%d, %s: fn = %p, caller = %p\n", call_depth, __FUNCTION__, fn, caller); \
        } while(0)


void __cyg_profile_func_enter(void* fn, void* caller) {
    call_depth++;
    printf("ENTER %s %d\n", resolve_function_name(fn).c_str(), call_depth);
//    print_fn_info(fn, caller);
}

void __cyg_profile_func_exit(void* fn, void* caller) {
    printf("EXIT %s %d\n", resolve_function_name(fn).c_str(), call_depth);
//    print_fn_info(fn, caller);
    call_depth--;
}

}
