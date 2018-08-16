from config import *

import numpy as np
import random
import glob
import pandas as pd
import datetime, time



# A small class to help with reading and storing command line arguments of different data types

class ArgMap:

    # Initializes the ArgMap
    def __init__(self):
        self._arg_map = {}

    # Inserts the given {key, val} mapping
    def insert(self, key, val):
        self._arg_map[key] = val


    def getString(self, key, required=False, default_val=""):
        """ 
        Returns the value for the given key as a string.
        If required and not present, errors.
        If not present but not requires, returns the default value.
        """
        if key in self._arg_map:
            return self._arg_map[key]

        assert not required, "The required key " + key + " is not present (getString)"
        return default_val

    def getInt(self, key, required=False, default_val=0):
        """ 
        Returns the value for the given key as an int.
        If required and not present, errors.
        If not present but not requires, returns the default value.
        """
        if key in self._arg_map:
            try:
                return int(self._arg_map[key])
            except:
                assert False, "The value " + self._arg_map[key] + " for the key " + key + " could not be converted to an int"

        assert not required, "The required key " + key + " is not present (getInt)"
        return default_val

    def getFloat(self, key, required=False, default_val=0.0):
        """ 
        Returns the value for the given key as a float.
        If required and not present, errors.
        If not present but not requires, returns the default value.
        """
        if key in self._arg_map:
            try:
                return float(self._arg_map[key])
            except:
                assert False, "The value " + self._arg_map[key] + " for the key " + key + " could not be converted to a float"

        assert not required, "The required key " + key + " is not present (getFloat)"
        return default_val

    def getBoolean(self, key, required=False, default_val=False):
        """ 
        Returns the value for the given key as a boolean.
        If required and not present, errors.
        If not present but not requires, returns the default value.
        """
        if key in self._arg_map and self._arg_map[key] == "True":
            return True
        if key in self._arg_map and self._arg_map[key] == "False":
            return False

        assert not required, "The required key " + key + " is not present (getBoolean)"
        return default_val

    def contains(self, key):
        """
        Returns whether this map contains the given key.
        """
        return key in self._arg_map



def parseArgs(args):
    """
    The given argument "args" is an array (typically just sys.argv).
    It is assumed that the arguments come in pairs.
    The first element of a pair is of the format --option.
    The second element of the pair is a value (either a string or an int)
    This function returns an ArgMap whose keys are the option elements, and whose values are the values.

    Example:

    args = ["--game", "hex", "--num_games", "100"]
    Returns {"game": "hex", "--num_games": "100"}

    """

    arg_map = ArgMap()

    # make sure the given args array has an even length
    assert len(args) % 2 == 0, "Args must have an even length"

    for pair_num in range(len(args) // 2):
        option = args[pair_num * 2]
        assert len(option) > 2 and option[0:2] == "--", "Arg options must start with double hyphen"
        option = option[2:]
        value = args[pair_num * 2 + 1]
        arg_map.insert(option, value)

    return arg_map




#### Utilities for Reading from CSV files #####


def sortCSVFilesByNumber(filenames):
    """
    Given a list of complete filenames for CSV files, sorts them by filenumber.
    For example, given
    ['data/mcts/hex/5/best_model/states/2.csv', 'data/mcts/hex/5/best_model/states/1.csv', 'data/mcts/hex/5/best_model/states/3.csv'], returns
    ['data/mcts/hex/5/best_model/states/0.csv', 'data/mcts/hex/5/best_model/states/1.csv', 'data/mcts/hex/5/best_model/states/2.csv']
    """
    def getFileNum(filename):
        try:
            spl = filename.split('.')[0].split('/')
            filenum = int(spl[-1])
            return filenum
        except:
            assert False, "Error getting filenumber from filename " + filename

    def getPrefix(filename):
        try:
            spl = filename.split('.')[0].split('/')
            prefix = ""
            for s in spl[0:-1]:
                prefix += s + "/"
            return prefix
        except:
            assert False, "Error getting filenumber from filename " + filename
    try:
        prefix = getPrefix(filenames[0])
        suffix = ".csv"
        file_nums = [getFileNum(filename) for filename in filenames]
        file_nums.sort()
        return [prefix + str(file_num) + suffix for file_num in file_nums]
    except:
        assert False, "Error sorting csv files by number"

def readCSVFiles(dirname, begin_from=DEFAULT_BEGIN_FROM, max_rows=DEFAULT_MAX_ROWS):
    """
    Reads in CSV files from the given directory.
    The directory is assumed to have .csv files, beginning at 0.csv.
    This function starts reading from <begin_from>.csv, then proceeds to <begin_from + 1>.csv, and so on.
    It reads at most MAX_ROWS lines.
    Returns a DataFrame which contains all the rows read.
    """

    assert begin_from >= 0, "Cannot being reading from file " + str(begin_from) + ".csv"
    assert max_rows >= 0, "Cannot read a maximum of " + str(max_rows) + " rows"

    # Grab the names of CSV files in the directory and sort them by file number
    csv_files = glob.glob(dirname + "*.csv")
    csv_files = sortCSVFilesByNumber(csv_files)

    data_frames = []
    num_rows_read, num_rows_left = 0, max_rows

    for filename in csv_files[begin_from:]:
        df = pd.read_csv(filename, header=None, nrows=num_rows_left)
        data_frames.append(df)
        num_rows_read += df.shape[0]
        num_rows_left -= df.shape[0]

        if num_rows_read >= max_rows or num_rows_left <= 0:
            break
    return pd.concat(data_frames)




##### Utilities for Writing to CSV Files #####

def writeCSV(self, output, outfile):
    """
    Writes the given numpy array ARR to the given OUTFILE, which must be a .csv file.
    """
    assert len(outfile) > 4 and outfile[-4:] == ".csv"
    np.savetxt(outfile, output, delimiter=',', fmt='%0.4f')









##### Assorted Neural Network Utilities #######

def createBatch(data, labels, batch_size, dataset_length):
    """
    Given a batch of training vectors (data), and labels, both of total size DATASET_LENGTH creates a batch of size BATCH_SIZE.
    It is expected that DATA and LABELS are numpy arrays
    Creates batch with random samples and returns a (batch_x, batch_y) tuple.
    """
    batch_mask = np.random.choice(dataset_length, batch_size) # truly random every time

    input_num_units, output_num_units = data.shape[1], labels.shape[1]
    
    batch_x = data[[batch_mask]].reshape(-1, input_num_units)
    batch_y = labels[[batch_mask]].reshape(-1, output_num_units)
        
    return (batch_x, batch_y)



##### Other Assorted Utilities #####

LOGGING = True

def writeLog(s, should_log=True):
    """ 
    Print the given string (and the timestamp) unless the logging flag is off.
    """
    if LOGGING and should_log:
        ts = time.time()
        ts = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
        print s, ": ", ts


def printNumpyArray(arr, int_arr=False):
    """
    Prints the given numpy array in a readable format.
    Used just for debugging.
    """
    dim = len(arr.shape)
    if dim == 1:
        printNumpyArray1D(arr, int_arr)
    elif dim == 2:
        printNumpyArray2D(arr, int_arr)
    elif dim == 3:
        printNumpyArray3D(arr, int_arr)
    else:
        assert False, "cannot print an array of more than 3 dimensions"


def printNumpyArray1D(arr, int_arr=False):
    dim = arr.shape[0]
    for i in range(dim):
        if int_arr:
            print int(arr[i]), " ",
        else:
            print "{:.3f}".format(arr[i]), " ",
    print


def printNumpyArray2D(arr, int_arr=False):
    r, c = arr.shape[0], arr.shape[1]
    for i in range(r):
        for j in range(c):
            if int_arr:
                print int(arr[i,j]), " ",
            else:
                print "{:.3f}".format(arr[i,j]), " ",
        print
    print

def printNumpyArray3D(arr, int_arr=False):
    r, c, d = arr.shape[0], arr.shape[1], arr.shape[2]
    for channel in range(d):
        for i in range(r):
            for j in range(c):
                if int_arr:
                    print int(arr[i,j, channel]), " ",
                else:
                    print "{:.3f}".format(arr[i,j, channel]), " ",
            print
        print





def convert_dist_to_argmax(dist_labels, num_classes=DEFAULT_HEX_DIM**2):
    """
    Convert class labels from scalars to one-hot vectors
    """
    argmax_labels = np.zeros_like(dist_labels)
    argmax_labels[np.arange(len(dist_labels)), dist_labels.argmax(1)] = 1
    return argmax_labels

def sampleProportionalToScores(scores):
    """
    Returns an index i (between 0 and len(scores) - 1), sampled the probability proportional to scores[i].
    """

    if len(scores.shape) == 1:
        num_scores = len(scores)
        return np.random.choice(np.arange(num_scores), p=scores)

    elif len(scores.shape) == 2:
        n, d = scores.shape[0], scores.shape[1]
        chosen_scores = np.zeros(n)
        for i in range(n):
            chosen_scores[i] = np.random.choice(np.arange(d), p=scores[i])
        return chosen_scores



    # accum_sum = 0.0
    # thresholds = [0.0 for s in range(len(scores))] 
    # for pos in range(len(scores)):
    #     accum_sum += scores[pos]
    #     thresholds[pos] = accum_sum

    # r = random.uniform(0.0, accum_sum)
    # for pos in range(1, len(scores) + 1):
    #     if r <= thresholds[pos - 1]:
    #         return pos - 1

    # assert False, "Cannot properly sample an action!"


def weightedSample(weights, vals=None):
    """
    Return an element from val, from an index sampled proportional to the weights.
    """
    if vals is None:
        vals = list(range(len(weights)))

    assert len(weights) > 0 and len(vals) <= len(weights), "Must have at least as many weights as values"
    if len(vals) < len(weights):
        weights = weights[0:len(vals)]

    # make sure probabilities sum to 1
    if sum(weights) != 1:
        weights = [w / float(sum(weights)) for w in weights]

    chosen_index = np.random.choice(np.arange(len(vals)), p=weights)
    return vals[chosen_index]

    # accum_sum = 0.0
    # thresholds = [0.0 for s in range(len(vals))] 
    # for pos in range(len(vals)):
    #     accum_sum += weights[pos]
    #     thresholds[pos] = accum_sum

    # r = random.uniform(0.0, accum_sum)
    # for pos in range(1, len(vals) + 1):
    #     if r <= thresholds[pos - 1]:
    #         return vals[pos - 1]

    # assert False, "Could not sample a value"

def decision(p):
    """
    Returns True with probability p, and False with probability 1 - p
    """
    return random.random() <= p



