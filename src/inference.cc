#include "hex_state.h"
#include "inference.h"

using namespace std;
using namespace tensorflow;


/**
 * Returns the Tensorflow::DataType that corresponds to this string.
 */
DataType dataTypeFromString(const string& data_type) {

    ASSERT(data_type == "float" || data_type == "int", "No support for data type " << data_type);

    if (data_type == "float") {
        return DT_FLOAT;
    } 
    return DT_INT32;

}


/**
 * Creates a 4D tensor of type T out of the values in the given vec.
 * The tensor's shape is given by the SHAPE argument.
 * The string data_type tells this function which Tensorflow datatype to use (DT_INT, DT_FLOAT, etc).

 * Let N, H, W, C be the dimensions of the tensor (as given in SHAPE).
 * This is how the tensor is laid out:
 * tensor[n, h, w, c] = vec[n][((h * H) + w) + ((h * w) * c)].
 *  
 * This function returns the tensor.
 */
void make4DTensor(Tensor* tensor, const vector<vector<double>>& vec, const vector<int>& shape, const string& data_type) {

    ASSERT(shape.size() == 4, "4D tensor requires a 4D shape");
    int N = shape[0], H = shape[1], W = shape[2], C = shape[3];
    ASSERT(vec.size() >= N, "Vector must have at least size << N");

    // populate the tensor
    for (int n = 0; n < N; n++) {
        ASSERT(vec[n].size() >= H*W*C, "Vector must have at least size " << H*W*C);
        for (int h = 0; h < H; h++) {
            for (int w = 0; w < W; w++) {
                for (int c = 0; c < C; c++) {
                    auto data = tensor->tensor<float, 4>();
                    data(n, h, w, c) = vec[n][ ((h * H) + w) + (c * 81) ] ;
                }
            }
        }
    }

    
}


/**
 * Creates a 4D tensor of type T out of the values in the given vec.
 * Unlike above, the values given correspond to a 3-D vector, so an artificial dimension of 1 is added.
 * So the tensor returned has shape {1, shape[0], shape[1], shape[2]}.
 * This is useful when running inference on batches of size 1, like when playing games.
 */
/*Tensor make4DTensor(const vector<double>& vec, const vector<int>& shape, const string& data_type) {

    ASSERT(shape.size() == 3, "To make a singleton 4D tensor, must provide 3D shape");

    cerr << "Here 1" << endl;

    // turn VEC into a singleton vector of vectors
    vector<vector<double>> batch_vec = {vec};

    cerr << "Here 2" << endl;

    // add a placeholder 1 into the shape vector
    vector<int> batch_shape = {1};
    for (int dim : shape) {
        batch_shape.push_back(dim);
    }

    cerr << "Here 3" << endl;

    // call the other make4DTensor function
    return make4DTensor(batch_vec, batch_shape, data_type);
}*/

/**
 * Creates a 2D tensor of type T out of the values in the given vec.
 * The tensor's shape is given by the SHAPE argument.
 * The string data_type tells this function which Tensorflow datatype to use (DT_INT, DT_FLOAT, etc).

 * Let N, D be the dimensions of the tensor (as given in SHAPE).
 * This is how the tensor is laid out:
 * tensor[n, d] = vec[n][d].
 *  
 * This function returns the tensor.
 */
void make2DTensor(Tensor* tensor, const vector<vector<double>>& vec, const vector<int>& shape, const string& data_type) {

    ASSERT(shape.size() == 2, "2D tensor requires a 2D shape");
    int N = shape[0], D = shape[1];
    ASSERT(vec.size() >= N, "Vector must have at least size << N");

    // populate the tensor
    for (int n = 0; n < N; n++) {
        ASSERT(vec[n].size() >= D, "Vector must have at least size " << D);
        for (int d = 0; d < D; d++) {
            auto data = tensor->tensor<float, 2>();
            data(n, d) = vec[n][d]; 
        }
    }

}



/** 
 * Makes a tensor out of the values in VEC.
 * Each of the vectors in vec is meant to represent a single data point, and together they form the batch.
 * The batch tensor's shape is given by the SHAPE argument.
 * The string data_type tells this function which Tensorflow datatype to use (DT_INT, DT_FLOAT, etc).
 * This function returns the tensor.
 */
void makeTensorBatch(Tensor* tensor, const vector<vector<double>>& vec, const vector<int>& shape, const string& data_type) {
    int num_dims = shape.size();
    ASSERT(num_dims == 2 || num_dims == 4, "No support for making batch tensors with " << num_dims << " dimensions");
    if (num_dims == 2) {
        make2DTensor(tensor, vec, shape, data_type);
    } else {
        make4DTensor(tensor, vec, shape, data_type);
    }   
}


/**
 * Makes a singleton batch.
 * The single data point is given by VEC.
 * This is transformed into a batch whose single element is this data point.
 * The string data_type tells this function which Tensorflow datatype to use (DT_INT, DT_FLOAT, etc).
 * This function returns the tensor.
 */
void makeTensor(Tensor* tensor, const vector<double>& vec, const vector<int>& shape, const string& data_type) {

    // turn VEC into a singleton vector of vectors
    vector<vector<double>> batch_vec = {vec};
    
    // add a placeholder 1 into the shape vector
    vector<int> batch_shape = {1};
    for (int dim : shape) {
        batch_shape.push_back(dim);
    }

    // call the other make4DTensor function
    makeTensorBatch(tensor, batch_vec, batch_shape, data_type);
}





/**
 * Evaluates the ops in OUTPUT_OPS, feeding the ops in FEED_DICT.
 * Uses the given SESSION.
 * Populated the OUTPUT_TENSORS vector with the resulting values of all the ops in OUTPUT_OPS.
 */
Status predictBatch(Session* session, const vector<pair<string, Tensor>>& feed_dict,
    const vector<string>& output_ops, vector<Tensor>* output_tensors) {

    ASSERT(session != NULL, "Cannot have a null session");
    ASSERT(output_tensors != NULL, "Cannot have a null output_tensors vector");

    Status status = session->Run(feed_dict, output_ops, {}, output_tensors);
    return status;
}



/**
 * Given a session, a pointer to a MetaGraphDef object.
 * Reads the metagaph stored at META_GRAPH_PATH into the MetaGraphDef object.
 * Then adds the MetaGraph to the session.
 * Then reads in all the ops in the TF collection, based on the model checkpoint at MODEL_CHECKPOINT_PATH.
 * Returns the status of these operations.
 */
Status restoreModelGraph(Session* session, MetaGraphDef* graph_def, string meta_graph_path, string model_checkpoint_path) {

    ASSERT(session != NULL, "Cannot have a null session");
    ASSERT(graph_def != NULL, "Cannot have a null graph_def");

    // read in metagraph protobuf
    Status status = ReadBinaryProto(Env::Default(), meta_graph_path, graph_def);
    ASSERT(status.ok(), "Error reading meta graph from " << meta_graph_path << ": " << status.ToString());

    // add graph to session
    status = session->Create(graph_def->graph_def());
    ASSERT(status.ok(), "Error adding session to graph: " << status.ToString());

    // restore all the ops from the "collection"
    Tensor checkpoint_path_tensor(DT_STRING, TensorShape());
    checkpoint_path_tensor.scalar<string>()() = model_checkpoint_path;
    status = session->Run(
                            {{graph_def->saver_def().filename_tensor_name(), checkpoint_path_tensor}},
                            {},
                            {graph_def->saver_def().restore_op_name()},
                            nullptr);
    ASSERT(status.ok(), "Error loading ops from collection from model at " << model_checkpoint_path);
    return status;
}




// assumes hex states
void createTensorsFromStates(int batch_size, const vector<EnvState*>& states, vector<pair<string, Tensor>>* feed_dict, vector<string>* output_ops) {

    ASSERT(feed_dict != NULL, "Cannot have a null feed dict");
    ASSERT(output_ops != NULL, "Cannot have a null output ops vector");
    ASSERT(states.size() >= batch_size, "Must have at least " << batch_size << " states in the states vector");

    if (batch_size == 0) return;
    EnvState* state = states[0];

    int channel_dim = sqrt(states[0]->numActions()) + 4;
    int channel_size = pow(channel_dim, 2);
    int num_channels = 2;
    int x_dim = channel_size * num_channels;
    vector<int> x_shape = {batch_size, channel_dim, channel_dim, num_channels};

    // initialize the x batch and turn mask batch
    vector<vector<double>> x_batch(batch_size);
    vector<vector<double>> turn_mask_batch(batch_size);

    // populate the batches
    for (int state_num = 0; state_num < batch_size; state_num++) {
        
        // if this state is terminal, just create a dummy x vector
        if (states[state_num]->isTerminalState()) {
            vector<double> dummy_x(x_dim, 0.0);
            x_batch[state_num] = dummy_x;
            turn_mask_batch[state_num] = {1.0, 0.0};
            continue;
        }

        // if state is non terminal, create add it to a batch of x and turn_mask tensors
        EnvState* state = states[state_num];
        vector<double> state_vector;
        state->makeStateVector(&state_vector);
        x_batch[state_num] = state_vector;
        
        vector<double> turn_mask;
        if (state->turn() == 1) {
            turn_mask = {1.0, 0.0};
        } else if (state->turn() == -1) {
            turn_mask = {0.0, 1.0};
        }
        turn_mask_batch[state_num] = turn_mask;


    }

    Tensor x(DT_FLOAT, TensorShape({batch_size, channel_dim, channel_dim, 2}));
    makeTensorBatch(&x, x_batch, x_shape);

    Tensor turn_mask(DT_FLOAT, TensorShape({batch_size, 2}));
    makeTensorBatch(&turn_mask, turn_mask_batch, {batch_size, 2});

    feed_dict->push_back(make_pair("x", x));
    feed_dict->push_back(make_pair("turn_mask", turn_mask));

    output_ops->push_back("output");


}





/*int foo(int argc, char* argv[]) {

    cout << endl << endl;

    // set up your input paths
    const string meta_graph_path = "models/hex/5/test_models/exp_model.meta";
    const string model_checkpoint_path = "models/hex/5/test_models/exp_model";
    

    // initialize the session
    Session* session = NewSession(SessionOptions());
    ASSERT(session != NULL, "Session is NULL");

    // Read in the protobuf graph we exported, and add it to the session
    MetaGraphDef graph_def;
    Status status = restoreModelGraph(session, &graph_def, meta_graph_path, model_checkpoint_path);
    ASSERT(status.ok(), "Error restoring model graph");
    cout << "Successfully loaded metagraph and all nodes from model checkpoint at " << model_checkpoint_path << endl;


    //populate tensors
    vector<int> b = {0,0,1,0,-1,
                     0,1,0,0,-1,
                     0,1,0,-1,1,
                     0,-1,0,0,0,
                     1,-1,0,0,0};
    HexState board(5, b);
    vector<double>* state_vector = new vector<double>(164);
    board.makeStateVector(state_vector);
    cout << "Board: " << endl;
    board.printBoard();
    cout << endl;


    Tensor x = makeTensor(*state_vector, {9,9,2}, "float");

    vector<double> y_vec(25, 1.0/25);
    Tensor y = makeTensor(y_vec, {25}, "float");

    vector<double> turn_mask_vec = {1.0, 0.0};
    Tensor turn_mask = makeTensor(turn_mask_vec, {2}, "float");


    vector<pair<string, Tensor> > feed_dict = {{"x:0", x}, {"turn_mask:0", turn_mask}};
    vector<string> output_ops = {"output:0"};
    std::vector<tensorflow::Tensor> output_tensors;
    status = predictBatch(session, feed_dict, output_ops, &output_tensors);


    Tensor output = output_tensors[0];

    auto output_vals = output.matrix<float>();
    cout << "action distribution: " << endl;
    for (int i = 0; i < 25; i++) {
        cout << output_vals(0, i) << " ";
    }
    cout << endl << endl;

    // Free any resources used by the session
    status = session->Close();

    cout << endl << endl;
    return 0;
}*/




