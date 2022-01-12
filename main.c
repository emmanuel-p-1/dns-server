#include "server.h"

// Runs the program
int main(int argc, char* argv[]) {

    // arg1 is the IPv4 address, arg2 is the port
    run_server(argv[1], atoi(argv[2]));

    return 0;
}
