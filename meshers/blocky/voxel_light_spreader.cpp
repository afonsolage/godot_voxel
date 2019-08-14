#include "voxel_light_spreader.h"
#include "../../voxel_buffer.h"

VoxelLightSpreader::VoxelLightSpreader(int thread_count, int block_size_pow2) {
    
    Processor processors[Mgr::MAX_JOBS];

    for (int i = 0; i < thread_count; ++i) {
        Processor &p = processors[i];
        p.block_size_pow2 = block_size_pow2;
    }

    _mgr = memnew(Mgr(thread_count, 500, processors, true));
} 

VoxelLightSpreader::~VoxelLightSpreader() {
    if (_mgr) {
        memdelete(_mgr);
    }
}

void VoxelLightSpreader::Processor::process_block(const InputBlockData &input, OutputBlockData &poutput, Vector3i block_position, unsigned int lod) {
    //TODO: Do the magic!
}