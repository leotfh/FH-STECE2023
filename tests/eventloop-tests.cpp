#include <door/utilities/eventloop.h>
#include <gtest/gtest.h>
#include <sys/socket.h>


struct TestInputHandler : public InputHandler
{
    TestInputHandler(int fd) : fd(fd), seen(false) {}
    EventAction ready(int fd) override
    {
        seen = true;
        return EventAction::Continue;
    }

    int fd;
    bool seen;
};

struct TestPriorityHandler : public PriorityHandler
{
    TestPriorityHandler(int fd) : fd(fd), seen(false) {}
    EventAction ready(int fd) override
    {
        seen = true;
        return EventAction::Continue;
    }

    int fd;
    bool seen;
};

TEST(eventloop_suite, input)
{
    int pair[2];
    int rv = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
    ASSERT_EQ(rv, 0);

    Eventloop loop;
    TestInputHandler h(pair[0]);
    loop.register_input(pair[0], &h);

    // send regular stream data
    char c = 'x';
    ssize_t nwritten = write(pair[1], &c, 1);
    ASSERT_EQ(nwritten, 1);

    int count = 10;
    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = 1000*1000,
    };
    while (count-- && !h.seen) {
        loop.run_one();
        rv = nanosleep(&interval, nullptr);
        ASSERT_EQ(rv, 0);
    }

    ASSERT_TRUE(h.seen);
}

TEST(eventloop_suite, priority)
{
    int pair[2];
    int rv = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
    ASSERT_EQ(rv, 0);

    Eventloop loop;
    TestPriorityHandler h(pair[0]);
    loop.register_priority(pair[0], &h);

    // send priority data
    char c = 'x';
    ssize_t nwritten = send(pair[1], &c, 1, MSG_OOB);
    ASSERT_EQ(nwritten, 1);

    int count = 10;
    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = 1000*1000,
    };
    while (count-- && !h.seen) {
        loop.run_one();
        rv = nanosleep(&interval, nullptr);
        ASSERT_EQ(rv, 0);
    }

    ASSERT_TRUE(h.seen);
}

TEST(eventloop_suite, input_and_priority_on_same_fd)
{
    int pair[2];
    int rv = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
    ASSERT_EQ(rv, 0);

    Eventloop loop;
    TestInputHandler hi(pair[0]);
    TestPriorityHandler hp(pair[0]);
    loop.register_input(pair[0], &hi);
    loop.register_priority(pair[0], &hp);

    // send regular stream data, as well as priority data
    char c = 'x';
    ssize_t nwritten = send(pair[1], &c, 1, MSG_OOB);
    ASSERT_EQ(nwritten, 1);
    nwritten = write(pair[1], &c, 1);
    ASSERT_EQ(nwritten, 1);

    int count = 10;
    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = 1000*1000,
    };
    while (count-- && !hi.seen && !hp.seen) {
        loop.run_one();
        rv = nanosleep(&interval, nullptr);
        ASSERT_EQ(rv, 0);
    }

    ASSERT_TRUE(hi.seen);
    ASSERT_TRUE(hp.seen);
}

