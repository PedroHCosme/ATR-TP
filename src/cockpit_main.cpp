
#include "interface_caminhao.h"
#include <chrono>
#include <iostream>
#include <string> // Required for std::stoi
#include <thread>

int main(int argc, char *argv[]) {
  int truck_id = 0;
  if (argc > 1) {
    try {
      truck_id = std::stoi(argv[1]);
    } catch (const std::invalid_argument &e) {
      std::cerr << "Invalid argument for truck ID: " << argv[1]
                << ". Using default ID 0." << std::endl;
    } catch (const std::out_of_range &e) {
      std::cerr << "Truck ID out of range: " << argv[1]
                << ". Using default ID 0." << std::endl;
    }
  }

  try {
    InterfaceCaminhao cockpit(truck_id);
    cockpit.run();
    cockpit.close();
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error in Cockpit: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unknown Fatal Error in Cockpit" << std::endl;
    return 1;
  }
  return 0;
}
