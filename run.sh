# Run random episodes and save one state from each at random
python src/run_episodes.py \
    --game hex \
    --num_episodes 4096 \
    --hex_dim 5 \
    \
    --p1_agent random \
    \
    --p2_agent random \
    \
    --random_first_move 0.0 \
    \
    --log_every 512 \
    --save_path data/nn_states/hex/5/best_model/0/ \
    --states_per_file 1024


# Run MCTS on these states, without using a neural net apprentice
make
./run-mcts \
 	--game hex \
 	--hex_dim 5 \
 	\
 	--num_states 4096 \
 	--input_data_path data/nn_states/hex/5/best_model/0/ \
 	\
 	--num_simulations 1000 \
 	--max_depth 4 \
 	\
 	--minibatch_size 256 \
 	--num_threads 4 \
 	--log_every 512 \
    \
    --output_data_path data/mcts/hex/5/best_model/ \
    --states_per_file 1024 \


# Train the neural network from scratch for this first time
python src/train_nn.py \
	--game hex \
	--hex_dim 5 \
	\
	--training_data_path data/mcts/hex/5/best_model/ \
	--batch_size 256 \
	--num_epochs 500 \
	--log_every 50 \
	--save_every 100 \
	--learning_rate 0.01 \
	--from_scratch True \
	\
	--model_path models/hex/5/best_model/ \
	--model_spec models/hex/5/best_model/spec.json \


# Run episodes using the neural net agent and save one state from each at random
# NEED TO IMPLEMENT RANDOM_FIRST_MOVE
python src/run_episodes.py \
    --game hex \
    --num_episodes 4096 \
    --hex_dim 5 \
    \
    --p1_agent nn \
    --p1_model_path models/hex/5/best_model/ \
    --p1_model_spec models/hex/5/best_model/spec.json \
    --p1_agent_sample True \
    \
    --p2_agent nn \
    --p2_model_path models/hex/5/best_model/ \
    --p2_model_spec models/hex/5/best_model/spec.json \
    --p2_agent_sample True \
    \
    --random_first_move 0.25 \
    \
    --log_every 512 \
    --save_path data/nn_states/hex/5/best_model/1 \
    --states_per_file 1024




# Run MCTS using the neural net apprentice
make
./run-mcts \
 	--game hex \
 	--hex_dim 5 \
 	\
 	--num_states 4096 \
 	--input_data_path data/nn_states/hex/5/best_model/1 \
 	\
 	--num_simulations 1000 \
 	--max_depth 4 \
 	\
 	--minibatch_size 256 \
 	--num_threads 4 \
 	--log_every 512 \
    \
    --use_nn True \
    --model_spec models/hex/5/best_model/spec.json \
    --model_path models/hex/5/best_model/ \
    --nn_script src/nn_query.py \
    \
    --output_data_path data/mcts/hex/5/best_model/ \
    --states_per_file 1024 \


# Train the neural network, resuming from the save model
python src/train_nn.py \
	--game hex \
	--hex_dim 5 \
	\
	--training_data_path data/mcts/hex/5/best_model/ \
	--batch_size 256 \
	--num_epochs 500 \
	--log_every 50 \
	--save_every 100 \
	--learning_rate 0.01 \
	\
	--model_path models/hex/5/best_model/ \
	--model_spec models/hex/5/best_model/spec.json \




# Repeat the last 3 steps continuously

