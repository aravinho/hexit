load(
    "//tensorflow:tensorflow.bzl",
    "tf_cc_binary",
)

tf_cc_binary(
    name = "loader",
    srcs = ["loader.cc"],
    deps = [
        "//tensorflow/core:tensorflow",
    ],
)


cc_library(
	name = "config",
	srcs = [
		"config.h",
		"config.cc"
		],
)

cc_library(
	name = "utils",
	srcs = [
		"utils.h",
		"utils.cc"
		],
)

cc_library(
	name = "env_state",
	srcs = [
		"env_state.h",
		"env_state.cc",
		"hex_state.h"
		],
	deps = [
		":config",
		":utils"
	]

)

cc_library(
	name = "hex_state",
	srcs = [
		"hex_state.h",
		"hex_state.cc",
		],
	deps = [
		":env_state",
		":utils"
	]
)

cc_library(
	name = "mcts",
	srcs = [
		"mcts.h",
		"mcts.cc"
		],
	deps = [
		":config",
		":env_state",
		":hex_state",
		":utils"
	]
)

cc_binary(
	name = "run_mcts",
	srcs = [
		"main.h",
		"main.cc"
		],
	deps = [
		":config",
		":mcts",
		":utils"
	]
)



