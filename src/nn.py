import numpy as np 
import scipy as sp 
import tensorflow as tf 
import os

import pandas as pd
import glob
from sklearn.metrics import accuracy_score
import datetime, time
import random



def convert_dist_to_argmax(dist_labels, num_classes=9):
    """Convert class labels from scalars to one-hot vectors"""
    argmax_labels = np.zeros_like(dist_labels)
    argmax_labels[np.arange(len(dist_labels)), dist_labels.argmax(1)] = 1
    return argmax_labels

class ExitNN:

    def __init__(self, print_every=100, save_every=1000):
        self.print_every = print_every
        self.save_every = save_every

        self.input_num_units = 9
        self.num_hidden_layers = 1
        self.hidden_num_units = 200
        self.output_num_units = 9
        self.weights = {}
        self.biases = {}



    def saveCheckpoint(self, sess, model_ckpt_dir):
        saver = tf.train.Saver()
        if not os.path.exists(model_ckpt_dir):
            os.makedirs(model_ckpt_dir)
        save_path = saver.save(sess, model_ckpt_dir)

        ts = time.time()
        ts = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')



    def restoreCheckpoint(self, sess, model_ckpt_dir):
        if not os.path.exists(model_ckpt_dir):
            raise IOError("Cannot restore model from " + model_ckpt_dir)
        saver = tf.train.import_meta_graph(model_ckpt_dir + '.meta')
        saver.restore(sess, model_ckpt_dir)
        
        ts = time.time()
        ts = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')


    def loadTrainData(self, base_dir, split=0.75, use_argmax=True):
        
        if base_dir[-1] != '/':
            base_dir += '/'
        data_dir = base_dir + 'data/'
        labels_dir = base_dir + 'labels/'

        assert os.path.exists(base_dir), "Cannot load data from directory does not exist " + data_dir
        assert os.path.exists(data_dir), "Cannot load training data from directory " + data_dir
        assert os.path.exists(labels_dir), "Cannot load training labels from directory " + labels_dir

        data_df = pd.concat([pd.read_csv(f, header=None) for f in glob.glob(data_dir + '*.csv')])
        labels_df = pd.concat([pd.read_csv(f, header=None) for f in glob.glob(labels_dir + '*.csv')])

        data_x = data_df.values.astype(np.float32)
        data_y = labels_df.values.astype(np.float32)
        if use_argmax:
            data_y = convert_dist_to_argmax(data_y)

        num_datapoints = data_x.shape[0]

        split_size = int(num_datapoints * split)

        train_x, val_x = data_x[:split_size], data_x[split_size:]
        train_y, val_y = data_y[:split_size], data_y[split_size:]

        return (train_x, train_y, val_x, val_y)

    def loadPredictData(self, data_file):
        assert os.path.exists(data_file), "Cannot load prediction data from " + data_file
        data_df = pd.read_csv(data_file, header=None)
        pred_x = data_df.values.astype(np.float32)
        return pred_x

    def writePredOutputToFile(self, output, outfile):
        np.savetxt(outfile, output, delimiter=',', fmt='%0.4f')



    def createBatch(self, data, labels, batch_size, dataset_length):
        """Create batch with random samples and return appropriate format"""
        batch_mask = np.random.choice(dataset_length, batch_size) # truly random every time

        input_num_units, output_num_units = data.shape[1], labels.shape[1]
        
        batch_x = data[[batch_mask]].reshape(-1, input_num_units)
        batch_y = labels[[batch_mask]].reshape(-1, output_num_units)
            
        return batch_x, batch_y


    def buildGraph(self):

        # define placeholders
        self.x = tf.placeholder(tf.float32, [None, self.input_num_units])
        self.y = tf.placeholder(tf.float32, [None, self.output_num_units])

        ### define weights and biases of the neural network
        
        # do we need seed?
        seed = 128

        self.weights = {
            'hidden': tf.Variable(tf.random_normal([self.input_num_units, self.hidden_num_units], seed=seed)),
            'output': tf.Variable(tf.random_normal([self.hidden_num_units, self.output_num_units], seed=seed))
        }

        self.biases = {
            'hidden': tf.Variable(tf.random_normal([self.hidden_num_units], seed=seed)),
            'output': tf.Variable(tf.random_normal([self.output_num_units], seed=seed))
        }

        print "weights", self.weights['hidden'].shape, self.weights['output'].shape
        print "biases", self.biases['hidden'].shape, self.biases['output'].shape

        hidden_layer = tf.add(tf.matmul(self.x, self.weights['hidden']), self.biases['hidden'])
        hidden_layer = tf.nn.relu(hidden_layer)

        output_layer = tf.matmul(hidden_layer, self.weights['output']) + self.biases['output']
        softmax_layer = tf.nn.softmax(output_layer);
        labels = self.y
        loss_layer = tf.nn.softmax_cross_entropy_with_logits_v2(labels=labels, logits=output_layer)
        cost = tf.reduce_mean(loss_layer)
        
        return (output_layer, softmax_layer, cost)



    def predictBatch(self, pred_data_file, model_ckpt_dir, outfile):

        # load data
        pred_x = self.loadPredictData(pred_data_file)
       
        with tf.Session() as sess:
            self.restoreCheckpoint(sess, model_ckpt_dir)
            softmax = tf.get_collection('softmax')[0]
            x = tf.get_collection('x')[0]

            num_pred_datapoints = pred_x.shape[0]

            # run the optimizer
            #softmax_output, = sess.run([softmax], feed_dict={x: pred_x})
            softmax_vals = softmax.eval({x: pred_x.reshape(-1, self.input_num_units)})


            # write the softmax outputs to outfile
            self.writePredOutputToFile(softmax_vals, outfile)


    # Not implemented yet
    def sampleProportionalToScores(self, scores):
        thresholds = [0.0 for s in range(len(scores))]

        accum_sum = 0.0
        for pos in range(len(scores)):
            accum_sum += scores[pos]
            thresholds[pos] = accum_sum

        r = random.uniform(0.0, accum_sum)
        for pos in range(1, len(scores) + 1):
            if r <= thresholds[pos - 1]:
                return pos - 1

        raise ValueError("Cannot properly sample an action!")

            
    def predictSingle(self, state, model_ckpt_dir, sample=True):

        # load data
        pred_x = np.array([state])
       
        with tf.Session() as sess:
            self.restoreCheckpoint(sess, model_ckpt_dir)
            softmax = tf.get_collection('softmax')[0]
            output = tf.get_collection('output')[0]
            x = tf.get_collection('x')[0]

            num_pred_datapoints = pred_x.shape[0]

            # run the optimizer
            #softmax_output, = sess.run([softmax], feed_dict={x: pred_x})
            softmax_vals = softmax.eval({x: pred_x.reshape(-1, self.input_num_units)})[0]
            output_vals = output.eval({x: pred_x.reshape(-1, self.input_num_units)})[0]

            if sample:
                action = self.sampleProportionalToScores(softmax_vals) # might need to change base of softmax
            else:
                action = np.argmax(softmax_vals)



            return action




    # model_ckpt_dir must end with a /
    def train(self, data_dir, model_ckpt_dir, from_scratch=True, num_epochs=50, batch_size=128, learning_rate=0.01):
        
        print "Training model", model_ckpt_dir, "on data", data_dir

        with tf.Session() as sess:

            if from_scratch:
                # build computation graph
                output, softmax, cost = self.buildGraph()
                optimizer = tf.train.AdamOptimizer(learning_rate=learning_rate).minimize(cost)
                init = tf.global_variables_initializer()
                sess.run(init)

                # Add relevant ops to the collection
                tf.add_to_collection('output', output)
                tf.add_to_collection('cost', cost)
                tf.add_to_collection('optimizer', optimizer)
                tf.add_to_collection('softmax', softmax)
                tf.add_to_collection('x', self.x)
                tf.add_to_collection('y', self.y)

                x = self.x
                y = self.y

            else:
                # restore computation graph and variables
                self.restoreCheckpoint(sess, model_ckpt_dir)
                optimizer = tf.get_collection('optimizer')[0]
                cost = tf.get_collection('cost')[0]
                output = tf.get_collection('output')[0]
                x = tf.get_collection('x')[0]
                y =tf.get_collection('y')[0]
        

            # load the training/validation data
            train_x, train_y, val_x, val_y = self.loadTrainData(data_dir)
            num_train_datapoints, num_val_datapoints = train_x.shape[0], val_x.shape[0]
            assert batch_size != 0, "Batch size cannot be 0"
            num_batches = int(num_train_datapoints / batch_size)
            print "num_train_datapoints:", num_train_datapoints, ", num_val_datapoints:", num_val_datapoints
            print "num_batches:", num_batches
    
            # Run training epochs
            for epoch in range(num_epochs):
                avg_cost = 0
                
                for i in range(num_batches):

                    # Create a minibatch
                    batch_x, batch_y = self.createBatch(train_x, train_y, batch_size, num_train_datapoints)

                    # run the optimizer
                    _, c = sess.run([optimizer, cost], feed_dict={x: batch_x, y: batch_y})
                    
                    # Update the average cost
                    avg_cost += c / num_batches
                    
                # log if necessary
                if (epoch % self.print_every == 0):
                    print "Epoch:", (epoch+1), "cost =", "{:.5f}".format(avg_cost)

                # save a checkpoint if necessary
                if (epoch % self.save_every == 0):
                    self.saveCheckpoint(sess, model_ckpt_dir + str(epoch) + '/')


            # Save the final model
            self.saveCheckpoint(sess, model_ckpt_dir)            
            print "\nTraining complete!"

            # find predictions on train set
            ole = output.eval({x: train_x.reshape(-1, self.input_num_units), y: train_y})
            ame = tf.argmax(output, 1).eval({x: train_x.reshape(-1, self.input_num_units), y: train_y})

            pred_temp = tf.equal(tf.argmax(output, 1), tf.argmax(y, 1))
            pte = pred_temp.eval({x: train_x.reshape(-1, self.input_num_units), y: train_y})
            print ole[19]
            print ame[19]
            print pte[19]
            
            accuracy = tf.reduce_mean(tf.cast(pred_temp, "float")).eval({x: train_x.reshape(-1, self.input_num_units), y: train_y})
            print "Train Accuracy:", accuracy


            ole = output.eval({x: val_x.reshape(-1, self.input_num_units), y: val_y})
            ame = tf.argmax(output, 1).eval({x: val_x.reshape(-1, self.input_num_units), y: val_y})

            pred_temp = tf.equal(tf.argmax(output, 1), tf.argmax(y, 1))
            pte = pred_temp.eval({x: val_x.reshape(-1, self.input_num_units), y: val_y})
            print ole[19]
            print ame[19]
            print pte[19]
            
            accuracy = tf.reduce_mean(tf.cast(pred_temp, "float")).eval({x: val_x.reshape(-1, self.input_num_units), y: val_y})
            print "Validation Accuracy:", accuracy




