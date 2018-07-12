/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#include "XsensSuit.h"

#include "XSensMVNDriver.h"

#include <yarp/os/LogStream.h>

#include <map>
#include <mutex>
#include <string>

using namespace wearable::devices;

const std::string logPrefix = "XsensSuit : ";

class XsensSuit::impl
{
public:
    class XsensFreeBodyAccelerationSensor;
    class XsensPositionSensor;
    class XsensOrientationSensor;
    class XsensPoseSensor;
    class XsensMagnetometer;
    class XsensVirtualLinkKinSensor;
    class XsensVirtualSphericalJointKinSensor;

    std::unique_ptr<xsensmvn::XSensMVNDriver> driver;
    impl()
        : driver(nullptr)
    {}

    const std::map<std::string, xsensmvn::CalibrationQuality> calibrationQualities{
        {"Unknown", xsensmvn::CalibrationQuality::UNKNOWN},
        {"Good", xsensmvn::CalibrationQuality::GOOD},
        {"Acceptable", xsensmvn::CalibrationQuality::ACCEPTABLE},
        {"Poor", xsensmvn::CalibrationQuality::POOR},
        {"Failed", xsensmvn::CalibrationQuality::FAILED}};

    const std::vector<std::string> allowedBodyDimensions{"ankleHeight",
                                                         "armSpan",
                                                         "bodyHeight",
                                                         "footSize",
                                                         "hipHeight",
                                                         "hipWidth",
                                                         "kneeHeight",
                                                         "shoulderWidth",
                                                         "shoeSoleHeight"};

    const std::map<xsensmvn::DriverStatus, wearable::sensor::SensorStatus> driverToSensorStatusMap{
        {xsensmvn::DriverStatus::Disconnected, sensor::SensorStatus::Error},
        {xsensmvn::DriverStatus::Unknown, sensor::SensorStatus::Unknown},
        {xsensmvn::DriverStatus::Recording, sensor::SensorStatus::Ok},
        {xsensmvn::DriverStatus::Calibrating, sensor::SensorStatus::Calibrating},
        {xsensmvn::DriverStatus::CalibratedAndReadyToRecord, sensor::SensorStatus::Error},
        {xsensmvn::DriverStatus::Connected, sensor::SensorStatus::Error},
        {xsensmvn::DriverStatus::Scanning, sensor::SensorStatus::Error}};

    template <typename T>
    struct driverToDeviceSensors
    {
        std::shared_ptr<T> xsSensor;
        size_t driverIndex;
    };

    std::map<std::string, driverToDeviceSensors<XsensFreeBodyAccelerationSensor>>
        freeBodyAccerlerationSensorsMap;
    std::map<std::string, driverToDeviceSensors<XsensPositionSensor>> positionSensorsMap;
    std::map<std::string, driverToDeviceSensors<XsensOrientationSensor>> orientationSensorsMap;
    std::map<std::string, driverToDeviceSensors<XsensPoseSensor>> poseSensorsMap;
    std::map<std::string, driverToDeviceSensors<XsensMagnetometer>> magnetometersMap;
    std::map<std::string, driverToDeviceSensors<XsensVirtualLinkKinSensor>>
        virtualLinkKinSensorsMap;
    std::map<std::string, driverToDeviceSensors<XsensVirtualSphericalJointKinSensor>>
        virtualJointKinSensorsMap;

    // ------------------------
    // Custom utility functions
    // ------------------------
    void setAllSensorStates(wearable::sensor::SensorStatus aStatus);
};

// ===================================================
// Xsens implementation of IFreeBodyAccelerationSensor
// ===================================================
class XsensSuit::impl::XsensFreeBodyAccelerationSensor
    : public wearable::sensor::IFreeBodyAccelerationSensor
{
public:
    // ------------------------
    // Constructor / Destructor
    // ------------------------
    XsensFreeBodyAccelerationSensor(
        XsensSuit::impl* xsSuitImpl,
        const wearable::sensor::SensorName aName = {},
        const wearable::sensor::SensorStatus aStatus = wearable::sensor::SensorStatus::Unknown)
        : IFreeBodyAccelerationSensor(aName, aStatus)
        , m_suitImpl(xsSuitImpl)
    {}

    ~XsensFreeBodyAccelerationSensor() override = default;

    // -------------------------------------
    // IFreeBodyAccelerationSensor interface
    // -------------------------------------
    bool getFreeBodyAcceleration(wearable::Vector3& fba) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& sensorData = m_suitImpl->driver->getSensorDataSample().data.at(
            m_suitImpl->freeBodyAccerlerationSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != sensorData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        fba = sensorData.freeBodyAcceleration;
        return true;
    }

    // ------------------------
    // Custom utility functions
    // ------------------------
    inline void setStatus(const wearable::sensor::SensorStatus aStatus) { m_status = aStatus; }

private:
    // ---------
    // Variables
    // ---------
    const XsensSuit::impl* m_suitImpl = nullptr;
};

// =======================================
// Xsens implementation of IPositionSensor
// =======================================
class XsensSuit::impl::XsensPositionSensor : public wearable::sensor::IPositionSensor
{
public:
    // ------------------------
    // Constructor / Destructor
    // ------------------------
    XsensPositionSensor(
        XsensSuit::impl* xsSuitImpl,
        const wearable::sensor::SensorName aName = {},
        const wearable::sensor::SensorStatus aStatus = wearable::sensor::SensorStatus::Unknown)
        : IPositionSensor(aName, aStatus)
        , m_suitImpl(xsSuitImpl)
    {}

    ~XsensPositionSensor() override = default;

    // -------------------------
    // IPositionSensor interface
    // -------------------------
    bool getPosition(wearable::Vector3& pos) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& sensorData = m_suitImpl->driver->getSensorDataSample().data.at(
            m_suitImpl->positionSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != sensorData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        pos = sensorData.position;
        return true;
    }

    // ------------------------
    // Custom utility functions
    // ------------------------
    inline void setStatus(const wearable::sensor::SensorStatus aStatus) { m_status = aStatus; }

private:
    XsensSuit::impl* m_suitImpl = nullptr;
};

// ==========================================
// Xsens implementation of IOrientationSensor
// ==========================================
class XsensSuit::impl::XsensOrientationSensor : public wearable::sensor::IOrientationSensor
{
public:
    // ------------------------
    // Constructor / Destructor
    // ------------------------
    XsensOrientationSensor(
        XsensSuit::impl* xsSuitImpl,
        const wearable::sensor::SensorName aName = {},
        const wearable::sensor::SensorStatus aStatus = wearable::sensor::SensorStatus::Unknown)
        : IOrientationSensor(aName, aStatus)
        , m_suitImpl(xsSuitImpl)
    {}

    ~XsensOrientationSensor() override = default;

    // -------------------------
    // IOrientationSensor interface
    // -------------------------
    bool getOrientationAsQuaternion(wearable::Quaternion& quat) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& sensorData = m_suitImpl->driver->getSensorDataSample().data.at(
            m_suitImpl->orientationSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != sensorData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        quat = sensorData.orientation;
        return true;
    }

    // ------------------------
    // Custom utility functions
    // ------------------------
    inline void setStatus(const wearable::sensor::SensorStatus aStatus) { m_status = aStatus; }

private:
    XsensSuit::impl* m_suitImpl = nullptr;
};

// ===================================
// Xsens implementation of IPoseSensor
// ===================================
class XsensSuit::impl::XsensPoseSensor : public wearable::sensor::IPoseSensor
{
public:
    // ------------------------
    // Constructor / Destructor
    // ------------------------
    XsensPoseSensor(
        XsensSuit::impl* xsSuitImpl,
        const wearable::sensor::SensorName aName = {},
        const wearable::sensor::SensorStatus aStatus = wearable::sensor::SensorStatus::Unknown)
        : IPoseSensor(aName, aStatus)
        , m_suitImpl(xsSuitImpl)
    {}

    ~XsensPoseSensor() override = default;

    // -------------------------
    // IOrientationSensor interface
    // -------------------------
    bool getPose(Quaternion& orientation, Vector3& position) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& sensorData = m_suitImpl->driver->getSensorDataSample().data.at(
            m_suitImpl->orientationSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != sensorData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        orientation = sensorData.orientation;
        position = sensorData.position;
        return true;
    }

    // ------------------------
    // Custom utility functions
    // ------------------------
    inline void setStatus(const wearable::sensor::SensorStatus aStatus) { m_status = aStatus; }

private:
    XsensSuit::impl* m_suitImpl = nullptr;
};

// =====================================
// Xsens implementation of IMagnetometer
// =====================================
class XsensSuit::impl::XsensMagnetometer : public wearable::sensor::IMagnetometer
{
public:
    // ------------------------
    // Constructor / Destructor
    // ------------------------
    XsensMagnetometer(
        XsensSuit::impl* xsSuitImpl,
        const wearable::sensor::SensorName aName = {},
        const wearable::sensor::SensorStatus aStatus = wearable::sensor::SensorStatus::Unknown)
        : IMagnetometer(aName, aStatus)
        , m_suitImpl(xsSuitImpl)
    {}

    ~XsensMagnetometer() override = default;

    // -------------------------
    // IMagnetometer interface
    // -------------------------
    bool getMagneticField(wearable::Vector3& mf) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& sensorData = m_suitImpl->driver->getSensorDataSample().data.at(
            m_suitImpl->magnetometersMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != sensorData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        mf = sensorData.magneticField;
        return true;
    }

    // ------------------------
    // Custom utility functions
    // ------------------------
    inline void setStatus(const wearable::sensor::SensorStatus aStatus) { m_status = aStatus; }

private:
    XsensSuit::impl* m_suitImpl = nullptr;
};

// =============================================
// Xsens implementation of IVirtualLinkKinsensor
// =============================================
class XsensSuit::impl::XsensVirtualLinkKinSensor : public wearable::sensor::IVirtualLinkKinSensor
{
public:
    // ------------------------
    // Constructor / Destructor
    // ------------------------
    XsensVirtualLinkKinSensor(
        XsensSuit::impl* xsSuitImpl,
        const wearable::sensor::SensorName aName = {},
        const wearable::sensor::SensorStatus aStatus = wearable::sensor::SensorStatus::Unknown)
        : IVirtualLinkKinSensor(aName, aStatus)
        , m_suitImpl(xsSuitImpl)
    {}

    ~XsensVirtualLinkKinSensor() override = default;

    // -------------------------------
    // IVirtualLinkKinSensor interface
    // -------------------------------
    bool getLinkAcceleration(Vector3& linear, Vector3& angular) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& linkData = m_suitImpl->driver->getLinkDataSample().data.at(
            m_suitImpl->virtualLinkKinSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != linkData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        linear = linkData.linearAcceleration;
        angular = linkData.angularAcceleration;
        return true;
    }

    bool getLinkPose(Vector3& position, Quaternion& orientation) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& linkData = m_suitImpl->driver->getLinkDataSample().data.at(
            m_suitImpl->virtualLinkKinSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != linkData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        position = linkData.position;
        orientation = linkData.orientation;
        return true;
    }

    bool getLinkVelocity(Vector3& linear, Vector3& angular) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& linkData = m_suitImpl->driver->getLinkDataSample().data.at(
            m_suitImpl->virtualLinkKinSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != linkData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        linear = linkData.linearVelocity;
        angular = linkData.angularVelocity;
        return true;
    }

    // ------------------------
    // Custom utility functions
    // ------------------------
    inline void setStatus(const wearable::sensor::SensorStatus aStatus) { m_status = aStatus; }

private:
    XsensSuit::impl* m_suitImpl = nullptr;
};

// ==============================================
// Xsens implementation of IVirtualJointKinSensor
// ==============================================
class XsensSuit::impl::XsensVirtualSphericalJointKinSensor
    : public wearable::sensor::IVirtualSphericalJointKinSensor
{
public:
    // ------------------------
    // Constructor / Destructor
    // ------------------------
    XsensVirtualSphericalJointKinSensor(
        XsensSuit::impl* xsSuitImpl,
        const wearable::sensor::SensorName aName = {},
        const wearable::sensor::SensorStatus aStatus = wearable::sensor::SensorStatus::Unknown)
        : IVirtualSphericalJointKinSensor(aName, aStatus)
        , m_suitImpl(xsSuitImpl)
    {}

    ~XsensVirtualSphericalJointKinSensor() override = default;

    // -----------------------------------------
    // IVirtualSphericalJointKinSensor interface
    // -----------------------------------------
    bool getJointAnglesAsRPY(Vector3 angleAsRPY) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& jointData = m_suitImpl->driver->getJointDataSample().data.at(
            m_suitImpl->virtualJointKinSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != jointData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        angleAsRPY = jointData.angles;
        return true;
    }

    bool getJointVelocities(Vector3 velocities) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& jointData = m_suitImpl->driver->getJointDataSample().data.at(
            m_suitImpl->virtualJointKinSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != jointData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        velocities = jointData.velocities;
        return true;
    }

    bool getJointAccelerations(Vector3 accelerations) const override
    {
        // Retrieve data sample directly form XSens Driver
        const auto& jointData = m_suitImpl->driver->getJointDataSample().data.at(
            m_suitImpl->virtualJointKinSensorsMap.at(this->m_name).driverIndex);

        // TODO: This should be guaranteed, runtime check should be removed
        // Check if suit sensor and driver data sample have the same name
        if (this->m_name != jointData.name) {
            yError() << logPrefix << "Driver has non entry with my name";
            return false;
        }

        // Fill argument with retrieved data
        accelerations = jointData.accelerations;
        return true;
    }

    // ------------------------
    // Custom utility functions
    // ------------------------
    inline void setStatus(const wearable::sensor::SensorStatus aStatus) { m_status = aStatus; }

private:
    XsensSuit::impl* m_suitImpl = nullptr;
};

// ==========================================
// XsensSuit utility functions implementation
// ==========================================
void XsensSuit::impl::setAllSensorStates(wearable::sensor::SensorStatus aStatus)
{
    for (const auto& fbas : freeBodyAccerlerationSensorsMap) {
        fbas.second.xsSensor->setStatus(aStatus);
    }
    for (const auto& ps : positionSensorsMap) {
        ps.second.xsSensor->setStatus(aStatus);
    }
    for (const auto& os : orientationSensorsMap) {
        os.second.xsSensor->setStatus(aStatus);
    }
    for (const auto& ps : poseSensorsMap) {
        ps.second.xsSensor->setStatus(aStatus);
    }
    for (const auto& m : magnetometersMap) {
        m.second.xsSensor->setStatus(aStatus);
    }
    for (const auto& vlks : virtualLinkKinSensorsMap) {
        vlks.second.xsSensor->setStatus(aStatus);
    }
    for (const auto& vjks : virtualJointKinSensorsMap) {
        vjks.second.xsSensor->setStatus(aStatus);
    }
}

// =================================================
// XsensSuit constructor / destructor implementation
// =================================================
XsensSuit::XsensSuit()
    : pImpl{new impl()}
{}

XsensSuit::~XsensSuit() = default;

// ======================
// DeviceDriver interface
// ======================
bool XsensSuit::open(yarp::os::Searchable& config)
{

    // Read from config file the Xsens rundeps folder
    if (!config.check("xsens-rundeps-dir")) {
        yError() << logPrefix << "REQUIRED parameter <xsens-rundeps-dir> NOT found";
        return false;
    }
    const std::string rundepsFolder = config.find("xsens-rundeps-dir").asString();

    // Read from config file the Xsens suit configuration to use.
    if (!config.check("suit-config")) {
        yError() << logPrefix << "REQUIRED parameter <suit-config> NOT found";
        return false;
    }
    const std::string suitConfiguration = config.find("suit-config").asString();

    // Read from config file the Xsens acquisition scenario to use.
    // Since it is optional, if not found it is safe to use an empty string
    std::string acquisitionScenario;
    if (!config.check("acquisition-scenario")) {
        yWarning() << logPrefix << "OPTIONAL parameter <acquisition-scenario> NOT found";
        acquisitionScenario = "";
    }
    else {
        acquisitionScenario = config.find("acquisition-scenario").asString();
    }

    // Read from config file the calibration routine to be used as default.
    // Since it is optional, if not found it is safe to use an empty string
    std::string defaultCalibrationType;
    if (!config.check("default-calibration-type")) {
        yWarning() << logPrefix << "OPTIONAL parameter <default-calibration-type> NOT found";
        defaultCalibrationType = "";
    }
    else {
        acquisitionScenario = config.find("default-calibration-type").asString();
    }

    // Read from config file the minimum required calibration quality.
    // If not provided using POOR
    xsensmvn::CalibrationQuality minCalibrationQualityRequired;
    if (!config.check("minimum-calibration-quality-required")) {
        yWarning() << logPrefix
                   << "OPTIONAL parameter <minimum-calibration-quality-required> NOT found";
        yWarning() << logPrefix << "Using POOR as minimum required calibration quality";
        minCalibrationQualityRequired = pImpl->calibrationQualities.at("Poor");
    }
    else {
        std::string tmpLabel = config.find("minimum-calibration-quality-required").asString();
        if (pImpl->calibrationQualities.find(tmpLabel) != pImpl->calibrationQualities.end()) {
            minCalibrationQualityRequired = pImpl->calibrationQualities.at(tmpLabel);
        }
        else {
            yWarning() << logPrefix
                       << "OPTIONAL parameter <minimum-calibration-quality-required> INVALID";
            yWarning() << logPrefix << "Using POOR as minimum required calibration quality";
            minCalibrationQualityRequired = pImpl->calibrationQualities.at("Poor");
        }
    }

    // Read from config file the scan-for-suit timeout.
    // If not provided ENABLING endless scan mode
    int scanTimeout = -1;
    if (!config.check("scan-timeout")) {
        yWarning() << logPrefix << "OPTIONAL parameter <scan-timeout> NOT found";
        yWarning() << logPrefix << "Endless scan mode ENABLED";
    }
    else {
        scanTimeout = config.find("scan-timeout").asInt();
    }

    // Get subject-specific body dimensions from the configuration file and push them to
    // subjectBodyDimensions
    // TODO: remove the hardcoded allowed list
    xsensmvn::bodyDimensions subjectBodyDimensions;
    yarp::os::Bottle bodyDimensionSet = config.findGroup("body-dimensions", "");
    if (bodyDimensionSet.isNull()) {
        yWarning() << logPrefix << "OPTIONAL parameter group <body-dimensions> NOT found";
        yWarning() << logPrefix
                   << "USING default body dimensions, this may affect estimation quality";
    }
    for (const auto& abd : pImpl->allowedBodyDimensions) {
        double tmpDim = bodyDimensionSet.check(abd, yarp::os::Value(-1.0)).asDouble();
        if (tmpDim != -1.0) {
            subjectBodyDimensions.insert({abd, tmpDim});
        }
    }

    // Read from config file the selected output stream configuration.
    // If not provided USING default configuration, Joints: OFF, Links: ON, Sensors: ON
    xsensmvn::DriverDataStreamConfig outputStreamConfig;
    yarp::os::Bottle streamGroup = config.findGroup("output-stream-configuration", "");
    if (streamGroup.isNull()) {
        yWarning() << logPrefix
                   << "OPTIONAL parameters group <output-stream-configuration> NOT found";
        yWarning() << logPrefix
                   << "USING default configuration, Joints: OFF, Links: ON, Sensors: ON";
    }
    outputStreamConfig.enableJointData =
        streamGroup.check("enable-joint-data", yarp::os::Value(false)).asBool();
    outputStreamConfig.enableLinkData =
        streamGroup.check("enable-link-data", yarp::os::Value(true)).asBool();
    outputStreamConfig.enableSensorData =
        streamGroup.check("enable-sensor-data", yarp::os::Value(true)).asBool();

    xsensmvn::DriverConfiguration driverConfig{rundepsFolder,
                                               suitConfiguration,
                                               acquisitionScenario,
                                               defaultCalibrationType,
                                               minCalibrationQualityRequired,
                                               scanTimeout,
                                               subjectBodyDimensions,
                                               outputStreamConfig};

    pImpl->driver.reset(new xsensmvn::XSensMVNDriver(driverConfig));

    if (!pImpl->driver->configureAndConnect()) {
        yError() << logPrefix << "Unable to configure the driver and connect to the suit";
        return false;
    }

    std::string fbasPrefix = getWearableName() + sensor::IFreeBodyAccelerationSensor::getPrefix();
    std::string posPrefix = getWearableName() + sensor::IPositionSensor::getPrefix();
    std::string orientPrefix = getWearableName() + sensor::IOrientationSensor::getPrefix();
    std::string posePrefix = getWearableName() + sensor::IPoseSensor::getPrefix();
    std::string magPrefix = getWearableName() + sensor::IMagnetometer::getPrefix();
    std::string vlksPrefix = getWearableName() + sensor::IVirtualLinkKinSensor::getPrefix();
    std::string vjksPrefix =
        getWearableName() + sensor::IVirtualSphericalJointKinSensor::getPrefix();

    std::vector<std::string> sensorNames = pImpl->driver->getSuitSensorLabels();
    for (size_t s = 0; s < sensorNames.size(); ++s) {
        pImpl->freeBodyAccerlerationSensorsMap.emplace(
            fbasPrefix + sensorNames[s],
            impl::driverToDeviceSensors<impl::XsensFreeBodyAccelerationSensor>{
                std::make_shared<impl::XsensFreeBodyAccelerationSensor>(
                    pImpl.get(), fbasPrefix + sensorNames[s]),
                s});
        pImpl->positionSensorsMap.emplace(posPrefix + sensorNames[s],
                                          impl::driverToDeviceSensors<impl::XsensPositionSensor>{
                                              std::make_shared<impl::XsensPositionSensor>(
                                                  pImpl.get(), posPrefix + sensorNames[s]),
                                              s});
        pImpl->orientationSensorsMap.emplace(
            orientPrefix + sensorNames[s],
            impl::driverToDeviceSensors<impl::XsensOrientationSensor>{
                std::make_shared<impl::XsensOrientationSensor>(pImpl.get(),
                                                               orientPrefix + sensorNames[s]),
                s});
        pImpl->poseSensorsMap.emplace(
            posePrefix + sensorNames[s],
            impl::driverToDeviceSensors<impl::XsensPoseSensor>{
                std::make_shared<impl::XsensPoseSensor>(pImpl.get(), posePrefix + sensorNames[s]),
                s});
        pImpl->magnetometersMap.emplace(
            magPrefix + sensorNames[s],
            impl::driverToDeviceSensors<impl::XsensMagnetometer>{
                std::make_shared<impl::XsensMagnetometer>(pImpl.get(), magPrefix + sensorNames[s]),
                s});
    }

    std::vector<std::string> linkNames = pImpl->driver->getSuitLinkLabels();
    for (size_t s = 0; s < linkNames.size(); ++s) {
        pImpl->virtualLinkKinSensorsMap.emplace(
            vlksPrefix + linkNames[s],
            impl::driverToDeviceSensors<impl::XsensVirtualLinkKinSensor>{
                std::make_shared<impl::XsensVirtualLinkKinSensor>(pImpl.get(),
                                                                  vlksPrefix + linkNames[s]),
                s});
    }

    std::vector<std::string> jointNames = pImpl->driver->getSuitJointLabels();
    for (size_t s = 0; s < jointNames.size(); ++s) {
        pImpl->virtualJointKinSensorsMap.emplace(
            vjksPrefix + jointNames[s],
            impl::driverToDeviceSensors<impl::XsensVirtualSphericalJointKinSensor>{
                std::make_shared<impl::XsensVirtualSphericalJointKinSensor>(
                    pImpl.get(), vjksPrefix + jointNames[s]),
                s});
    }

    return true;
}

bool XsensSuit::close()
{
    return true;
}

// =========================
// IPreciselyTimed interface
// =========================
yarp::os::Stamp XsensSuit::getLastInputStamp()
{
    // Stamp count should be always zero
    return yarp::os::Stamp(0, pImpl->driver->getTimeStamps().systemTime);
}

// ==========================
// IXsensMVNControl interface
// ==========================

bool XsensSuit::setBodyDimensions(const std::map<std::string, double>& dimensions)
{
    if (!pImpl->driver) {
        return false;
    }

    return pImpl->driver->setBodyDimensions(dimensions);
}

bool XsensSuit::getBodyDimensions(std::map<std::string, double>& dimensions) const
{
    if (!pImpl->driver) {
        return false;
    }

    return pImpl->driver->getBodyDimensions(dimensions);
}

bool XsensSuit::getBodyDimension(const std::string bodyName, double& dimension) const
{
    if (!pImpl->driver) {
        return false;
    }

    return pImpl->driver->getBodyDimension(bodyName, dimension);
}

// Calibration methods
bool XsensSuit::calibrate(const std::string& calibrationType)
{

    if (!pImpl->driver) {
        return false;
    }

    pImpl->setAllSensorStates(sensor::SensorStatus::Calibrating);

    bool calibrationSuccess = pImpl->driver->calibrate(calibrationType);

    if (calibrationSuccess) {
        pImpl->setAllSensorStates(sensor::SensorStatus::WaitingForFirstRead);
    }
    else {
        pImpl->setAllSensorStates(sensor::SensorStatus::Error);
    }

    return calibrationSuccess;
}

bool XsensSuit::abortCalibration()
{
    if (!pImpl->driver) {
        return false;
    }

    bool abortCalibrationSuccess = pImpl->driver->abortCalibration();

    if (abortCalibrationSuccess) {
        pImpl->setAllSensorStates(sensor::SensorStatus::Unknown);
    }
    else {
        pImpl->setAllSensorStates(sensor::SensorStatus::Error);
    }

    return abortCalibrationSuccess;
}

// Acquisition methods
bool XsensSuit::startAcquisition()
{
    if (!pImpl->driver) {
        return false;
    }

    bool startAcquisitionSuccess = pImpl->driver->startAcquisition();

    if (startAcquisitionSuccess) {
        pImpl->setAllSensorStates(sensor::SensorStatus::Ok);
    }
    else {
        pImpl->setAllSensorStates(sensor::SensorStatus::WaitingForFirstRead);
    }

    return startAcquisitionSuccess;
}

bool XsensSuit::stopAcquisition()
{
    if (!pImpl->driver) {
        return false;
    }

    bool stopAcquisitionSuccess = pImpl->driver->stopAcquisition();
    if (stopAcquisitionSuccess) {
        pImpl->setAllSensorStates(sensor::SensorStatus::WaitingForFirstRead);
    }
    else {
        pImpl->setAllSensorStates(sensor::SensorStatus::Ok);
    }

    return stopAcquisitionSuccess;
}

// ===============
// IWear interface
// ===============

// ---------------
// Generic Methods
// ---------------

wearable::WearableName XsensSuit::getWearableName() const
{
    return "XsensSuit_";
}

wearable::WearStatus XsensSuit::getStatus() const
{
    return pImpl->driverToSensorStatusMap.at(pImpl->driver->getStatus());
}

wearable::TimeStamp XsensSuit::getTimeStamp() const
{
    // Stamp count should be always zero
    return {pImpl->driver->getTimeStamps().systemTime, 0};
}

// ---------------------------
// Implemented Sensors Methods
// ---------------------------

wearable::SensorPtr<const wearable::sensor::ISensor>
XsensSuit::getSensor(const wearable::sensor::SensorName name) const
{
    wearable::VectorOfSensorPtr<const wearable::sensor::ISensor> sensors = getAllSensors();
    for (const auto& s : sensors) {
        if (s->getSensorName() == name) {
            return s;
        }
    }
    yWarning() << logPrefix << "User specified name <" << name << "> not found";
    return nullptr;
}

wearable::VectorOfSensorPtr<const wearable::sensor::ISensor>
XsensSuit::getSensors(const wearable::sensor::SensorType aType) const
{
    wearable::VectorOfSensorPtr<const wearable::sensor::ISensor> outVec;
    switch (aType) {
        case sensor::SensorType::FreeBodyAccelerationSensor: {
            outVec.reserve(pImpl->freeBodyAccerlerationSensorsMap.size());
            for (const auto& fbas : pImpl->freeBodyAccerlerationSensorsMap) {
                outVec.push_back(
                    static_cast<std::shared_ptr<sensor::ISensor>>(fbas.second.xsSensor));
            }
            break;
        }
        case sensor::SensorType::PositionSensor: {
            outVec.reserve(pImpl->positionSensorsMap.size());
            for (const auto& ps : pImpl->positionSensorsMap) {
                outVec.push_back(static_cast<std::shared_ptr<sensor::ISensor>>(ps.second.xsSensor));
            }
            break;
        }
        case sensor::SensorType::OrientationSensor: {
            outVec.reserve(pImpl->orientationSensorsMap.size());
            for (const auto& os : pImpl->orientationSensorsMap) {
                outVec.push_back(static_cast<std::shared_ptr<sensor::ISensor>>(os.second.xsSensor));
            }
            break;
        }
        case sensor::SensorType::PoseSensor: {
            outVec.reserve(pImpl->poseSensorsMap.size());
            for (const auto& ps : pImpl->poseSensorsMap) {
                outVec.push_back(static_cast<std::shared_ptr<sensor::ISensor>>(ps.second.xsSensor));
            }
            break;
        }
        case sensor::SensorType::Magnetometer: {
            outVec.reserve(pImpl->magnetometersMap.size());
            for (const auto& m : pImpl->magnetometersMap) {
                outVec.push_back(static_cast<std::shared_ptr<sensor::ISensor>>(m.second.xsSensor));
            }
            break;
        }
        case sensor::SensorType::VirtualLinkKinSensor: {
            outVec.reserve(pImpl->virtualLinkKinSensorsMap.size());
            for (const auto& vlks : pImpl->virtualLinkKinSensorsMap) {
                outVec.push_back(
                    static_cast<std::shared_ptr<sensor::ISensor>>(vlks.second.xsSensor));
            }
            break;
        }
        case sensor::SensorType::VirtualSphericalJointKinSensor: {
            outVec.reserve(pImpl->virtualJointKinSensorsMap.size());
            for (const auto& vjks : pImpl->virtualJointKinSensorsMap) {
                outVec.push_back(
                    static_cast<std::shared_ptr<sensor::ISensor>>(vjks.second.xsSensor));
            }
            break;
        }
        default: {
            yWarning() << logPrefix << "Selected sensor type is not supported by XsensSuit";
            outVec.clear();
        }
    }
    return outVec;
}

wearable::SensorPtr<const wearable::sensor::IFreeBodyAccelerationSensor>
XsensSuit::getFreeBodyAccelerationSensor(const wearable::sensor::SensorName name) const
{
    // Check if user-provided name corresponds to an available sensor
    if (pImpl->freeBodyAccerlerationSensorsMap.find(static_cast<std::string>(name))
        == pImpl->freeBodyAccerlerationSensorsMap.end()) {
        yError() << logPrefix << "Invalid sensor name";
        return nullptr;
    }

    // Return a shared pointer to the required sensor
    return static_cast<std::shared_ptr<sensor::IFreeBodyAccelerationSensor>>(
        pImpl->freeBodyAccerlerationSensorsMap.at(static_cast<std::string>(name)).xsSensor);
}

wearable::SensorPtr<const wearable::sensor::IPositionSensor>
XsensSuit::getPositionSensor(const wearable::sensor::SensorName name) const
{
    // Check if user-provided name corresponds to an available sensor
    if (pImpl->positionSensorsMap.find(static_cast<std::string>(name))
        == pImpl->positionSensorsMap.end()) {
        yError() << logPrefix << "Invalid sensor name";
        return nullptr;
    }

    // Return a shared pointer to the required sensor
    return static_cast<std::shared_ptr<sensor::IPositionSensor>>(
        pImpl->positionSensorsMap.at(static_cast<std::string>(name)).xsSensor);
}

wearable::SensorPtr<const wearable::sensor::IOrientationSensor>
XsensSuit::getOrientationSensor(const wearable::sensor::SensorName name) const
{
    // Check if user-provided name corresponds to an available sensor
    if (pImpl->orientationSensorsMap.find(static_cast<std::string>(name))
        == pImpl->orientationSensorsMap.end()) {
        yError() << logPrefix << "Invalid sensor name";
        return nullptr;
    }

    // Return a shared pointer to the required sensor
    return static_cast<std::shared_ptr<sensor::IOrientationSensor>>(
        pImpl->orientationSensorsMap.at(static_cast<std::string>(name)).xsSensor);
}

wearable::SensorPtr<const wearable::sensor::IPoseSensor>
XsensSuit::getPoseSensor(const wearable::sensor::SensorName name) const
{
    // Check if user-provided name corresponds to an available sensor
    if (pImpl->poseSensorsMap.find(static_cast<std::string>(name)) == pImpl->poseSensorsMap.end()) {
        yError() << logPrefix << "Invalid sensor name";
        return nullptr;
    }

    // Return a shared pointer to the required sensor
    return static_cast<std::shared_ptr<sensor::IPoseSensor>>(
        pImpl->poseSensorsMap.at(static_cast<std::string>(name)).xsSensor);
}

wearable::SensorPtr<const wearable::sensor::IMagnetometer>
XsensSuit::getMagnetometer(const wearable::sensor::SensorName name) const
{
    // Check if user-provided name corresponds to an available sensor
    if (pImpl->magnetometersMap.find(static_cast<std::string>(name))
        == pImpl->magnetometersMap.end()) {
        yError() << logPrefix << "Invalid sensor name";
        return nullptr;
    }

    // Return a shared pointer to the required sensor
    return static_cast<std::shared_ptr<sensor::IMagnetometer>>(
        pImpl->magnetometersMap.at(static_cast<std::string>(name)).xsSensor);
}

wearable::SensorPtr<const wearable::sensor::IVirtualLinkKinSensor>
XsensSuit::getVirtualLinkKinSensor(const wearable::sensor::SensorName name) const
{
    // Check if user-provided name corresponds to an available sensor
    if (pImpl->virtualLinkKinSensorsMap.find(static_cast<std::string>(name))
        == pImpl->virtualLinkKinSensorsMap.end()) {
        yError() << logPrefix << "Invalid sensor name";
        return nullptr;
    }

    // Return a shared pointer to the required sensor
    return static_cast<std::shared_ptr<sensor::IVirtualLinkKinSensor>>(
        pImpl->virtualLinkKinSensorsMap.at(static_cast<std::string>(name)).xsSensor);
}

wearable::SensorPtr<const wearable::sensor::IVirtualSphericalJointKinSensor>
XsensSuit::getVirtualSphericalJointKinSensor(const wearable::sensor::SensorName name) const
{
    // Check if user-provided name corresponds to an available sensor
    if (pImpl->virtualJointKinSensorsMap.find(static_cast<std::string>(name))
        == pImpl->virtualJointKinSensorsMap.end()) {
        yError() << logPrefix << "Invalid sensor name";
        return nullptr;
    }

    // Return a shared pointer to the required sensor
    return static_cast<std::shared_ptr<sensor::IVirtualSphericalJointKinSensor>>(
        pImpl->virtualJointKinSensorsMap.at(static_cast<std::string>(name)).xsSensor);
}