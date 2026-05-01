load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "markus",
    hdrs = [
        "markus.h",
    ],
)

cc_binary(
    name = "main",
    srcs = [
        "main.cc",
    ],
    deps = [
        ":markus",
    ],
)
