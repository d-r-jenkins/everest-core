load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def everest_core_repos():
    http_archive(
        name = "rules_foreign_cc",
        sha256 = "476303bd0f1b04cc311fc258f1708a5f6ef82d3091e53fd1977fa20383425a6a",
        strip_prefix = "rules_foreign_cc-0.10.1",
        url = "https://github.com/bazelbuild/rules_foreign_cc/releases/download/0.10.1/rules_foreign_cc-0.10.1.tar.gz",
    )

    maybe(
        http_archive,
        name = "com_github_nelhage_rules_boost",
        url = "https://github.com/nelhage/rules_boost/archive/f02f84fac7673c56bbcfe69dea68044e6e40f92b.tar.gz",
        sha256 = "4f1f7e809960615ba8e40e3a96300acb8b4d7e6e2fb24423af7347383e4dd1bb",
        strip_prefix = "rules_boost-f02f84fac7673c56bbcfe69dea68044e6e40f92b",
    )

    maybe(
        git_repository,
        name = "everest-framework",
        remote = "https://github.com/qwello/everest-framework.git",
        commit = "2306559764824a61406da6641985517e3b61f193",
    )

    maybe(
        git_repository,
        name = "everest-utils",
        remote = "https://github.com/Qwello/everest-utils.git",
        commit = "8d8c1b7172d114a6f4a0fc5f4d6e2d8ae1a0da82",
    )

    maybe(
        http_archive,
        name = "rules_python",
        sha256 = "9acc0944c94adb23fba1c9988b48768b1bacc6583b52a2586895c5b7491e2e31",
        strip_prefix = "rules_python-0.27.0",
        url = "https://github.com/bazelbuild/rules_python/releases/download/0.27.0/rules_python-0.27.0.tar.gz",
    )

    maybe(
        git_repository,
        name = "pugixml",
        remote = "https://github.com/zeux/pugixml.git",
        tag = "v1.12.1",
        build_file_content = """
load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)

cmake(
    name = "libpugixml",
    cache_entries = {
    },
    lib_source = "@pugixml//:all_srcs",
    visibility = ["//visibility:public"],
)
        """
    )

    maybe(
        git_repository,
        name = "libsunspec",
        remote = "https://github.com/EVerest/libsunspec.git",
        tag = "v0.2.0",
        build_file_content = """
cc_library(
    name = "libsunspec",
    srcs = glob(["src/**/*.cpp"]),
    hdrs = glob(["include/**/*.hpp"]),
    deps = [
        "@com_github_everest_liblog//:liblog",
        "@com_github_nlohmann_json//:json",
        "@libmodbus//:libmodbus",
        "@//third-party/bazel:boost_program_options",
    ],
    strip_include_prefix = "include",
)
        """
    )

    maybe(
        git_repository,
        name = "libmodbus",
        remote = "https://github.com/EVerest/libmodbus.git",
        tag = "v0.3.0",
        build_file_content = """
cc_library(
    name = "libmodbus_connection",
    srcs = glob(["lib/connection/src/**/*.cpp"]),
    hdrs = glob(["lib/connection/include/**/*.hpp"]),
    strip_include_prefix = "lib/connection/include",
    deps = [
        "@@com_github_everest_liblog//:liblog",
    ]
)

cc_library(
    name = "libmodbus",
    srcs = glob(["src/**/*.cpp"]),
    hdrs = glob(["include/**/*.hpp"]),
    deps = [
        "@com_github_everest_liblog//:liblog",
        ":libmodbus_connection",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
        """
    )

    maybe(
        git_repository,
        name = "sigslot",
        remote = "https://github.com/palacaze/sigslot",
        tag = "v1.2.0",
        build_file_content = """
load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)

cmake(
    name = "sigslot",
    cache_entries = {
        "SIGSLOT_COMPILE_EXAMPLES": "OFF",
    },
    lib_source = "@sigslot//:all_srcs",
    out_headers_only = True,
    visibility = ["//visibility:public"],
)
        """
    )

    
    maybe(
        git_repository,
        name = "libsunspec",
        remote = "https://github.com/EVerest/libsunspec.git",
        tag = "v0.2.0",
        build_file_content = """
cc_library(
    name = "libsunspec",
    srcs = glob(["src/**/*.cpp"]),
    hdrs = glob(["include/**/*.hpp"]),
    deps = [
        "@com_github_everest_liblog//:liblog",
        "@com_github_nlohmann_json//:json",
        "@libmodbus//:libmodbus",
        "@//third-party/bazel:boost_program_options",
    ],
    strip_include_prefix = "include",
)
        """
    )

    maybe(
        git_repository,
        name = "libtimer",
        remote = "https://github.com/EVerest/libtimer.git",
        tag = "v0.1.1",
        build_file_content = """
cc_library(
    name = "libtimer",
    hdrs = ["include/everest/timer.hpp"],
    deps = [],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
        """
    )
