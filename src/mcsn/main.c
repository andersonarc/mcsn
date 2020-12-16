/**
 * @file main.c
 * @author andersonarc (e.andersonarc@gmail.com)
 * @brief mcsn entry point
 * @version 0.1
 * @date 2020-12-14
 */
    /* includes */
#include "main.h" /* this */

/**
 * @brief mcsn entry point and main function
 * 
 * @param argc argument count
 * @param argv arguments
 * @return int exit status
 */
int main(int argc, const char* argv[]) {
    mcsn_connect(argv[1], 25565);
}