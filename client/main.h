#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include "../sdk/framework/utils/stackstring.h"

#ifdef __linux__
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#else
#define ANSI_COLOR_RED 4
#define ANSI_COLOR_GREEN 2
#define ANSI_COLOR_YELLOW 6
#define ANSI_COLOR_BLUE 1
#define ANSI_COLOR_MAGENTA 5
#define ANSI_COLOR_CYAN 3
#define ANSI_COLOR_RESET 7
#endif

#ifdef __linux__
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#else
#include "../sdk/framework/wincludes.h"
#include <winternl.h>
#include <io.h>
#endif

#define STPRINT(text) printf("%s", (const char*)ST(text))

#ifdef __linux__
#define SetColor(col) printf(col)
#else
inline void SetColor(int col)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, col);
}
#endif

//We are really desperate for this to be inlined hence the function is in the header
inline void DrawSplash()
{
	int columns = 0;
#ifdef __linux__
	struct winsize size;
	ioctl(STDOUT_FILENO,TIOCGWINSZ,&size);
	columns = size.ws_col;
#else
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#endif

	//TODO: consider not stack stringing the logos due to binary size
	SetColor(ANSI_COLOR_GREEN);
	if (columns < 81) {
	    STPRINT("        _                          \n");
	    STPRINT(" ._ _  / \\  _|     |  _. ._   _  _ \n");
	    STPRINT(" | | | \\_/ (_| |_| | (_| | o (_ (_ \n");
	    STPRINT("                                   \n\n");
	} else {
#ifdef _WIN32
		if (columns > 150)
		    STPRINT("_________________________/\\\\\\\\\\\\\\____________/\\\\\\_________________/\\\\\\\\\\\\___________________________________________________________________        \n"
				" _______________________/\\\\\\/////\\\\\\_________\\/\\\\\\________________\\////\\\\\\___________________________________________________________________       \n"
				"  ______________________/\\\\\\____\\//\\\\\\________\\/\\\\\\___________________\\/\\\\\\___________________________________________________________________      \n"
				"   ____/\\\\\\\\\\__/\\\\\\\\\\___\\/\\\\\\_____\\/\\\\\\________\\/\\\\\\___/\\\\\\____/\\\\\\____\\/\\\\\\_____/\\\\\\\\\\\\\\\\\\_____/\\\\/\\\\\\\\\\\\\\____________/\\\\\\\\\\\\\\\\_____/\\\\\\\\\\\\\\\\_     \n"
				"    __/\\\\\\///\\\\\\\\\\///\\\\\\_\\/\\\\\\_____\\/\\\\\\___/\\\\\\\\\\\\\\\\\\__\\/\\\\\\___\\/\\\\\\____\\/\\\\\\____\\////////\\\\\\___\\/\\\\\\/////\\\\\\_________/\\\\\\//////____/\\\\\\//////__    \n"
				"     _\\/\\\\\\_\\//\\\\\\__\\/\\\\\\_\\/\\\\\\_____\\/\\\\\\__/\\\\\\////\\\\\\__\\/\\\\\\___\\/\\\\\\____\\/\\\\\\______/\\\\\\\\\\\\\\\\\\\\__\\/\\\\\\___\\///_________/\\\\\\__________/\\\\\\_________   \n"
				"      _\\/\\\\\\__\\/\\\\\\__\\/\\\\\\_\\//\\\\\\____/\\\\\\__\\/\\\\\\__\\/\\\\\\__\\/\\\\\\___\\/\\\\\\____\\/\\\\\\_____/\\\\\\/////\\\\\\__\\/\\\\\\_______________\\//\\\\\\________\\//\\\\\\________  \n"
				"       _\\/\\\\\\__\\/\\\\\\__\\/\\\\\\__\\///\\\\\\\\\\\\\\/___\\//\\\\\\\\\\\\\\/\\\\_\\//\\\\\\\\\\\\\\\\\\___/\\\\\\\\\\\\\\\\\\_\\//\\\\\\\\\\\\\\\\/\\\\_\\/\\\\\\__________/\\\\\\__\\///\\\\\\\\\\\\\\\\__\\///\\\\\\\\\\\\\\\\_ \n"
				"        _\\///___\\///___\\///_____\\///////______\\///////\\//___\\/////////___\\/////////___\\////////\\//__\\///__________\\///_____\\////////_____\\////////__\n\n");
		else
			STPRINT("\n"
				"               $$$$$$\\        $$\\           $$\\                                         \n"
				"              $$$ __$$\\       $$ |          $$ |                                        \n"
				"$$$$$$\\$$$$\\  $$$$\\ $$ | $$$$$$$ |$$\\   $$\\ $$ | $$$$$$\\   $$$$$$\\   $$$$$$$\\  $$$$$$$\\ \n"
				"$$  _$$  _$$\\ $$\\$$\\$$ |$$  __$$ |$$ |  $$ |$$ | \\____$$\\ $$  __$$\\ $$  _____|$$  _____|\n"
				"$$ / $$ / $$ |$$ \\$$$$ |$$ /  $$ |$$ |  $$ |$$ | $$$$$$$ |$$ |  \\__|$$ /      $$ /      \n"
				"$$ | $$ | $$ |$$ |\\$$$ |$$ |  $$ |$$ |  $$ |$$ |$$  __$$ |$$ |      $$ |      $$ |      \n"
				"$$ | $$ | $$ |\\$$$$$$  /\\$$$$$$$ |\\$$$$$$  |$$ |\\$$$$$$$ |$$ |$$\\   \\$$$$$$$\\ \\$$$$$$$\\ \n"
				"\\__| \\__| \\__| \\______/  \\_______| \\______/ \\__| \\_______|\\__|\\__|   \\_______| \\_______|\n"
				"                                                                                        \n"
				"                                                                                        \n"
				"                                                                                        \n"
				"");
#else
		STPRINT("\n"
			   "███╗   ███╗ ██████╗ ██████╗ ██╗   ██╗██╗      █████╗ ██████╗     ██████╗ ██████╗\n"
			   "████╗ ████║██╔═████╗██╔══██╗██║   ██║██║     ██╔══██╗██╔══██╗   ██╔════╝██╔════╝\n"
			   "██╔████╔██║██║██╔██║██║  ██║██║   ██║██║     ███████║██████╔╝   ██║     ██║     \n"
			   "██║╚██╔╝██║████╔╝██║██║  ██║██║   ██║██║     ██╔══██║██╔══██╗   ██║     ██║     \n"
			   "██║ ╚═╝ ██║╚██████╔╝██████╔╝╚██████╔╝███████╗██║  ██║██║  ██║██╗╚██████╗╚██████╗\n"
			   "╚═╝     ╚═╝ ╚═════╝ ╚═════╝  ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝ ╚═════╝ ╚═════╝\n"
			   "                                                                                \n"
			   "");
#endif
	}
}


void DisableEcho();
void EnableEcho();
void EchoInput(char* buf, size_t size, char mask);

#endif
