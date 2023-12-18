GO_LIBRARY()

SRCS(
    assert.go
    channel_with_cancellation.go
    channel_with_inflight_queue.go
    cond.go
    inflight_queue.go
    marshal.go
    progress_saver.go
    util.go
)

GO_TEST_SRCS(
    inflight_queue_test.go
)

END()

RECURSE(
    protos
)

RECURSE_FOR_TESTS(
    tests
)
