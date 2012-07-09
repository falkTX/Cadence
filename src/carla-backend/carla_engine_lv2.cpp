
#ifdef CARLA_ENGINE_LV2
class CarlaEngineLv2 : public CarlaEngine
{
public:
    CarlaEngineLv2();
    ~CarlaEngineLv2();

    // -------------------------------------

    bool init(const char* const name);
    bool close();

    bool isOnAudioThread();
    bool isOffline();
    bool isRunning();

    CarlaEngineClient* addClient(CarlaPlugin* const plugin);
};
#endif
