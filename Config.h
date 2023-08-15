#pragma once
#include <windef.h>

class Config
{
public:
    float BoxThick = 2.0f, SkeletonThick = 1.0f, Fov = 100.0f, Distance = 1000.0f, menu = 0.4, AimSmooth = 0.45, AimSleep = 1;
    int Mode = 1, Target = 1, ItemMode = 1;
    int RadarDist = 200;
    bool DeathBox = true, Box = true, Skeleton = true, Item = false, HealthBar = true, DebugMode = false , Prediction = true, Menu = true, LineESP = true, DistanceESP = true, WeaponVisible = false, VehicleESP = false, ESPColor = true, AimVisible = true, PlayerName = true, fightmode = false, SuperJump = true, SpeedHack = true, AimBot = true, YSmooth = false, NoRecoil = false;
    bool radar = true;
    bool MagicBullet = true;

    DWORD AimKey = VK_RBUTTON;
};

Config* cfg = new Config;
