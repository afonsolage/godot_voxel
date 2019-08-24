#include "voxel_light_spreader.h"
#include "../cube_tables.h"
#include "../voxel_buffer.h"
#include "../voxel_library.h"

VoxelLightSpreader::VoxelLightSpreader(int thread_count, int block_size_pow2, unsigned int padding, Ref<VoxelLibrary> library) {
	Processor processors[Mgr::MAX_JOBS];

	for (int i = 0; i < thread_count; ++i) {
		Processor &p = processors[i];
		p.max_boundary = Vector3i((1 << block_size_pow2) + padding);
		p.min_boundary = Vector3i(padding);
		p.light_channel = VoxelBuffer::CHANNEL_LIGHT;
		p.type_channel = VoxelBuffer::CHANNEL_TYPE;
		p.library = library;
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

static inline void update_artificial_light(VoxelBuffer &buffer, Vector3i position, int value, int light_channel) {
	int current_voxel_value = buffer.get_voxel(position, light_channel);
	int new_voxel_value = set_art_light(current_voxel_value, value);
	buffer.set_voxel(new_voxel_value, position, light_channel);
	//print_line(String("Updating art light - pos: {0}, {1}, {2} - current: {3} - new value: {4}").format(varray(position.x, position.y, position.z, get_art_light(current_voxel_value), value)));
}

bool VoxelLightSpreader::Processor::is_transparent(const VoxelBuffer &buffer, Vector3i position) {
	CRASH_COND(!library.is_valid());

	int type = buffer.get_voxel(position, type_channel);

	CRASH_COND(!library->has_voxel(type))

	return library->get_voxel_const(type).is_transparent();
}

int VoxelLightSpreader::Processor::get_padded_voxel(const VoxelBuffer &buffer, unsigned int channel, Vector3i position) {
	return buffer.get_voxel(position + padding, channel);
}

void VoxelLightSpreader::Processor::process_block(const InputBlockData &input, OutputBlockData &output, Vector3i block_position, unsigned int lod) {
	print_line(String("Processing block {0}, {1}, {2} [{3}]").format(varray(block_position.x, block_position.y, block_position.z, input.spread_data.size())));

	CRASH_COND(input.voxels.is_null());

	const VoxelBuffer &input_buffer = **input.voxels;
	const uint8_t light_channel = VoxelBuffer::CHANNEL_LIGHT;

	output.voxels.instance();
	output.voxels->create(input_buffer.get_size().x, input_buffer.get_size().y, input_buffer.get_size().z);
	output.voxels->copy_from(input_buffer, light_channel);

	VoxelBuffer &out_buffer = **output.voxels;

	for (int i = 0; i < input.spread_data.size(); i++) {
		const VoxelLightData &data = input.spread_data[i];
		bool is_add_light = data.new_value > 0;
		Vector3i voxel_pos = data.voxel + padding;

		//There are four possible queues: add and remove natural and artifical light
		std::queue<BFSNode> &queue = get_queue(data.type, is_add_light);

		int voxel_light_value = input_buffer.get_voxel(voxel_pos, light_channel);
		int light_value = (data.type == VoxelLightType::ARTIFICIAL) ? get_art_light(voxel_light_value) : get_nat_light(voxel_light_value);

		//print_line(String("is_add_light: {0} - light_value: {1} - data.new_value: {2}").format(varray(is_add_light, light_value, data.new_value)));

		if (is_add_light && data.new_value > light_value) {
			//This is a light addition and new value is greater than current one
			if (data.type == VoxelLightType::ARTIFICIAL) {
				update_artificial_light(out_buffer, voxel_pos, data.new_value, light_channel);
			} else {
				//update_natural_light(out_buffer, voxel_pos, data.new_value, light_channel);
			}
			queue.emplace(voxel_pos, data.new_value - LIGHT_DECREASE_POWER);
		} else if (data.new_value == 0 && light_value > 0) {
			//We don't need to store the new_vale of a light removal, because it's always 0.
			//Instead, we need to store the old value, because we'll need it. Trust me.
			if (data.type == VoxelLightType::ARTIFICIAL) {
				update_artificial_light(out_buffer, voxel_pos, 0, light_channel);
			} else {
				//update_natural_light(out_buffer, voxel_pos, 0, light_channel);
			}
			queue.emplace(voxel_pos, light_value);
		}
	}

	//print_line(String("Add queue: {0} - Remove queue: {1}").format(varray(art_add_queue.size(), art_remove_queue.size())));

	remove_artificial_light(input_buffer, out_buffer);
	add_artificial_light(input_buffer, out_buffer);
}

void VoxelLightSpreader::Processor::add_artificial_light(const VoxelBuffer &input_buffer, VoxelBuffer &buffer) {
	//print_line(String("Adding art light!"));
	while (!art_add_queue.empty()) {
		const BFSNode &node = art_add_queue.front();
		art_add_queue.pop();

		//print_line(String("Updating art light - pos: {0}, {1}, {2} - value: {3}").format(varray(node.position.x, node.position.y, node.position.z, node.value)));

		//Spread light addition over neighbors on all six directions
		for (unsigned int side = 0; side < Cube::SIDE_COUNT; ++side) {
			Vector3i npos = node.position + Cube::g_side_normals[side];

			//Can't spread light to a opaque block
			if (!is_transparent(input_buffer, npos))
				continue;

			//Can't spread to same voxel or to invalid voxel
			if (!npos.is_contained_in(min_boundary, max_boundary)) {
				//TODO: Spread to neighbors
				continue;
			}

			//Only spread if it's value is greater
			if (node.value <= get_art_light(buffer.get_voxel(npos, light_channel))) {
				continue;
			}

			update_artificial_light(buffer, npos, node.value, light_channel);

			//Only spread to neighbors if the light has enough power left
			if (node.value > LIGHT_DECREASE_POWER) {
				art_add_queue.emplace(npos, node.value - LIGHT_DECREASE_POWER);
			}
		}
	}
}

void VoxelLightSpreader::Processor::remove_artificial_light(const VoxelBuffer &input_buffer, VoxelBuffer &buffer) {
	//print_line(String("Removing art light!"));
	while (!art_remove_queue.empty()) {
		const BFSNode &node = art_remove_queue.front();
		art_remove_queue.pop();

		//We need to check all neighbors if the old light is greater than their, we add them to light removal queue
		//Else if their light value is greater than current node old value, we add this voxel to add light queue, reducing light by 1
		for (unsigned int side = 0; side < Cube::SIDE_COUNT; ++side) {
			Vector3i npos = node.position + Cube::g_side_normals[side];

			if (!npos.is_contained_in(min_boundary, max_boundary)) {
				//print_line(String("Out of bounds: {0}, {1}, {2} - {3}").format(varray(npos.x, npos.y, npos.z, node.value)));
				//TODO: Spread to neighbors
				continue;
			}
			
			int neighbor_light = get_art_light(buffer.get_voxel(npos, light_channel));

			if (neighbor_light < LIGHT_DECREASE_POWER) {
				//print_line(String("Light too low: {0}, {1}, {2} - {3}").format(varray(npos.x, npos.y, npos.z, neighbor_light)));
				continue;
			}

			if (neighbor_light < node.value) {
				//print_line(String("Removing	art light: {0}, {1}, {2} - {3}").format(varray(npos.x, npos.y, npos.z, neighbor_light)));
				update_artificial_light(buffer, npos, 0, light_channel);
				art_remove_queue.emplace(npos, neighbor_light);
			} else if (neighbor_light >= node.value) {
				//print_line(String("Propagating art light: {0}, {1}, {2} - {3}").format(varray(npos.x, npos.y, npos.z, neighbor_light)));
				art_add_queue.emplace(npos, neighbor_light - LIGHT_DECREASE_POWER);
			}
		}
	}
}