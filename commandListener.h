/* NVIDIA CORPORATION gave permission to FLIR Systems, Inc to modify this code
  * and distribute it as part of the ADAS GMSL Kit.
  * http://www.flir.com/
  * October-2019
*/
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
        bool userCancel = false;
};

}

#endif