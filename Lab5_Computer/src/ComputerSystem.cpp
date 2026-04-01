#include "ComputerSystem.h"
#include "ATCTimer.h"
#include <ctime>        // For std::time_t, std::localtime
#include <iomanip>      // For std::put_time
#include <cmath>
#include <sys/dispatch.h>
#include <cstring> // For memcpy
#include <algorithm>

// Achal & Parsa - ComputerSystem
// collision detection samples future positions instead of computing exact intersection times

// COEN320 Task 3.1, set the display channel name (it should be the same as the Coen320_Lab (Task0) Display channel name)
// using ATC_DISPLAY_CHANNEL from Msg_structs.h


ComputerSystem::ComputerSystem() 
    : collisionLookahead(COLLISION_TIME_HORIZON),
      shm_fd(-1), 
      shared_mem(nullptr), 
      running(false),
      computerChannel(nullptr) {}

ComputerSystem::~ComputerSystem() {
    running.store(false);
    joinThread();
    cleanupSharedMemory();
}

bool ComputerSystem::initializeSharedMemory() {
	// Open the shared memory object
	while (running.load()) {
        // COEN320 Task 3.2
		// Attempt to open the shared memory object (You need to use the same name as Task 2 in Radar)
        // In case of error, retry until successful
        shm_fd = shm_open(ATC_SHM_NAME, O_RDWR, 0666);
        if (shm_fd == -1) {
            std::cout << "ComputerSystem: Waiting for Radar to create shared memory..." << std::endl;
            sleep(1);
            continue;
        }
        
        // COEN320 Task 3.3
		// Map the shared memory object into the process's address space
        // The shared memory should be mapped to "shared_mem" (check for errors)
        shared_mem = static_cast<SharedMemory*>(
            mmap(NULL, ATC_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)
        );
        
        if (shared_mem == MAP_FAILED) {
            perror("ComputerSystem: mmap failed");
            shared_mem = nullptr;
            close(shm_fd);
            shm_fd = -1;
            sleep(1);
            continue;
        }
        
        std::cout << "Shared memory initialized successfully." << std::endl;
        return true;
	}
	return false;
}

void ComputerSystem::cleanupSharedMemory() {
    if (shared_mem && shared_mem != MAP_FAILED) {
        munmap(shared_mem, ATC_SHM_SIZE);
        shared_mem = nullptr;
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
    if (computerChannel != nullptr) {
        name_detach(computerChannel, 0);
        computerChannel = nullptr;
    }
}

bool ComputerSystem::startMonitoring() {
    running.store(true);
    
    if (initializeSharedMemory()) {
        std::cout << "Starting monitoring threads." << std::endl;
        monitorThread = std::thread(&ComputerSystem::monitorAirspace, this);
        operatorThread = std::thread(&ComputerSystem::operatorListener, this);
        return true;
    } else {
        std::cerr << "Failed to initialize shared memory. Monitoring not started.\n";
        running.store(false);
        return false;
    }
}

void ComputerSystem::joinThread() {
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
    
    // Send exit message to unblock operator listener
    if (operatorThread.joinable()) {
        int coid = name_open(ATC_COMPUTER_CHANNEL, 0);
        if (coid != -1) {
            Message_inter_process exitMsg{};
            exitMsg.header = true;
            exitMsg.type = MessageType::EXIT;
            int reply;
            MsgSend(coid, &exitMsg, sizeof(exitMsg), &reply, sizeof(reply));
            name_close(coid);
        }
        operatorThread.join();
    }
}

void ComputerSystem::operatorListener() {
    computerChannel = name_attach(NULL, ATC_COMPUTER_CHANNEL, 0);
    if (computerChannel == nullptr) {
        perror("ComputerSystem: Failed to create channel");
        running.store(false);
        return;
    }
    
    while (running.load()) {
        Message_inter_process msg{};
        int rcvid = MsgReceive(computerChannel->chid, &msg, sizeof(msg), NULL);
        if (rcvid == -1) continue;
        if (rcvid == 0) continue;  // pulse
        
        handleOperatorMessage(msg, rcvid);
    }
}

void ComputerSystem::handleOperatorMessage(const Message_inter_process& msg, int rcvid) {
    int status = 0;
    
    switch (msg.type) {
        case MessageType::REQUEST_CHANGE_OF_HEADING:
        case MessageType::REQUEST_CHANGE_POSITION:
        case MessageType::REQUEST_CHANGE_ALTITUDE:
            status = forwardToAircraft(msg) ? 0 : -1;
            break;
            
        case MessageType::CHANGE_TIME_CONSTRAINT_COLLISIONS:
            if (msg.dataSize >= sizeof(int)) {
                int newLookahead;
                std::memcpy(&newLookahead, msg.data.data(), sizeof(int));
                collisionLookahead = std::max(1, newLookahead);
                std::cout << "Collision lookahead changed to " << collisionLookahead << " seconds" << std::endl;
            }
            break;
            
        case MessageType::REQUEST_AUGMENTED_INFO: {
            // Find the plane and send info to display
            pthread_mutex_lock(&shared_mem->accessLock);
            for (int i = 0; i < shared_mem->count; ++i) {
                if (shared_mem->plane_data[i].id == msg.planeID) {
                    msg_plane_info plane = shared_mem->plane_data[i];
                    pthread_mutex_unlock(&shared_mem->accessLock);
                    
                    // Print info locally
                    std::cout << "Aircraft " << plane.id << ": pos=(" 
                              << plane.PositionX << "," << plane.PositionY << "," << plane.PositionZ
                              << ") vel=(" << plane.VelocityX << "," << plane.VelocityY << "," << plane.VelocityZ << ")" << std::endl;
                    
                    MsgReply(rcvid, 0, &status, sizeof(status));
                    return;
                }
            }
            pthread_mutex_unlock(&shared_mem->accessLock);
            status = -1;  // plane not found
            break;
        }
            
        case MessageType::EXIT:
            running.store(false);
            break;
            
        default:
            status = -1;
            break;
    }
    
    MsgReply(rcvid, 0, &status, sizeof(status));
}

bool ComputerSystem::forwardToAircraft(const Message_inter_process& msg) {
    std::string channelName = getAircraftChannel(msg.planeID);
    int aircraftCoid = name_open(channelName.c_str(), 0);
    if (aircraftCoid == -1) {
        std::cerr << "ComputerSystem: Cannot reach aircraft " << msg.planeID << std::endl;
        return false;
    }
    
    int reply;
    int result = MsgSend(aircraftCoid, &msg, sizeof(msg), &reply, sizeof(reply));
    name_close(aircraftCoid);
    
    return (result != -1 && reply == 0);
}

void ComputerSystem::monitorAirspace() {
	ATCTimer timer(1,0);
	std::vector<msg_plane_info> planeData;
	bool sawPlanes = false;
	int emptyCount = 0;
	
    // Keep monitoring indefinitely until `stopMonitoring` is called
	while (running.load()) {
	    // Lock shared memory and read data
	    pthread_mutex_lock(&shared_mem->accessLock);
	    bool isEmpty = shared_mem->is_empty.load();
	    bool started = shared_mem->start;
	    uint64_t timestamp = shared_mem->timestamp;
	    
	    planeData.clear();
	    if (!isEmpty && started) {
	        for (int i = 0; i < shared_mem->count && i < MAX_PLANES_IN_AIRSPACE; ++i) {
	            planeData.push_back(shared_mem->plane_data[i]);
	        }
	    }
	    pthread_mutex_unlock(&shared_mem->accessLock);
	    
	    if (!started) {
	        timer.waitTimer();
	        continue;
	    }
	    
	    if (planeData.empty()) {
	        if (sawPlanes) {
	            emptyCount++;
	            if (emptyCount >= 3) {
	                std::cout << "No planes in airspace. Stopping monitoring.\n";
	                running.store(false);
	                break;
	            }
	        }
	    } else {
	        sawPlanes = true;
	        emptyCount = 0;
	        
	        //**************Call Collision Detector*********************
	        if (planeData.size() > 1) {
                checkCollision(timestamp, planeData);
	        }
	    }
	    
        timer.waitTimer();
    }
	std::cout << "Exiting monitoring loop." << std::endl;
}

void ComputerSystem::checkCollision(uint64_t currentTime, const std::vector<msg_plane_info>& planes) {
    // COEN320 Task 3.4
    // detect collisions between planes in the airspace within the time constraint
    // You need to Iterate through each pair of planes and in case of collision,
    // store the pair of plane IDs that are predicted to collide
    
    std::set<std::pair<int,int>> currentCollisions;
    std::vector<std::pair<int,int>> newCollisions;
    
    // Check all pairs of planes
    for (size_t i = 0; i < planes.size(); ++i) {
        for (size_t j = i + 1; j < planes.size(); ++j) {
            if (willCollide(planes[i], planes[j])) {
                int id1 = std::min(planes[i].id, planes[j].id);
                int id2 = std::max(planes[i].id, planes[j].id);
                std::pair<int,int> collisionPair = {id1, id2};
                
                currentCollisions.insert(collisionPair);
                
                // Only report if this is a NEW collision
                if (activeCollisions.find(collisionPair) == activeCollisions.end()) {
                    newCollisions.push_back(collisionPair);
                }
            }
        }
    }
    
    // Update active collisions
    activeCollisions = currentCollisions;
    
    // COEN320 Task 3.5
    // in case of collision, send message to Display system
    if (!newCollisions.empty()) {
        std::cout << "*** COLLISION WARNING at tick " << currentTime << ": ";
        for (const auto& pair : newCollisions) {
            std::cout << "(" << pair.first << "," << pair.second << ") ";
        }
        std::cout << "***" << std::endl;
        
        // Prepare the message
        Message_inter_process msg_to_send{};
        msg_to_send.header = true;
        msg_to_send.planeID = -1;
        msg_to_send.type = MessageType::COLLISION_DETECTED;
        msg_to_send.dataSize = std::min(newCollisions.size() * sizeof(std::pair<int, int>), 
                                        msg_to_send.data.size());
        std::memcpy(msg_to_send.data.data(), newCollisions.data(), msg_to_send.dataSize);
        sendCollisionToDisplay(msg_to_send);
    }
}

// replaced checkAxes with this - samples future positions instead of computing exact intersection
bool ComputerSystem::willCollide(const msg_plane_info& p1, const msg_plane_info& p2) {
    // COEN320 Task 3.4
    // A collision is defined as two planes entering the defined airspace constraints within the time constraint
    // You need to implement the logic to check if plane1 and plane2 will collide within the time constraint
    // Return true if they will collide, false otherwise
    
    // sample every 10 sec up to lookahead horizon
    for (int t = 0; t <= collisionLookahead; t += 10) {
        // project positions forward
        double futureX1 = p1.PositionX + p1.VelocityX * t;
        double futureY1 = p1.PositionY + p1.VelocityY * t;
        double futureZ1 = p1.PositionZ + p1.VelocityZ * t;
        
        double futureX2 = p2.PositionX + p2.VelocityX * t;
        double futureY2 = p2.PositionY + p2.VelocityY * t;
        double futureZ2 = p2.PositionZ + p2.VelocityZ * t;
        
        // check if within safety box
        if (std::fabs(futureX1 - futureX2) < CONSTRAINT_X &&
            std::fabs(futureY1 - futureY2) < CONSTRAINT_Y &&
            std::fabs(futureZ1 - futureZ2) < CONSTRAINT_Z) {
            return true;
        }
    }
    return false;
}

double ComputerSystem::predictDistance(const msg_plane_info& p1, const msg_plane_info& p2, double t) {
    double x1 = p1.PositionX + p1.VelocityX * t;
    double y1 = p1.PositionY + p1.VelocityY * t;
    double z1 = p1.PositionZ + p1.VelocityZ * t;
    
    double x2 = p2.PositionX + p2.VelocityX * t;
    double y2 = p2.PositionY + p2.VelocityY * t;
    double z2 = p2.PositionZ + p2.VelocityZ * t;
    
    return std::sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
}

void ComputerSystem::sendCollisionToDisplay(const Message_inter_process& msg){
	int display_channel = name_open(ATC_DISPLAY_CHANNEL, 0);
	if (display_channel == -1) {
	    // Display might not be running yet - just print warning
		std::cerr << "ComputerSystem: Display not available" << std::endl;
		return;
	}
	int reply;

	int status = MsgSend(display_channel, &msg, sizeof(msg), &reply, sizeof(reply));
	if (status == -1) {
		perror("Computer system: Error occurred while sending message to display channel");
	}
	name_close(display_channel);
}
