#ifndef INFERENCE_H
#define INFERENCE_H

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/protobuf/meta_graph.pb.h"

using namespace std;
using namespace tensorflow;


/****** Main Publicly Exposed Functions *****/

/**
 * Given a session, a pointer to a MetaGraphDef object.
 * Reads the metagaph stored at META_GRAPH_PATH into the MetaGraphDef object.
 * Then adds the MetaGraph to the session.
 * Then reads in all the ops in the TF collection, based on the model checkpoint at MODEL_CHECKPOINT_PATH.
 * Returns the status of these operations.
 */
Status restoreModelGraph(Session* session, MetaGraphDef* graph_def, string meta_graph_path, string model_checkpoint_path);


/** 
 * Makes a tensor out of the values in VEC.
 * Each of the vectors in vec is meant to represent a single data point, and together they form the batch.
 * The batch tensor's shape is given by the SHAPE argument.
 * The string data_type tells this function which Tensorflow datatype to use (DT_INT, DT_FLOAT, etc).
 * This function fills the given tensor.
 */
void makeTensorBatch(Tensor* tensor, const vector<vector<double>>& vec, const vector<int>& shape, const string& data_type="float");


/**
 * Makes a singleton batch.
 * The single data point is given by VEC.
 * This is transformed into a batch whose single element is this data point.
 * The string data_type tells this function which Tensorflow datatype to use (DT_INT, DT_FLOAT, etc).
 * This function fills the given tensor.
 * This function has the same name as the previous one but is different in that it is fed only one data point,
 * and converts it to a singleton batch.
 */
void makeTensor(Tensor* tensor, const vector<double>& vec, const vector<int>& shape, const string& data_type="float");



/**
 * Evaluates the ops in OUTPUT_OPS, feeding the ops in FEED_DICT.
 * Uses the given SESSION.
 * Populated the OUTPUT_TENSORS vector with the resulting values of all the ops in OUTPUT_OPS.
 */
Status predictBatch(Session* session, const vector<pair<string, Tensor>>& feed_dict,
    const vector<string>& output_ops, vector<Tensor>* output_tensors);


/**
 * Assumes batch tensors for each of the states in STATES.
 * For now, assumes they are all hex states.
 * Creates a batch of X (the channel representation) and a batch of turn_mask.
 * Maps "x" to the x tensor, and "turn_mask" to the turn_mask tensor in feed_dict.
 * Adds "output" to output_ops.
 */
void createTensorsFromStates(int batch_size, const vector<EnvState*>& states, vector<pair<string, Tensor>>* feed_dict, vector<string>* output_ops);



/***** Helper Functions *****/


/**
 * Creates a 4D tensor of type T out of the values in the given vec.
 * The tensor's shape is given by the SHAPE argument.
 * The string data_type tells this function which Tensorflow datatype to use (DT_INT, DT_FLOAT, etc).

 * Let N, H, W, C be the dimensions of the tensor (as given in SHAPE).
 * This is how the tensor is laid out:
 * tensor[n, h, w, c] = vec[n][((h * H) + w) + ((h * w) * c)].
 *  
 * This function fills the given tensor.
 */
void make4DTensor(Tensor* tensor, const vector<vector<double>>& vec, const vector<int>& shape, const string& data_type="float"); 

/**
 * Creates a 4D tensor of type T out of the values in the given vec.
 * Unlike above, the values given correspond to a 3-D vector, so an artificial dimension of 1 is added.
 * So the tensor returned has shape {1, shape[0], shape[1], shape[2]}.
 * This is useful when running inference on batches of size 1, like when playing games.
 */
/*Tensor make4DTensor(const vector<double>& vec, const vector<int>& shape, const string& data_type="float");*/

/**
 * Creates a 2D tensor of type T out of the values in the given vec.
 * The tensor's shape is given by the SHAPE argument.
 * The string data_type tells this function which Tensorflow datatype to use (DT_INT, DT_FLOAT, etc).

 * Let N, D be the dimensions of the tensor (as given in SHAPE).
 * This is how the tensor is laid out:
 * tensor[n, d] = vec[n][d].
 *  
 * This function fills the given tensor.
 */
void make2DTensor(Tensor* tensor, const vector<vector<double>>& vec, const vector<int>& shape, const string& data_type="float");


/**
 * Returns the Tensorflow::DataType that corresponds to this string.
 */
DataType dataTypeFromString(const string& data_type="float");




#endif