#!/usr/bin/env python
import sys

def main():
	state_vector_file = open(sys.argv[1])
	action_distribution_file = open(sys.argv[2], 'w')

	for line in state_vector_file:
		action_distribution_file.write(line)
	
	state_vector_file.close()
	action_distribution_file.close()


if __name__ == '__main__':
	main()

