/*included headers*/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*defines*/

#define CTRL_KEY(k) ((k) & 0x1f)

/*data*/

struct termios orig_termios;

/*terminal functions*/

void die(const char *s) {
    /*prints out errno global var*/
    perrors(s);
    /*exits with status of 1*/
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) die ("tcgetattr");

    /*from stdlib.h -> will automatically call specified function when program exits*/
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    
    /*c_iflag is for input flags specifically
    IXON controls CTRLS/CTRLQ characters
    ICRNL controls carriage return/ newline translation
    BRKINT controls whether break conditions will cause a SIGINT signal to be sent like ctrl c
    INPCK controls parity checking, not really relevant for modern terminals but is a legacy practice
    ISTRIP causes the 8th bit of each input to be set to 0 - probably already off but just to be safe
    ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON) is performing a NOT operation on the BRKINT,ICRNL,INPCK,ISTRIP,IXON flags*/
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /*c_oflag is for output flags specifically
    OPOST controls output processing
    ~(OPOST) is performing a NOT operation on the OPOST flag*/
    raw.c_oflag &= ~(OPOST);
    /*CS8 is a bit mask which is set using the bitwise OR operator ( | ) , sets character size to 8 bits per byte.*/
    raw.c_cflag |= (CS8);
    /*c_lflag is for local flags*/
    /*now truly reading text byte by byte*/
    /*~(ECHO | ICANON | ISIG) is performing a NOT operation on the ECHO, ICANON, AND ISIG, IEXTEN bitflag
    ECHO flag just controls whether input is output back to the terminal
    ICANON flag controls whether the terminal is in raw mode or cooked mode
    ISIG controls whether control characters like CTRLZ and CTRLC work inside the program
    IEXTEN controls the CTRL V & CTRL O*/
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /*VMIN controls the minimum number of bytes needed before read() can return
    setting it to 0 lets it return as soon as there is anything to be read
    VTIME is the maximum amount of time to wait before read() returns, in 1/10ths of a second
    if read() times out it will return 0*/
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;


    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
    int nread;
    char c;
    /*if read returns -1, it failed.
        except on CYGWIN, where it will also return -1 and set errno to EAGAIN on time out
        instead of just returning 0*/
    while((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errono != EAGAIN) die("read");
    }
    return c;
}

/*input*/

void editorProcessKeypress() {
    char c = editorReadKey();

    /*switch case for check the different types of ctrl keys*/
    switch (c) {
        case CTRL_KEY('q'):
        exit(0);
        break;
    }
}

/*init func*/

int main(){
    enableRawMode();

    char c;
    //read in chars: STDIN_FILENO is the std input, should quit when q is entered
    while (1){
        editorProcessKeypress();
    }

   return 0;
}