#ifndef VOXEL_LIGHT_SPREADER_H
#define VOXEL_LIGHT_SPREADER_H

#include "../terrain/block_thread_manager.h"
#include "queue"
#include "voxel_light.h"

class VoxelBuffer;

class VoxelLightSpreader {
public:
	struct InputBlockData {
		Ref<VoxelBuffer> voxels;
		std::vector<VoxelLightData> spread_data;
	};

	struct OutputBlockData {
		Ref<VoxelBuffer> voxels;
		std::vector<Vector3i> affected_blocks;
	};

	struct Processor {
		struct BFSNode {
			Vector3i position;
			int value;
		};

		void process_block(const InputBlockData &input, OutputBlockData &output, Vector3i block_position, unsigned int log);

		int block_size_pow2 = 0;
		int light_channel = 0;

		std::queue<BFSNode> art_add_queue;
		std::queue<BFSNode> art_remove_queue;
		std::queue<BFSNode> nat_add_queue;
		std::queue<BFSNode> nat_remove_queue;

	private:
		void add_artificial_light(VoxelBuffer &buffer);
		void remove_artificial_light(VoxelBuffer &buffer);
		void add_natural_light(VoxelBuffer &buffer);
		void remove_natural_light(VoxelBuffer &buffer);

		inline std::queue<BFSNode> &get_queue(const int light_type, const bool id_add) {
			if (light_type == VoxelLightType::ARTIFICIAL)
				return (id_add) ? art_add_queue : art_remove_queue;
			else
				(id_add) ? nat_add_queue : nat_remove_queue;
		}
	};

	typedef VoxelBlockThreadManager<InputBlockData, OutputBlockData, Processor> Mgr;
	typedef Mgr::InputBlock InputBlock;
	typedef Mgr::OutputBlock OutputBlock;
	typedef Mgr::Input Input;
	typedef Mgr::Output Output;
	typedef Mgr::Stats Stats;
	typedef Processor::BFSNode BFSNode;

	VoxelLightSpreader(int thread_count, int block_size_pow2);
	~VoxelLightSpreader();

	void push(const Input &input) { _mgr->push(input); }
	void pop(Output &output) { _mgr->pop(output); }

private:
	Mgr *_mgr = nullptr;
};

#endif