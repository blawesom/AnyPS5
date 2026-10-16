#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <typeinfo>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <csignal>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <atomic>
#include <new>
#include "prx/libc/include/exceptions/Runtime.hpp"
#else
#include <unwind.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#endif

extern "C" unsigned __cxa_uncaught_exceptions_nid_postfix();
extern "C" void* __cxa_current_primary_exception_nid_postfix();
extern "C" void __cxa_decrement_exception_refcount_nid_postfix(void*);
extern "C" void __cxa_rethrow_primary_exception_nid_postfix(void*);

extern "C" [[noreturn]] void _ZSt14_Xout_of_rangePKc_nid_postfix(const char*);
extern "C" [[noreturn]] void _ZSt13_Xrange_errorPKc_nid_postfix(const char*);
extern "C" [[noreturn]] void _ZNSt8__sce_v219_Xbad_function_callEv_nid_postfix();
extern "C" [[noreturn]] void __cxa_bad_cast_nid_postfix();
extern "C" [[noreturn]] void _ZSt19_Throw_bad_weak_ptrv_nid_postfix();
extern "C" [[noreturn]] void _ZNKSt9exception6_RaiseEv_nid_postfix(const void*);
extern "C" void* __cxa_vec_new3_nid_postfix(std::size_t, std::size_t, std::size_t, void(*)(void*), void(*)(void*), void*(*)(std::size_t), void(*)(void*, std::size_t));
extern "C" void __cxa_vec_delete3_nid_postfix(void*, std::size_t, std::size_t, void(*)(void*), void(*)(void*, std::size_t));
extern "C" _Unwind_Reason_Code _Unwind_Backtrace_nid_postfix(_Unwind_Trace_Fn, void*);
extern "C" _Unwind_Reason_Code _Unwind_ForcedUnwind_nid_postfix(_Unwind_Exception*, _Unwind_Stop_Fn, void*);
extern "C" std::uintptr_t _Unwind_GetIP_nid_postfix(_Unwind_Context*);
extern "C" std::uintptr_t _Unwind_GetGR_nid_postfix(_Unwind_Context*, int);
extern "C" void (*_ZSt13set_terminatePFvvE_nid_postfix(void(*)()))();
#if defined(__APPLE__)
extern "C" LibcException::Header* __cxa_init_primary_exception_nid_postfix(void*, std::type_info*, void(*)(void*));
extern "C" void __cxa_increment_exception_refcount_nid_postfix(void*);
extern "C" std::type_info* __cxa_current_exception_type_nid_postfix();
extern "C" void* __cxa_get_exception_ptr_nid_postfix(void*);
extern "C" [[noreturn]] void __cxa_call_terminate_nid_postfix(void*);
extern "C" void* __dynamic_cast_nid_postfix(const void*, const std::type_info*, const std::type_info*, std::ptrdiff_t);
extern "C" void (*_ZSt13get_terminatev_nid_postfix())();
extern "C" _Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception*);
#endif

thread_local int destroyed = 0;
struct Guard {
#if defined(__APPLE__)
    ~Guard() { assert(std::uncaught_exceptions() > 0); ++destroyed; }
#else
    ~Guard() { assert(__cxa_uncaught_exceptions_nid_postfix() > 0); ++destroyed; }
#endif
};
thread_local int counted = 0;
struct Counted {
    ~Counted() { ++counted; }
};
struct Base { virtual ~Base() = default; int value = 7; };
struct Other { virtual ~Other() = default; int padding = 9; };
struct Derived : Other, Base {};
struct Virtual : virtual Base {};

[[gnu::noinline]] void ThrowInt() { Guard guard; throw 42; }
[[gnu::noinline]] void ThrowClass() { Guard guard; throw Derived(); }

struct Left : Base {};
struct Right : Base {};
struct Repeated : Left, Right {};
struct LeftVirtual : virtual Base {};
struct RightVirtual : virtual Base {};
struct Diamond : LeftVirtual, RightVirtual {};
struct PrivateDerived : private Derived {
    Base* source() { return this; }
    Derived* target() { return this; }
};
[[gnu::noinline]] Derived* ToDerived(Base* p) { return dynamic_cast<Derived*>(p); }
[[gnu::noinline]] Repeated* ToRepeated(Base* p) { return dynamic_cast<Repeated*>(p); }
void CheckRtti() {
    Repeated repeated;
    Base* left = static_cast<Left*>(&repeated);
    assert(ToRepeated(left) == &repeated);
    assert(dynamic_cast<Right*>(left) == static_cast<Right*>(&repeated));
    PrivateDerived hidden;
    assert(ToDerived(hidden.source()) == hidden.target());
    try { throw static_cast<Repeated*>(nullptr); }
    catch (Base*) { assert(false); }
    catch (Repeated* p) { assert(p == nullptr); }
    try { throw static_cast<Diamond*>(nullptr); }
    catch (Base* p) { assert(p == nullptr); }
    try { throw nullptr; }
    catch (Base* p) { assert(p == nullptr); }
    int value = 1; int* pointer = &value;
    try { throw &pointer; }
    catch (const int**) { assert(false); }
    catch (int** p) { assert(p == &pointer); }
    try { throw &pointer; }
    catch (const int* const* p) { assert(**p == value); }
}
int foreignDeleted;
void CheckForeign() {
    auto* exception = new _Unwind_Exception {};
    exception->exception_class = 0x54455354464f5200;
    exception->exception_cleanup = [](_Unwind_Reason_Code reason, _Unwind_Exception* p) {
        assert(reason == _URC_FOREIGN_EXCEPTION_CAUGHT); ++foreignDeleted; delete p;
    };
    try {
        try { _Unwind_RaiseException(exception); assert(false); }
        catch (...) { assert(__cxa_current_primary_exception_nid_postfix() == nullptr); throw; }
    } catch (...) {}
    assert(foreignDeleted == 1);
}

int staticAttempts = 0;
[[gnu::noinline]] int StaticValue() {
    static int value = [] { if (++staticAttempts == 1) throw 91; return 37; }();
    return value;
}
void CheckStaticInitialization() {
    try { StaticValue(); assert(false); } catch (int value) { assert(value == 91); }
    assert(StaticValue() == 37 && StaticValue() == 37 && staticAttempts == 2);
}

int arrayConstructed, arrayDestroyed, arrayFreed;
bool failArray;
void ArrayConstruct(void* pointer) {
    if (failArray && arrayConstructed == 2) throw 73;
    *static_cast<int*>(pointer) = arrayConstructed++;
}
void ArrayDestroy(void* pointer) {
    assert(*static_cast<int*>(pointer) == arrayConstructed - 1 - arrayDestroyed);
    ++arrayDestroyed;
}
void ArrayFree(void* pointer, std::size_t size) {
    assert(size == 4 * sizeof(int) + sizeof(std::size_t));
    ++arrayFreed;
    std::free(pointer);
}
void CheckArrays() {
    failArray = true;
    try {
        __cxa_vec_new3_nid_postfix(4, sizeof(int), sizeof(std::size_t), ArrayConstruct, ArrayDestroy, std::malloc, ArrayFree);
        assert(false);
    } catch (int value) { assert(value == 73); }
    assert(arrayConstructed == 2 && arrayDestroyed == 2 && arrayFreed == 1);
    arrayConstructed = arrayDestroyed = arrayFreed = 0;
    failArray = false;
    void* array = __cxa_vec_new3_nid_postfix(4, sizeof(int), sizeof(std::size_t), ArrayConstruct, ArrayDestroy, std::malloc, ArrayFree);
    __cxa_vec_delete3_nid_postfix(array, sizeof(int), sizeof(std::size_t), ArrayDestroy, ArrayFree);
    assert(arrayConstructed == 4 && arrayDestroyed == 4 && arrayFreed == 1);
}
#if defined(__APPLE__)
std::atomic<int> guestDestroyed;
template<class T> void DestroyGuest(void* pointer) { static_cast<T*>(pointer)->~T(); ++guestDestroyed; }
template<class T> void* Raise(const T& value) {
    void* object = __cxa_allocate_exception_nid_postfix(sizeof(T));
    new (object) T(value);
    __cxa_init_primary_exception_nid_postfix(object, const_cast<std::type_info*>(&typeid(T)), DestroyGuest<T>);
    __cxa_increment_exception_refcount_nid_postfix(object);
    return object;
}
_Unwind_Exception* UnwindOf(void* object) { return &LibcException::FromObject(object)->unwind; }
constexpr std::uintptr_t handlerRegion = 0x10000;
bool Handles(_Unwind_Exception* exception, const std::type_info* caught) {
    unsigned char lsda[24] = {0xff, 0x00, 16, 0x01, 4, 0x00, 0x20, 0x40, 0x01, 0x01, 0x00};
    std::memcpy(lsda + 11, &caught, sizeof(caught));
    _Unwind_Context context {};
    context.region = handlerRegion;
    context.lsda = reinterpret_cast<std::uintptr_t>(lsda);
    context.registers[16] = handlerRegion + 0x10;
    const auto search = __gxx_personality_v0_nid_postfix(1, _UA_SEARCH_PHASE, exception->exception_class, exception, &context);
    if (search == _URC_CONTINUE_UNWIND) return false;
    assert(search == _URC_HANDLER_FOUND);
    assert(__gxx_personality_v0_nid_postfix(1, _UA_CLEANUP_PHASE | _UA_HANDLER_FRAME, exception->exception_class, exception, &context) == _URC_INSTALL_CONTEXT);
    assert(context.registers[0] == reinterpret_cast<std::uintptr_t>(exception) && context.registers[1] == 1 && context.registers[16] == handlerRegion + 0x40);
    return true;
}
template<class T, class TInspect> bool Catch(const T& value, const std::type_info& caught, TInspect inspect) {
    void* object = Raise(value);
    auto* exception = UnwindOf(object);
    if (!Handles(exception, &caught)) {
        __cxa_decrement_exception_refcount_nid_postfix(object);
        return false;
    }
    inspect(__cxa_begin_catch_nid_postfix(exception));
    __cxa_end_catch_nid_postfix();
    return true;
}
template<class T> bool Catch(const T& value, const std::type_info& caught) { return Catch(value, caught, [](void*) {}); }
#endif
void* ThreadTest(void*) {
    for (int i = 0; i < 100; ++i) {
        try { ThrowInt(); } catch (int value) { assert(value == 42); }
        assert(__cxa_uncaught_exceptions_nid_postfix() == 0);
#if defined(__APPLE__)
        void* object = Raise(i);
        assert(__cxa_begin_catch_nid_postfix(UnwindOf(object)) == object);
        assert(__cxa_current_exception_type_nid_postfix() == &typeid(int) && *static_cast<int*>(object) == i);
        __cxa_end_catch_nid_postfix();
        assert(__cxa_current_exception_type_nid_postfix() == nullptr);
#endif
    }
    assert(destroyed == 100);
    return nullptr;
}
void CheckThreads() {
    pthread_t threads[4];
    for (auto& thread : threads) assert(pthread_create(&thread, nullptr, ThreadTest, nullptr) == 0);
    for (auto& thread : threads) assert(pthread_join(thread, nullptr) == 0);
}
[[gnu::noinline]] void UncaughtThrow() { throw 19; }
[[gnu::noinline]] void NoexceptThrow() noexcept { UncaughtThrow(); }
void CheckTerminate(void (*function)()) {
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        _ZSt13set_terminatePFvvE_nid_postfix([] { _exit(61); });
        function();
        _exit(62);
    }
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 61);
}
void CheckDefaultTerminateMessage() {
    const std::string message = std::string(3000, 'w') + " end of message";
    int channel[2];
    assert(pipe(channel) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(channel[0]);
        dup2(channel[1], 2);
#ifdef __linux__
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
#endif
        throw std::runtime_error(message);
    }
    close(channel[1]);
    std::string output;
    char buffer[4096];
    for (ssize_t count; (count = read(channel[0], buffer, sizeof(buffer))) > 0;) output.append(buffer, static_cast<std::size_t>(count));
    close(channel[0]);
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(output.find("  what():  " + message + "\n") != std::string::npos);
}
struct ForcedGuard { ~ForcedGuard() { ++destroyed; } };
[[gnu::noinline]] void ForceUnwind() {
    ForcedGuard guard;
    auto* exception = new _Unwind_Exception {};
    _Unwind_ForcedUnwind_nid_postfix(exception,
        [](int, _Unwind_Action actions, std::uint64_t, _Unwind_Exception*, _Unwind_Context*, void*) {
            if (actions & _UA_END_OF_STACK) _exit(destroyed == 1 ? 63 : 64);
            return _URC_NO_REASON;
        }, nullptr);
    _exit(65);
}
void CheckForcedUnwind() {
    pid_t child = fork(); assert(child >= 0);
    if (child == 0) { destroyed = 0; ForceUnwind(); }
    int status; assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 63);
}

#if defined(__APPLE__)
struct GuestExpectation {
    const char* type;
    const char* what;
    const std::type_info* base;
    const std::type_info* rejected;
    int value;
};
GuestExpectation expectation;
int InspectGuestTerminate() {
    if (__cxa_uncaught_exceptions_nid_postfix() != 0) return 70;
    const auto* type = __cxa_current_exception_type_nid_postfix();
    if (!type || std::strcmp(type->name(), expectation.type) != 0) return 71;
    void* object = __cxa_current_primary_exception_nid_postfix();
    if (!object) return 72;
    if (std::strcmp(expectation.type, "i") == 0 && *static_cast<int*>(object) != expectation.value) return 73;
    if (expectation.what) {
        using What = const char* (*)(const void*);
        const auto* table = *static_cast<const What* const*>(object);
        const char* what = table[2](object);
        if (!what || std::strcmp(what, expectation.what) != 0) return 74;
    }
    if (expectation.base && !Handles(UnwindOf(object), expectation.base)) return 75;
    if (expectation.rejected && Handles(UnwindOf(object), expectation.rejected)) return 76;
    return 61;
}
void ExpectGuestTerminate(void (*raise)(), const GuestExpectation& expected) {
    expectation = expected;
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        _ZSt13set_terminatePFvvE_nid_postfix([] { _exit(InspectGuestTerminate()); });
        raise();
        _exit(62);
    }
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 61);
}
void* retainedForRethrow;
void CheckGuestCatch() {
    const int before = guestDestroyed;
    assert(Catch(42, typeid(int), [](void* value) { assert(*static_cast<int*>(value) == 42); }));
    assert(guestDestroyed == before + 1);
    assert(!Catch(42, typeid(long)));
    assert(guestDestroyed == before + 2);
    assert(Catch(Derived(), typeid(Base), [](void* value) {
        assert(static_cast<Base*>(value)->value == 7);
        assert(__cxa_current_exception_type_nid_postfix() == &typeid(Derived));
    }));
    assert(Catch(Derived(), typeid(Other), [](void* value) { assert(static_cast<Other*>(value)->padding == 9); }));
    assert(Catch(Virtual(), typeid(Base), [](void* value) { assert(static_cast<Base*>(value)->value == 7); }));
    assert(Catch(13, typeid(int), [](void* outer) {
        assert(Catch(14, typeid(int), [](void* inner) { assert(*static_cast<int*>(inner) == 14); }));
        assert(__cxa_current_exception_type_nid_postfix() == &typeid(int));
        assert(*static_cast<int*>(outer) == 13);
    }));
    assert(__cxa_current_exception_type_nid_postfix() == nullptr);
    void* twice = Raise(15);
    assert(__cxa_begin_catch_nid_postfix(UnwindOf(twice)) == twice);
    assert(__cxa_begin_catch_nid_postfix(UnwindOf(twice)) == twice);
    assert(__cxa_get_exception_ptr_nid_postfix(UnwindOf(twice)) == twice);
    __cxa_end_catch_nid_postfix();
    assert(__cxa_current_exception_type_nid_postfix() == &typeid(int));
    __cxa_end_catch_nid_postfix();
    assert(__cxa_current_exception_type_nid_postfix() == nullptr);
    assert(__cxa_uncaught_exceptions_nid_postfix() == 0);
}
void CheckGuestExceptionPointers() {
    const int before = guestDestroyed;
    void* thrown = Raise(27);
    assert(__cxa_begin_catch_nid_postfix(UnwindOf(thrown)) == thrown);
    retainedForRethrow = __cxa_current_primary_exception_nid_postfix();
    assert(retainedForRethrow == thrown);
    __cxa_end_catch_nid_postfix();
    assert(guestDestroyed == before && *static_cast<int*>(retainedForRethrow) == 27);
    ExpectGuestTerminate([] { __cxa_rethrow_primary_exception_nid_postfix(retainedForRethrow); }, {"i", nullptr, &typeid(int), nullptr, 27});
    __cxa_decrement_exception_refcount_nid_postfix(retainedForRethrow);
    assert(guestDestroyed == before + 1);
    ExpectGuestTerminate([] {
        void* object = __cxa_allocate_exception_nid_postfix(sizeof(int));
        *static_cast<int*>(object) = 42;
        __cxa_throw_nid_postfix(object, const_cast<std::type_info*>(&typeid(int)), nullptr);
    }, {"i", nullptr, &typeid(int), &typeid(long), 42});
    ExpectGuestTerminate([] {
        __cxa_begin_catch_nid_postfix(UnwindOf(Raise(13)));
        __cxa_rethrow_nid_postfix();
    }, {"i", nullptr, nullptr, nullptr, 13});
    ExpectGuestTerminate([] { __cxa_call_terminate_nid_postfix(UnwindOf(Raise(5))); }, {"i", nullptr, nullptr, nullptr, 5});
}
void CheckGuestStandardExceptions() {
    ExpectGuestTerminate([] { _ZSt14_Xout_of_rangePKc_nid_postfix("test message"); },
        {"St12out_of_range", "test message", &typeid(std::logic_error), &typeid(std::runtime_error), 0});
    ExpectGuestTerminate([] { _ZSt13_Xrange_errorPKc_nid_postfix("range message"); },
        {"St11range_error", "range message", &typeid(std::range_error), nullptr, 0});
    ExpectGuestTerminate([] { _ZSt13_Xrange_errorPKc_nid_postfix(nullptr); },
        {"St11range_error", "", &typeid(std::runtime_error), nullptr, 0});
    ExpectGuestTerminate([] { _ZSt13_Xrange_errorPKc_nid_postfix("not a logic error"); },
        {"St11range_error", "not a logic error", &typeid(std::range_error), &typeid(std::logic_error), 0});
    ExpectGuestTerminate([] { _ZNSt8__sce_v219_Xbad_function_callEv_nid_postfix(); },
        {"St17bad_function_call", nullptr, &typeid(std::exception), &typeid(std::logic_error), 0});
    ExpectGuestTerminate([] { __cxa_bad_cast_nid_postfix(); }, {"St8bad_cast", nullptr, &typeid(std::exception), nullptr, 0});
    ExpectGuestTerminate([] { _ZSt19_Throw_bad_weak_ptrv_nid_postfix(); },
        {"St12bad_weak_ptr", "bad_weak_ptr", &typeid(std::exception), nullptr, 0});
    static Derived raised;
    ExpectGuestTerminate([] { _ZNKSt9exception6_RaiseEv_nid_postfix(&raised); },
        {"St9exception", "std::exception", &typeid(std::exception), &typeid(std::logic_error), 0});
}
void CheckGuestRtti() {
    Repeated repeated;
    Base* left = static_cast<Left*>(&repeated);
    assert(__dynamic_cast_nid_postfix(left, &typeid(Base), &typeid(Repeated), -1) == &repeated);
    assert(__dynamic_cast_nid_postfix(left, &typeid(Base), &typeid(Right), -1) == static_cast<Right*>(&repeated));
    PrivateDerived hidden;
    assert(__dynamic_cast_nid_postfix(hidden.source(), &typeid(Base), &typeid(Derived), -1) == hidden.target());
    Derived object;
    Base* pointer = &object;
    assert(__dynamic_cast_nid_postfix(pointer, &typeid(Base), &typeid(Derived), -1) == &object);
    assert(__dynamic_cast_nid_postfix(pointer, &typeid(Base), &typeid(Other), -1) == static_cast<Other*>(&object));
    void* caught = nullptr;
    assert(Catch(&object, typeid(Base*), [&](void* value) { caught = value; }) && caught == pointer);
    assert(!Catch(static_cast<Repeated*>(nullptr), typeid(Base*)));
    assert(Catch(static_cast<Repeated*>(nullptr), typeid(Repeated*), [](void* value) { assert(value == nullptr); }));
    assert(Catch(static_cast<Diamond*>(nullptr), typeid(Base*), [](void* value) { assert(value == nullptr); }));
    assert(Catch(nullptr, typeid(Base*), [](void* value) { assert(value == nullptr); }));
    static int value = 1;
    static int* target = &value;
    assert(!Catch(&target, typeid(const int**)));
    assert(Catch(&target, typeid(int**), [](void* caughtPointer) { assert(caughtPointer == &target); }));
    assert(Catch(&target, typeid(const int* const*), [](void* caughtPointer) { assert(**static_cast<const int* const*>(caughtPointer) == value); }));
}
int guestForeignDeleted;
void CheckGuestForeign() {
    auto* exception = new _Unwind_Exception {};
    exception->exception_class = 0x54455354464f5200;
    exception->exception_cleanup = [](_Unwind_Reason_Code reason, _Unwind_Exception* p) {
        assert(reason == _URC_FOREIGN_EXCEPTION_CAUGHT); ++guestForeignDeleted; delete p;
    };
    assert(Handles(exception, nullptr));
    assert(!Handles(exception, &typeid(int)));
    assert(__cxa_get_exception_ptr_nid_postfix(exception) == exception + 1);
    assert(__cxa_begin_catch_nid_postfix(exception) == exception + 1);
    assert(__cxa_current_primary_exception_nid_postfix() == nullptr && __cxa_current_exception_type_nid_postfix() == nullptr);
    assert(__cxa_begin_catch_nid_postfix(exception) == exception + 1);
    __cxa_end_catch_nid_postfix();
    assert(guestForeignDeleted == 0);
    __cxa_end_catch_nid_postfix();
    assert(guestForeignDeleted == 1);
}
void CheckGuestTerminateMessage() {
    static const std::string message = std::string(3000, 'w') + " end of message";
    int channel[2];
    assert(pipe(channel) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(channel[0]);
        dup2(channel[1], 2);
        _ZSt14_Xout_of_rangePKc_nid_postfix(message.c_str());
    }
    close(channel[1]);
    std::string output;
    char buffer[4096];
    for (ssize_t count; (count = read(channel[0], buffer, sizeof(buffer))) > 0;) output.append(buffer, static_cast<std::size_t>(count));
    close(channel[0]);
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(output.find("terminate called after throwing an instance of 'std::out_of_range'\n  what():  " + message + "\n") != std::string::npos);
}
void CheckGuestUnwindInterface() {
    void (*handler)() = [] { _exit(66); };
    auto* previous = _ZSt13set_terminatePFvvE_nid_postfix(handler);
    assert(_ZSt13get_terminatev_nid_postfix() == handler);
    _ZSt13set_terminatePFvvE_nid_postfix(previous);
    assert(_ZSt13get_terminatev_nid_postfix() == previous);
    _Unwind_Exception forced {};
    assert(_Unwind_ForcedUnwind_nid_postfix(&forced, nullptr, nullptr) == _URC_FATAL_PHASE2_ERROR);
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        _Unwind_ForcedUnwind_nid_postfix(&forced,
            [](int, _Unwind_Action actions, std::uint64_t, _Unwind_Exception*, _Unwind_Context* context, void*) {
                if (actions & _UA_END_OF_STACK) _exit(63);
                if (_Unwind_GetIP_nid_postfix(context) == 0) _exit(64);
                return _URC_NO_REASON;
            }, nullptr);
        _exit(65);
    }
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 63);
    int frames = 0;
    auto backtrace = _Unwind_Backtrace_nid_postfix([](_Unwind_Context* context, void* argument) {
        assert(_Unwind_GetIP_nid_postfix(context) != 0);
        ++*static_cast<int*>(argument);
        return _URC_NO_REASON;
    }, &frames);
    assert(backtrace == _URC_END_OF_STACK && frames >= 2);
    _Unwind_Context context {};
    context.registers[16] = 0x401000;
    context.registers[7] = 0x7fff0000;
    context.registers[3] = 0x33;
    const auto* slots = *reinterpret_cast<const std::uintptr_t* const*>(&context);
    const auto validRegister = reinterpret_cast<bool (*)(_Unwind_Context*, int)>(slots[2]);
    const auto getRegister = reinterpret_cast<std::uintptr_t (*)(_Unwind_Context*, int)>(slots[3]);
    const auto setRegister = reinterpret_cast<void (*)(_Unwind_Context*, int, std::uintptr_t)>(slots[4]);
    const auto validFloatRegister = reinterpret_cast<bool (*)(_Unwind_Context*, int)>(slots[5]);
    const auto isSignalFrame = reinterpret_cast<bool (*)(_Unwind_Context*)>(slots[11]);
    assert(validRegister(&context, -1) && validRegister(&context, -2) && validRegister(&context, 16) && !validRegister(&context, 17));
    assert(getRegister(&context, -1) == _Unwind_GetIP_nid_postfix(&context) && getRegister(&context, -1) == 0x401000);
    assert(getRegister(&context, -2) == _Unwind_GetGR_nid_postfix(&context, 7));
    const auto rbx = getRegister(&context, 3);
    setRegister(&context, 3, rbx + 1);
    assert(_Unwind_GetGR_nid_postfix(&context, 3) == rbx + 1);
    setRegister(&context, 3, rbx);
    assert(!validFloatRegister(&context, 17) && !isSignalFrame(&context));
}
#endif

int main() {
#if defined(__APPLE__)
    try { ThrowInt(); assert(false); } catch (int value) { assert(value == 42); }
    assert(destroyed == 1);
    try { ThrowClass(); assert(false); } catch (const Base& value) { assert(value.value == 7); }
    assert(destroyed == 2);
    assert(__cxa_uncaught_exceptions_nid_postfix() == 0);
    {
        auto made = std::make_exception_ptr(Counted());
        try { std::rethrow_exception(made); assert(false); } catch (const Counted&) {}
    }
    assert(counted == 2);
    try { _ZNKSt9exception6_RaiseEv_nid_postfix(nullptr); assert(false); }
    catch (const std::invalid_argument&) {}
    CheckGuestCatch();
    CheckGuestExceptionPointers();
    CheckGuestStandardExceptions();
    CheckGuestRtti();
    CheckGuestForeign();
    CheckStaticInitialization();
    CheckArrays();
    CheckThreads();
    CheckTerminate(UncaughtThrow);
    CheckTerminate(NoexceptThrow);
    CheckDefaultTerminateMessage();
    CheckGuestTerminateMessage();
    CheckGuestUnwindInterface();
    std::puts("exception runtime tests passed");
#else
    try { ThrowInt(); assert(false); } catch (int value) { assert(value == 42); }
    assert(destroyed == 1);
    try { ThrowClass(); assert(false); } catch (const Base& value) { assert(value.value == 7); }
    assert(destroyed == 2);
    try {
        try { throw 13; } catch (int value) { assert(value == 13); throw; }
    } catch (int value) { assert(value == 13); }
    assert(__cxa_uncaught_exceptions_nid_postfix() == 0);
    void* retained = nullptr;
    try { throw 27; } catch (...) { retained = __cxa_current_primary_exception_nid_postfix(); }
    try { __cxa_rethrow_primary_exception_nid_postfix(retained); assert(false); }
    catch (int value) { assert(value == 27); }
    __cxa_decrement_exception_refcount_nid_postfix(retained);
    {
        auto made = std::make_exception_ptr(Counted());
        try { std::rethrow_exception(made); assert(false); } catch (const Counted&) {}
    }
    assert(counted == 2);
    Derived object;
    Base* pointer = &object;
    try { throw &object; } catch (Base* value) { assert(value == pointer); }
    assert(dynamic_cast<Derived*>(pointer) == &object);
    assert(dynamic_cast<Other*>(pointer) == static_cast<Other*>(&object));
    try { throw Virtual(); } catch (Base& value) { assert(value.value == 7); }
    try { _ZSt14_Xout_of_rangePKc_nid_postfix("test message"); }
    catch (const std::logic_error& value) { assert(std::strcmp(value.what(), "test message") == 0); }
    try { _ZSt13_Xrange_errorPKc_nid_postfix("range message"); }
    catch (const std::range_error& value) { if (std::strcmp(value.what(), "range message") != 0) std::abort(); }
    try { _ZSt13_Xrange_errorPKc_nid_postfix(nullptr); }
    catch (const std::runtime_error& value) { if (std::strcmp(value.what(), "") != 0) std::abort(); }
    try { _ZSt13_Xrange_errorPKc_nid_postfix("not a logic error"); }
    catch (const std::logic_error&) { std::abort(); }
    catch (const std::range_error&) {}
    bool badFunctionCall = false;
    try { _ZNSt8__sce_v219_Xbad_function_callEv_nid_postfix(); }
    catch (const std::bad_function_call&) { badFunctionCall = true; }
    if (!badFunctionCall) std::abort();
    try { __cxa_bad_cast_nid_postfix(); }
    catch (const std::exception& value) { assert(value.what() != nullptr); }
    try { _ZSt19_Throw_bad_weak_ptrv_nid_postfix(); std::abort(); }
    catch (const std::bad_weak_ptr& value) { if (std::strcmp(value.what(), "bad_weak_ptr") != 0) std::abort(); }
    try { _ZNKSt9exception6_RaiseEv_nid_postfix(nullptr); assert(false); }
    catch (const std::invalid_argument&) {}
    try { _ZNKSt9exception6_RaiseEv_nid_postfix(&object); assert(false); }
    catch (const std::exception& value) { assert(std::strcmp(value.what(), "std::exception") == 0); }
    CheckRtti();
    CheckForeign();
    CheckStaticInitialization();
    CheckArrays();
    CheckThreads();
    CheckTerminate(UncaughtThrow);
    CheckTerminate(NoexceptThrow);
    CheckDefaultTerminateMessage();
    CheckForcedUnwind();
    int frames = 0;
    auto backtrace = _Unwind_Backtrace_nid_postfix([](_Unwind_Context* context, void* argument) {
        assert(_Unwind_GetIP_nid_postfix(context) != 0);
        ++*static_cast<int*>(argument);
        return _URC_NO_REASON;
    }, &frames);
    assert(backtrace == _URC_END_OF_STACK && frames >= 2);
    frames = 0;
    backtrace = _Unwind_Backtrace_nid_postfix([](_Unwind_Context* context, void* argument) {
        const auto* slots = *reinterpret_cast<const std::uintptr_t* const*>(context);
        const auto validRegister = reinterpret_cast<bool (*)(_Unwind_Context*, int)>(slots[2]);
        const auto getRegister = reinterpret_cast<std::uintptr_t (*)(_Unwind_Context*, int)>(slots[3]);
        const auto setRegister = reinterpret_cast<void (*)(_Unwind_Context*, int, std::uintptr_t)>(slots[4]);
        const auto validFloatRegister = reinterpret_cast<bool (*)(_Unwind_Context*, int)>(slots[5]);
        const auto isSignalFrame = reinterpret_cast<bool (*)(_Unwind_Context*)>(slots[11]);
        assert(validRegister(context, -1) && validRegister(context, -2) && validRegister(context, 16) && !validRegister(context, 17));
        assert(getRegister(context, -1) == _Unwind_GetIP_nid_postfix(context) && getRegister(context, -1) != 0);
        assert(getRegister(context, -2) == _Unwind_GetGR_nid_postfix(context, 7));
        const auto rbx = getRegister(context, 3);
        setRegister(context, 3, rbx + 1);
        assert(_Unwind_GetGR_nid_postfix(context, 3) == rbx + 1);
        setRegister(context, 3, rbx);
        assert(!validFloatRegister(context, 17) && !isSignalFrame(context));
        ++*static_cast<int*>(argument);
        return _URC_NO_REASON;
    }, &frames);
    assert(backtrace == _URC_END_OF_STACK && frames >= 2);
    std::puts("exception runtime tests passed");
#endif
}
