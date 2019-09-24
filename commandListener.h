#ifndef __COMMAND_LISTENER_H__
#define __COMMAND_LISTENER_H__

#include <iostream>
#include <string.h>

#include "nvidiaInterface.h"

namespace BosonAPI {

class CommandListener {
    public:
        CommandListener(NvidiaInterface *interface);
        ~CommandListener();
        void listen();
        void stop();
    private:
        NvidiaInterface *interface;
};

}

#endif