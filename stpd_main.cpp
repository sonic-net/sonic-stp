/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>

extern "C" int stpd_main();

int main(int argc, char **argv)
{
    /* stpd_main() returns -1 when it cannot set itself up (IPC socket,
     * netlink, packet socket, event loop). Exit non-zero then, so
     * supervisord sees an unexpected exit and the container's listener
     * restarts it, instead of a clean exit that leaves ports forwarding
     * with no protocol daemon. */
    return (stpd_main() == 0) ? 0 : 1;
}

