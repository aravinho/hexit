#include "env_state.h"
#include "hex_state.h"
#include <string>
#include <vector>

using namespace std;

EnvState* stateFromCSVString(string game, string csv_string, const ArgMap& options) {

	if ("game == hex") {
		ASSERT(options.contains("hex_dim"), "Options map must contain hex_dim");
		ASSERT(options.contains("hex_reward_type"), "Options map must contain hex_reward_type");

		int hex_dim = options.getInt("hex_dim");
		string hex_reward_type = options.getString("hex_reward_type");
		vector<int> board = readCSVString(csv_string);

		return new HexState(hex_dim, board, hex_reward_type);
	}

	ASSERT(false, "For now, the only game supported is hex");

}


EnvState* initialState(string game, const ArgMap& options) {

	if ("game == hex") {
		int hex_dim = options.getInt("hex_dim", DEFAULT_HEX_DIM);
		string hex_reward_type = options.getString("hex_reward_type", DEFAULT_HEX_REWARD_TYPE);
		vector<int> board(pow(hex_dim, 2), 0);
		return new HexState(hex_dim, board, hex_reward_type);
	}

	ASSERT(false, "For now, the only game supported is hex");

}




