import random

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

		assert not required, "The required key " + key + " is not present (getBoolean)"
		return default_val

	def contains(self, key):
		"""
		Returns whether this map contains the given key.
		"""
		return key in self._arg_map





# def parseArgs(args):
#     """
#     The given argument "args" is an array (typically just sys.argv).
#     It is assumed that the arguments come in pairs.
#     The first element of a pair is of the format --option.
#     The second element of the pair is a value (either a string or an int)
#     This function returns a dictionary whose keys are the option elements, and whose values are the values.

#     Example:

#     args = ["--game", "hex", "--num_games", "100"]
#     Returns {"game": "hex", "--num_games": "100"}

#     """

#     arg_dict = {}

#     # make sure the given args array has an even length
#     assert len(args) % 2 == 0, "Args must have an even length"

#     for pair_num in range(len(args) // 2):
#         option = args[pair_num * 2]
#         assert len(option) > 2 and option[0:2] == "--", "Arg options must start with double hyphen"
#         option = option[2:]
#         value = args[pair_num * 2 + 1]
#         arg_dict[option] = value

#     return arg_dict

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


def decision(p):
	"""
	Returns True with probability p, and False with probability 1 - p
	"""
	return random.random() <= p