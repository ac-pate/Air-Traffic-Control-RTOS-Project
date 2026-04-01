#pragma once
#include <atomic>
#include <array>
#include <string>
#include <cstdint>

// Achal & Parsa - COEN 320 ATC Project

// Coen320_Lab (Task0): channel/shm names with group id to avoid conflicts
#define ATC_RADAR_CHANNEL      "Achal_Parsa_320_radar"
#define ATC_DISPLAY_CHANNEL    "Achal_Parsa_320_display"
#define ATC_COMPUTER_CHANNEL   "Achal_Parsa_320_computer"
#define ATC_SHM_NAME           "/shm_Achal_Parsa_320"
#define ATC_SHM_SIZE           sizeof(SharedMemory)

// returns channel name for specific aircraft
inline std::string getAircraftChannel(int aircraftId) {
    return "Achal_Parsa_320_plane_" + std::to_string(aircraftId);
}

#define MAX_PLANES_IN_AIRSPACE  100
#define COLLISION_TIME_HORIZON  180

enum class MessageType {
    ENTER_AIRSPACE,
    EXIT_AIRSPACE,
    POSITION_UPDATE,
	REQUEST_POSITION,
	REQUEST_CHANGE_OF_HEADING,
	REQUEST_CHANGE_POSITION,
	REQUEST_CHANGE_ALTITUDE,
	REQUEST_AUGMENTED_INFO,
	CHANGE_TIME_CONSTRAINT_COLLISIONS,
	EXIT,
	COLLISION_DETECTED
};

struct Message {
    bool header; //intra process; 1: interprocess
	MessageType type;
    int planeID;
    void* data;  // Pointer to the message data
    size_t dataSize;  // Size of the serialized data
};

typedef struct {
	int id;
	double PositionX, PositionY, PositionZ, VelocityX, VelocityY, VelocityZ;
} msg_plane_info;

typedef struct {
	int ID;
	double VelocityX, VelocityY, VelocityZ;
	double altitude;
} msg_change_heading;

typedef struct {
	double x,y,z;
} msg_change_position;

// Shared memory structure
struct SharedMemory {
    msg_plane_info plane_data[MAX_PLANES_IN_AIRSPACE];
    int count;  // Keep track of the number of planes in the buffer
    std::atomic<bool> is_empty;  // New flag to indicate if there are no planes in the buffer
    bool start;
    uint64_t timestamp;  // Timestamp of the last write
    pthread_mutex_t accessLock;  // using mutex instead of semaphore for sync
};

struct Message_inter_process {
	bool header; //intra process; 1: interprocess
	MessageType type;
    int planeID;
    std::array<char, 256> data;  // Pointer to the message data
    size_t dataSize;  // Size of the serialized data
};

