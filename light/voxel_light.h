#ifndef VOXEL_LIGHT_H
#define VOXEL_LIGHT_H

#define LIGHT_MAX_VALUE 15

#define NATURAL_LIGHT_MASK 0xF0
#define ARTIFICIAL_LIGHT_MASK 0x0F

#define LIGHT_DECREASE_POWER 1

enum VoxelLightType {
	NATURAL, //SUN, MOON, ETC
	ARTIFICIAL, //TORCH, LAMP, ETC
};

struct VoxelLightData {
	VoxelLightType type;
	uint8_t new_value;
	Vector3i voxel;
};

static inline int get_art_light(int value) {
    return value & ARTIFICIAL_LIGHT_MASK;
}

static inline int get_nat_light(int value) {
    return (value & NATURAL_LIGHT_MASK) >> 4;
}

static inline int set_art_light(int value, int new_value) {
    value &= ~ARTIFICIAL_LIGHT_MASK;
    value |= (new_value & ARTIFICIAL_LIGHT_MASK);
    return value;
}

static inline int set_nat_light(int value, int new_value) {
    value &= ~NATURAL_LIGHT_MASK;
    value |= (new_value & ARTIFICIAL_LIGHT_MASK) >> 4;
    return value;
}

//TODO: Add RGBA colors
static inline int get_final_light(int value) {
    int art_light = get_art_light(value);
    int nat_light = get_nat_light(value);
    return max(art_light, nat_light);
}

#endif