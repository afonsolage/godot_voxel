#ifndef VOXEL_LIGHT_SPREADER_H
#define VOXEL_LIGHT_SPREADER_H

#include "../../terrain/block_thread_manager.h"

class VoxelBuffer;

class VoxelLightSpreader {
public:
	struct InputBlockData {
		Ref<VoxelBuffer> voxels;
		//Input light data
	};

	struct OutputBlockData {
		Ref<VoxelBuffer> voxels;
		//Output block pos that need to be light updated
	};

	struct Processor {
		void process_block(const InputBlockData &input, OutputBlockData &output, Vector3i block_position, unsigned int log);

		int block_size_pow2 = 0;
	};

	typedef VoxelBlockThreadManager<InputBlockData, OutputBlockData, Processor> Mgr;
	typedef Mgr::InputBlock InputBlock;
	typedef Mgr::OutputBlock OutputBlock;
	typedef Mgr::Input Input;
	typedef Mgr::Output Output;
	typedef Mgr::Stats Stats;

	VoxelLightSpreader(int thread_count, int block_size_pow2);
	~VoxelLightSpreader();

	void push(const Input &input) { _mgr->push(input); }
	void pop(Output &output) { _mgr->pop(output); }

private:
	Mgr *_mgr = nullptr;
};

#endif