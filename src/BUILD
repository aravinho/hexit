


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
	deps = [
		":config",
	]
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




load(
    "//tensorflow:tensorflow.bzl",
    "tf_cc_binary",
)

cc_library(
    name = "inference",
    srcs = ["inference.cc", "inference.h"],
    deps = [
        "//tensorflow/core:tensorflow",
        ":hex_state"
    ],
)

tf_cc_binary(
    name = "agents",
    srcs = ["agents.cc", "agents.h"],
    deps = [
        "//tensorflow/core:tensorflow",
        ":hex_state",
        ":mcts",
        ":inference"
    ],
)

cc_library(
	name = "mcts_thread_manager",
	srcs = [
		"mcts_thread_manager.cc",
		"mcts_thread_manager.h"
	],
	deps = [
		":mcts"
	]

)

tf_cc_binary(
	name = "run_mcts",
	srcs = [
		"main.h",
		"main.cc"
		],
	deps = [
		":config",
		":mcts",
		":utils",
		":inference",
		":mcts_thread_manager"
	]
)






