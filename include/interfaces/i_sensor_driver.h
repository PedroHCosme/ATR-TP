#ifndef I_SENSOR_DRIVER_H
#define I_SENSOR_DRIVER_H

#include "../dados.h"

class ISensorDriver {
public:
    virtual ~ISensorDriver() = default;
    virtual CaminhaoFisico readSensorData(int id_caminhao) = 0;
};



#endif // I_SENSOR_DRIVER_H