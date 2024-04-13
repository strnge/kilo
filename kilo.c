#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}


void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);

    //from stdlib.h -> will automatically call specified function when program exits
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    //c_lflag is for local flags
    //~(ECHO | ICANON) is performing a NOT operation on the ECHO and ICANON bitflag
    //now truly reading text byte by byte
    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(){
    enableRawMode();


    char c;
    //read in chars: STDIN_FILENO is the std input, should quit when q is entered
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    


    return 0;

}