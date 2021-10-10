#include <string>
#include <stdlib.h>
#include <vector>

std::string openFileDialog(const std::string message, const std::string defaultPath, const std::vector<const char*> filter);
std::string saveFileDialog(const std::string message, const std::string defaultPath, const std::vector<const char*> filter);

bool fileExists (const std::string& name);