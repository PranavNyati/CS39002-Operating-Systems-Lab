#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;


#define CTRL_CZ -1
#define CTRL_A 1
#define CTRL_E 5
#define BACKSPACE 127
#define ENTER 10
#define ESC 27

string getCmd(bool echo = false);

char *get_cmd();