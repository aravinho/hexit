# HExIt: An Expert Iteration Agent for the Board Game Hex
A Spring/Summer 2018 project by Aravind Sivakumar (aravinho@gmail.com).  Started as a course project for Professor John Canny's course on *Designing, Understanding and Visualizing Deep Neural Networks* at UC Berkeley (with colleagues Yash Kirloskar and Nikhilesh Vegesna), and continued as an individual independent research project.

Inspired by the [Thinking Fast and Slow With Deep Learning and Tree Search](https://papers.nips.cc/paper/7120-thinking-fast-and-slow-with-deep-learning-and-tree-search)  paper from Anthony, Tian and Barber,  (NIPS 2017).


## The Expert Iteration Algorithm
This project uses the Expert Iteration algorithm -- presented by Anthony, et al and similar to DeepMind's [Alpha Go Zero](https://deepmind.com/blog/alphago-zero-learning-scratch/) -- to train an agent tabula rasa to play the board game Hex.  Expert Iteration uses two learners, and "expert" (typically a tree search algorithm) and an "apprentice" (typically a neural network) that iteratively improve each other.

In this implementation (as in Anthony, et al.) the expert is implemented from scratch using Monte Carlo Tree Search (MCTS), and the apprentice is implemented using a policy network (given a state, predicts a distribution over all the legal actions).  The network uses convolutional and fully connected layers, with ReLu activations and batch normalization. 

At each timestep, the algorithm works as follows:

* The apprentice runs several episodes of self-play, and samples on state at random from each of the episodes.

* **Expert Improvement Step**: From each state *s*, the expert runs several *N-MCTS* simulations.  *N-MCTS* (Neural-*MCTS*) is a modification of traditional *MCTS* that is bootstrapped with the apprentice (expert improvement), which helps choosing actions (see Appendix).  At the end of all the simulations from state *s*, *N-MCTS* will have produced an "expert" distribution over the actions available from *s*.

* **Imitation Learning Step**: The apprentice is trained to predict action distributions that mimic those produced by the expert.  Given a state *s*, the network seeks to minimize the cross-entropy between its predicted distribution and that of the expert.

This process repeats.  The newly improved apprentice generates more realistic game states through self-play.  The expert, aided by the improved apprentice, produces better expert distributions for each of these states.  The apprentice improves by learning to mimic these new expert policies.

## The Game Hex


A [Hex](https://en.wikipedia.org/wiki/Hex_(board_game)) board is made up of hexagonal spaces arranged on an *n* by *n* rhombus-shaped board.  Players take turns placing stones on the board.  Player 1 (Red) wins if his pieces form a connected path from the north row of the board to the south row.  Player 2 (Blue) wins if her pieces form a connected path from the west column to the east.  In this image of an 11 x 11 board, Player 2 has won.

<p align="center">
	<img src="http://www.krammer.nl/hex/wp-content/uploads/2014/11/Hex-bord-met-stenen.png" style="width:150px;height:100px;">
</p>

Hex is easily representable using an MDP; there are clear notions of states (a board configuration), actions (where to play the next stone), and rewards (a game of Hex always ends with a winner and a loser).  It is proven that there is an optimal strategy for Player 1, which adds a layer of interest to the problem of creating a Hex-playing agent.

## Tools Used
* The implementation of the MDP environment and the implemention of the *N-MCTS* expert are both built from scratch in C++, using multi-threading for significant speed-up.
* The apprentice neural network is built with Tensorflow, and is trained using Python.
* Inference of the apprentice network is done using Tensorflow's C++ API, to allow for rapid interaction between the expert and apprentice during N-MCTS.
* Bazel is used as the build system.


## Results
My 5 x 5 apprentice has trained to become "as good as it can be".  Whenever the apprentice plays as Player 1, it wins.  It beats random opponents every time, it beats me every time, and it beats a very powerful MCTS agent every time.  I say this is "as good as it can be", because it has, in essence, learned the optimal Player-1 strategy for a 5x5 board.  This apprentice trained on about 1 million data points, generated over the course of 3 Expert Improvement iterations.  This took about 2 days of training in total.

When the apprentice plays as Player 2, it beats a random opponent 98 games out of 100, on average.  However, it loses to the MCTS expert every time when MCTS plays as Player 1, and the apprentice network plays as Player 2.  This is expected, because with enough power (number of simulations and depth), the MCTS agent will play the optimal strategy every time.

I am currently (August 28. 2018) training a 7 x 7 agent.


## Appendix
### N-MCTS (Neural Monte Carlo Tree Search)

N-MCTS is a modification of the traditional [MCTS](https://en.wikipedia.org/wiki/Monte_Carlo_tree_search) algorithm that uses the neural network apprentice to aid in making decisions.  Given a state *s* , N-MCTS builds up a search tree by running several simulations (I use 2000), each of which starts at *s*.  Every node in the tree corresponds to some state (a Hex board configuration), and edges correspond to actions.  Every node keeps statistics on how many times it was visited, how many times it took each action *a*, and for each action, tracks the total reward obtained from simulations in which this node took action *a*. 

Each simulation starts at the "root" node of the tree, the node corresponding to state *s*.  The simulation can be broken into three phases, **selection**, **rollout** and **backpropagation**.

* **Selection Phase**: We start at the root node, which corresponds to state *s*, and need to decide which action to take.  We compute "scores" for each of the actions.  The score *U(s, a)* for action *a* is a function of *n(s)*, the number of visits to node *s*, *n(s, a)*, the number of times we've taken action *a* from state *s*, and *r(s, a)*, the total reward over all simulations in which we took action *a* from state *s*.  The formula (shown below) also contains a term that weights the prediction of the apprentice (which is why it's called *Neural*-MCTS).  We act greedily and take the action that maximizes *U(s,a)* to reach a new node *s'*.  We repeat this process until some pre-determined tree depth, or until we reach a terminal state.

<p align="center">
U(s, a) = r(s,a)/n(s,a)   +   beta * sqrt(log n(s) / n(s,a))   +    gamma * apprentice(s, a).
</p>

(*beta* and *gamma* are hyperparameters (I use 0.05 and 40 respectively).

* **Rollout Phase**: Once we've hit the max depth for the selection phase, we perform "rollout", and randomly take actions until we reach a terminal state.  

* **Backpropagation** (Do not confuse with Gradient Descent Backpropagation): Once we've reached a terminal state, observe the reward; call it *R*.  Propagate back up the tree, along the path for this simulation, and update the statistics for every node.  At every node *v* along this path, increment *n(v)* and *n(v, a*)*, where *a* * is the action we took from state *v* in this simulation.  Add *R* to *r(v, a)*.  Once all the statistics have been updated, start the next simulation from the root node.

After all the simulations are finished, we calculate the number of times each action was chosen from the root.  This is what we defined as our "expert" distribution for state *s*.

The motivation behind MCTS is that, while initially, actions may be chosen at random, over the course of many simulations, nodes will learn which actions have been fruitful, and which should be avoided.  The motivation behind **N**-MCTS is that the prediction of the neural network can guide the expert in useful directions during the early simulations, in which nodes have sparse statistics.

**Note**: The Anthony, et al. paper calls the action scores *UCT-NN(s, a)* since they are derived from * **U**pper **C**onfidence Bounds for **T**rees, and are bootstrapped with a **Neural** **Network**.*

**Note**: Several optimizations can be made to this basic template.  Some that work well include:
* Sampling actions proportional to their *U*-scores, rather than acting greedily.
* Running multiple rollouts in parallel, and averaging the eventual reward for a lower variance estimate of reward.
* Defining a maximum depth after which rollout must begin, in order to avoid taking the same path every simulation.
* Anthony, et al. propose a variant of the *UCT-NN* formula, with an added term called the Rapid Action Value Estimation (RAVE) term.  This uses an all-moves-as-first heuristic to estimate the value of an action during early simulations in which statistics are scarce.  See Anthony, et al. paper for details.


