#include "voxel_light_spreader.h"
#include "../voxel_buffer.h"

VoxelLightSpreader::VoxelLightSpreader(int thread_count, int block_size_pow2, unsigned int padding) {
	Processor processors[Mgr::MAX_JOBS];

	for (int i = 0; i < thread_count; ++i) {
		Processor &p = processors[i];
		p.block_size_pow2 = block_size_pow2;
		p.light_channel = VoxelBuffer::CHANNEL_LIGHT;
		p.padding = padding;
	}

	_mgr = memnew(Mgr(thread_count, 500, processors));
	_padding = padding;
}

VoxelLightSpreader::~VoxelLightSpreader() {
	if (_mgr) {
		memdelete(_mgr);
	}
}

void VoxelLightSpreader::Processor::process_block(const InputBlockData &input, OutputBlockData &output, Vector3i block_position, unsigned int lod) {
	print_line(String("Processing block {0}, {1}, {2}").format(varray(block_position.x, block_position.y, block_position.z)));

	CRASH_COND(input.voxels.is_null());

	const VoxelBuffer &input_buffer = **input.voxels;
	Ref<VoxelBuffer> out_buffer;

	const uint8_t light_channel = VoxelBuffer::CHANNEL_LIGHT;

	out_buffer.instance();
	out_buffer->create(input_buffer.get_size().x, input_buffer.get_size().y, input_buffer.get_size().z);
	out_buffer->copy_from(input_buffer, light_channel);

	std::queue<BFSNode> art_remove_queue;
	std::queue<BFSNode> art_add_queue;

	for (int i = 0; i < input.spread_data.size(); i++) {
		const VoxelLightData &data = input.spread_data[i];
		bool is_add_light = data.new_value > 0;

		//There are four possible queues: add and remove natural and artifical light
		std::queue<BFSNode> queue = get_queue(data.type, is_add_light);

		int voxel_light_value = input_buffer.get_voxel(data.voxel, light_channel);
		int light_value = (data.type == VoxelLightType::ARTIFICIAL) ? get_art_light(voxel_light_value) : get_nat_light(voxel_light_value);

		if (is_add_light && data.new_value > light_value) {
			//This is a light addition and new value is greater than current one
			queue.emplace(BFSNode{ data.voxel, data.new_value });
		} else if (data.new_value < light_value) {
			//We don't need to store the new_vale of a light removal, because it's always 0.
			//Instead, we need to store the old value, because we'll need it. Trust me.
			queue.emplace(BFSNode{ data.voxel, light_value });
		}
	}

	remove_artificial_light(**out_buffer);
	add_artificial_light(**out_buffer);

	output.voxels = out_buffer;
}

static inline void update_artificial_light(VoxelBuffer &buffer, Vector3i position, int value, int light_channel) {
	int current_voxel_value = buffer.get_voxel(position, light_channel);
	int new_voxel_value = set_art_light(current_voxel_value, value);
	buffer.set_voxel(new_voxel_value, position, light_channel);
}

void VoxelLightSpreader::Processor::add_artificial_light(VoxelBuffer &buffer) {
	int max = 1 << block_size_pow2;

	while (!art_add_queue.empty()) {
		BFSNode &node = art_add_queue.front();
		art_add_queue.pop();

		update_artificial_light(buffer, node.position, node.value, light_channel);

		//When spreading light it loses it's power by LIGHT_DECREASE_POWER
		int new_value = node.value - LIGHT_DECREASE_POWER;

		//Only spread to neighbors if the light has enough power left
		if (new_value > LIGHT_DECREASE_POWER) {
			continue;
		}

		//Spread light addition over neighbors on all six directions
		Vector3i ndir;
		for (ndir.z = -1; ndir.z < 2; ++ndir.z) {
			for (ndir.x = -1; ndir.x < 2; ++ndir.x) {
				for (ndir.y = -1; ndir.y < 2; ++ndir.y) {
					Vector3i npos = node.position + ndir;

					//Can't spread to same voxel or to invalid voxel
					if (npos.x < 0 || npos.x >= max || npos.y < 0 || npos.y >= max || npos.z < 0 || npos.z >= max || npos == node.position) {
						//TODO: Spread to neighbors
						continue;
					}

					//Only spread if it's value is greater
					if (new_value <= get_art_light(buffer.get_voxel(npos, light_channel))) {
						continue;
					}

					art_add_queue.emplace(BFSNode{ npos, new_value });
				}
			}
		}
	}
}

void VoxelLightSpreader::Processor::remove_artificial_light(VoxelBuffer &buffer) {
	int max = 1 << block_size_pow2;

	while (!art_remove_queue.empty()) {
		BFSNode &node = art_remove_queue.front();
		art_remove_queue.pop();

		update_artificial_light(buffer, node.position, 0, light_channel);

		//We need to check all neighbors if the old light is greater than their, we add them to light removal queue
		//Else if their light value is greater than current node old value, we add this voxel to add light queue, reducing light by 1
		Vector3i ndir;
		for (ndir.z = -1; ndir.z < 2; ++ndir.z) {
			for (ndir.x = -1; ndir.x < 2; ++ndir.x) {
				for (ndir.y = -1; ndir.y < 2; ++ndir.y) {
					Vector3i npos = node.position + ndir;

					if (npos.x < 0 || npos.x >= max || npos.y < 0 || npos.y >= max || npos.z < 0 || npos.z >= max || npos == node.position) {
						//TODO: Spread to neighbors
						continue;
					}

					int neighbor_light = get_art_light(buffer.get_voxel(npos, light_channel));

					if (neighbor_light < node.value) {
						art_remove_queue.emplace(BFSNode{ npos, neighbor_light });
					} else if (neighbor_light > LIGHT_DECREASE_POWER) {
						art_add_queue.emplace(BFSNode{ npos, neighbor_light - LIGHT_DECREASE_POWER });
					}
				}
			}
		}
	}
}