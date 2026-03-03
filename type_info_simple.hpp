#pragma once

#include "il2cpp_lib.hpp"

namespace dumper {
    // Simple, safe type info dumping
    extern void dump_type_info_addresses_simple();
    
    // Find static instance address for a class
    extern uint64_t find_static_instance_address(il2cpp::il2cpp_class_t* klass);
    
    // Simple lookup function
    extern uint64_t get_type_info_simple(const char* class_name, const char* namespace_name = "");
    
    // Overload for convenience
    inline uint64_t get_type_info_simple(const char* class_name) {
        return get_type_info_simple(class_name, "");
    }
}
