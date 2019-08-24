#ifndef VOXEL_LIGHT_SPREADER_H
#define VOXEL_LIGHT_SPREADER_H

#include "../terrain/block_thread_manager.h"
#include "queue"
#include "voxel_light.h"

class VoxelLibrary;
class VoxelBuffer;

class VoxelLightSpreader {
public:
	struct InputBlockData {
		Ref<VoxelBuffer> voxels;
		Vector<VoxelLightData> spread_data;
	};

	struct OutputBlockData {
		Ref<VoxelBuffer> voxels;
		VoxelLightMap affected_blocks; //Block neighbors that were affect by this light spreader and which need to be updated
	};

	struct Processor {
		struct BFSNode {
			Vector3i position;
			unsigned int value;

			BFSNode(Vector3i pos, int val) {
				position = pos;
				value = val;
			}
		};

		void process_block(const InputBlockData &input, OutputBlockData &output, Vector3i block_position, unsigned int log);

		Vector3i min_boundary;
		Vector3i max_boundary;
		unsigned int light_channel = 0;
		unsigned int type_channel = 0;
		unsigned int padding = 0;

		Ref<VoxelLibrary> library;

		std::queue<BFSNode> art_add_queue;
		std::queue<BFSNode> art_remove_queue;
		std::queue<BFSNode> nat_add_queue;
		std::queue<BFSNode> nat_remove_queue;

	private:
		void add_artificial_light(const VoxelBuffer &input_buffer, VoxelBuffer &buffer, Vector3i block_position, VoxelLightMap &affected_blocks);
		void remove_artificial_light(const VoxelBuffer &input_buffer, VoxelBuffer &buffer, Vector3i block_position, VoxelLightMap &affected_blocks);

		void add_natural_light(VoxelBuffer &buffer);
		void remove_natural_light(VoxelBuffer &buffer);

		int get_padded_voxel(const VoxelBuffer &buffer, unsigned int channel, Vector3i position);
		bool is_transparent(const VoxelBuffer &buffer, Vector3i position);

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

	VoxelLightSpreader(int thread_count, int block_size_pow2, unsigned int padding, Ref<VoxelLibrary> library);
	~VoxelLightSpreader();

	void push(const Input &input) { _mgr->push(input); }
	void pop(Output &output) { _mgr->pop(output); }

	unsigned int get_padding() const { return _padding; }

private:
	Mgr *_mgr = nullptr;
	unsigned int _padding = 0;
};

#endif