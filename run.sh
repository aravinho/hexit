#!/bin/bash


python src/run_episodes.py \
    --game hex \
    --num_episodes 307200 \
    --batch_size  128 \
    --hex_dim 5 \
    --display_state False \
    \
    --p1_agent nn \
    --p1_model_path models/hex/5/model0/ \
    --p1_agent_sample True \
    \
    --p2_agent nn \
    --p2_model_path models/hex/5/model0/ \
    --p2_agent_sample True \
    \
    \
    --random_first_move 0.0 \
    \
    --log_every 2048000000000 \
    --save_path data/nn_states/hex/5/model0/2048/ \
    --states_per_file 2048 \

# Run random episodes and save one state from each at random
counter=0
make
while [ $counter -le 149 ]
do
./bin/run-mcts \
    --game hex \
    --hex_reward_type win_fast \
    --hex_dim 5 \
    \
    --num_states 2048 \
    --input_data_path data/nn_states/hex/5/model0/2048/ \
    --start_at $counter \
    \
    --num_simulations 2000 \
    --max_depth 10 \
    --num_threads 4 \
    \
    --log_every 256 \
    --output_data_path data/mcts/hex/5/model0/sim2000_depth10/ \
    --states_per_file 2048
((counter++))
done
echo All done


# Train the neural network from scratch for this first time
# python src/train_nn.py \
#     --game hex \
#     --hex_dim 5 \
#     \
#     --training_data_path data/mcts/hex/5/model0/1/ \
#     --start_at 0 \
#     --dataset_size 16384 \
#     --batch_size 128 \
#     --num_epochs 10 \
#     --log_every 2560000 \
#     --save_every 50 \
#     --learning_rate 0.01 \
#     --from_scratch True \
#     \
#     --model_path models/hex/5/test_models/exp_model \
#     --model_spec models/hex/5/dev_model/spec.json \

# python src/run_episodes.py \
#     --game hex \
#     --num_episodes 1 \
#     --batch_size  128 \
#     --hex_dim 5 \
#     --display_state True \
#     \
#     --p1_agent nn \
#     --p1_model_path models/hex/5/model0/ \
#     --p1_agent_sample True \
#     \
#     --p2_agent user \
#     \
#     \
#     --random_first_move 0.0 \
#     \
#     --log_every 5 \
    # --save_path data/nn_states/hex/5/model0/1/ \
    # --states_per_file 1024 \


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







