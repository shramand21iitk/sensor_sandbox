#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std::chrono_literals;

static std::shared_ptr<Camera> camera; 

int main(){
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();
    auto cameras = cm->cameras();
    if(cameras.empty()){
        std::cout << "No cameras were identified on the system." << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }
    //Retreive the name and Id of the first available camera
    std::string cameraId = cameras[0]->id();
    camera = cm->get(cameraId);
    camera->acquire(); //Acquire an exclusive lock on it

    //Configure the camera
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration({StreamRole::Viewfinder});
    //Access the first StreamConfiguration item in the CameraConfiguration
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default viewfinder configuration is:" << streamConfig.toString() << std::endl;
    
    return 0;
}