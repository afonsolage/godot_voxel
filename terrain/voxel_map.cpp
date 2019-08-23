#include "voxel_map.h"
#include "../cube_tables.h"
#include "voxel_block.h"

#include "core/os/os.h"

VoxelMap::VoxelMap() :
		_last_accessed_block(NULL) {

	// TODO Make it configurable in editor (with all necessary notifications and updatings!)
	set_block_size_pow2(DEFAULT_BLOCK_SIZE_PO2);

	for (unsigned int i = 0; i < VoxelBuffer::MAX_CHANNELS; ++i) {
		_default_voxel[i] = 0;
	}

	_default_voxel[VoxelBuffer::CHANNEL_ISOLEVEL] = 255;
}

VoxelMap::~VoxelMap() {
	print_line("Destroying VoxelMap");
	clear();
}

void VoxelMap::set_block_size_pow2(unsigned int p) {

	// Limit to 8, 16, 32
	ERR_FAIL_COND(p < 3 || p > 5);

	_block_size_pow2 = p;
	_block_size = 1 << _block_size_pow2;
	_block_size_mask = _block_size - 1;
}

void VoxelMap::set_lod_index(int lod) {
	_lod_index = lod;
}

unsigned int VoxelMap::get_lod_index() const {
	return _lod_index;
}

int VoxelMap::get_voxel(Vector3i pos, unsigned int c) {
	Vector3i bpos = voxel_to_block(pos);
	VoxelBlock *block = get_block(bpos);
	if (block == NULL) {
		return _default_voxel[c];
	}
	return block->voxels->get_voxel(to_local(pos), c);
}

VoxelBlock *VoxelMap::get_or_create_block_at_voxel_pos(Vector3i pos) {

	Vector3i bpos = voxel_to_block(pos);
	VoxelBlock *block = get_block(bpos);

	if (block == NULL) {

		Ref<VoxelBuffer> buffer(memnew(VoxelBuffer));
		buffer->create(_block_size, _block_size, _block_size);
		buffer->set_default_values(_default_voxel);

		block = VoxelBlock::create(bpos, buffer, _block_size, _lod_index);

		set_block(bpos, block);
	}

	return block;
}

void VoxelMap::set_voxel(int value, Vector3i pos, unsigned int c) {

	VoxelBlock *block = get_or_create_block_at_voxel_pos(pos);
	block->voxels->set_voxel(value, to_local(pos), c);
}

float VoxelMap::get_voxel_f(int x, int y, int z, unsigned int c) {

	Vector3i pos(x, y, z);
	Vector3i bpos = voxel_to_block(pos);
	VoxelBlock *block = get_block(bpos);
	if (block == NULL) {
		return _default_voxel[c];
	}
	Vector3i lpos = to_local(pos);
	return block->voxels->get_voxel_f(lpos.x, lpos.y, lpos.z, c);
}

void VoxelMap::set_voxel_f(real_t value, int x, int y, int z, unsigned int c) {

	Vector3i pos(x, y, z);
	VoxelBlock *block = get_or_create_block_at_voxel_pos(pos);
	Vector3i lpos = to_local(pos);
	block->voxels->set_voxel_f(value, lpos.x, lpos.y, lpos.z, c);
}

void VoxelMap::set_default_voxel(int value, unsigned int channel) {
	ERR_FAIL_INDEX(channel, VoxelBuffer::MAX_CHANNELS);
	_default_voxel[channel] = value;
}

int VoxelMap::get_default_voxel(unsigned int channel) {
	ERR_FAIL_INDEX_V(channel, VoxelBuffer::MAX_CHANNELS, 0);
	return _default_voxel[channel];
}

VoxelBlock *VoxelMap::get_block(Vector3i bpos) {
	if (_last_accessed_block && _last_accessed_block->position == bpos) {
		return _last_accessed_block;
	}
	VoxelBlock **p = _blocks.getptr(bpos);
	if (p) {
		_last_accessed_block = *p;
		CRASH_COND(_last_accessed_block == NULL); // The map should not contain null blocks
		return _last_accessed_block;
	}
	return NULL;
}

const VoxelBlock *VoxelMap::get_block(Vector3i bpos) const {
	if (_last_accessed_block && _last_accessed_block->position == bpos) {
		return _last_accessed_block;
	}
	const VoxelBlock *const *p = _blocks.getptr(bpos);
	if (p) {
		const VoxelBlock *block = *p;
		CRASH_COND(block == NULL); // The map should not contain null blocks
		return block;
	}
	return NULL;
}

void VoxelMap::set_block(Vector3i bpos, VoxelBlock *block) {
	ERR_FAIL_COND(block == NULL);
	if (_last_accessed_block == NULL || _last_accessed_block->position == bpos) {
		_last_accessed_block = block;
	}
	_blocks.set(bpos, block);
}

void VoxelMap::remove_block_internal(Vector3i bpos) {
	// This function assumes the block is already freed
	_blocks.erase(bpos);
}

VoxelBlock *VoxelMap::set_block_buffer(Vector3i bpos, Ref<VoxelBuffer> buffer) {
	ERR_FAIL_COND_V(buffer.is_null(), nullptr);
	VoxelBlock *block = get_block(bpos);
	if (block == NULL) {
		block = VoxelBlock::create(bpos, *buffer, _block_size, _lod_index);
		set_block(bpos, block);
	} else {
		block->voxels = buffer;
	}
	return block;
}

void VoxelMap::set_block_channel_buffer(Vector3i bpos, const Ref<VoxelBuffer> buffer, unsigned int channel, unsigned int buffer_padding) {
	ERR_FAIL_COND(buffer.is_null());
	VoxelBlock *block = get_block(bpos);
	if (block == NULL) {
		//TODO: Create a pool for voxel buffers?
		Ref<VoxelBuffer> voxels;
		voxels.instance();
		voxels->create(buffer->get_size().x, buffer->get_size().y, buffer->get_size().z);
		block = VoxelBlock::create(bpos, *voxels, _block_size, _lod_index);
		set_block(bpos, block);
	}
	if (buffer_padding == 0) {
		block->voxels->copy_from(**buffer, channel);
	} else {
		block->voxels->copy_from(**buffer, Vector3i(buffer_padding), buffer->get_size() - buffer_padding, Vector3i(), channel);
	}
}

bool VoxelMap::has_block(Vector3i pos) const {
	return /*(_last_accessed_block != NULL && _last_accessed_block->pos == pos) ||*/ _blocks.has(pos);
}

bool VoxelMap::is_block_surrounded(Vector3i pos) const {
	// TODO If that check proves to be too expensive with all blocks we deal with, cache it in VoxelBlocks
	for (unsigned int i = 0; i < Cube::MOORE_NEIGHBORING_3D_COUNT; ++i) {
		Vector3i bpos = pos + Cube::g_moore_neighboring_3d[i];
		if (!has_block(bpos)) {
			return false;
		}
	}
	return true;
}

void VoxelMap::get_buffer_copy(Vector3i min_pos, VoxelBuffer &dst_buffer, unsigned int channels_mask) {

	Vector3i max_pos = min_pos + dst_buffer.get_size();

	Vector3i min_block_pos = voxel_to_block(min_pos);
	Vector3i max_block_pos = voxel_to_block(max_pos - Vector3i(1, 1, 1)) + Vector3i(1, 1, 1);
	// TODO Why is this function limited by this check?
	ERR_FAIL_COND((max_block_pos - min_block_pos) != Vector3i(3, 3, 3));

	const Vector3i block_size_v(_block_size, _block_size, _block_size);

	for (unsigned int channel = 0; channel < VoxelBuffer::MAX_CHANNELS; ++channel) {

		if (((1 << channel) & channels_mask) == 0) {
			continue;
		}

		Vector3i bpos;
		for (bpos.z = min_block_pos.z; bpos.z < max_block_pos.z; ++bpos.z) {
			for (bpos.x = min_block_pos.x; bpos.x < max_block_pos.x; ++bpos.x) {
				for (bpos.y = min_block_pos.y; bpos.y < max_block_pos.y; ++bpos.y) {

					VoxelBlock *block = get_block(bpos);
					if (block) {

						VoxelBuffer &src_buffer = **block->voxels;
						Vector3i offset = block_to_voxel(bpos);
						// Note: copy_from takes care of clamping the area if it's on an edge
						dst_buffer.copy_from(src_buffer,
								min_pos - offset,
								max_pos - offset,
								offset - min_pos,
								channel);

					} else {
						Vector3i offset = block_to_voxel(bpos);
						dst_buffer.fill_area(
								_default_voxel[channel],
								offset - min_pos,
								offset - min_pos + block_size_v,
								channel);
					}
				}
			}
		}
	}
}

void VoxelMap::clear() {
	const Vector3i *key = NULL;
	while ((key = _blocks.next(key))) {
		VoxelBlock *block_ptr = _blocks.get(*key);
		if (block_ptr == NULL) {
			OS::get_singleton()->printerr("Unexpected NULL in VoxelMap::clear()");
		}
		memdelete(block_ptr);
	}
	_blocks.clear();
	_last_accessed_block = NULL;
}

int VoxelMap::get_block_count() const {
	return _blocks.size();
}

void VoxelMap::_bind_methods() {

	ClassDB::bind_method(D_METHOD("get_voxel", "x", "y", "z", "c"), &VoxelMap::_get_voxel_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_voxel", "value", "x", "y", "z", "c"), &VoxelMap::_set_voxel_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_voxel_f", "x", "y", "z", "c"), &VoxelMap::get_voxel_f, DEFVAL(VoxelBuffer::CHANNEL_ISOLEVEL));
	ClassDB::bind_method(D_METHOD("set_voxel_f", "value", "x", "y", "z", "c"), &VoxelMap::set_voxel_f, DEFVAL(VoxelBuffer::CHANNEL_ISOLEVEL));
	ClassDB::bind_method(D_METHOD("get_voxel_v", "pos", "c"), &VoxelMap::_get_voxel_v_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_voxel_v", "value", "pos", "c"), &VoxelMap::_set_voxel_v_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_default_voxel", "channel"), &VoxelMap::get_default_voxel, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_default_voxel", "value", "channel"), &VoxelMap::set_default_voxel, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("has_block", "x", "y", "z"), &VoxelMap::_has_block_binding);
	ClassDB::bind_method(D_METHOD("get_buffer_copy", "min_pos", "out_buffer", "channel"), &VoxelMap::_get_buffer_copy_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_block_buffer", "block_pos", "buffer"), &VoxelMap::_set_block_buffer_binding);
	ClassDB::bind_method(D_METHOD("voxel_to_block", "voxel_pos"), &VoxelMap::_voxel_to_block_binding);
	ClassDB::bind_method(D_METHOD("block_to_voxel", "block_pos"), &VoxelMap::_block_to_voxel_binding);
	ClassDB::bind_method(D_METHOD("get_block_size"), &VoxelMap::get_block_size);

	//ADD_PROPERTY(PropertyInfo(Variant::INT, "iterations"), _SCS("set_iterations"), _SCS("get_iterations"));
}

void VoxelMap::_get_buffer_copy_binding(Vector3 pos, Ref<VoxelBuffer> dst_buffer_ref, unsigned int channel) {
	ERR_FAIL_COND(dst_buffer_ref.is_null());
	get_buffer_copy(Vector3i(pos), **dst_buffer_ref, channel);
}
