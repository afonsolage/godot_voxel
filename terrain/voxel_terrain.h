#ifndef VOXEL_TERRAIN_H
#define VOXEL_TERRAIN_H

#include "../math/rect3i.h"
#include "../math/vector3i.h"
#include "../streams/voxel_stream.h"
#include "../util/zprofiling.h"
#include "voxel_data_loader.h"
#include "voxel_mesh_updater.h"
#include "../light/voxel_light_spreader.h"

#include <scene/3d/spatial.h>

class VoxelMap;
class VoxelLibrary;
class VoxelLightSpreader;

// Infinite paged terrain made of voxel blocks all with the same level of detail.
// Voxels are polygonized around the viewer by distance in a large cubic space.
// Data is streamed using a VoxelStream.
class VoxelTerrain : public Spatial {
	GDCLASS(VoxelTerrain, Spatial)
public:
	// TODO Remove this, it's obsoleted by VoxelBlock::MeshState
	enum BlockDirtyState {
		BLOCK_NONE, // There is no block
		BLOCK_LOAD, // The block is loading
		BLOCK_LIGHT_NOT_SENT, // The block needs to spread light addition or removal but wasn't sent yet
		BLOCK_LIGHT_SENT, // the block needs to spread light addition or removal which was sent
		BLOCK_UPDATE_NOT_SENT, // The block needs an update but wasn't sent yet
		BLOCK_UPDATE_SENT, // The block needs an update which was sent
		BLOCK_IDLE // The block is up to date
	};

	VoxelTerrain();
	~VoxelTerrain();

	void set_stream(Ref<VoxelStream> stream);
	Ref<VoxelStream> get_stream() const;

	void set_voxel_library(Ref<VoxelLibrary> library);
	Ref<VoxelLibrary> get_voxel_library() const;

	void set_voxel_natural_light(Vector3 pos, int new_value);
	void set_voxel_artificial_light(Vector3 pos, int new_value);
	void _set_voxel_light(VoxelLightType type, Vector3 pos, int new_value);

	void make_block_dirty(Vector3i bpos);
	//void make_blocks_dirty(Vector3i min, Vector3i size);
	void make_voxel_dirty(Vector3i pos);
	void make_area_dirty(Rect3i box);
	bool is_block_dirty(Vector3i bpos) const;

	void set_generate_collisions(bool enabled);
	bool get_generate_collisions() const { return _generate_collisions; }

	void set_light_enabled(bool enabled);
	bool get_light_enabled() const { return _light_enabled; }

	int get_view_distance() const;
	void set_view_distance(int distance_in_voxels);

	void set_viewer_path(NodePath path);
	NodePath get_viewer_path() const;

	void set_material(unsigned int id, Ref<Material> material);
	Ref<Material> get_material(unsigned int id) const;

	bool is_smooth_meshing_enabled() const;
	void set_smooth_meshing_enabled(bool enabled);

	Ref<VoxelMap> get_map() { return _map; }

	struct Stats {
		VoxelMeshUpdater::Stats updater;
		VoxelLightSpreader::Stats light_spreader;
		VoxelDataLoader::Stats stream;
		uint32_t mesh_alloc_time;
		int updated_blocks;
		int spread_blocks;
		int dropped_stream_blocks;
		int dropped_updater_blocks;
		int remaining_main_thread_blocks;
		uint64_t time_detect_required_blocks;
		uint64_t time_send_load_requests;
		uint64_t time_process_load_responses;
		uint64_t time_send_light_requests;
		uint64_t time_process_light_responses;
		uint64_t time_send_update_requests;
		uint64_t time_process_update_responses;

		Stats() :
				mesh_alloc_time(0),
				updated_blocks(0),
				dropped_stream_blocks(0),
				dropped_updater_blocks(0),
				remaining_main_thread_blocks(0),
				time_detect_required_blocks(0),
				time_send_load_requests(0),
				time_process_load_responses(0),
				time_send_update_requests(0),
				time_process_update_responses(0) {}
	};

protected:
	void _notification(int p_what);

private:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	void _process();

	void make_all_view_dirty_deferred();
	void reset_updater();

	Spatial *get_viewer(NodePath path) const;

	void immerge_block(Vector3i bpos);

	Dictionary get_statistics() const;

	static void _bind_methods();

	// Convenience
	Vector3 _voxel_to_block_binding(Vector3 pos);
	Vector3 _block_to_voxel_binding(Vector3 pos);
	//void _force_load_blocks_binding(Vector3 center, Vector3 extents) { force_load_blocks(center, extents); }
	void _make_voxel_dirty_binding(Vector3 pos) { make_voxel_dirty(pos); }
	void _make_area_dirty_binding(AABB aabb);
	Variant _raycast_binding(Vector3 origin, Vector3 direction, real_t max_distance);
	void set_voxel(Vector3 pos, int value, int c);
	int get_voxel(Vector3 pos, int c);
	BlockDirtyState get_block_state(Vector3 p_bpos) const;

private:
	// Voxel storage
	Ref<VoxelMap> _map;

	// How many blocks to load around the viewer
	int _view_distance_blocks;

	bool _lighting_enabled;

	// TODO Terrains only need to handle the visible portion of voxels, which reduces the bounds blocks to handle.
	// Therefore, could a simple grid be better to use than a hashmap?


	HashMap<Vector3i, Vector<VoxelLightData>, Vector3iHasher> _pending_light_data;

	Vector<Vector3i> _blocks_pending_load;
	Vector<Vector3i> _blocks_pending_update;
	HashMap<Vector3i, BlockDirtyState, Vector3iHasher> _dirty_blocks; // TODO Rename _block_states
	Vector<VoxelMeshUpdater::OutputBlock> _blocks_pending_main_thread_update;

	Ref<VoxelStream> _stream;
	VoxelDataLoader *_stream_thread;

	Ref<VoxelLibrary> _library;
	VoxelMeshUpdater *_block_updater;

	VoxelLightSpreader *_light_spreader;

	NodePath _viewer_path;
	Vector3i _last_viewer_block_pos;
	int _last_view_distance_blocks;

	bool _generate_collisions;
	bool _run_in_editor;
	bool _smooth_meshing_enabled;
	bool _light_enabled;

	Ref<Material> _materials[VoxelMesherBlocky::MAX_MATERIALS];

	Stats _stats;
};

VARIANT_ENUM_CAST(VoxelTerrain::BlockDirtyState)

#endif // VOXEL_TERRAIN_H
