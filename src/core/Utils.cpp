#include "Utils.h"
#include "tinyfiledialogs.h"
#include <sys/stat.h>
#include <cstring>
#include <math.h>
#include <time.h>

std::string openFileDialog(const std::string message, const std::string defaultPath, const std::vector<const char*> filter)
{
    char const *selected = tinyfd_openFileDialog(message.c_str(), defaultPath.c_str(), filter.size(), filter.data(), NULL, 0);
    return (selected) ? std::string(selected) : "";
}

std::string saveFileDialog(const std::string message, const std::string defaultPath, const std::vector<const char*> filter)
{
    char const *selected = tinyfd_saveFileDialog(message.c_str(), defaultPath.c_str(), filter.size(), filter.data(), NULL);
    return (selected) ? std::string(selected) : "";
}

bool fileExists (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

void get_time_str(char * s, float elapsedTime)
{
	int hours,mins,secs;
		
	hours=(int)floor(elapsedTime/3600);
	mins=(int)floor((elapsedTime-hours*3600)/60);
	secs=(int)floor(elapsedTime-hours*3600-mins*60);
		
	s[0] = 0;	
		
	if(elapsedTime>=60)
		{
		 if(hours>0) sprintf(s,"%s%dh",s,hours);
		 if(mins>0) 
			{
			if(hours>0) strcat(s," ");
			sprintf(s,"%s%dm %ds",s,mins,secs);
			} else 
			if(secs>0)
			{
				if(hours>0) strcat(s," ");
				sprintf(s,"%s%ds",s,secs);
			}
		}
	else {
		sprintf(s,"%.1fs",elapsedTime);
	}
}

void show_elapsed_time(clock_t time2, clock_t time1)
{	
		char timestr[100];
		
		get_time_str(timestr, (float)(time2-time1)/(float)CLOCKS_PER_SEC);
		printf("%s\n", timestr);
}

const float gigo = 1000.0f*1000.0f*1000.0f;
const float mego = 1000.0f*1000.0f;

void convert_mega_giga(const float &num, float &conv_num, char &kb_mega_giga)
{	
	conv_num = num;
	kb_mega_giga = '\0';
	if(num >= gigo)
	{
		kb_mega_giga = 'G';
		conv_num = float(num) / gigo;
	}
	else
	if(num >= mego)
	{
		kb_mega_giga = 'm';
		conv_num = float(num) / mego;
	}
	else
	if(num >= 1000.0f)
	{
		kb_mega_giga = 'k';
		conv_num = float(num) / 1000.0f;
	}
}