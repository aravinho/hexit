import numpy as np 
import scipy as sp 
import tensorflow as tf 
import os

import pandas as pd
from sklearn.metrics import accuracy_score
import datetime, time


def convert_dist_to_argmax(dist_labels, num_classes=9):
    """Convert class labels from scalars to one-hot vectors"""
    argmax_labels = np.zeros_like(dist_labels)
    argmax_labels[np.arange(len(dist_labels)), dist_labels.argmax(1)] = 1
    return argmax_labels

print("Hello from nn.py")

# https://www.analyticsvidhya.com/blog/2016/10/an-introduction-to-implementing-neural-networks-using-tensorflow/

root_dir = os.path.abspath('./')
src_dir = os.path.join(root_dir, 'src')
data_dir = os.path.join(root_dir, 'data')
in_data_dir = os.path.join(data_dir, 'in')
out_data_dir = os.path.join(data_dir, 'out')

# To stop potential randomness
seed = 128
rng = np.random.RandomState(seed)

# check for existence
assert os.path.exists(root_dir)
assert os.path.exists(src_dir)
assert os.path.exists(data_dir)
assert os.path.exists(in_data_dir)
assert os.path.exists(out_data_dir)

train_x_pd = pd.read_csv(os.path.join(data_dir, 'in', 'train', 'random_batch_1024', 'data.csv'), header=None)
train_y_pd = pd.read_csv(os.path.join(data_dir, 'in', 'train', 'random_batch_1024', 'labels.csv'), header=None)

train_x = train_x_pd.values.astype(np.float32)
train_y = convert_dist_to_argmax(train_y_pd.values.astype(np.float32))

split_size = int(train_x.shape[0]*0.75)

train_x, val_x = train_x[:split_size], train_x[split_size:]
train_y, val_y = train_y[:split_size], train_y[split_size:]
print "train_x: ", train_x.shape, "val_x: ", val_x.shape, "train_x: ", train_x.shape, "train_y", train_y.shape
print "sample train_x:"
print train_x[51]
print "sample train_y:"
print train_y[51]






### BUILD GRAPH

# number of neurons in each layer
input_num_units = 9
hidden_num_units = 200
output_num_units = 9

# define placeholders
x = tf.placeholder(tf.float32, [None, input_num_units])
y = tf.placeholder(tf.float32, [None, output_num_units])

# set remaining variables
epochs = 1000
print_every = 100
batch_size = 128
learning_rate = 0.01

checkpoint_dir = "models/tictactoe/random_batch_1024/"


### define weights and biases of the neural network (refer this article if you don't understand the terminologies)

weights = {
    'hidden': tf.Variable(tf.random_normal([input_num_units, hidden_num_units], seed=seed)),
    'output': tf.Variable(tf.random_normal([hidden_num_units, output_num_units], seed=seed))
}

biases = {
    'hidden': tf.Variable(tf.random_normal([hidden_num_units], seed=seed)),
    'output': tf.Variable(tf.random_normal([output_num_units], seed=seed))
}

print "weights", weights['hidden'].shape, weights['output'].shape
print "biases", biases['hidden'].shape, biases['output'].shape

hidden_layer = tf.add(tf.matmul(x, weights['hidden']), biases['hidden'])
hidden_layer = tf.nn.relu(hidden_layer)

output_layer = tf.matmul(hidden_layer, weights['output']) + biases['output']
labels = y
softmax_layer = tf.nn.softmax_cross_entropy_with_logits_v2(labels=y, logits=output_layer)
cost = tf.reduce_mean(softmax_layer)

optimizer = tf.train.AdamOptimizer(learning_rate=learning_rate).minimize(cost)

### END BUILD GRAPH



def batch_creator(batch_size, dataset_length, dataset_name):
    """Create batch with random samples and return appropriate format"""
    #batch_mask = rng.choice(dataset_length, batch_size) # seeded rng controls randomness
    batch_mask = np.random.choice(dataset_length, batch_size) # truly random every time
    
    batch_x = eval(dataset_name + '_x')[[batch_mask]].reshape(-1, input_num_units)
    batch_y = eval(dataset_name + '_y')[[batch_mask]].reshape(-1, output_num_units)
        
    return batch_x, batch_y


def saveCheckpoint(sess, checkpoint_dir):
    saver = tf.train.Saver()
    if not os.path.exists(checkpoint_dir):
        os.makedirs(checkpoint_dir)
    save_path = saver.save(sess, checkpoint_dir)

    ts = time.time()
    ts = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
    print "Saved to", save_path, "at", ts

def restoreCheckpoint(sess, checkpoint_dir):
    saver = tf.train.Saver()
    saver.restore(sess, checkpoint_dir)
    
    ts = time.time()
    ts = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
    print "Restored from to", checkpoint_dir, "at", ts


##INIT
init = tf.global_variables_initializer()

# RUN SESS
with tf.Session() as sess:
    # create initialized variables
    sess.run(init)
    
    ### for each epoch, do:
    ###   for each batch, do:
    ###     create pre-processed batch
    ###     run optimizer by feeding batch
    ###     find cost and reiterate to minimize
    
    for epoch in range(epochs):
        avg_cost = 0
        # total_cost = 0
        # hidden_l, output_l, lab, softmax_l, op, c = sess.run([hidden_layer, output_layer, labels, softmax_layer, optimizer, cost], feed_dict = {x: train_x, y: train_y})
        # total_cost += c
        # print "\noutput layer: ", output_l, "\n"
        # print "\nlabels: ", lab, "\n"
        # print "\nsoftmax layer: ", softmax_l, "\n"
        # print "\ncost: ", c

        num_batches = int(train_x.shape[0]/batch_size)
        if epoch == 1:
            print "num_batches: ", num_batches
        for i in range(num_batches):
            batch_x, batch_y = batch_creator(batch_size, train_x.shape[0], 'train')
            hidden_layer_node, output_layer_node, labels_node, softmax_layer_node, optimizer_node, c = sess.run([hidden_layer, output_layer, labels, softmax_layer, optimizer, cost], feed_dict = {x: batch_x, y: batch_y})
            
            avg_cost += c / num_batches
            
        if (epoch % print_every == 0):
            print "Epoch:", (epoch+1), "cost =", "{:.5f}".format(avg_cost)
    
    print "\nTraining complete!"

    
    
    # find predictions on train set
    ole = output_layer.eval({x: train_x.reshape(-1, input_num_units), y: train_y})
    ame = tf.argmax(output_layer, 1).eval({x: train_x.reshape(-1, input_num_units), y: train_y})

    pred_temp = tf.equal(tf.argmax(output_layer, 1), tf.argmax(y, 1))
    pte = pred_temp.eval({x: train_x.reshape(-1, input_num_units), y: train_y})
    print ole[19]
    print ame[19]
    print pte[19]
    
    accuracy = tf.reduce_mean(tf.cast(pred_temp, "float")).eval({x: train_x.reshape(-1, input_num_units), y: train_y})
    print "Train Accuracy:", accuracy


    ole = output_layer.eval({x: val_x.reshape(-1, input_num_units), y: val_y})
    ame = tf.argmax(output_layer, 1).eval({x: val_x.reshape(-1, input_num_units), y: val_y})

    pred_temp = tf.equal(tf.argmax(output_layer, 1), tf.argmax(y, 1))
    pte = pred_temp.eval({x: val_x.reshape(-1, input_num_units), y: val_y})
    print ole[19]
    print ame[19]
    print pte[19]
    
    accuracy = tf.reduce_mean(tf.cast(pred_temp, "float")).eval({x: val_x.reshape(-1, input_num_units), y: val_y})
    print "Validation Accuracy:", accuracy



    # Save checkpoint
    saveCheckpoint(sess, checkpoint_dir)

# Train more
with tf.Session() as sess:
    restoreCheckpoint(sess, checkpoint_dir)
    for epoch in range(epochs):
        avg_cost = 0
        # total_cost = 0
        # hidden_l, output_l, lab, softmax_l, op, c = sess.run([hidden_layer, output_layer, labels, softmax_layer, optimizer, cost], feed_dict = {x: train_x, y: train_y})
        # total_cost += c
        # print "\noutput layer: ", output_l, "\n"
        # print "\nlabels: ", lab, "\n"
        # print "\nsoftmax layer: ", softmax_l, "\n"
        # print "\ncost: ", c

        num_batches = int(train_x.shape[0]/batch_size)
        if epoch == 1:
            print "num_batches: ", num_batches
        for i in range(num_batches):
            batch_x, batch_y = batch_creator(batch_size, train_x.shape[0], 'train')
            hidden_layer_node, output_layer_node, labels_node, softmax_layer_node, optimizer_node, c = sess.run([hidden_layer, output_layer, labels, softmax_layer, optimizer, cost], feed_dict = {x: batch_x, y: batch_y})
            
            avg_cost += c / num_batches
            
        if (epoch % print_every == 0):
            print "Epoch:", (epoch+1), "cost =", "{:.5f}".format(avg_cost)


     # find predictions on train set
    ole = output_layer.eval({x: train_x.reshape(-1, input_num_units), y: train_y})
    ame = tf.argmax(output_layer, 1).eval({x: train_x.reshape(-1, input_num_units), y: train_y})

    pred_temp = tf.equal(tf.argmax(output_layer, 1), tf.argmax(y, 1))
    pte = pred_temp.eval({x: train_x.reshape(-1, input_num_units), y: train_y})

    
    accuracy = tf.reduce_mean(tf.cast(pred_temp, "float")).eval({x: train_x.reshape(-1, input_num_units), y: train_y})
    print "Train Accuracy:", accuracy


    ole = output_layer.eval({x: val_x.reshape(-1, input_num_units), y: val_y})
    ame = tf.argmax(output_layer, 1).eval({x: val_x.reshape(-1, input_num_units), y: val_y})

    pred_temp = tf.equal(tf.argmax(output_layer, 1), tf.argmax(y, 1))
    pte = pred_temp.eval({x: val_x.reshape(-1, input_num_units), y: val_y})

    
    accuracy = tf.reduce_mean(tf.cast(pred_temp, "float")).eval({x: val_x.reshape(-1, input_num_units), y: val_y})
    print "Validation Accuracy:", accuracy

    checkpoint_dir = "models/tictactoe/random_batch_1024/"
    saveCheckpoint(sess, checkpoint_dir)
    








