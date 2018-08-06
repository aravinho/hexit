# HExIt: An Expert Iteration Agent for the Board Game Hex
A Spring/Summer 2018 project by Aravind Sivakumar (aravinho@gmail.com), UC Berkeley.  

Inspired by the [Thinking Fast and Slow With Deep Learning and Tree Search](https://papers.nips.cc/paper/7120-thinking-fast-and-slow-with-deep-learning-and-tree-search)  paper from Anthony, Tian and Barber,  (NIPS 2017).


## The Expert Iteration Algorithm
This project uses the Expert Iteration algorithm -- presented by Anthony, et al. and inspired by DeepMind's AlphaGo Zero paper -- to train an agent tabula rasa to play the board game Hex.  Expert Iteration uses two learners, and "expert" (typically a tree search algorithm) and an "apprentice" (typically a neural network) that iteratively improve each other.

In this implementation (as in Anthony, et al.) the expert is implemented using Monte Carlo Tree Search (MCTS), and the apprentice is implemented using a policy network (given a state, predicts a distribution over all the legal actions).  The network uses convolutional and fully connected layers, with ReLu activations and batch normalization. 

At each timestep, the algorithm works as follows:

* The apprentice runs several episodes of self-play, and samples on state at random from each of the episodes.

* **Expert Improvement Step**: From each state *s*, the expert runs several *N-MCTS* simulations.  *N-MCTS* (Neural-*MCTS*) is a modification of traditional *MCTS* that is bootstrapped with the apprentice (expert improvement), which helps choosing actions (see Appendix).  At the end of all the simulations from state *s*, *N-MCTS* will have produced an "expert" distribution over the actions available from *s*.

* **Imitation Learning Step**: The apprentice is trained to predict action distributions that mimic those produced by the expert.  Given a state *s*, the network seeks to minimize the cross-entropy between its predicted distribution and that of the expert.

This process repeats.  The apprentice generates states through self-play.  The expert, aided by an improved apprentice, produces expert distributions for each of these states.  The apprentice improves by learning to mimic these expert policies.

## The Game Hex


A [Hex](https://en.wikipedia.org/wiki/Hex_(board_game)) board is made up of hexagonal spaces arranged on an *n* by *n* hexagonal board.  Players take turns placing stones on the board.  Player 1 (Red) wins if her pieces form a connected path from the north row of the board to the south row.  Player 2 (Blue) wins if his pieces form a connected path from the west column to the east.  In this image, Player 2 has won.

<p align="center">
	<img src="http://www.krammer.nl/hex/wp-content/uploads/2014/11/Hex-bord-met-stenen.png" style="width:300px;height:200px;">
</p>

Hex is easily representable using an MDP; there are a clear notion of states (a board configuration), actions (where to play the next stone), and reward (a game of Hex always ends with a winner and a loser).  It is proven that there is an optimal strategy for Player 1, which adds a layer of interest to the problem of creating a Hex-playing agent.

## Tools Used
* The implementation of the MDP environment, and the implemention of MCTS expert are both built from scratch in C++, using multi-threading for significant speed-up.
* The apprentice neural network is built with Tensorflow, and is trained using Python.
* Inference of the apprentice network is done using Tensorflow's C++ API, to allow for rapid interaction between the expert and apprentice during N-MCTS.
* Bazel is used as the build system.


## Results
This is a work in progress.  Currently (August 6, 2018) , on a 5x5 Hex board:
* A pure MCTS agent beats a random opponent 100 games out of 100.
* The apprentice policy network agent (trained on 100,000 states) beats a random opponent 93 games out of 100, on average.
* The policy network agent beats me (an average player) about 30% of the time.

The network is currently being trained, and will hopefully improve considerably in the next week.


## Appendix
### N-MCTS (Neural Monte Carlo Tree Search)

N-MCTS is a modification of the traditional [MCTS](https://en.wikipedia.org/wiki/Monte_Carlo_tree_search) algorithm that uses the neural network apprentice to aid in making decisions.  Given a state *s* , N-MCTS builds up a search tree by running several simulations (I use 2000), each of which starts at *s*.  Every node in the tree corresponds to some state (a Hex board configuration), and edges correspond to actions.  Every node keeps statistics on how many times it was visited, how many times it took each action, and for each action, tracks the total reward obtained from simulations in which this node took that action. 

Each simulation starts at the "root" node of the tree, the node corresponding to state *s*.  The simulation can be broken into three phases, **selection**, **rollout** and **backpropagation**.

* **Selection Phase**: We start at the root node, which corresponds to state *s*, and need to decide which action to take.  We compute "scores" for each of the actions.  The score *U(s, a)* for action *a* is a function of the number of visited *s* -- *n(s)* --, the number of times we've taken action *a* from state *s* -- *n(s, a)* -- , and the total reward over all simulations in which we took action *a* from state *s* -- *r(s, a)*.  The formula (below) also contains a term that weights the prediction of the apprentice (which is why it's called *Neural*-MCTS).  We act greedily and take the action that maximizes *U(s,a)* to reach a new node *s'*.  We repeat this process until some pre-determined tree depth, or until we reach a terminal state.

<p align="center">
U(s, a) = r(s,a)/n(s,a)   +   beta * sqrt(log n(s) / n(s,a))   +    gamma * apprentice(s, a).
</p>

(*beta* and *gamma* are hyperparameters (I use 0.05 and 40 respectively).

* **Rollout Phase**: Once we've hit the max depth for the selection phase, we perform "rollout", and randomly take actions until we reach a terminal state.  

* **Backpropagation** (Do not confuse with Gradient Descent Backpropagation): Once we've reached a terminal state, observe the reward; call it *R*.  Traverse back up the tree along the path for this simulation, and update the statistics for every node.  At every node *v* along this path, increment *n(v)* and *n(v, a*)*, where *a* * is the action we took from state *v* in this simulation.  Increment *r(v, a)* by *R*.  Once all the statistics have been updated, start the next simulation from the root node.

After all the simulations are finished, calculate the number of times each action was chosen from the root.  This is what we defined as our "expert" distribution for state *s*.

The idea behind MCTS is that, while initially, actions may be chosen at random, over the course of many simulations, nodes will learn which actions have been fruitful, and which should be avoided.  The motivation behind **N**-MCTS is that the prediction of the neural 

**Note**: The Anthony, et al. paper calls the action scores *UCT-NN(s, a)* since they are derived from * **U**pper **C**onfidence Bounds for **T**rees, and are bootstrapped with a **Neural** **Network**.*

**Note**: Several optimizations can be made to this basic template.  Some that work well include:
* Sampling actions proportional to their *U*-scores, rather than acting greedily.
* Running multiple rollouts in parallel, and averaging the eventual reward for a lower variance estimate of reward.
* Defining a maximum depth after which rollout must begin, in order to avoid taking the same path every simulation.


