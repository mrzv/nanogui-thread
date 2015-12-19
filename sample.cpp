#if defined(WIN32)
#include <windows.h>
#endif

#include <iostream>
#include <thread>
#include <mutex>
#include <csetjmp>

#include <nanogui/screen.h>
namespace ng = nanogui;

#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

struct Environment
{
    void    exec_nanogui()
    {
        ng::init();

        app = new ng::Screen(ng::Vector2i { 800, 600 }, "Test");

        app->drawAll();
        app->setVisible(true);
        ng::mainloop();

        delete app;
        ng::shutdown();
    }

    static void swap_threads(std::jmp_buf& c1, std::mutex& m1, std::jmp_buf& c2, std::mutex& m2, volatile bool& done1, volatile bool& done2)
    {
        std::jmp_buf  c_intermediate;
        std::mutex    m_intermediate;
        m_intermediate.lock();

        done1 = false;

        std::thread t = std::thread([&]()
        {
            if (!setjmp(c_intermediate))
            {
                m_intermediate.unlock();
                while (!done1);
            } else
            {
                m1.unlock();
                m2.lock();
                longjmp(c2, 1);
            }
        });
        t.detach();

        if (!setjmp(c1))
        {
            m_intermediate.lock();
            longjmp(c_intermediate, 1);
        } else
        {
            //m2.unlock();
            done2 = true;
        }
    }

    void run()
    {
        m_original.lock();
        m_thread1.lock();

        t = std::thread([&]()
        {
            swap_threads(c_thread1, m_thread1, c_original, m_original, done2, done1);

            exec_nanogui();         // continue nanogui loop in the main thread

            swap_threads(c_original, m_original, c_thread1, m_thread1, done1, done2);
        });
        t.detach();

        swap_threads(c_original, m_original, c_thread1, m_thread1, done1, done2);
    }

    void finalize()
    {
        swap_threads(c_thread1, m_thread1, c_original, m_original, done2, done1);
    }

    ng::Screen*     app;

    volatile bool   done1;
    volatile bool   done2;

    std::jmp_buf    c_original;
    std::jmp_buf    c_thread1;

    std::mutex      m_original;
    std::mutex      m_thread1;

    std::thread     t;
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
