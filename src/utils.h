#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>
#include <iostream>

using namespace std;

/* ASSERT Macro. */

#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif


/* Small class to read and store command line arguments. */

class ArgMap {
public:

	/* Initializes an empty ArgMap. */
	ArgMap();

	/* Inserts the key-value pair into the map if it doesn't already exist. */
	void insert(string key, string val);

	/* Returns the value for the given key, as a string.
	 * If not present, returns the default string.
	 * If no default string is passed, errors.
	 */
	string getString(string key, string default_val="jdaflkdjalf") const;

	/* Returns the value for the given key, as an int.
	 * If not present, returns the default int.
	 * If no default int is passed, errors.
	 */
	int getInt(string key, int default_val=342324324) const;

	/* Returns the value for the given key, as a double.
	 * If not present, returns the default double.
	 * If no default double is passed, errors.
	 */
	double getDouble(string key, double default_val=243423.324) const;

	/* Returns the value for the given key, as a bool.
	 * If not present, returns the default bool.
	 */
	bool getBool(string key, bool default_val=false) const;


	/* Returns whether this map contains the given key. */
	bool contains(string key) const;

private:
	map<string, string> _arg_map;
};





/**** Utility Functions to read from files ****/



/**
 * Helper function for reading/writing to CSV files in directories.
 * This function takes in the name of a directory, and returns a vector with the (full) filepath of all CSV files in that directory.
 * Only includes files whose file numbers are greater than or equal to START_AT.
 * Errors if the given directory does not exist or cannot be opened.
 */
vector<string> csvFilesInDir(string dirname, int start_at=0);

/* Returns an int vector that corresponds to the given CSV string. */
vector<int> readCSVString(string s);




/**** Utility Functions to write to files ****/

/**
 * Returns the number of the next available CSV file in the given directory.
 * For example, if the directory is empty, will return 0.
 * If the directory contains 0.csv and 1.csv, will return 2.
 */
int nextAvailableFileNum(string dirname);


/**
 * Creates the given directory (and all the necessary parent directories) if it doesn't already exist.
 * Returns the name of the directory.
 */
string createDir(string dirname);


/**
 * Prepares /states and /action_distributions subdirectories of the given BASE_DIRNAME.
 * Returns a pair of the names.
 */
pair<string, string> prepareDirectories(string base_dirname);


/* Returns a CSV string representation of the given vector of doubles. */
string asCSVString(const vector<double>& vec);



/**** Other Assorted Utilities ****/

/* Returns a double uniformly at random between the given bounds. */
double randomDouble(double lower_bound, double upper_bound);

/* Prints a vector of doubles in a human-readable format, and also prints the given name. */
void printVector(const vector<double>& vec, string name);

/* Prints a vector of ints in a human-readable format, and also prints the given name. */
void printVector(const vector<int>& vec, string name);

/* Given a vector of weights, returns K with probability proportional to the K-th weight. */
int sampleProportionalToWeights(const vector<double>& weights);

/**
 * Computes the softmax of the logits vector, but gives no weight to any element at an index K such that mask[K] == false.
 * Populates the softmaxes vector with the softmaxes.
 * Expects that logits and mask are of the same size.
 */
void computeSoftmaxWithMask(const vector<double>& logits, const vector<bool>& mask, vector<double>* softmaxes);

/**
 * Returns the index of the largest element in vals, provided that mask[index] == true. 
 * Expects that vals and mask are of the same size.
 */
int argmaxWithMask(const vector<double>& vals, const vector<bool>& mask);

/** 
 * Log the given message, followed by the time.
 */
void logTime(string message);






#endif
