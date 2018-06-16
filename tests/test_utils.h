#ifndef TEST_UTILS_H

using namespace std;

void assertWithMessage(bool cond, string test_file, string test_name, string message) {
	if (!cond) {
		cout << "ASSERTION FAILED: " << test_file << "::" << test_name << " -- " << message << endl;
		//exit(1);
	}	
}


#endif
