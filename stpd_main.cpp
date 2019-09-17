/*
 * Copyright 2019 Broadcom. All rights reserved. 
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include <iostream>

extern "C" void stpd_main();

int main(int argc, char **argv)
{
    stpd_main();
    return 0;
}

