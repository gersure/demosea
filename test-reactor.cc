/*
 * test-reactor.cc
 *
 *  Created on: Aug 2, 2014
 *      Author: avi
 */

#include "reactor.hh"
#include <iostream>
#include <thread>
#include <chrono>

struct test {
    reactor r;
    std::unique_ptr<pollable_fd> listener;
    struct connection {
        connection(reactor& r, std::unique_ptr<pollable_fd> fd) : r(r), fd(std::move(fd)) {}
        reactor& r;
        std::unique_ptr<pollable_fd> fd;
        char buffer[8192];
        void copy_data() {
            r.read_some(*fd, buffer, sizeof(buffer)).then([this] (future<size_t> fut) {
                    auto n = fut.get();
                    std::cout << "got data: " << n << "\n";
                    });
        }
    };
    void new_connection(accept_result&& accepted) {
        std::cout << "got connection\n";
        auto c = new connection(r, std::move(std::get<0>(accepted)));
        c->copy_data();
    }
    void start_accept() {
        r.accept(*listener).then([this] (future<accept_result> fut) {
            std::cout << "accept future returned\n";
            new_connection(fut.get());
            start_accept();
        });
    }
};

int main(int ac, char** av)
{
    test t;
    ipv4_addr addr{{}, 10000};
    listen_options lo;
    lo.reuse_address = true;
    t.listener = t.r.listen(make_ipv4_address(addr), lo);
    t.start_accept();
    t.r.run();
    return 0;
}

