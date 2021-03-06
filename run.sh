#!/bin/bash

# 8/10, generated 128 files, each of 1024.  Files 623 to 750 in all_data

# python src/run_episodes.py \
#     --game hex \
#     --num_episodes 1 \
#     --hex_dim 7 \
#     --display_state True \
#     \
#     --p1_agent nn \
#     --p1_model_path models/hex/7/v1 \
#     --p1_agent_sample True \
#     \
#     --p2_agent user \
#     \
#     \
#     --random_first_move 0.0 \
#     \
#     --log_every 1024 \



#     --save_path data/nn_states/hex/7/v1 \
#     --states_per_file 128 \

# counter=512
# bazel build src:run_mcts
# while [ $counter -le 1030 ]
# do
# ../bazel-bin/hexit/src/run_mcts \
#     --game hex \
#     --hex_reward_type win_fast \
#     --hex_dim 7 \
#     \
#     --num_states 128 \
#     --input_data_path data/nn_states/hex/7/v1 \
#     --start_at $counter \
#     \
#     --num_simulations 2000 \
#     --max_depth 10 \
#     \
#     --num_threads 4 \
#     --log_every 16 \
#     \
#     --requires_nn false \
#     --use_rave false \
#     \
#     --output_data_path data/mcts/hex/7/v1 \
#     --states_per_file 128 \

# ((counter++))
# done
# echo All done


# python src/train_nn.py \
#     --game hex \
#     --hex_dim 7 \
#     \
#     --training_data_path data/mcts/hex/7/v1 \
#     --start_at 0 \
#     --dataset_size 131968 \
#     --batch_size 128 \
#     --num_epochs 20 \
#     --log_every 256 \
#     --save_every 50 \
#     --learning_rate 0.001 \
#     --from_scratch True \
#     \
#     --model_path models/hex/7/v1 \


python src/run_episodes.py \
    --game hex \
    --num_episodes 65536 \
    --batch_size  128 \
    --hex_dim 7 \
    --display_state False \
    \
    --p1_agent nn \
    --p1_model_path models/hex/7/v1 \
    --p1_agent_sample True \
    \
    --p2_agent nn \
    --p2_model_path models/hex/7/v1 \
    --p2_agent_sample True \
    \
    \
    --random_first_move 0.0 \
    \
    --log_every 128 \
    --save_path data/nn_states/hex/7/v1 \
    --states_per_file 128 \



counter=2055
bazel build src:run_mcts
while [ $counter -le 2566 ]
do
../bazel-bin/hexit/src/run_mcts \
    --game hex \
    --hex_reward_type win_fast \
    --hex_dim 7 \
    \
    --num_states 128 \
    --input_data_path data/nn_states/hex/7/v1 \
    --start_at $counter \
    \
    --num_simulations 3000 \
    --max_depth 15 \
    \
    --num_threads 4 \
    --log_every 256 \
    \
    --requires_nn false \
    --use_rave false \
    \
    --output_data_path data/mcts/hex/7/v1 \
    --states_per_file 128 \

((counter++))
done
echo All done


# bazel build src:run_mcts && ../bazel-bin/hexit/src/run_mcts \
#     --game hex \
#     --hex_reward_type win_fast \
#     --hex_dim 7 \
#     \
#     --num_states 256 \
#     --input_data_path data/nn_states/hex/7/v0/1 \
#     --start_at 0 \
#     \
#     --num_simulations 2000 \
#     --max_depth 10 \
#     \
#     --minibatch_size 64 \
#     --num_threads 4 \
#     --log_every 4 \
#     \
#     --requires_nn true \
#     --model_path models/hex/7/v0 \
#     \
#     --output_data_path data/mcts/hex/7/v0/1 \
#     --states_per_file 1024 \


# Run MCTS using the neural net apprentice
# IMPLEMENT THREAD MANAGER LOG EVERY
# make; ./bin/run-mcts \
#     --game hex \
#     --hex_reward_type win_fast \
#     --hex_dim 5 \
#     \
#     --num_states 4096 \
#     --input_data_path data/nn_states/hex/5/best_model/1 \
#     \
#     --num_simulations 1000 \
#     --max_depth 4 \
#     \
#     --minibatch_size 256 \
#     --num_threads 4 \
#     --log_every 512 \
#     \
#     --use_nn True \
#     --model_spec models/hex/5/best_model/spec.json \
#     --model_path models/hex/5/best_model/ \
#     --nn_script src/nn_query.py \
#     \
#     --output_data_path data/mcts/hex/5/best_model/ \
#     --states_per_file 1024 \




# python src/train_nn.py \
#     --game hex \
#     --hex_dim 5 \
#     \
#     --training_data_path data/mcts/hex/5/all_data/ \
#     --start_at 0 \
#     --dataset_size 906240 \
#     --batch_size 128 \
#     --num_epochs 20 \
#     --log_every 256 \
#     --save_every 50 \
#     --learning_rate 0.001 \
#     --from_scratch False \
#     \
#     --model_path models/hex/5/fresh_model/v1 \


#Train the neural network from scratch for this first time
# python src/train_nn.py \
#     --game hex \
#     --hex_dim 5 \
#     \
#     --training_data_path data/mcts/hex/5/model0/0/ \
#     --start_at 0 \
#     --dataset_size 163840 \
#     --batch_size 128 \
#     --num_epochs 20 \
#     --log_every 64 \
#     --save_every 50 \
#     --learning_rate 0.01 \
#     --from_scratch True \
#     \
#     --model_path models/hex/5/model0/fresh_model/ \


# python src/train_nn.py \
#     --game hex \
#     --hex_dim 5 \
#     \
#     --training_data_path data/mcts/hex/5/model0/1/ \
#     --start_at 0 \
#     --dataset_size 205824 \
#     --batch_size 128 \
#     --num_epochs 20 \
#     --log_every 64 \
#     --save_every 50 \
#     --learning_rate 0.01 \
#     --from_scratch False \
#     \
#     --model_path models/hex/5/model0/fresh_model/ \

# python src/train_nn.py \
#     --game hex \
#     --hex_dim 5 \
#     \
#     --training_data_path data/mcts/hex/5/model0/sim2000_depth10/ \
#     --start_at 0 \
#     --dataset_size 307200 \
#     --batch_size 128 \
#     --num_epochs 20 \
#     --log_every 64 \
#     --save_every 50 \
#     --learning_rate 0.01 \
#     --from_scratch False \
#     \
#     --model_path models/hex/5/model0/fresh_model/ \

# python src/train_nn.py \
#     --game hex \
#     --hex_dim 5 \
#     \
#     --training_data_path data/mcts/hex/5/model0/sim2000_depth4/ \
#     --start_at 0 \
#     --dataset_size 229376 \
#     --batch_size 128 \
#     --num_epochs 20 \
#     --log_every 64 \
#     --save_every 50 \
#     --learning_rate 0.01 \
#     --from_scratch False \
#     \
#     --model_path models/hex/5/model0/fresh_model/ \






# # Play MCTS vs NN
# bazel build src:agents && ../bazel-bin/hexit/src/agents \
#     --game hex \
#     --num_episodes 32 \
#     --batch_size 32 \
#     --hex_dim 5 \
#     \
#     --p2_agent nn \
#     --p2_model_path models/hex/5/fresh_model/v2 \
#     \
#     --p1_agent mcts \
#     --max_depth 10 \
#     --num_simulations 10000 \
#     \
#     --log_every 1 \

# Play MCTS vs NN
# bazel build -c opt src:agents && ../bazel-bin/hexit/src/agents \
#     --game hex \
#     --num_episodes 1 \
#     --batch_size 1 \
#     --hex_dim 7 \
#     --display_state true \
#     \
#     --p1_agent mcts \
#     \
#     --p2_agent mcts \
#     --max_depth 10 \
#     --c_b 0.25 \
#     --num_simulations 5000 \
#     --use_rave false \
#     \
#     --log_every 1 \


# bazel build src:agents && ../bazel-bin/hexit/src/agents \
#     --game hex \
#     --num_episodes 1 \
#     --batch_size 1 \
#     --hex_dim 7 \
#     --display_state true \
#     \
#     --p1_agent user \
#     \
#     --p2_agent user \
#     \
#     --log_every 1 \

# # Play MCTS vs MCTS
# bazel build src:agents && ../bazel-bin/hexit/src/agents \
#     --game hex \
#     --num_episodes 1 \
#     --batch_size 1 \
#     --hex_dim 7 \
#     --display_state true \
#     \
#     --p2_agent user \
#     \
#     --p1_agent mcts \
#     --max_depth 10 \
#     --num_simulations 2000 \
#     \
#     --log_every 1 \





