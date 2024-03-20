#include <config_file.h>
#include <unordered_map>
#include <global_defs.h>
#include <string>
#include <string.h>

static std::unordered_map<std::string, std::string> items;

extern "C" {

void loadConfiguration() {
    char line[1024], name[128], value[32], *tmp;
    FILE *file;

    if (!(file = fopen("params.txt", "r")))
        return;

    rewind(file);
    while (fgets(line, 1024, file)) {
        if (strchr(line, '#') || !(tmp = strchr(line, '=')))
            continue;

        strcpy(name, strtok(line, " =\t\n"));
        strcpy(value, strtok(NULL, " =\t\n"));
        items[name] = value;
    }

    fclose(file);
}

#define getItemValue(convertion) \
    auto it = items.find(itemName); \
    if (it == items.end()) return defaultValue; \
    return convertion;

const char* getItemAsString(const char *itemName, const char  *defaultValue) {getItemValue(it->second.c_str())}
int         getItemAsInt   (const char *itemName, const int    defaultValue) {getItemValue(atoi(it->second.c_str()))}
double      getItemAsDouble(const char *itemName, const double defaultValue) {getItemValue(atof(it->second.c_str()))}
bool        getItemAsBool  (const char *itemName, const bool   defaultValue) {getItemValue(!strncmp(it->second.c_str(), "true", 4))}

#undef getItemValue

}
