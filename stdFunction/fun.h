//
// Created by Yaroslav on 19/01/2019.
//

#ifndef STDFUNCTION_FUN_H
#define STDFUNCTION_FUN_H


#include <memory>
#include <cstring>

template<typename>
class fun;

template<typename R, typename ... Arg>
class fun<R(Arg ...)> {

    class concept {
    public:

        virtual ~concept() = default;

        virtual R call(Arg ... args) = 0;

        virtual std::unique_ptr<concept> copy() = 0;

        virtual void small_copy(void *buffer) = 0;

        virtual void small_move(void *buffer) = 0;
    };


    template<typename T>
    class model : public concept {
    public:
        model(T const &function) : t(function) {}

        R call(Arg ... args) {
            return t(std::forward<Arg>(args) ...);
        }

        std::unique_ptr<concept> copy() {
            return std::make_unique<model<T>>(t);
        }

        void small_copy(void *buffer) {
            new(buffer)model<T>(t);
        }

        void small_move(void *buffer) {
            new(buffer) model<T>(std::move(t));
        }

    private:
        T t;
    };


    concept* my_cast(const int8_t *a) {
        reinterpret_cast<concept *>(const_cast<int8_t *>(a));
    }



public:
    fun() noexcept : is_big(true), my_ptr(nullptr) {} //OK

    fun(std::nullptr_t) noexcept : is_big(true), my_ptr(nullptr) {} //Ok

    fun(fun const &other) : is_big(false) {
        if (other.is_big) {
            is_big = true;
            my_ptr = other.my_ptr->copy();
        } else {
            concept *p = reinterpret_cast<concept *>(const_cast<int8_t *>(other.buffer));
            p->small_copy(buffer);
        }
    }

    fun(fun &&other) : is_big(true), my_ptr(nullptr) { // copy constructor
        swap(other);
        other.~fun();
    }

    template<typename T>
    fun(T function) : is_big(false) {
        if (sizeof(T) <= 64) {
            new(buffer) model<T>(std::move(function));
        } else {
            is_big = true;
            my_ptr = std::make_unique<model<T>>(std::move(function));
        }
    }

    ~fun() { //destructor
        if (is_big) {
            my_ptr.~unique_ptr();
        } else {
            concept* p = reinterpret_cast<concept *>(const_cast<int8_t *>(buffer));
            //concept *p = my_cast(buffer);
            p->~concept();
        }
    }

    fun& operator=(fun const &other) {
        fun tmp(other);
        swap(tmp);
        return *this;
    }

    fun& operator=(fun&& other) noexcept {
        swap(other);
        return *this;
    }

    void swap(fun &other) noexcept {
        std::swap(is_big, other.is_big);
        std::swap(buffer, other.buffer);
    }

    explicit operator bool() const noexcept {  // bool
        return !is_big || static_cast<bool>(my_ptr);
    }


    R operator()(Arg ... args) const { // function
        if (is_big) {
            return my_ptr->call(std::forward<Arg>(args)...);
        } else {
            concept* p = reinterpret_cast<concept *>(const_cast<int8_t *>(buffer));
            return p->call(std::forward<Arg>(args)...);
        }
    }




private:
    bool is_big;
    union {
        std::unique_ptr<concept> my_ptr;
        int8_t buffer[64];
    };
};



#endif //STDFUNCTION_FUN_H
