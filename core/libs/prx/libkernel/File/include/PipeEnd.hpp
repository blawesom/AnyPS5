#ifndef CORE_LIBS_PRX_LIBKERNEL_FILE_PIPEEND_HPP
#define CORE_LIBS_PRX_LIBKERNEL_FILE_PIPEEND_HPP

#include <optional>

namespace File {

enum class PipeEnd { Read, Write };

void RememberPipeEnd(int fd, PipeEnd end);
std::optional<PipeEnd> FindPipeEnd(int fd);
void ForgetPipeEnd(int fd);

}

#endif
