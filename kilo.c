#include <unistd.h>

int main(){
    
    char c;
    //read in chars: STDIN_FILENO is the std input
    while (read(STDIN_FILENO, &c, 1) == 1);
    
    return 0;

}