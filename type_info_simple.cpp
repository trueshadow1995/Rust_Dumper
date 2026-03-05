#include "dumper.hpp"
#include "util.hpp"

namespace dumper {
    // Simple, safe type info dumping - just the addresses
    void dump_type_info_addresses_simple() {
        write_to_log("[SIMPLE] Starting simple type info dump...\n");
        write_to_log("[SIMPLE] Type info table address: 0x%llx\n", (uint64_t)type_info_definition_table);
        
        if (!type_info_definition_table) {
            write_to_log("[INFO] Type info table is null, dumping without type info addresses...\n");
            write_to_file("\n// Type Info Addresses (Simple Mode - No Type Info Table)\n");
            write_to_file("namespace TypeInfo {\n");
            write_to_file("\t// Type info table not available - using alternative methods\n");
            write_to_file("\tconstexpr const static uint64_t table_address = 0x0;\n");
            write_to_file("}\n\n");
            
            // Try to dump some basic Unity classes anyway
            write_to_file("// Basic Unity Classes (dumped via IL2CPP)\n");
            write_to_file("namespace UnityClasses {\n");
            
            const char* basic_classes[] = {"Object", "GameObject", "Component", "Transform", "Camera"};
            for (const char* class_name : basic_classes) {
                __try {
                    il2cpp::il2cpp_class_t* klass = il2cpp::get_class_by_name(class_name, "UnityEngine");
                    if (is_valid_ptr(klass) && is_valid_ptr(klass->name())) {
                        uint64_t rva = (uint64_t)klass - (uint64_t)dumper::game_base;
                        write_to_file("\tconstexpr const static uint64_t %s = 0x%llx;\n", class_name, rva);
                        write_to_log("[SIMPLE] Found class %s at 0x%llx\n", class_name, (uint64_t)klass);
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    write_to_log("[SIMPLE] Exception getting class %s\n", class_name);
                }
            }
            
            write_to_file("}\n\n");
            write_to_log("[SIMPLE] Basic dump completed without type info table\n");
            return;
        }

        write_to_file("\n// Type Info Addresses (Simple Mode)\n");
        write_to_file("namespace TypeInfo {\n");
        write_to_file("\tconstexpr const static uint64_t table_address = 0x%llx;\n", (uint64_t)type_info_definition_table);
        
        int found_count = 0;
        int max_scan = 10000; // Conservative limit
        
        write_to_log("[SIMPLE] Starting to scan %d entries...\n", max_scan);
        
        for (int i = 0; i < max_scan; i++) {
            __try {
                // Basic null check
                if (!type_info_definition_table[i]) {
                    continue;
                }
                
                // Basic pointer validation
                if (!is_valid_ptr(type_info_definition_table[i])) {
                    continue;
                }
                
                il2cpp::il2cpp_class_t* klass = type_info_definition_table[i];
                
                // Validate name and namespace pointers
                if (!is_valid_ptr(klass->name()) || !is_valid_ptr(klass->namespaze())) {
                    continue;
                }
                
                // Skip if names are empty/invalid
                if (!klass->name()[0] || !klass->namespaze()[0]) {
                    continue;
                }
                
                // Get class info - look for actual Rust game classes
                const char* name = klass->name();
                const char* namespaze = klass->namespaze();
                
                // Debug: Log first few classes to see what we're scanning
                if (found_count < 10) {
                    write_to_log("[DEBUG] Scanning: %s.%s\n", namespaze, name);
                }
                
                // Check for important classes using exact string matching
                bool is_important_class = false;
                const char* clean_name = "";
                
                // Use flexible string matching to find obfuscated classes
                if (strstr(name, "BaseNetworkable") && strcmp(namespaze, "") == 0) {
                    is_important_class = true;
                    clean_name = "BaseNetworkable";
                    write_to_log("[MATCH] Found BaseNetworkable: %s\n", name);
                }
                else if (strstr(name, "BasePlayer") && strcmp(namespaze, "") == 0) {
                    is_important_class = true;
                    clean_name = "BasePlayer";
                    write_to_log("[MATCH] Found BasePlayer: %s\n", name);
                }
                else if (strstr(name, "ProgressBar") && strstr(namespaze, "UnityEngine.UI")) {
                    is_important_class = true;
                    clean_name = "ProgressBar";
                    write_to_log("[MATCH] Found ProgressBar: %s\n", name);
                }
                else if (strstr(name, "MainCamera") && strcmp(namespaze, "") == 0) {
                    is_important_class = true;
                    clean_name = "MainCamera";
                    write_to_log("[MATCH] Found MainCamera: %s\n", name);
                }
                else if (strstr(name, "GameManager") && strcmp(namespaze, "") == 0) {
                    is_important_class = true;
                    clean_name = "GameManager";
                    write_to_log("[MATCH] Found GameManager: %s\n", name);
                }
                else if (strstr(name, "ItemManager") && strcmp(namespaze, "") == 0) {
                    is_important_class = true;
                    clean_name = "ItemManager";
                    write_to_log("[MATCH] Found ItemManager: %s\n", name);
                }
                // Add more flexible matches as needed
                
                // Skip if not an important class
                if (!is_important_class) {
                    continue;
                }
                
                // Skip system junk
                if (strstr(namespaze, "System.") || 
                    strstr(namespaze, "UnityEngine") ||
                    strstr(namespaze, "Microsoft") ||
                    strstr(namespaze, "Epic.OnlineServices")) {
                    continue;
                }
                
                // Write the RVA instead of heap address
                uint64_t rva = (uint64_t)klass - (uint64_t)dumper::game_base;
                write_to_file("\tconstexpr const static uint64_t %s = 0x%llx;\n", 
                             clean_name, rva);
                 
                found_count++;
                
                // Stop after finding enough good entries
                if (found_count >= 2000) {
                    write_to_log("[SIMPLE] Found %d good entries, stopping\n", found_count);
                    break;
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                write_to_log("[SIMPLE] Exception at index %d, skipping\n", i);
                continue;
            }
        }
        
        write_to_file("\tconstexpr const static int total_entries = %d;\n", found_count);
        write_to_file("}\n\n");
        
        write_to_log("[SIMPLE] Simple dump complete: %d entries\n", found_count);
    }

    // Simple lookup function
    uint64_t get_type_info_simple(const char* class_name, const char* namespace_name) {
        if (!type_info_definition_table) {
            return 0;
        }

        for (int i = 0; i < 2000; i++) { // Conservative limit
            if (!type_info_definition_table[i] || !is_valid_ptr(type_info_definition_table[i])) {
                continue;
            }
            
            il2cpp::il2cpp_class_t* klass = type_info_definition_table[i];
            
            if (!is_valid_ptr(klass->name()) || !is_valid_ptr(klass->namespaze())) {
                continue;
            }
            
            if (strcmp(klass->name(), class_name) == 0 && 
                strcmp(klass->namespaze(), namespace_name) == 0) {
                return (uint64_t)klass;
            }
        }
        
        return 0;
    }
}
