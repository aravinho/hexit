from utils import *
from config import *

import numpy as np 
import scipy as sp 
import tensorflow as tf 
import os

from sklearn.metrics import accuracy_score
import datetime, time
import random
from sklearn.model_selection import train_test_split



"""
A class for a Neural Network apprentice for the Expert Iteration Algorithm.
Implements common functionality like train/predict, save/restore
Inherited by subclasses (HexNN, etc) which implement the specific model parameters specific to a certain MDP/NN architecture.
"""
class ExitNN(object):


    def saveCheckpoint(self, sess, model_ckpt_dir):
        """
        Saves the session to the given directory, creating the directory if it doesn't already exist.
        """
        saver = tf.train.Saver()
        if not os.path.exists(model_ckpt_dir):
            os.makedirs(model_ckpt_dir)
        save_path = saver.save(sess, model_ckpt_dir)

        writeLog("Model saved to " + model_ckpt_dir)



    def restoreCheckpoint(self, sess, model_ckpt_dir):
        """
        Restores the meta-graph and model from from the given directory.
        The directory must exist.
        """
        assert os.path.exists(model_ckpt_dir), "Cannot restore model from " + model_ckpt_dir
        saver = tf.train.import_meta_graph(model_ckpt_dir + '.meta')
        saver.restore(sess, model_ckpt_dir)
        
        writeLog("Model restored from " + model_ckpt_dir, should_log=False)


    def loadData(self, base_dir, begin_from=DEFAULT_BEGIN_FROM, max_rows=DEFAULT_MAX_ROWS):
        """
        Reads in data from CSV files.
        The "states" (X) are stored in BASE_DIR/states/*.csv, where * is a file number (0 indexed).
        The "action distributions" (Y) are stored in BASE_DIR/action_distributions/*.csv, where files are indexed as above.
        These directories must exist.
        Reads at most MAX_ROWS points.
        Begins reading from the file numbered BEGIN_FROM.
        Reads the data into a Pandas Dataframe, and then converts it to Numpy arrays
        Returns the tuple of Numpy arrays (x, y), where "x" corresponds to states, and "y" to values.
        """

        writeLog("Loading training data from " + base_dir)

        # Grab the X and Y subdirectories and make sure they exist
        if base_dir[-1] != '/':
            base_dir += '/'
        data_dir = base_dir + 'states/'
        labels_dir = base_dir + 'action_distributions/'

        assert os.path.exists(base_dir), "Cannot load data from directory does not exist " + data_dir
        assert os.path.exists(data_dir), "Cannot load training data from directory " + data_dir
        assert os.path.exists(labels_dir), "Cannot load training labels from directory " + labels_dir


        # Read in both states and action distributions as Pandas DataFrames
        data_df = readCSVFiles(data_dir, begin_from=begin_from, max_rows=max_rows)
        labels_df = readCSVFiles(labels_dir, begin_from=begin_from, max_rows=max_rows)
        
        # Convert these to Numpy arrays
        x = data_df.values.astype(np.float32)
        y = labels_df.values.astype(np.float32)
        
        assert x.shape[0] == y.shape[0], "Must be same number of training points and labels"
        num_points = x.shape[0]
        writeLog("Loaded " + str(num_points) + " states and " + str(num_points) + " action_distributions")
        return (x, y)
        
        # train_x, train_y, val_x, val_y = self.randomSplit(train_x, train_y, split)

        # Split into Train and Val sets
        num_datapoints = data_x.shape[0]
        train_x, val_x, train_y, val_y = train_test_split(data_x, data_y, test_size=(1 - split)) # sklearn function


        writeLog("Loaded " + str(num_train_datapoints) + " train points and " + str(num_val_datapoints) + " validation points")

        return (train_x, train_y, val_x, val_y)



    def loadPredictData(self, data_file, max_rows=DEFAULT_MAX_ROWS):
        """
        Reads in data from a single CSV file, first to a Pandas DataFrame.
        Convert the DataFrame to a numpy array and returns it.
        The file must exist.
        Reads at most max_rows rows.
        """
        assert len(data_file) > 4 and data_file[-4:] == ".csv", data_file + " is not a valid CSV file"
        assert os.path.exists(data_file), "Cannot load prediction data from " + data_file

        data_df = pd.read_csv(data_file, header=None, nrows=max_rows)
        pred_x = data_df.values.astype(np.float32)
        return pred_x







    def predictBatch(self, pred_data_file, model_ckpt_dir, outfile):
        """
        Runs a forward pass through the NN for all of the points in PRED_DATA_FIlE.
        Restores the model from the given MODEL_CKPT_DIR.
        Finally, writes the matrix of predicted labels to the given OUTFILE.
        """

        # load data
        pred_x = self.loadPredictData(pred_data_file)
        # might need to reshape here
        pred_x = pred_x.reshape(-1, self.input_num_units)
       
        # Restore the session and grab the appropriate nodes
        with tf.Session() as sess:
            self.restoreCheckpoint(sess, model_ckpt_dir)
            nodes = self.getFromCollection()
            output = nodes['output']

            # make the forward pass
            feed_dict = self.createFeedDict(nodes, x=pred_x)
            output_vals = output.eval(feed_dict)

            # write the softmax outputs to outfile
            self.writeCSV(output_vals, outfile)





    def predict(self, states, sess, model_ckpt_dir, nodes=None, sample=DEFAULT_SAMPLE_PREDICTION):
        """
        Runs a forward pass through the NN for a single point or a batch (given as a state-vector representation).
        Restores the model from the given MODEL_CKPT_DIR, or just simply grabs the output node from the given NODES array.
        If the SAMPLE flag is set, samples a single index proportional to the output of the NN.
        Otherwise, takes the argmax.
        This function is used to predict an action given an Environment state.
        """

        # convert the single D-Dimenional numpy array, convert it into a singleton numpy array of dimension D + 1
        if len(states.shape) == 1:
            pred_x = np.array([states])
        else:
            pred_x = states
       
        # Restore the session and grab the appropriate nodes
        if nodes is None:
            self.restoreCheckpoint(sess, model_ckpt_dir)
            nodes = self.getFromCollection()
        output = nodes['output']

        # make the forward pass
        feed_dict = self.createFeedDict(nodes, x=pred_x)
        action_scores = output.eval(feed_dict)

        if sample:
            actions = sampleProportionalToScores(action_scores) # might need to change base of softmax
        else:
            actions = np.argmax(action_scores, axis=1)

        # If single state, unpack into int
        if len(states.shape) == 1:
            return int(actions[0])

        return actions.astype(int)



    def createFeedDict(self, x, y=None):
        """
        This is the default implementation, which subclasses override.
        The purpose of this function is to reshape any inputs/labels to the model, as necessary.
        For example, the HexNN's implementation of this method reshapes the state vector into channels, and separates out the 2-bit turn mask.
        This function returns a feed_dict, which is meant to be fed directly into any call to sess.run() or eval().
        """
        return {"x": x, "y": y}


    def addToCollection(self, nodes):
        """
        NODES is a dictionary of name to node mappings.
        Add all of these mappings to the "collection", which will be saved to file (and can later be used to restore the model)
        """
        for node_name in nodes:
            tf.add_to_collection(node_name, nodes[node_name])


    






    def train(self, data_dir, model_ckpt_dir, new_model_ckpt_dir, from_scratch=DEFAULT_FROM_SCRATCH, dataset_size=DEFAULT_DATASET_SIZE,
        split=DEFAULT_TRAIN_VAL_SPLIT, begin_from=DEFAULT_BEGIN_FROM,
        num_epochs=DEFAULT_NUM_EPOCHS, batch_size=DEFAULT_BATCH_SIZE,
        learning_rate=DEFAULT_LEARNING_RATE, log_every=DEFAULT_LOG_EVERY_BATCH, save_every=DEFAULT_SAVE_EVERY):
        """
        Trains this NN using data from the given DATA_DIR.
        It is expected that the state vectors are stored in numbered CSV files (0.csv, 1.csv) in DATA_DIR/states/.
        It is expected that the action distributions (labels) are in matching numbered CSV files in DATA_DIR/action_distributions/.

        If the FROM_SCRATCH flag is true, this starts training a new model from scratch.
        It also prepares the necessary nodes that must persist (using tf.add_to_collection).
        Otherwise, it begins by restoring a model and necessary nodes from MODEL_CKPT_DIR, and then resumes training.

        Splits the data into train and validation sets; the fraction of train points is given by the SPLIT argument.

        It trains for the given number of epochs, with minibatches of the given size, using the given learning rate.
        It logs every LOG_EVERY minibatches, and saves a model checkpoint every SAVE_EVERY epochs.

        It then runs forward pass predictions on the training and validation set, and reports training and validation accuracies.
        The definition of accuracy is specified by inheriting subclasses.

        Returns a tuple (train_accuracy, validation_accuracy)

        """

        writeLog("Training model " + str(new_model_ckpt_dir) + " on data from " + str(data_dir))
        print "learning rate:", learning_rate
        if data_dir[-1] != '/':
            data_dir += '/'
        # if model_ckpt_dir[-1] != '/':
        #     model_ckpt_dir += '/'

        with tf.Session() as sess:

            # If training from scratch, build the graph
            # define the optimizer nodes, and any other important nodes that need to persist (x, y, output at least)
            # Add the important nodes to the "collection" so they can persist across saves/restores
            if from_scratch:
                # build computation graph, which returns a dictionary of node names to nodes
                nodes = self.buildGraph()

                # grab important nodes
                cost = nodes["cost"]
                x, y, output = nodes["x"], nodes["y"], nodes["output"]
                turn_mask = nodes["turn_mask"]
                cost_vec, fc_output = nodes['cost_vec'], nodes['fc_output']
                
                # Define the optimizer node and run the initializer
                optimizer = tf.train.AdamOptimizer(learning_rate=learning_rate).minimize(cost)
                nodes['optimizer'] = optimizer
                init = tf.global_variables_initializer()
                sess.run(init)

                # Add important nodes to the collection
                self.addToCollection(nodes)

            # If restoring model from a checkpoint directory, restore the model, and the important nodes
            else:
                self.restoreCheckpoint(sess, model_ckpt_dir)
                nodes = self.getFromCollection()
                optimizer = nodes['optimizer']
                cost = nodes['cost']
                output = nodes['output']
                x = nodes['x']
                y = nodes['y']

        

            # load the training data and labels
            x, y = self.loadData(data_dir, begin_from=begin_from, max_rows=dataset_size)    
            

            # Run training epochs
            train_x, val_x, train_y, val_y = train_test_split(x, y, test_size=(1 - split))
            for epoch in range(num_epochs):
                
                num_train_datapoints, num_val_datapoints = train_x.shape[0], val_x.shape[0]
                assert batch_size != 0, "Batch size cannot be 0"
                num_batches = int(num_train_datapoints / batch_size)
                
                avg_cost = 0
                
                for batch_num in range(num_batches):
                    # Create a minibatchF
                    batch_x, batch_y = createBatch(train_x, train_y, batch_size, num_train_datapoints)
                    # run the optimizer
                    feed_dict = self.createFeedDict(nodes, x=batch_x, y=batch_y)
                    _, c = sess.run([optimizer, cost], feed_dict)

                    # Update the average cost
                    avg_cost += c / num_batches
               
                    # log if necessary
                    if (batch_num % log_every == 0 and batch_num != 0):
                        writeLog("Completed " + str(batch_num) + " out of " + str(num_batches) + " batches in epoch " + str(epoch))

                # log after every epoch
                writeLog("Epoch " + str(epoch) + " cost = " + "{:.5f}".format(avg_cost))

                # save a checkpoint if necessary
                if (epoch % save_every == 0 and epoch > 0):
                    self.saveCheckpoint(sess, new_model_ckpt_dir + "_" + str(epoch) + '/')

                # Evaluate training and validation accuracy
                train_accuracy, l1_dist_train = self.accuracy(nodes, x=train_x, y = train_y, num_points=int(0.25 * num_train_datapoints))
                val_accuracy, l1_dist_val = self.accuracy(nodes, x=val_x, y=val_y, num_points=int(1 * num_val_datapoints))

                writeLog("Train Accuracy: " + str(train_accuracy))
                writeLog("Validation Accuracy: " + str(val_accuracy))
                writeLog("Mean L1 Distance (Train): " + str(l1_dist_train))
                writeLog("Mean L1 Distance (Val): " + str(l1_dist_val))
 

            # Save the final model
            self.saveCheckpoint(sess, new_model_ckpt_dir)            
            writeLog("\nTraining complete!")



            





