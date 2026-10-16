#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_MAINTHREAD_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_MAINTHREAD_HPP

extern "C" bool LibcMainThreadServing_nid_postfix();
extern "C" void LibcPostToMainThread_nid_postfix(void (*task)(void*), void* context);
extern "C" [[noreturn]] void LibcServeMainThread_nid_postfix();

#endif
