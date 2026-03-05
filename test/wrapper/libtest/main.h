#ifndef CAPI_MAIN_H
#define CAPI_MAIN_H

struct A {
    virtual const char* greeting() {
        return "Hello, this is the base class!";
    }
};

struct B : public A {
    const char* greeting() override {
        return "Hello, this is the sub class!";
    }
};
#endif //CAPI_MAIN_H
