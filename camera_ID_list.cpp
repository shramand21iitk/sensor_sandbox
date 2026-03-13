#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std::chrono_literals;

//instance of CameraManager running for lifetime of application. 
static std::shared_ptr<Camera> camera; //Global shared variable ptr to support event callback

int main(){
    //create a single camera manager instance. store in a unique_ptr to automate deletion when not in use.
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();
    //Iterate available cameras in the system
    for (auto const &camera : cm->cameras()){
        std::cout << camera->id() << std::endl; //Print camera id lists
    }
    auto cameras = cm->cameras();
    if(cameras.empty()){
        std::cout << "No cameras were identified on the system." << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }
    return 0;
}