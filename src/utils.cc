#include "utils.h"

#include <random>
#include <cfloat>       // std::numeric_limits
#include <dirent.h>
#include <sstream>
#include <iomanip>      // std::setprecision
#include <ctime>


using namespace std;



/**** ArgMap Class for Argument passing *****/

ArgMap::ArgMap() {

}


void ArgMap::insert(string key, string val) {
	_arg_map.insert(make_pair(key,val));
}


string ArgMap::getString(string key, string default_val) const {
	if (this->contains(key)) {
		return _arg_map.at(key);
	}
	else if (default_val != "jdaflkdjalf") {
		return default_val;
	}
	ASSERT(false, "The key " << key << " does not exist in ArgMap (getString)");
}


int ArgMap::getInt(string key, int default_val) const {
	if (this->contains(key)) {
		try {
			return stoi(_arg_map.at(key));
		}
		catch (int e) {
			ASSERT(false, "The value " << _arg_map.at(key) << " for key " << key << " cannot be converted to an int");
		}
	}
	else if (default_val != 342324324) {
		return default_val;
	}
	ASSERT(false, "The key " << key << " does not exist in ArgMap (getInt)");
}


double ArgMap::getDouble(string key, double default_val) const {
	if (this->contains(key)) {
		try {
			return stod(_arg_map.at(key));
		}
		catch (int e) {
			ASSERT(false, "The value " << _arg_map.at(key) << " for key " << key << " cannot be converted to an int");
		}
	}
	else if (default_val != 342324324) {
		return default_val;
	}
	ASSERT(false, "The key " << key << " does not exist in ArgMap (getDouble)");
	
}

bool ArgMap::getBool(string key, bool default_val) const {
	if (this->contains(key)) {
		if (_arg_map.at(key) == "true") {
			return true;
		}
		if (_arg_map.at(key) == "false") {
			return false;
		}
	}

	return default_val;

}


bool ArgMap::contains(string key) const {
	return _arg_map.count(key) > 0;
}







/**** Utility Functions to read from files *****/


// list of all CSV files in a directory
vector<string> csvFilesInDir(string dirname, int start_at) {

	// make sure the given directory name ends with a slash
	ASSERT(dirname.size() > 0, "Cannot have empty directory name");
	if (dirname.substr(dirname.size() - 1) != "/") {
		dirname += "/";
	}
	

	// open the directory
	struct dirent *entry;
    DIR *dp;
    dp = opendir(dirname.c_str());
    ASSERT(dp != NULL, "opendir failed for path " << dirname);

    // iterate through all files and grab the numbers of all the CSV files
    vector<int> csv_file_numbers;

    while ((entry = readdir(dp))) {
        string filename = string(entry->d_name);

        // if the file is a csv file, add it to the list
        if (filename.size() > 4 ) {
        	string ext = filename.substr(filename.size() - 4);
        	if (ext == ".csv") {
        		int file_num = stoi(filename.substr(0, filename.size() - 4));
        		if (file_num >= start_at) {
        			csv_file_numbers.push_back(file_num);
        		}
        	}
        }
    }

    closedir(dp);

    // sort the vector of file numbers
    sort(csv_file_numbers.begin(), csv_file_numbers.end());

    // convert each file number back to the original filename
    vector<string> csv_filenames;
    for (int file_num : csv_file_numbers) {
    	csv_filenames.push_back(dirname + to_string(file_num) + ".csv");
    }

    // return the sorted list of filenames
    return csv_filenames;

}



// reads in a CSV string corresponding to an int vector (used for parsing game states)
vector<int> readCSVString(string s) {
	stringstream sstream(s);
	vector<int> v;

	while(sstream.good()){
    	string substr;
    	getline(sstream, substr, ',' );
    	try {
    		v.push_back(stoi(substr));
    	} 
    	catch (int e) {
    		ASSERT(false, substr << " cannot be parsed into an int");
    	}
	}

	return v;
}




/***** Utility functions to write to files ****/


// number of the next CSV file available to write to in the given directory
int nextAvailableFileNum(string dirname) {

	// make sure the given directory name ends with a slash
	ASSERT(dirname.size() > 0, "Cannot have empty directory name");
	if (dirname.substr(dirname.size() - 1) != "/") {
		dirname += "/";
	}

	// open directory
	struct dirent *entry;
    DIR *dp;
    dp = opendir(dirname.c_str());
    ASSERT(dp != NULL, "opendir failed for path " << dirname);

    // iterate through all the files in the directory, keeping track of the highest indexed CSV file
    int highest_file_num = 0;
    bool csv_files_exist = false;

    while ((entry = readdir(dp))) {
        
        string filename = string(entry->d_name);

        // ignore non-CSV files
        if (filename.size() < 4) {
        	continue;
        }
        string ext = filename.substr(filename.size() - 4);
        if (ext != ".csv") {
        	continue;
        }

        csv_files_exist = true;

        // grab the file number and see if it is the largest so far
        string filename_no_ext = filename.substr(0, filename.size() -4);
        int file_num = stoi(filename_no_ext);
		if (file_num > highest_file_num) {
			highest_file_num = file_num;
		}
    }

    closedir(dp);

    if (!csv_files_exist) {
    	return 0;
    }

    return highest_file_num + 1;

	
}

// creates the directory if if doesn't exist
string createDir(string dirname) {
	if (opendir(dirname.c_str()) == NULL) {
		const int dir_err = system(("mkdir -p " + dirname).c_str());
		if (dir_err == -1) {
			throw invalid_argument("Unable to create directory" + dirname);
		}
	}
	return dirname;
}


// creates a /states and an /action_distributions subdirectory for the given directory
pair<string, string> prepareDirectories(string base_dirname) {

	string x_dirname, y_dirname;

	if (base_dirname.back() != '/') {
		x_dirname = base_dirname + "/states/";
		y_dirname = base_dirname + "/action_distributions/";
	} else {
		x_dirname = base_dirname + "states/";
		y_dirname = base_dirname + "action_distributions/";
	}

	// create directories (including necessary parent directories) if they do not exist
	createDir(x_dirname);
	createDir(y_dirname);

	return make_pair(x_dirname, y_dirname);

}



// returns a double vector as a CSV string (for writing action distributions)
string asCSVString(const vector<double>& vec) {
	
	string s = "";
	
	for (int pos = 0; pos < vec.size(); pos++) {
		s += to_string(vec[pos]);
		if (pos != vec.size() - 1) {
			s += ",";
		}
	}
	return s;
}




/****** Other Assorted Utilities *****/


double randomDouble(double lower_bound, double upper_bound) {
	random_device rd;  //Will be used to obtain a seed for the random number engine
	mt19937 rng(rd()); //Standard mersenne_twister_engine seeded with rd()
	
	uniform_real_distribution<> dist(lower_bound, upper_bound);
	return dist(rng);
}


void printVector(const vector<double>& vec, string name) {
	cout << name << ":\t\t\t";
	for (double d : vec) {
		cout << setprecision(3) << d << "\t\t";
	}
	cout << endl;
}


void printVector(const vector<int>& vec, string name) {
	cout << name << ":\t\t\t";
	for (int d : vec) {
		cout << d << "\t\t";
	}
	cout << endl;
}


int sampleProportionalToWeights(const vector<double>& weights) {
	
	vector<double> thresholds(weights.size(), 0);

	double accum_sum = 0;
	for (int pos = 0; pos < weights.size(); pos++) {
		accum_sum += weights[pos];
		thresholds[pos] = accum_sum;
	}

	double r = randomDouble(0, accum_sum);
	for (int pos = 1; pos < weights.size() + 1; pos++) {
		if (r <= thresholds[pos - 1]) {
			return pos - 1;
		}
	} 

	ASSERT(false, "Cannot properly sample a legal action");

}


void computeSoftmaxWithMask(const vector<double>& logits, const vector<bool>& mask, vector<double>* softmaxes) {

	double SOFTMAX_BASE = 2.7; // e?


	ASSERT(softmaxes != NULL, "softmaxes vector cannot be NULL");
	ASSERT(logits.size() == mask.size(), "Logits and mask vectors must be of same size");
	ASSERT(logits.size() <= softmaxes->size(), "Logits vector and softmaxes vector must be of same size");

	vector<double> softmax_numerators(logits.size(), 0.0);
	double softmax_denominator = 0;

	double raw_score, exp_score;

	// determine the softmax numerators for each score
	for (int element_num = 0; element_num < logits.size(); element_num++) {
		
		raw_score = logits[element_num];

		if (mask[element_num]) {
			exp_score = pow(SOFTMAX_BASE, raw_score); // perhaps e is not the best base for softmax samplimg
			softmax_numerators[element_num] = exp_score;
			softmax_denominator += exp_score;
		}
	}

	ASSERT(softmax_denominator != 0.0, "Cannot sample a legal action if all weights are 0");

	// collect the softmax of all the action scores
	for (int element_num = 0; element_num < logits.size(); element_num++) {
		softmaxes->at(element_num) = softmax_numerators[element_num]/ softmax_denominator;
	}
}


int argmaxWithMask(const vector<double>& vals, const vector<bool>& mask) {

	ASSERT(vals.size() == mask.size(), "Vals and mask vectors must be of same size");

	double max_val = -1 * DBL_MAX;
	int max_index = 0;

	for (int element_num = 0; element_num < vals.size(); element_num++) {
		if (vals[element_num] > max_val && mask[element_num]) {
			max_val = vals[element_num];
			max_index = element_num;
		}
	}
	return max_index;

}


void logTime(string message) {
	time_t now = time(NULL);
	tm* ptm = localtime(&now);
	char time_str[32];
	// Format: Mo, 15.06.2009 20:20:00
	strftime(time_str, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);  

	cout << message << ": " << time_str << endl;
}




