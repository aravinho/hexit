import numpy as np 
import scipy as sp 
import tensorflow as tf 
import os
import sys

import pandas as pd
import glob
from sklearn.metrics import accuracy_score
import datetime, time

from nn import ExitNN

def main():
	assert len(sys.argv) == 4, "Incorrect number of arguments"
	data_file = sys.argv[1]
	model_dir = sys.argv[2]
	predictions_file = sys.argv[3]

	# turn off logging
	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 

	nn = ExitNN()
	nn.predictBatch(data_file, model_dir, predictions_file)


if __name__ == '__main__':
	main()