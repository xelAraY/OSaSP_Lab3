#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main() {
    pid_t pid;
    pid = fork();

    if(pid > 0){
        printf("This is parent procc. His pid is %d\n", getpid());
        pid = fork();
        if(!pid){
            printf("This is child procc. His pid is %d\n", getpid());
            printf("Pid his parent procc is %d\n", getppid());
            GetTime();
        }
        else if(pid < 0){
            perror("Mistake to create a new procc(2)...\n");
        }
        else {
            GetTime();
        }
    }
    else if(pid < 0){
        perror("Mistake to create a new procc(1)...\n");
    }else{
        printf("This is child procc. His pid is %d\n", getpid());
        printf("Pid his parent procc is %d\n", getppid());
        GetTime();
    }
    
    if(pid){
        system("ps -x");
    }
    wait();

    return 0;
}

void GetTime() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    char buff[100];
    strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
    printf("Current time: %s.%03ld\n", buff, ts.tv_nsec);
}