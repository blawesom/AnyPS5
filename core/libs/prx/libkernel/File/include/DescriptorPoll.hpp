#ifndef CORE_LIBS_PRX_LIBKERNEL_FILE_DESCRIPTORPOLL_HPP
#define CORE_LIBS_PRX_LIBKERNEL_FILE_DESCRIPTORPOLL_HPP

#include "prx/libkernel/Socket/include/SocketPoll.hpp"

namespace File {

int PollHostDescriptors(KernelSocketPoll::Entry* entries, int count, int timeoutMilliseconds, const char* caller);

}

#endif
