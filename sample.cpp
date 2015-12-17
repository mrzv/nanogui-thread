#if defined(WIN32)
#include <windows.h>
#endif

#include <iostream>
#include <thread>

#include <nanogui/screen.h>
namespace ng = nanogui;

#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include "coro.h"
typedef     coro_context                    context_t;

struct Environment
{
    static void exec_nanogui(void* self_)
    {
        Environment* self = static_cast<Environment*>(self_);
        self->m_nanogui.lock();
        self->m_original.unlock();

        ng::init();

        self->app = new ng::Screen(ng::Vector2i { 800, 600 }, "Test");

        self->app->drawAll();
        self->app->setVisible(true);
        ng::mainloop();

        delete self->app;
        ng::shutdown();

        self->m_nanogui.unlock();
        self->m_original.lock();
        coro_transfer(&self->c_nanogui, &self->c_original);
    }

    void run()
    {
        coro_create(&c_original, NULL, NULL, NULL, 0);
        coro_stack_alloc(&stack, 8*1024*1024);     // 8MB stack

        m_original.lock();

        t = std::thread([&]()
        {
            coro_create(&c_thread1, NULL, NULL, NULL, 0);
            m_original.lock();
            coro_transfer(&c_thread1, &c_original);        // continue the original context in thread1
            m_original.unlock();
        });
        t.detach();

        coro_create(&c_nanogui, exec_nanogui, this, stack.sptr, stack.ssze);
        coro_transfer(&c_original, &c_nanogui);           // continue nanogui loop in the main thread
    }

    void finalize()
    {
        m_nanogui.lock();       // make sure nanogui is done; possibly signal to the app to shut down
        coro_transfer(&c_original, &c_thread1);
    }

    ng::Screen*     app;

    std::mutex      m_original;
    std::mutex      m_nanogui;

    std::thread     t;
    context_t       c_thread1;
    context_t       c_original;
    context_t       c_nanogui;
    coro_stack      stack;

};

PYBIND11_PLUGIN(sample)
{
    py::module mod("sample", "Sample nanogui bindings");

    py::class_<Environment>(mod, "Environment")
        .def(py::init<>())
        .def("run",         &Environment::run)
        .def("finalize",    &Environment::finalize)
    ;

    return mod.ptr();
}
