#include <string>
#include <stdlib.h>
#include <vector>
#include <time.h>

std::string openFileDialog(const std::string message, const std::string defaultPath, const std::vector<const char*> filter);
std::string saveFileDialog(const std::string message, const std::string defaultPath, const std::vector<const char*> filter);

bool fileExists (const std::string& name);

void get_time_str(char * s, float elapsedTime);
void show_elapsed_time(clock_t time2, clock_t time1);

void convert_mega_giga(const float &num, float &conv_num, char &kb_mega_giga);