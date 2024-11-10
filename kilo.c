/*included headers*/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*defines*/

#define CTRL_KEY(k) ((k) & 0x1f)
#define KILO_VERSION "0.0.1"

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

/*data*/

struct editorConfig {
    int cx,cy;
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

/*terminal functions*/

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    /*prints out errno global var*/
    perror(s);
    /*exits with status of 1*/
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die ("tcgetattr");

    /*from stdlib.h -> will automatically call specified function when program exits*/
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    
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

int editorReadKey() {
    int nread;
    char c;
    /*
     *  if read returns -1, it failed.
     *  except on CYGWIN, where it will also return -1 and set errno to EAGAIN on time out
     *  instead of just returning 0
     */
    while((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    /*
     * for arrow key input instead of wasd, we must read multiple bytes
     * and translate the arrow key/pgup/pgdn to the appropriate key value
     */
    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            
            if(seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~'){
                    switch(seq[1]){
                        case '1': return HOME_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {   
                switch(seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
                }           
            }
        } else if(seq[0] == 'O'){
            switch(seq[1]){
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;
    
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    
    while(i < sizeof(buf) -1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if(buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    /*ioctl(TIOCGWINSZ) gets the current terminal window size*/
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*append buffer*/

struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    if(new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

/*output*/

void editorDrawRows(struct abuf *ab){
    int y;
    for (y = 0; y<E.screenrows; y++) {
        if (y == E.screenrows / 3) {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome),
            "Kilo editor -- version %s", KILO_VERSION);
            if (welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) / 2;
            if (padding) {
                abAppend(ab, "~", 1);
                padding--;
            }
            while (padding--) abAppend(ab, " ", 1);
            abAppend(ab, welcome, welcomelen);
        } else {
            abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[K", 3);
        if(y < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

/*
 * write 4 bytes out to the std out(terminal)  - first byte is \x1b(escape character, 27 dec), other 3 are 2[J
 * this is an escape sequence, ie \x1b#[*LETTER* making up the sequence - the 2 option for command J says clear the entire screen
 * determined by VT100 escseq, but for max compatibility ncurses lib might be better - VT100 is supported by most modern terminals though
 * \x1b[H positions the cursor at the top
 */
void editorRefreshScreen(){
    struct abuf ab = ABUF_INIT;
    
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);
    
    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);
    
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*input*/

void editorMoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            if(E.cx != 0){
                E.cx--;
            }
            break;  
        case ARROW_RIGHT:
            if(E.cx != E.screencols - 1){
                E.cx++;
            }
            break;
        case ARROW_UP:
            if(E.cy != 0){
                E.cy--;
            }
            break;          
        case ARROW_DOWN:
            if(E.cy != E.screenrows - 1){
                E.cy++;
            }
            break;
    }
}

void editorProcessKeypress() {
    int c = editorReadKey();

    /*switch case for check the different types of ctrl keys*/
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case HOME_KEY:
            E.cx
        case PAGE_UP:
        case PAGE_DOWN:
            {/*normally can't declare vars in a sc but the {} allow it sort of like an anonymous function almost??*/
                int times = E.screenrows;

                while(times--)
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);/*ternary - if c == page up, use arrow_up, else use arrow_down*/
            }    
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

/*init func*/

void initEditor(){
    E.cx = 0;
    E.cy = 0;

    if( getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){
    enableRawMode();
    initEditor();

    //read in chars: STDIN_FILENO is the std input, should quit when q is entered
    while (1){
        editorRefreshScreen();
        editorProcessKeypress();
    }

   return 0;
}