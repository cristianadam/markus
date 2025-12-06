cc_library(
    name = "hyde",
    hdrs = [
        "hyde.h",
    ],
)

cc_test(
    name = "hyde_test",
    srcs = [
        "hyde_test.cc",
    ],
    deps = [
        ":hyde",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "main",
    srcs = [
        "main.cc",
    ],
    deps = [
        ":hyde",
    ],
)
