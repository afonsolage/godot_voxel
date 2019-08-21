#ifndef VOXEL_LIGHT_SPREADER_H
#define VOXEL_LIGHT_SPREADER_H

#include "queue"
#include "voxel_light.h"
#include "../terrain/block_thread_manager.h"

class VoxelBuffer;

class VoxelLightSpreader {
public:
	struct InputBlockData {
		Ref<VoxelBuffer> voxels;
		Vector<VoxelLightData> spread_data;
	};

	struct OutputBlockData {
		Ref<VoxelBuffer> voxels;
		Vector<HashMap<Vector3i, VoxelLightData> > affected_blocks; //Block neighbors that were affect by this light spreader and which need to be updated
	};

	struct Processor {
		struct BFSNode {
			Vector3i position;
			int value;
		};

		void process_block(const InputBlockData &input, OutputBlockData &output, Vector3i block_position, unsigned int log);

		int block_size_pow2 = 0;
		int light_channel = 0;
		unsigned int padding = 0;

		std::queue<BFSNode> art_add_queue;
		std::queue<BFSNode> art_remove_queue;
		std::queue<BFSNode> nat_add_queue;
		std::queue<BFSNode> nat_remove_queue;

	private:
		void add_artificial_light(VoxelBuffer &buffer);
		void remove_artificial_light(VoxelBuffer &buffer);
		void add_natural_light(VoxelBuffer &buffer);
		void remove_natural_light(VoxelBuffer &buffer);

		inline std::queue<BFSNode> &get_queue(const int light_type, const bool is_add) {
			if (light_type == VoxelLightType::ARTIFICIAL)
				return (is_add) ? art_add_queue : art_remove_queue;
			else
				return (is_add) ? nat_add_queue : nat_remove_queue;
		}
	};

public:
	typedef VoxelBlockThreadManager<InputBlockData, OutputBlockData, Processor> Mgr;
	typedef Mgr::InputBlock InputBlock;
	typedef Mgr::OutputBlock OutputBlock;
	typedef Mgr::Input Input;
	typedef Mgr::Output Output;
	typedef Mgr::Stats Stats;
	typedef Processor::BFSNode BFSNode;

	VoxelLightSpreader(int thread_count, int block_size_pow2, unsigned int padding);
	~VoxelLightSpreader();

	void push(const Input &input) { _mgr->push(input); }
	void pop(Output &output) { _mgr->pop(output); }

	unsigned int get_padding() const { return _padding; }

private:
	Mgr *_mgr = nullptr;
	unsigned int _padding = 0;
};

#endif