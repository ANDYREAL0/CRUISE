#include "defines.h"
#include "DriverInterface.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <Windows.h>
#include <string>
#include <tchar.h>
#include "Dwmapi.h"
#include <time.h>
#include <algorithm>
#include "Config.h"
#include <winhttp.h>
#include <urlmon.h>
#include <cstring>
#include <fstream>
#include <timeapi.h>
#include <mmsystem.h>
#include "UGameData.h"
#include <Shlwapi.h>
//#include "C:\Users\Siege\Desktop\//VMProtect_Ultimate_3.4\Include\C\//VMProtectSDK.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "Overlay.h"
#include "Images.h"
#include "xUtils.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "dwmapi.lib")
//#pragma comment(lib, "//VMProtectSDK64.lib")
#define wclass "UnrealWindow"
#define wname NULL
using namespace std;
#define M_PI_F		( (float)(M_PI)                        )
#define DEG2RAD(x)  ( (float)(x) * (float)(M_PI_F / 180.f) )
#define RAD2DEG(x)  ( (float)(x) * (float)(180.f / M_PI_F) )

extern Vector3 WorldToScreen(Vector3 WorldLocation, FCameraCacheEntry CameraCacheL);
extern D3DMATRIX MatrixMultiplication(D3DMATRIX pM1, D3DMATRIX pM2);
RECT rc, rc2;
HWND hwnd, gwnd, topw;
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

HRGN rgn;
DWM_BLURBEHIND blur;
bool CreateDeviceD3D(HWND hWnd), menu = true;
void CleanupDeviceD3D();
void ResetDevice();
HWND nwnd;
MSG msg;
int WarningCount = 0;
int Width = GetSystemMetrics(SM_CXSCREEN), Height = GetSystemMetrics(SM_CYSCREEN), teamid, myteamid, d;
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

wstring get_utf16(const string& str, int codepage)
{
    if (str.empty()) return wstring();
    int sz = MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), 0, 0);
    wstring res(sz, 0);
    MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), &res[0], sz);
    return res;
}

static const int B64index[256] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,
    0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63,
    0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

const string b64decode(const void* data, const size_t& len)
{
    //VMProtectBeginUltra("Base64");
    if (len == 0) return "";

    unsigned char* p = (unsigned char*)data;
    size_t j = 0,
        pad1 = len % 4 || p[len - 1] == '=',
        pad2 = pad1 && (len % 4 > 2 || p[len - 2] != '=');
    const size_t last = (len - pad1) / 4 << 2;
    string result(last / 4 * 3 + pad1 + pad2, '\0');
    unsigned char* str = (unsigned char*)&result[0];

    for (size_t i = 0; i < last; i += 4)
    {
        int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }
    if (pad1)
    {
        int n = B64index[p[last]] << 18 | B64index[p[last + 1]] << 12;
        str[j++] = n >> 16;
        if (pad2)
        {
            n |= B64index[p[last + 2]] << 6;
            str[j++] = n >> 8 & 0xFF;
        }
    }
    return result;
    //VMProtectEnd();
}

string encrypt(string msg, string key) {
    string tmp(key);
    while (key.size() < msg.size()) key += tmp;
    for (std::string::size_type i = 0; i < msg.size(); ++i) msg[i] ^= key[i];
    return msg;
}

static const char alphanum[] =
"0123456789"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz";

int stringLength = sizeof(alphanum) - 1;

char genRandom()
{
    return alphanum[rand() % stringLength];
}

std::string get_system_uuid()
{
    if (std::system("wmic csproduct get uuid > HWID.txt") == 0)
    {
        auto file = ::fopen("HWID.txt", "rt, ccs=UNICODE"); // open the file for unicode input

        enum { BUFFSZ = 1000, UUID_SZ = 36 };
        wchar_t wbuffer[BUFFSZ]; // buffer to hold unicode characters

        if (file && // file was succesffully opened
            ::fgetws(wbuffer, BUFFSZ, file) && // successfully read (and discarded) the first line
            ::fgetws(wbuffer, BUFFSZ, file)) // yfully read the second line
        {
            char cstr[BUFFSZ]; // buffer to hold the converted c-style string
            if (::wcstombs(cstr, wbuffer, BUFFSZ) > UUID_SZ) // convert unicode to utf-8
            {
                std::string uuid = cstr;
                while (!uuid.empty() && std::isspace(uuid.back()))
                    uuid.pop_back(); // discard trailing white space
                return uuid;
            }
        }
    }
    return {}; // failed, return empty string
}

int NearCount = 0;

//Draw->AddImage(img, ImVec2(100, 100), ImVec2(244, 300));

void DrawStrokeText(const ImVec2& pos, ImU32 col, const char* str)
{
    ImGui::GetOverlayDrawList()->AddText(NULL, 15.f, ImVec2(pos.x, pos.y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), str);
    ImGui::GetOverlayDrawList()->AddText(NULL, 15.f, ImVec2(pos.x, pos.y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), str);
    ImGui::GetOverlayDrawList()->AddText(NULL, 15.f, ImVec2(pos.x - 1, pos.y), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), str);
    ImGui::GetOverlayDrawList()->AddText(NULL, 15.f, ImVec2(pos.x + 1, pos.y), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), str);
    ImGui::GetOverlayDrawList()->AddText(NULL, 15.f, ImVec2(pos.x, pos.y), col, str);
}

void InitImages() {
    //    AKM, M416, M16A4, SCAL, G36C, UMP, AUG, Groza, AWM, Mini14, SKS, QBZ, M249, MK47, Beryl, QBU, VectorGun,
    //    Kar98K, M24, VSS,
    //    Armor2, Armor3, Head2, Head3, Back2, Back3,
    //    FirstAid, MedKit, EnergyDrink, PainKiller, Adrenaline,
    //    Scope3x, Scope4x, Scope6x, Scope8x, ExtendedQuickDraw, SniperExtendedQuickDraw, Ammo_556, Ammo_762, Ammo_300,
    //    Suppressor, Stock, Compensator, SniperSuppressor, RedDot, HoloGram,
    //    Grenade, SmokeBomb, FlareGun, ProjGrenade, AirDropImg;

    /////////////////////////////////////////////// AirDrop ///////////////////////////////////////////

    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &AirDropSprite, sizeof(AirDropSprite), &AirDropImg);

    /////////////////////////////////////////////// AR ////////////////////////////////////////////

    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &AKMSprite, sizeof(AKMSprite), &AKM);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &M416Sprite, sizeof(M416Sprite), &M416);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &SCALSprite, sizeof(SCALSprite), &SCAL);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &M16A4Sprite, sizeof(M16A4Sprite), &M16A4);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &G36CSprite, sizeof(G36CSprite), &G36C);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &UMPSprite, sizeof(UMPSprite), &UMP);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &AUGSprite, sizeof(AUGSprite), &AUG);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &GrozaSprite, sizeof(GrozaSprite), &Groza);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &AWMSprite, sizeof(AWMSprite), &AWM);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Mini14Sprite, sizeof(Mini14Sprite), &Mini14);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &SKSSprite, sizeof(SKSSprite), &SKS);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &QBZSprite, sizeof(QBZSprite), &QBZ);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &M249Sprite, sizeof(M249Sprite), &M249);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &MK47Sprite, sizeof(MK47Sprite), &MK47);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &BerylSprite, sizeof(BerylSprite), &Beryl);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &QBUSprite, sizeof(QBUSprite), &QBU);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &VectorGunSprite, sizeof(VectorGunSprite), &VectorGun);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Mosin, sizeof(Mosin), &MosinNant);

    ////////////////////////////////////////////////// SR //////////////////////////////////////////////

    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Kar98KSprite, sizeof(Kar98KSprite), &Kar98K);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &M24Sprite, sizeof(M24Sprite), &M24);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &VSSSprite, sizeof(VSSSprite), &VSS);

    ///////////////////////////////////////////////// EquipMents ////////////////////////////////////////////////

    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Armor2Sprite, sizeof(Armor2Sprite), &Armor2);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Armor3Sprite, sizeof(Armor3Sprite), &Armor3);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Head2Sprite, sizeof(Head2Sprite), &Head2);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Head3Sprite, sizeof(Head3Sprite), &Head3);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Back2Sprite, sizeof(Back2Sprite), &Back2);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Back3Sprite, sizeof(Back3Sprite), &Back3);

    ///////////////////////////////////////////////// Heal ////////////////////////////////////////////////

    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &FirstAidSprite, sizeof(FirstAidSprite), &FirstAid);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &MedKitSprite, sizeof(MedKitSprite), &MedKit);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &EnergyDrinkSprite, sizeof(EnergyDrinkSprite), &EnergyDrink);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &PainKillerSprite, sizeof(PainKillerSprite), &PainKiller);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &AdrenalineSprite, sizeof(AdrenalineSprite), &Adrenaline);

    ///////////////////////////////////////////////// WeaponParts //////////////////////////////////////////////////

    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Scope3xSprite, sizeof(Scope3xSprite), &Scope3x);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Scope4xSprite, sizeof(Scope4xSprite), &Scope4x);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Scope6xSprite, sizeof(Scope6xSprite), &Scope6x);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Scope8xSprite, sizeof(Scope8xSprite), &Scope8x);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &ExtendedQuickDrawSprite, sizeof(ExtendedQuickDrawSprite), &ExtendedQuickDraw);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &SniperExtendedQuickDrawSprite, sizeof(SniperExtendedQuickDrawSprite), &SniperExtendedQuickDraw);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &SuppressorSprite, sizeof(SuppressorSprite), &Suppressor);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &StockSprite, sizeof(StockSprite), &Stock);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &CompensatorSprite, sizeof(CompensatorSprite), &Compensator);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &RedDotSprite, sizeof(RedDotSprite), &RedDot);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &HoloGramSprite, sizeof(HoloGramSprite), &HoloGram);
    
    ////////////////////////////////////////////////// Ammo //////////////////////////////////////////////////
        
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Ammo_556Sprite, sizeof(Ammo_556Sprite), &Ammo_556);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Ammo_762Sprite, sizeof(Ammo_762Sprite), &Ammo_762);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &Ammo_300Sprite, sizeof(Ammo_300Sprite), &Ammo_300);

    /////////////////////////////////////////////////// Grenade ///////////////////////////////////////////////

    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &GrenadeSprite, sizeof(GrenadeSprite), &Grenade);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &SmokeBombSprite, sizeof(SmokeBombSprite), &SmokeBomb);
    D3DXCreateTextureFromFileInMemory(g_pd3dDevice, &FlareGunSprite, sizeof(FlareGunSprite), &FlareGun);
}
int ReturnDist();
void DrawMenu()
{
    ImDrawList* Draw = ImGui::GetOverlayDrawList();
    if (cfg->AimVisible) {
        Draw->AddCircle(ImVec2(Width / 2, Height / 2), cfg->Fov + (50 * (80 / GameData::Camera.POV.FOV)), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 128, 1.0f);
        Draw->AddCircleFilled(ImVec2(Width / 2, Height / 2), 3, ImGui::GetColorU32(ImVec4(0, 1, 1, 1)), 256);
    }
    NearCount = 0;

    ImVec2 SpecSize = ImGui::CalcTextSize((u8"관전자 [" + to_string(GameData::SpectatedCount) + u8"] : 200M 내에 있는적 [" + to_string(NearCount) + u8"명] ").c_str());
    Draw->AddLine(ImVec2(Width / 2 - (SpecSize.x / 2) - 15, 107), ImVec2(Width / 2 + (SpecSize.x / 2) - 10, 107), ImGui::GetColorU32(ImVec4(0, 0.5, 0.5, 0.8)), 20);
    Draw->AddText(ImVec2(Width / 2 - (SpecSize.x / 2) - 10, 100), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), (u8"관전자 [" + to_string(GameData::SpectatedCount) + u8"] : 200M 내에 있는적 [" + to_string(NearCount) + u8"명]").c_str());

    ImVec2 DistSize = ImGui::CalcTextSize((u8"주의 : " + to_string(GameData::MinDist) + "M").c_str());

    if (GameData::MinDist > 0)
        Draw->AddLine(ImVec2((Width / 2) - (DistSize.x / 2) - 13, Height - 193), ImVec2((Width / 2) + (DistSize.x / 2) + 10, Height - 193), ImGui::GetColorU32(ImVec4(0, 0.5, 0.5, 0.8)), 20);
    if (GameData::MinDist > 0)
        Draw->AddText(ImVec2((Width / 2) - (DistSize.x / 2), Height - 200), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), (u8"주의 : " + to_string(GameData::MinDist) + "M").c_str());
    GameData::MinDist == 0;

    if (cfg->Menu) {
        ImU32 OnColor = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));
        ImU32 spColor = ImGui::GetColorU32(ImVec4(1, 1, 0, 1));
        ImU32 OffColor = ImGui::GetColorU32(ImVec4(0.7, 0.7, 0.7, 0.7));
        ImU32 SelectedColor = ImGui::GetColorU32(ImVec4(1, 0, 0, 1));
        ImVec2 OnSize = ImGui::CalcTextSize("On");
        ImVec2 OffSize = ImGui::CalcTextSize("Off");
        ImVec2 FPSSize = ImGui::CalcTextSize(to_string((int)round(ImGui::GetIO().Framerate)).c_str());
        ImVec2 OnOff = ImGui::CalcTextSize("< < <");
        ImVec2 OpcaitySize = ImGui::CalcTextSize(to_string((int)(cfg->menu * 100)).c_str());
        ImVec2 FovSize = ImGui::CalcTextSize(to_string((int)(cfg->Fov)).c_str());
        ImVec2 SThickSize = ImGui::CalcTextSize(to_string((int)(cfg->SkeletonThick * 10)).c_str());
        ImVec2 TargetTextSize1 = ImGui::CalcTextSize("머리");
        ImVec2 TargetTextSize2 = ImGui::CalcTextSize("목");
        ImVec2 TargetTextSize3 = ImGui::CalcTextSize("가슴");

        Draw->AddRectFilled(ImVec2(Width - 40, 150), ImVec2(Width - 320, 831), ImGui::GetColorU32(ImVec4(0.31, 0.31, 0.31, cfg->menu)));
        //
        Draw->AddRectFilled(ImVec2(Width - 40, 150), ImVec2(Width - 320, 180), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        Draw->AddText(ImVec2(Width - 250, 157), OnColor, u8"[CRUISE] INSERT 열기/닫기");
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 183), ImVec2(Width - 270, 203), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 1) {
            Draw->AddText(ImVec2(Width - 260, 186), SelectedColor, u8"설정창 투명도");
            Draw->AddText(ImVec2(Width - 80 - (OpcaitySize.x / 2), 186), SelectedColor, to_string((int)(cfg->menu * 100)).c_str());
        }
        else {
            Draw->AddText(ImVec2(Width - 260, 186), OnColor, u8"설정창 투명도");
            Draw->AddText(ImVec2(Width - 80 - (OpcaitySize.x / 2), 186), OnColor, to_string((int)(cfg->menu * 100)).c_str());
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 206), ImVec2(Width - 270, 226), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        Draw->AddText(ImVec2(Width - 260, 209), OnColor, u8"프레임");
        Draw->AddText(ImVec2(Width - 80 - (FPSSize.x / 2), 209), OnColor, to_string((int)round(ImGui::GetIO().Framerate)).c_str());
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 229), ImVec2(Width - 295, 249), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        Draw->AddText(ImVec2(Width - 285, 232), spColor, u8"= ESP설정 =");
        Draw->AddText(ImVec2(Width - 90 - (OnOff.x / 2), 232), spColor, "< < <");
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 252), ImVec2(Width - 270, 272), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 2)
        {
            if (cfg->Skeleton) {
                Draw->AddText(ImVec2(Width - 260, 255), SelectedColor, u8"스켈레톤 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 255), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 255), SelectedColor, u8"스켈레톤 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 255), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->Skeleton) {
                Draw->AddText(ImVec2(Width - 260, 255), OnColor, u8"스켈레톤 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 255), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 255), OffColor, u8"스켈레톤 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 255), OffColor, u8"Off");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 275), ImVec2(Width - 270, 295), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 3) {
            Draw->AddText(ImVec2(Width - 260, 278), SelectedColor, u8"스켈레톤 두깨");
            Draw->AddText(ImVec2(Width - 80 - (SThickSize.x / 2), 278), SelectedColor, to_string(((int)(cfg->SkeletonThick * 10))).c_str());
        }
        else {
            Draw->AddText(ImVec2(Width - 260, 278), OnColor, u8"스켈레톤 두깨");
            Draw->AddText(ImVec2(Width - 80 - (SThickSize.x / 2), 278), OnColor, to_string(((int)(cfg->SkeletonThick * 10))).c_str());
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 298), ImVec2(Width - 270, 318), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 4) {
            if (cfg->Box) {
                Draw->AddText(ImVec2(Width - 260, 301), SelectedColor, u8"박스 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 301), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 301), SelectedColor, u8"박스 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 301), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->Box) {
                Draw->AddText(ImVec2(Width - 260, 301), OnColor, u8"박스 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 301), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 301), OffColor, u8"박스 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 301), OffColor, u8"Off");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 321), ImVec2(Width - 270, 341), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 5) {
            if (cfg->HealthBar) {
                Draw->AddText(ImVec2(Width - 260, 324), SelectedColor, u8"HP 표시");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 324), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 324), SelectedColor, u8"HP 표시");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 324), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->HealthBar) {
                Draw->AddText(ImVec2(Width - 260, 324), OnColor, u8"HP 표시");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 324), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 324), OffColor, u8"HP 표시");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 324), OffColor, u8"Off");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 344), ImVec2(Width - 270, 364), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 6) {
            if (cfg->DistanceESP)
            {
                Draw->AddText(ImVec2(Width - 260, 347), SelectedColor, u8"거리 표시");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 347), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 347), SelectedColor, u8"거리 표시");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 347), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->DistanceESP)
            {
                Draw->AddText(ImVec2(Width - 260, 347), OnColor, u8"거리 표시");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 347), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 347), OffColor, u8"거리 표시");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 347), OffColor, u8"Off");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 367), ImVec2(Width - 270, 387), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 7) {
            if (cfg->PlayerName) {
                Draw->AddText(ImVec2(Width - 260, 370), SelectedColor, u8"팀번호 표시");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 370), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 370), SelectedColor, u8"팀번호 표시");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 370), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->PlayerName) {
                Draw->AddText(ImVec2(Width - 260, 370), OnColor, u8"팀번호 표시");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 370), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 370), OffColor, u8"팀번호 표시");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 370), OffColor, u8"Off");
            }
        }
		//
		Draw->AddRectFilled(ImVec2(Width - 70, 390), ImVec2(Width - 270, 410), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 8) {
            if (cfg->DeathBox) {
                Draw->AddText(ImVec2(Width - 260, 393), SelectedColor, u8"시체상자");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 393), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 393), SelectedColor, u8"시체상자");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 393), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->DeathBox) {
                Draw->AddText(ImVec2(Width - 260, 393), OnColor, u8"시체상자");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 393), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 393), OffColor, u8"시체상자");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 393), OffColor, u8"Off");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 413), ImVec2(Width - 270, 433), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 9) {
            if (cfg->VehicleESP) {
                Draw->AddText(ImVec2(Width - 260, 416), SelectedColor, u8"차량 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 416), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 416), SelectedColor, u8"차량 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 416), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->VehicleESP) {
                Draw->AddText(ImVec2(Width - 260, 416), OnColor, u8"차량 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 416), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 416), OffColor, u8"차량 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 416), OffColor, u8"Off");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 436), ImVec2(Width - 270, 456), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 10) {
            if (cfg->Item) {
                Draw->AddText(ImVec2(Width - 260, 439), SelectedColor, u8"아이템 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 439), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 439), SelectedColor, u8"아이템 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 439), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->Item) {
                Draw->AddText(ImVec2(Width - 260, 439), OnColor, u8"아이템 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 439), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 439), OffColor, u8"아이템 ESP");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 439), OffColor, u8"Off");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 459), ImVec2(Width - 270, 479), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 11) {
            if (cfg->ESPColor) {
                Draw->AddText(ImVec2(Width - 260, 462), SelectedColor, u8"ESP 색상");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 462), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 462), SelectedColor, u8"ESP 색상");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 462), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->ESPColor) {
                Draw->AddText(ImVec2(Width - 260, 462), OnColor, u8"ESP 색상");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 462), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 462), OffColor, u8"ESP 색상");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 462), OffColor, u8"Off");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 482), ImVec2(Width - 270, 502), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 12) {
            if (cfg->AimVisible) {
                Draw->AddText(ImVec2(Width - 260, 485), SelectedColor, u8"에임표시");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 485), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 485), SelectedColor, u8"에임표시");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 485), SelectedColor, u8"OFF");
            }
        }
        else {
            if (cfg->AimVisible) {
                Draw->AddText(ImVec2(Width - 260, 485), OnColor, u8"에임표시");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 485), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 485), OffColor, u8"에임표시");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 485), OffColor, u8"On");
            }
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 505), ImVec2(Width - 270, 525), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 13) {
            if (cfg->fightmode) {
                Draw->AddText(ImVec2(Width - 260, 508), SelectedColor, u8"ESP 기능잠금");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 508), SelectedColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 508), SelectedColor, u8"ESP 기능잠금");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 508), SelectedColor, u8"Off");
            }
        }
        else {
            if (cfg->fightmode) {
                Draw->AddText(ImVec2(Width - 260, 508), OnColor, u8"ESP 기능잠금");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 508), OnColor, u8"On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 508), OffColor, u8"ESP 기능잠금");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 508), OffColor, u8"Off");
            }
        }
        Draw->AddRectFilled(ImVec2(Width - 70, 528), ImVec2(Width - 295, 548), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        Draw->AddText(ImVec2(Width - 285, 531), spColor, u8"= 자동조준 =");
        Draw->AddText(ImVec2(Width - 90 - (OnOff.x / 2), 531), spColor, "< < <");
        //
        ImVec2 SizeofSmooth = ImGui::CalcTextSize(to_string((int)(cfg->AimSmooth * 100)).c_str());
        ImVec2 SizeofSleep = ImGui::CalcTextSize(to_string((int)(cfg->AimSleep)).c_str());
        ImVec2 RadarSize = ImGui::CalcTextSize(to_string((int)(cfg->RadarDist)).c_str());
        Draw->AddRectFilled(ImVec2(Width - 70, 551), ImVec2(Width - 270, 571), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 14) {
            Draw->AddText(ImVec2(Width - 260, 554), SelectedColor, u8"에임속도");
            Draw->AddText(ImVec2(Width - 80 - (SizeofSmooth.x / 2), 554), SelectedColor, to_string((int)(cfg->AimSmooth * 100)).c_str());
        }
        else {
            Draw->AddText(ImVec2(Width - 260, 554), OnColor, u8"에임속도");
            Draw->AddText(ImVec2(Width - 80 - (SizeofSmooth.x / 2), 554), OnColor, to_string((int)(cfg->AimSmooth * 100)).c_str());
        }
        //
        Draw->AddRectFilled(ImVec2(Width - 70, 574), ImVec2(Width - 270, 594), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 15) {
            Draw->AddText(ImVec2(Width - 260, 577), SelectedColor, u8"에임슬립");
            Draw->AddText(ImVec2(Width - 80 - (SizeofSleep.x / 2), 577), SelectedColor, to_string((int)(cfg->AimSleep)).c_str());
        }
        else {
            Draw->AddText(ImVec2(Width - 260, 577), OnColor, u8"에임슬립");
            Draw->AddText(ImVec2(Width - 80 - (SizeofSleep.x / 2), 577), OnColor, to_string((int)(cfg->AimSleep)).c_str());
        }
        Draw->AddRectFilled(ImVec2(Width - 70, 597), ImVec2(Width - 270, 620), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 16) {
            if (cfg->AimBot) {
                Draw->AddText(ImVec2(Width - 260, 600), SelectedColor, u8"자동조준");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 600), SelectedColor, "On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 600), SelectedColor, u8"자동조준");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 600), SelectedColor, "Off");
            }
        }
        else {
            if (cfg->AimBot) {
                Draw->AddText(ImVec2(Width - 260, 600), OnColor, u8"자동조준");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 600), OnColor, "On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 600), OffColor, u8"자동조준");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 600), OffColor, "Off");
            }
        }
        Draw->AddRectFilled(ImVec2(Width - 70, 623), ImVec2(Width - 270, 646), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 17) {
            if (cfg->Target == 1) {
                Draw->AddText(ImVec2(Width - 260, 626), SelectedColor, u8"에임봇 타겟");
                Draw->AddText(ImVec2(Width - 90 - (TargetTextSize1.x), 626), SelectedColor, u8"머리");
            }
            else if (cfg->Target == 2) {
                Draw->AddText(ImVec2(Width - 260, 626), SelectedColor, u8"에임봇 타겟");
                Draw->AddText(ImVec2(Width - 90 - (TargetTextSize2.x), 626), SelectedColor, u8"목");
            }
            else if (cfg->Target == 3) {
                Draw->AddText(ImVec2(Width - 260, 626), SelectedColor, u8"에임봇 타겟");
                Draw->AddText(ImVec2(Width - 90 - (TargetTextSize3.x), 626), SelectedColor, u8"배");
            }
        }
        else {
            if (cfg->Target == 1) {
                Draw->AddText(ImVec2(Width - 260, 626), OnColor, u8"에임봇 타겟");
                Draw->AddText(ImVec2(Width - 90 - (TargetTextSize1.x), 626), OnColor, u8"머리");
            }
            else if (cfg->Target == 2) {
                Draw->AddText(ImVec2(Width - 260, 626), OnColor, u8"에임봇 타겟");
                Draw->AddText(ImVec2(Width - 90 - (TargetTextSize2.x), 626), OnColor, u8"목");
            }
            else if (cfg->Target == 3) {
                Draw->AddText(ImVec2(Width - 260, 626), OnColor, u8"에임봇 타겟");
                Draw->AddText(ImVec2(Width - 90 - (TargetTextSize3.x), 626), OnColor, u8"배");
            }
        }

        Draw->AddRectFilled(ImVec2(Width - 70, 649), ImVec2(Width - 270, 672), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 18) {
            Draw->AddText(ImVec2(Width - 260, 652), SelectedColor, u8"FOV");
            Draw->AddText(ImVec2(Width - 80 - (FovSize.x / 2), 652), SelectedColor, to_string((int)(cfg->Fov)).c_str());
        }
        else {
            Draw->AddText(ImVec2(Width - 260, 652), OnColor, u8"FOV");
            Draw->AddText(ImVec2(Width - 80 - (FovSize.x / 2), 652), OnColor, to_string((int)(cfg->Fov)).c_str());
        }

        Draw->AddRectFilled(ImVec2(Width - 70, 675), ImVec2(Width - 295, 698), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        Draw->AddText(ImVec2(Width - 285, 678), spColor, u8"= 벽구분 / 인힛무반 =");
        Draw->AddText(ImVec2(Width - 90 - (OnOff.x / 2), 678), spColor, "< < <");

        Draw->AddRectFilled(ImVec2(Width - 70, 701), ImVec2(Width - 270, 724), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 19) {
            if (cfg->SpeedHack) {
                Draw->AddText(ImVec2(Width - 260, 703), SelectedColor, u8"에임봇 벽뒤구분");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 703), SelectedColor, "On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 703), SelectedColor, u8"에임봇 벽뒤구분");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 703), SelectedColor, "Off");
            }
        }
        else {
            if (cfg->SpeedHack) {
                Draw->AddText(ImVec2(Width - 260, 703), OnColor, u8"에임봇 벽뒤구분");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 703), OnColor, "On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 703), OffColor, u8"에임봇 벽뒤구분");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 703), OffColor, "Off");
            }
        }
        Draw->AddRectFilled(ImVec2(Width - 70, 727), ImVec2(Width - 270, 750), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 20) {
            if (cfg->NoRecoil) {
                Draw->AddText(ImVec2(Width - 260, 730), SelectedColor, u8"인힛 / 무반 [ 위험 ]");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 730), SelectedColor, "On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 730), SelectedColor, u8"인힛 / 무반 [ 위험 ]");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 730), SelectedColor, "Off");
            }
        }
        else {
            if (cfg->NoRecoil) {
                Draw->AddText(ImVec2(Width - 260, 730), OnColor, u8"인힛 / 무반 [ 위험 ]");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 730), OnColor, "On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 730), OffColor, u8"인힛 / 무반 [ 위험 ]");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 730), OffColor, "Off");
            }
        }
        Draw->AddRectFilled(ImVec2(Width - 70, 753), ImVec2(Width - 295, 776), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        Draw->AddText(ImVec2(Width - 285, 756), spColor, u8"= 레이더 / 범위조정 =");
        Draw->AddText(ImVec2(Width - 90 - (OnOff.x / 2), 756), spColor, "< < <");
        Draw->AddRectFilled(ImVec2(Width - 70, 779), ImVec2(Width - 270, 802), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 21) {
            if (cfg->radar) {
                Draw->AddText(ImVec2(Width - 260, 782), SelectedColor, u8"레이더");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 782), SelectedColor, "On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 782), SelectedColor, u8"레이더");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 782), SelectedColor, "Off");
            }
        }
        else {
            if (cfg->radar) {
                Draw->AddText(ImVec2(Width - 260, 782), OnColor, u8"레이더");
                Draw->AddText(ImVec2(Width - 80 - (OnSize.x / 2), 782), OnColor, "On");
            }
            else {
                Draw->AddText(ImVec2(Width - 260, 782), OffColor, u8"레이더");
                Draw->AddText(ImVec2(Width - 80 - (OffSize.x / 2), 782), OffColor, "Off");
            }
        }
        Draw->AddRectFilled(ImVec2(Width - 70, 805), ImVec2(Width - 270, 828), ImGui::GetColorU32(ImVec4(0.26, 0.26, 0.26, cfg->menu)));
        if (cfg->Mode == 22) {
            Draw->AddText(ImVec2(Width - 260, 808), SelectedColor, u8"레이더 거리");
            Draw->AddText(ImVec2(Width - 80 - (RadarSize.x / 2), 808), SelectedColor, to_string((int)(cfg->RadarDist)).c_str());
        }
        else {
            Draw->AddText(ImVec2(Width - 260, 808), OnColor, u8"레이더 거리");
            Draw->AddText(ImVec2(Width - 80 - (RadarSize.x / 2), 808), OnColor, to_string((int)(cfg->RadarDist)).c_str());
        }
    }
}

void DrawInfo(const ImVec2& Pos, ImU32 TagCol, int HeightSize, string Info, float EnHealth = 0) {
    ImDrawList* Draw = ImGui::GetOverlayDrawList();
    ImVec2 InfoSize = ImGui::CalcTextSize(Info.c_str());
    Draw->AddRectFilled(ImVec2(Pos.x - (InfoSize.x / 1.5), Pos.y + (HeightSize / 2)), ImVec2(Pos.x + (InfoSize.x / 1.5) + 1, Pos.y - (HeightSize / 2)), ImGui::GetColorU32(ImVec4(0,0,0,0.7)));
    Draw->AddRectFilled(ImVec2(Pos.x - (InfoSize.x / 1.5), Pos.y + (HeightSize / 2)), ImVec2(Pos.x - (InfoSize.x / 1.5) + 5, Pos.y - (HeightSize / 2)), TagCol);
    if (EnHealth != 0) {
        Draw->AddRectFilled(ImVec2(Pos.x - (InfoSize.x / 1.5) + 7, Pos.y + (HeightSize / 2) - 6),ImVec2(Pos.x - (InfoSize.x / 1.5) + 7 + ((abs((Pos.x - (InfoSize.x / 1.5) + 7) - (Pos.x + (InfoSize.x / 1.5)))/100.0f) * EnHealth), Pos.y + (HeightSize / 2) - 2), ImGui::GetColorU32(ImVec4(0, 1, 0, 1)));
    }
    DrawStrokeText(ImVec2(Pos.x - ((InfoSize.x / 1.5)) + 10, Pos.y - (HeightSize / 2)), ImGui::GetColorU32(ImVec4(1,1,1,1)), Info.c_str());
}

void DrawEdges(const ImVec2& top, const ImVec2& bot, const ImVec2& base, ImU32 col)
{
    ImDrawList* Draw = ImGui::GetOverlayDrawList();
    float scale = (bot.y - top.y) / 3.0f;
    float length = scale / 2;
    Draw->AddLine(ImVec2(base.x - scale, top.y), ImVec2(base.x - scale + length, top.y), col, 1.0f); //  --- Top left
    Draw->AddLine(ImVec2(base.x - scale, top.y), ImVec2(base.x - scale, top.y + length), col, 1.0f); // | Top left
    Draw->AddLine(ImVec2(base.x + scale / 3, top.y), ImVec2(base.x + scale / 3 + length, top.y), col, 1.0f); // --- Top right
    Draw->AddLine(ImVec2(base.x + scale / 3 + length, top.y), ImVec2(base.x + scale / 3 + length, top.y + length), col, 1.0f); // | Top right
    Draw->AddLine(ImVec2(base.x - scale, bot.y), ImVec2(base.x - scale + length, bot.y), col, 1.0f); // --- Bottom left
    Draw->AddLine(ImVec2(base.x - scale, bot.y), ImVec2(base.x - scale, bot.y - length), col, 1.0f); // | Bottom left
    Draw->AddLine(ImVec2(base.x + scale / 3 + length, bot.y), ImVec2(base.x + scale / 3, bot.y), col, 1.0f); // --- Bottom right
    Draw->AddLine(ImVec2(base.x + scale / 3 + length, bot.y), ImVec2(base.x + scale / 3 + length, bot.y - length), col, 1.0f); // | Bottom right
}

int ReturnDist() {
    float closest_dist = FLT_MAX;
    int cloest_index = -1;

    if (!GameData::Actors.size())
        return 0;

    Vector3 local_pos = GameData::Camera.POV.Location;

    for (int i = 0; i < GameData::Actors.size(); i++)
    {
        const auto ent = GameData::Actors[i].AActor_Base;
        if (GameData::Actors[i].TeamNum == GameData::mynumber || !GameData::Actors[i].Health > 0.0)
            continue;

        Vector3 ent_pos = GameData::Actors[i].Location;
        const auto dist = ent_pos.Distance(local_pos);

        if (dist < closest_dist)
        {
            closest_dist = dist;
            cloest_index = i;
        }
    }
    return cloest_index;
}

void DrawScense() {
    vector<AActor> UGameActors;
    UGameActors.clear();
    UGameActors.assign(GameData::Actors.begin(), GameData::Actors.end());
    //UGameActors = GameData::Actors;
    
    ImVec2 DeathDropTitle = ImGui::CalcTextSize("| 시체상자");
    ImDrawList* Draw = ImGui::GetOverlayDrawList();
    ImU32 cColor, dColor, bColor, VehicleColor;
    if (cfg->ESPColor) {
        cColor = ImGui::GetColorU32(ImVec4(0.843, 1, 0.29, 1));
        dColor = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));
        bColor = ImGui::GetColorU32(ImVec4(0.662, 0.952, 0.9, 1));
        VehicleColor = ImGui::GetColorU32(ImVec4(0, 1, 1, 1));
    }
    else {
        cColor = ImGui::GetColorU32(ImVec4(0, 1, 0, 1));
        dColor = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));
        bColor = ImGui::GetColorU32(ImVec4(1, 0, 0, 1));
        VehicleColor = ImGui::GetColorU32(ImVec4(0, 1, 1, 1));
    }
    FCameraCacheEntry DrawCamera = GameData::UpdateCamera();
    uint64_t PlayerPawn;
    //cout << "Num Actors : " << GameData::Actors.size() << endl;
    for (int i = 0; i < GameData::Actors.size(); i++)
    {
        if(UGameActors[i].AActor_Base != NULL)
			PlayerPawn = UGameActors[i].AActor_Base;
		ImVec2 HeadDist = ImGui::CalcTextSize(("[ " + to_string((int)UGameActors[i].Distance) + " ]").c_str());
		int MyRenderTime;
		Vector3 W2S;
        try {
            UGameActors[i].RenderTime = (int)GameData::getLastRenderTime(PlayerPawn);
        }catch(...){}
        try {
            MyRenderTime = (int)GameData::mytime;
        }catch(...){}
        try {
            W2S = WorldToScreen(UGameActors[i].Location, DrawCamera);
        }catch(...){}

        //Draw->AddText(ImVec2(W2S.x, W2S.y), cColor, to_string(UGameActors[i].ActorID).c_str());

        int distance;
        try {
            if ((int)UGameActors[i].Distance >= 0 && (int)UGameActors[i].Distance <= 16000)
                distance = (int)UGameActors[i].Distance;
        }catch(...){}
        try {
            if (UGameActors[i].Distance <= 200.0f && UGameActors[i].TeamNum != GameData::mynumber)
                NearCount++;
        }catch(...){}
        //for (int i = 0; i < GameData::Actors.size(); i++)
        //{
        //    if (UGameActors[i].Distance <= 200.0f && UGameActors[i].TeamNum != GameData::mynumber)
        //        NearCount++;
        //}

        int distid = ReturnDist();
        try {
            GameData::MinDist = GameData::Actors[distid].Distance;
        }catch(...){}

        if (UGameActors[i].TeamNum != GameData::mynumber && PlayerPawn != NULL) {
            Vector3 W2S = WorldToScreen(UGameActors[i].Location, DrawCamera);
            float Health = UGameActors[i].Health;
            ImVec2 HealthSize = ImGui::CalcTextSize((to_string(Health) + "/100").c_str());
            ImVec2 DistSize = ImGui::CalcTextSize((to_string((int)round(distance)) + "M").c_str());

            Vector3 w2s_1 = WorldToScreen(Vector3(UGameActors[i].Skeleton.ForeHead.x, UGameActors[i].Skeleton.ForeHead.y, UGameActors[i].Skeleton.ForeHead.z + 20.0f), DrawCamera);
            Vector3 boxtop = WorldToScreen(Vector3(UGameActors[i].Skeleton.pelvis.x, UGameActors[i].Skeleton.pelvis.y, UGameActors[i].Skeleton.pelvis.z + 90.0f), DrawCamera);
            Vector3 InfoTop = WorldToScreen(Vector3(UGameActors[i].Skeleton.pelvis.x, UGameActors[i].Skeleton.pelvis.y, UGameActors[i].Skeleton.ForeHead.z + 50.0f), DrawCamera);
            Vector3 boxbot = WorldToScreen(UGameActors[i].Skeleton.root, DrawCamera);
            Vector3 vforehead, vneck, vupperarm_l, varm_l, vhand_l, vupperarm_r, varm_r, vhand_r, vspine1, vspine2, vpelvis, vthigh_l, vcalf_l, vfoot_l, vthigh_r, vcalf_r, vfoot_r, vroot;

            if (UGameActors[i].Type == AirDrop && W2S.x < Width && W2S.x > 0 && W2S.y < Width && W2S.y > 0) {
                Draw->AddImage(AirDropImg, ImVec2(W2S.x - 36, W2S.y - 50), ImVec2(W2S.x + 36, W2S.y + 50));
                Draw->AddText(ImVec2(W2S.x - (HeadDist.x / 2), W2S.y), VehicleColor, ("[ " + to_string((int)distance) + " ]").c_str());
            }

            if (UGameActors[i].Type == Grenades && W2S.x < Width && W2S.x > 0 && W2S.y < Width && W2S.y > 0) {
                Draw->AddImage(AirDropImg, ImVec2(W2S.x - 16, W2S.y - 16), ImVec2(W2S.x + 16, W2S.y + 16));
                Draw->AddText(ImVec2(W2S.x - (HeadDist.x / 2), W2S.y), VehicleColor, ("[ " + to_string((int)distance) + " ]").c_str());
            }

            if (W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0) {

                vforehead = WorldToScreen(UGameActors[i].Skeleton.ForeHead, DrawCamera);
                vneck = WorldToScreen(UGameActors[i].Skeleton.Neck, DrawCamera);

                vupperarm_l = WorldToScreen(UGameActors[i].Skeleton.upperarm_l, DrawCamera);
                varm_l = WorldToScreen(UGameActors[i].Skeleton.arm_l, DrawCamera);
                vhand_l = WorldToScreen(UGameActors[i].Skeleton.hand_l, DrawCamera);

                vupperarm_r = WorldToScreen(UGameActors[i].Skeleton.upperarm_r, DrawCamera);
                varm_r = WorldToScreen(UGameActors[i].Skeleton.arm_r, DrawCamera);
                vhand_r = WorldToScreen(UGameActors[i].Skeleton.hand_r, DrawCamera);

                vspine1 = WorldToScreen(UGameActors[i].Skeleton.Spine1, DrawCamera);
                vspine2 = WorldToScreen(UGameActors[i].Skeleton.Spine2, DrawCamera);
                vpelvis = WorldToScreen(UGameActors[i].Skeleton.pelvis, DrawCamera);

                vthigh_l = WorldToScreen(UGameActors[i].Skeleton.thigh_l, DrawCamera);
                vcalf_l = WorldToScreen(UGameActors[i].Skeleton.calf_l, DrawCamera);
                vfoot_l = WorldToScreen(UGameActors[i].Skeleton.foot_l, DrawCamera);

                vthigh_r = WorldToScreen(UGameActors[i].Skeleton.thigh_r, DrawCamera);
                vcalf_r = WorldToScreen(UGameActors[i].Skeleton.calf_r, DrawCamera);
                vfoot_r = WorldToScreen(UGameActors[i].Skeleton.foot_r, DrawCamera);

                vroot = WorldToScreen(UGameActors[i].Skeleton.root, DrawCamera);
            }
            void DrawInfo(const ImVec2& Pos, ImU32 TagCol, int HeightSize, string Info, float EnHealth = 0);

            ImVec2 CalcedPlayerName = ImGui::CalcTextSize(("Team : " + to_string(UGameActors[i].TeamNum)).c_str());
            ImVec2 CalcedBotPlayerName = ImGui::CalcTextSize("[ AI : Bot ][ 봇 ]");
            if (distance < cfg->Distance && Health > 0.0 && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0 && cfg->PlayerName) {
                //Draw->AddText(ImVec2(vpelvis.x - (CalcedPlayerName.x / 2), vroot.y - 10), dColor, ("Team : " + to_string(UGameActors[i].TeamNum)).c_str());
                DrawStrokeText(ImVec2(vpelvis.x - (CalcedPlayerName.x / 2), vroot.y - 10), dColor, ("Team : " + to_string(UGameActors[i].TeamNum)).c_str());
                //DrawInfo(ImVec2(InfoTop.x, InfoTop.y), ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), 25, ("[ Team : " + to_string(UGameActors[i].TeamNum) + " ][ " + to_string(distance) + "M ][ HP : " + to_string((int)Health) + " ]"), Health);
            }
            if (distance < cfg->Distance && Health > 0.0 && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0 && cfg->HealthBar) {
                Draw->AddRectFilled(ImVec2(vpelvis.x - 21, vroot.y + 3), ImVec2(vpelvis.x + 21, vroot.y + 17), ImGui::GetColorU32(ImVec4(0, 0, 0, 1)));
                Draw->AddRectFilled(ImVec2(vpelvis.x - 20, vroot.y + 4), ImVec2(vpelvis.x + 20, vroot.y + 16), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
                Draw->AddRectFilled(ImVec2(vpelvis.x - 20, vroot.y + 4), ImVec2(vpelvis.x - 20 + (Health * 0.4), vroot.y + 16), ImGui::GetColorU32(ImVec4(0, 1, 0, 1)));
            }
            ImVec2 CalcedDistance = ImGui::CalcTextSize(to_string((int)distance).c_str());
            if (distance < cfg->Distance && Health > 0.0 && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0 && cfg->DistanceESP) {
                Draw->AddText(ImVec2(vpelvis.x - (CalcedDistance.x / 2), vroot.y + 4), ImGui::GetColorU32(ImVec4(0, 0, 0, 1)), to_string((int)distance).c_str());
                //DrawStrokeText(ImVec2(vpelvis.x - (CalcedDistance.x / 2), vroot.y + 4), ImGui::GetColorU32(ImVec4(0, 0, 0, 1)), to_string((int)distance).c_str());
            }
            if (distance < cfg->Distance && Health > 0.0 && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0) {
                if (UGameActors[i].ActorID == Vehicle::Bot1 || UGameActors[i].ActorID == Vehicle::Bot2 || UGameActors[i].ActorID == Vehicle::Bot3 || UGameActors[i].ActorID == Vehicle::Bot4) {
                    DrawStrokeText(ImVec2(vpelvis.x - (CalcedBotPlayerName.x / 2), vroot.y - 30), dColor, "[ AI : Bot ][ 봇 ]");
                }
            }
            Vector3 CrossHair = Vector3(Width / 2, Height / 2, 0);
            if (GameData::aimindex != -1 && GameData::Actors[GameData::aimindex].Health > 0.0) {
                Vector3 Target, HeadPos, NeckPos;

                if(cfg->Target == 1)
                    Target = GameData::Actors[GameData::aimindex].Skeleton.ForeHead;
                else if (cfg->Target == 2)
                    Target = GameData::Actors[GameData::aimindex].Skeleton.Neck;
                else if (cfg->Target == 3)
                    Target = GameData::Actors[GameData::aimindex].Skeleton.Spine1;
                if(GetAsyncKeyState(VK_SHIFT))
                    Target = GameData::Actors[GameData::aimindex].Skeleton.ForeHead;

                HeadPos = WorldToScreen(GameData::Actors[GameData::aimindex].Skeleton.ForeHead, GameData::Camera);
                NeckPos = WorldToScreen(GameData::Actors[GameData::aimindex].Skeleton.Neck, GameData::Camera);

                float Length = HeadPos.Distance(NeckPos);

                Vector3 W2S = WorldToScreen(Target, GameData::Camera);
                if(HeadPos.Distance(CrossHair) <= cfg->Fov + (50 * (80 / GameData::Camera.POV.FOV)))
                    Draw->AddRect(ImVec2(W2S.x - Length, W2S.y - Length), ImVec2(W2S.x + Length, W2S.y + Length), ImGui::GetColorU32(ImVec4(0, 1, 1, 1)), 0.0f, 15, 1.0f);
            }

            if (cfg->AimVisible && i == GameData::aimindex && vforehead.Distance(CrossHair) <= cfg->Fov + (50 * (80 / GameData::Camera.POV.FOV)) && GameData::Actors[GameData::aimindex].Health > 0.0 && UGameActors[i].Distance < cfg->Distance) {
                if(cfg->Target == 1)
                    Draw->AddLine(ImVec2(vforehead.x, vforehead.y), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32(ImVec4(0, 1, 1, 1)), 1.0f);
                else if (cfg->Target == 2)
                    Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32(ImVec4(0, 1, 1, 1)), 1.0f);
                else if (cfg->Target == 3)
                    Draw->AddLine(ImVec2(vspine1.x, vspine1.y), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32(ImVec4(0, 1, 1, 1)), 1.0f);
            }
            else if (cfg->AimVisible && vforehead.Distance(CrossHair) <= cfg->Fov + (50 * (80 / GameData::Camera.POV.FOV)) && UGameActors[i].Health > 0.0 && UGameActors[i].Distance < cfg->Distance)
            {
                if (cfg->Target == 1)
                    Draw->AddLine(ImVec2(vforehead.x, vforehead.y), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32(ImVec4(1, 1, 0, 1)), 1.0f);
                else if (cfg->Target == 2)
                    Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32(ImVec4(1, 1, 0, 1)), 1.0f);
                else if (cfg->Target == 3)
                    Draw->AddLine(ImVec2(vspine1.x, vspine1.y), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32(ImVec4(1, 1, 0, 1)), 1.0f);
            }

            if (UGameActors[i].RenderTime == MyRenderTime && distance < cfg->Distance && Health > 0.0 && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0 && cfg->Skeleton) {
                Draw->AddLine(ImVec2(vforehead.x, vforehead.y), ImVec2(vneck.x, vneck.y), cColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vupperarm_l.x, vupperarm_l.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vupperarm_l.x, vupperarm_l.y), ImVec2(varm_l.x, varm_l.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(varm_l.x, varm_l.y), ImVec2(vhand_l.x, vhand_l.y), cColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vupperarm_r.x, vupperarm_r.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vupperarm_r.x, vupperarm_r.y), ImVec2(varm_r.x, varm_r.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(varm_r.x, varm_r.y), ImVec2(vhand_r.x, vhand_r.y), cColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vspine2.x, vspine2.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vspine2.x, vspine2.y), ImVec2(vspine1.x, vspine1.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vspine1.x, vspine1.y), ImVec2(vpelvis.x, vpelvis.y), cColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vpelvis.x, vpelvis.y), ImVec2(vthigh_l.x, vthigh_l.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vthigh_l.x, vthigh_l.y), ImVec2(vcalf_l.x, vcalf_l.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vcalf_l.x, vcalf_l.y), ImVec2(vfoot_l.x, vfoot_l.y), cColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vpelvis.x, vpelvis.y), ImVec2(vthigh_r.x, vthigh_r.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vthigh_r.x, vthigh_r.y), ImVec2(vcalf_r.x, vcalf_r.y), cColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vcalf_r.x, vcalf_r.y), ImVec2(vfoot_r.x, vfoot_r.y), cColor, cfg->SkeletonThick);
            }
            else if (UGameActors[i].RenderTime != MyRenderTime && distance < cfg->Distance && Health > 0.0 && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0 && cfg->Skeleton) {
                Draw->AddLine(ImVec2(vforehead.x, vforehead.y), ImVec2(vneck.x, vneck.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vupperarm_l.x, vupperarm_l.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vupperarm_l.x, vupperarm_l.y), ImVec2(varm_l.x, varm_l.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(varm_l.x, varm_l.y), ImVec2(vhand_l.x, vhand_l.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vupperarm_r.x, vupperarm_r.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vupperarm_r.x, vupperarm_r.y), ImVec2(varm_r.x, varm_r.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(varm_r.x, varm_r.y), ImVec2(vhand_r.x, vhand_r.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vspine2.x, vspine2.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vspine2.x, vspine2.y), ImVec2(vspine1.x, vspine1.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vspine1.x, vspine1.y), ImVec2(vpelvis.x, vpelvis.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vpelvis.x, vpelvis.y), ImVec2(vthigh_l.x, vthigh_l.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vthigh_l.x, vthigh_l.y), ImVec2(vcalf_l.x, vcalf_l.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vcalf_l.x, vcalf_l.y), ImVec2(vfoot_l.x, vfoot_l.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vpelvis.x, vpelvis.y), ImVec2(vthigh_r.x, vthigh_r.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vthigh_r.x, vthigh_r.y), ImVec2(vcalf_r.x, vcalf_r.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vcalf_r.x, vcalf_r.y), ImVec2(vfoot_r.x, vfoot_r.y), bColor, cfg->SkeletonThick);
            }
            else if (distance < cfg->Distance && UGameActors[i].GroggyHealth > 0.0 && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0 && cfg->Skeleton) {
                Draw->AddLine(ImVec2(vforehead.x, vforehead.y), ImVec2(vneck.x, vneck.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vupperarm_l.x, vupperarm_l.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vupperarm_l.x, vupperarm_l.y), ImVec2(varm_l.x, varm_l.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(varm_l.x, varm_l.y), ImVec2(vhand_l.x, vhand_l.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vupperarm_r.x, vupperarm_r.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vupperarm_r.x, vupperarm_r.y), ImVec2(varm_r.x, varm_r.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(varm_r.x, varm_r.y), ImVec2(vhand_r.x, vhand_r.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vneck.x, vneck.y), ImVec2(vspine2.x, vspine2.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vspine2.x, vspine2.y), ImVec2(vspine1.x, vspine1.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vspine1.x, vspine1.y), ImVec2(vpelvis.x, vpelvis.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vpelvis.x, vpelvis.y), ImVec2(vthigh_l.x, vthigh_l.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vthigh_l.x, vthigh_l.y), ImVec2(vcalf_l.x, vcalf_l.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vcalf_l.x, vcalf_l.y), ImVec2(vfoot_l.x, vfoot_l.y), bColor, cfg->SkeletonThick);

                Draw->AddLine(ImVec2(vpelvis.x, vpelvis.y), ImVec2(vthigh_r.x, vthigh_r.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vthigh_r.x, vthigh_r.y), ImVec2(vcalf_r.x, vcalf_r.y), bColor, cfg->SkeletonThick);
                Draw->AddLine(ImVec2(vcalf_r.x, vcalf_r.y), ImVec2(vfoot_r.x, vfoot_r.y), bColor, cfg->SkeletonThick);
            }
            if (cfg->Box && distance < cfg->Distance && Health > 0.0) {
                if (UGameActors[i].RenderTime == MyRenderTime && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0)
                    DrawEdges(ImVec2(boxtop.x, boxtop.y), ImVec2(boxbot.x, boxbot.y), ImVec2(vpelvis.x, 0), cColor);
                else if (UGameActors[i].RenderTime != MyRenderTime && W2S.x < Width && W2S.x > 0 && W2S.y < Height && W2S.y > 0)
                    DrawEdges(ImVec2(boxtop.x, boxtop.y), ImVec2(boxbot.x, boxbot.y), ImVec2(vpelvis.x, 0), bColor);
            }
        }
        if (UGameActors[i].Type == DeathBox && PlayerPawn != NULL && cfg->DeathBox && !cfg->fightmode)
        {
            Vector3 ScPos = WorldToScreen(UGameActors[i].Location, DrawCamera);
            uint64_t DeathDropItems = mem->RVM<uint64_t>(PlayerPawn + offs_ITEM_PACKAGE);
            int DeathDropItemCount = mem->RVM<int>(PlayerPawn + offs_ITEM_PACKAGE + 0x8);
			if (DeathDropItemCount > 0 && DeathDropItems != NULL)
			{
                DrawStrokeText(ImVec2(ScPos.x, ScPos.y), ImGui::GetColorU32(ImVec4(0, 1, 0, 1)), u8"| 시체상자");
				ScPos.y += DeathDropTitle.y;
				for (int i = 0; i < DeathDropItemCount; i++)
				{
					uint64_t Item = mem->RVM<uint64_t>(DeathDropItems + (i * 0x8));
					int ItemID = mem->RVM<int>(Item + offs_UITEM_ID);
					std::string ItemName = GameData::GetItemNameByID(ItemID);
					if (ItemName != "NULL" && Item != NULL)
					{
						if (ItemName == "AKM" || ItemName == "M16A4" || ItemName == "SCAL-L" || ItemName == "G36-C" || ItemName == "UMP" || ItemName == "Mini14" || ItemName == "QBZ" || ItemName == "M249" || ItemName == "MK47" || ItemName == "BerylM762" || ItemName == "QBU" || ItemName == "Vector" || ItemName == "VSS")
                            DrawStrokeText(ImVec2(ScPos.x, ScPos.y), ImGui::GetColorU32(ImVec4(0, 1, 0, 1)), ("| " + ItemName).c_str());
						else if (ItemName == "M416" || ItemName == "AUG" || ItemName == "Groza" || ItemName == "SKS" || ItemName == "Kar98K" || ItemName == "M24" || ItemName == "AWM" || ItemName == u8"모신나강")
                            DrawStrokeText(ImVec2(ScPos.x, ScPos.y), ImGui::GetColorU32(ImVec4(0.5, 0, 1, 1)), ("| " + ItemName).c_str());
						else if (ItemName == u8"플레어건" || ItemName == u8"2레벨 갑옷" || ItemName == u8"3레벨 갑옷" || ItemName == u8"2레벨 헬맷" || ItemName == u8"3레벨 헬맷" || ItemName == u8"3레벨 가방" || ItemName == u8"소음기" || ItemName == u8"소음기(SR)" || ItemName == u8"개머리판" || ItemName == u8"보정기" || ItemName == u8"6배율" || ItemName == u8"4배율" || ItemName == u8"8배율" || ItemName == u8"대용량 퀵드로우 탄창" || ItemName == u8"대용량 퀵드로우 탄창(SR)")
                            DrawStrokeText(ImVec2(ScPos.x, ScPos.y), ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), ("| " + ItemName).c_str());
						else if (ItemName == "5.56mm" || ItemName == "7.62mm" || ItemName == "300 매그넘")
                            DrawStrokeText(ImVec2(ScPos.x, ScPos.y), ImGui::GetColorU32(ImVec4(1, 1, 0, 1)), ("| " + ItemName).c_str());
						else if (ItemName == u8"레드도트" || ItemName == u8"홀로그램" || ItemName == u8"수류탄" || ItemName == u8"연막탄" || ItemName == u8"2레벨 가방" || ItemName == u8"3배율")
                            DrawStrokeText(ImVec2(ScPos.x, ScPos.y), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), ("| " + ItemName).c_str());
						else if (ItemName == u8"구급상자" || ItemName == u8"의료용키트")
                            DrawStrokeText(ImVec2(ScPos.x, ScPos.y), ImGui::GetColorU32(ImVec4(1, 0.2, 0.6, 1)), ("| " + ItemName).c_str());
						else if (ItemName == u8"에너지드링크" || ItemName == u8"진통제" || ItemName == u8"아드레날린 주사기")
                            DrawStrokeText(ImVec2(ScPos.x, ScPos.y), ImGui::GetColorU32(ImVec4(0, 0, 0.5, 1)), ("| " + ItemName).c_str());
						ScPos.y += DeathDropTitle.y;
					}
                }
            }
        }
        if (UGameActors[i].Type == Item && cfg->Item && !cfg->fightmode)
        {
            uint64_t pItemArray = mem->RVM<uint64_t>(UGameActors[i].AActor_Base + offs_DroppedItemGroup);
            int pItemCount = mem->RVM<int>(UGameActors[i].AActor_Base + offs_DroppedItemGroup + 0x8);

			if (!pItemArray || !pItemCount || pItemCount > 100)
				continue;

			for (int j = 0; j < pItemCount; j++)
			{
                uint64_t pItemObject = mem->RVM<uint64_t>(pItemArray + (j * 0x10));
				if (pItemObject)
				{
                    uint64_t pUItemAddress = mem->RVM<uint64_t>(pItemObject + offs_DroppedItemGroup_UItem);
					if (pUItemAddress)
					{
						int pUItemID = mem->RVM<int>(pUItemAddress + offs_UITEM_ID);
						if (pUItemID > 0 && pUItemID < 399999)
						{
							string ItemName = GameData::GetItemNameByID(pUItemID);
							if (ItemName != "NULL")
							{
								Vector3 Pos = mem->RVM<Vector3>(pItemObject + __OFFSET__Location);
								Vector3 ScPos = WorldToScreen(Pos, DrawCamera);
								Pos.z -= 5.0f;
								Vector3 TextPos = WorldToScreen(Pos, DrawCamera);

								if (ScPos.x > 0 && ScPos.x < Width && ScPos.y > 0 && ScPos.y < Height)
								{
									if (ItemName == "AKM") {
										Draw->AddImage(AKM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

                                        Draw->AddImage(AKM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										Draw->AddImage(AKM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										Draw->AddImage(AKM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										Draw->AddImage(AKM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "M416") {
										Draw->AddImage(M416, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(M416, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M416, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M416, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M416, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "M16A4") {
										Draw->AddImage(M16A4, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(M16A4, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M16A4, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M16A4, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M16A4, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "SCAL-L") {
										Draw->AddImage(SCAL, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(SCAL, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SCAL, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SCAL, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SCAL, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "G36-C") {
										Draw->AddImage(G36C, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(G36C, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(G36C, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(G36C, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(G36C, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "UMP") {
										Draw->AddImage(UMP, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(UMP, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(UMP, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(UMP, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(UMP, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "AUG") {
										Draw->AddImage(AUG, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(AUG, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(AUG, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(AUG, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(AUG, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "Groza") {
										Draw->AddImage(Groza, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Groza, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Groza, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Groza, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Groza, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "AWM") {
										Draw->AddImage(AWM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(AWM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(AWM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(AWM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(AWM, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "Mini14") {
										Draw->AddImage(Mini14, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Mini14, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Mini14, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Mini14, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Mini14, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "SKS") {
										Draw->AddImage(SKS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(SKS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SKS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SKS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SKS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "QBZ") {
										Draw->AddImage(QBZ, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(QBZ, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(QBZ, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(QBZ, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(QBZ, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "M249") {
										Draw->AddImage(M249, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(M249, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M249, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M249, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M249, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "MK47") {
										Draw->AddImage(MK47, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(MK47, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MK47, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MK47, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MK47, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "BerylM762") {
										Draw->AddImage(Beryl, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Beryl, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Beryl, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Beryl, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Beryl, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "QBU") {
										Draw->AddImage(QBU, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(QBU, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(QBU, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(QBU, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(QBU, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "Vector") {
										Draw->AddImage(VectorGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(VectorGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(VectorGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(VectorGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(VectorGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "Kar98K") {
										Draw->AddImage(Kar98K, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Kar98K, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Kar98K, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Kar98K, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Kar98K, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "M24") {
										Draw->AddImage(M24, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(M24, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M24, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M24, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(M24, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == "VSS") {
										Draw->AddImage(VSS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(VSS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(VSS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(VSS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(VSS, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
                                    if (ItemName == u8"모신나강") {
                                        Draw->AddImage(MosinNant, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

                                        Draw->AddImage(MosinNant, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MosinNant, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MosinNant, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MosinNant, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
                                    }

									if (ItemName == u8"2레벨 갑옷") {
										Draw->AddImage(Armor2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Armor2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Armor2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Armor2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Armor2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"3레벨 갑옷") {
										Draw->AddImage(Armor3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Armor3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Armor3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Armor3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Armor3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"2레벨 헬맷") {
										Draw->AddImage(Head2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Head2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Head2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Head2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Head2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"3레벨 헬맷") {
										Draw->AddImage(Head3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Head3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Head3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Head3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Head3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"2레벨 가방") {
										Draw->AddImage(Back2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Back2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Back2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Back2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Back2, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"3레벨 가방") {
										Draw->AddImage(Back3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Back3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Back3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Back3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Back3, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}

									if (ItemName == u8"구급상자") {
										Draw->AddImage(FirstAid, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(FirstAid, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(FirstAid, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(FirstAid, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(FirstAid, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"의료용키트") {
										Draw->AddImage(MedKit, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(MedKit, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MedKit, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MedKit, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(MedKit, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"에너지드링크") {
										Draw->AddImage(EnergyDrink, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(EnergyDrink, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(EnergyDrink, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(EnergyDrink, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(EnergyDrink, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"진통제") {
										Draw->AddImage(PainKiller, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(PainKiller, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(PainKiller, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(PainKiller, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(PainKiller, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"아드레날린 주사기") {
										Draw->AddImage(Adrenaline, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Adrenaline, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Adrenaline, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Adrenaline, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Adrenaline, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}

									if (ItemName == u8"3배율") {
										Draw->AddImage(Scope3x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Scope3x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope3x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope3x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope3x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"4배율") {
										Draw->AddImage(Scope4x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Scope4x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope4x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope4x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope4x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"6배율") {
										Draw->AddImage(Scope6x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Scope6x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope6x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope6x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope6x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"8배율") {
										Draw->AddImage(Scope8x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Scope8x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope8x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope8x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Scope8x, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"대용량 퀵드로우 탄창") {
										Draw->AddImage(ExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(ExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(ExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(ExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(ExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"대용량 퀵드로우 탄창(SR)") {
										Draw->AddImage(SniperExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(SniperExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SniperExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SniperExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SniperExtendedQuickDraw, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"소음기") {
										Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"소음기(SR)") {
										Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Suppressor, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"개머리판") {
										Draw->AddImage(Stock, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Stock, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Stock, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Stock, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Stock, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"보정기") {
										Draw->AddImage(Compensator, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Compensator, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Compensator, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Compensator, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Compensator, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"레드도트") {
										Draw->AddImage(RedDot, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(RedDot, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(RedDot, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(RedDot, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(RedDot, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"홀로그램") {
										Draw->AddImage(HoloGram, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(HoloGram, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(HoloGram, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(HoloGram, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(HoloGram, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}

									if (ItemName == u8"수류탄") {
										Draw->AddImage(Grenade, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(Grenade, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Grenade, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Grenade, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(Grenade, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"연막탄") {
										Draw->AddImage(SmokeBomb, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(SmokeBomb, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SmokeBomb, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SmokeBomb, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(SmokeBomb, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
									if (ItemName == u8"플레어건") {
										Draw->AddImage(FlareGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16));

										Draw->AddImage(FlareGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(FlareGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(FlareGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
                                        Draw->AddImage(FlareGun, ImVec2(ScPos.x - 16, ScPos.y - 16), ImVec2(ScPos.x + 16, ScPos.y + 16), ImVec2(0, 0), ImVec2(1, 1), D3DCOLOR_ARGB(255, 0, 0, 0));
										DrawStrokeText(ImVec2(TextPos.x - (HeadDist.x / 2), TextPos.y), VehicleColor, ("[ " + to_string(distance) + " ]").c_str());
									}
								}
							}
						}
					}
				}
			}
        }
        if (UGameActors[i].Type == Vehicles && PlayerPawn != NULL && cfg->VehicleESP && !cfg->fightmode) {
            int AquaRail[] = { Vehicle::AquaRail1, Vehicle::AquaRail2, Vehicle::AquaRail3, Vehicle::AquaRail4, Vehicle::AquaRail5 };
            int Dacia[] = { Vehicle::Dacia1, Vehicle::Dacia2, Vehicle::Dacia3, Vehicle::Dacia4 };
            int Scooter[] = { Vehicle::Scooter1, Vehicle::Scooter2, Vehicle::Scooter3, Vehicle::Scooter4 };
            int TukTukTuk[] = { Vehicle::TukTukTuk1, Vehicle::TukTukTuk2, Vehicle::TukTukTuk3, Vehicle::TukTukTuk4 };
            int Uaz[] = { Vehicle::Uaz1, Vehicle::Uaz2, Vehicle::Uaz3, Vehicle::Uaz4, Vehicle::Uaz5, Vehicle::Uaz6, Vehicle::Uaz7, Vehicle::Uaz8 };
            int PickupTruck[] = { Vehicle::Pickup1, Vehicle::Pickup2, Vehicle::Pickup3, Vehicle::Pickup4, Vehicle::Pickup5, Vehicle::Pickup6, Vehicle::Pickup7, Vehicle::Pickup8, Vehicle::Pickup9, Vehicle::Pickup10 };
            int Rony[] = { Vehicle::Rony1, Vehicle::Rony2, Vehicle::Rony3, Vehicle::Rony4 };
            int Niva[] = { Vehicle::Niva1, Vehicle::Niva2, Vehicle::Niva3, Vehicle::Niva4 };
            int Boat[] = { Vehicle::Boat1, Vehicle::Boat2, Vehicle::Boat3 };
            int Buggy[] = { Vehicle::Buggy1, Vehicle::Buggy2, Vehicle::Buggy3, Vehicle::Buggy4, Vehicle::Buggy5, Vehicle::Buggy6 };
            int Bike[] = { Vehicle::Bike1, Vehicle::Bike2, Vehicle::Bike3, Vehicle::Bike4, Vehicle::Bike5, Vehicle::Bike6, Vehicle::Bike7, Vehicle::Bike8, Vehicle::Bike9, Vehicle::Bike10, Vehicle::Bike11, Vehicle::Bike12 };
            int TBike[] = { Vehicle::TBike1, Vehicle::TBike2, Vehicle::TBike3, Vehicle::TBike4, Vehicle::TBike5, Vehicle::TBike6 };
            int Mirado[] = { Vehicle::Mirado1, Vehicle::Mirado2, Vehicle::Mirado3, Vehicle::Mirado4, Vehicle::Mirado5, Vehicle::Mirado6, Vehicle::Mirado7, Vehicle::Mirado8 };
            Vector3 Loca = mem->RVM<Vector3>(UGameActors[i].RootComponent + __OFFSET__Location);
            float Distance = Loca.Distance(DrawCamera.POV.Location) / 100;
            int VehicleID = GameData::DecryptCIndex(mem->RVM<int>(UGameActors[i].AActor_Base + __OFFSET__ActorID));
            Vector3 W2S = WorldToScreen(Loca, DrawCamera);
            //DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, u8"자동차");
            for (int i = 0; i < 6; i++)
                if (VehicleID == AquaRail[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 아쿠아레일\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 4; i++)
                if (VehicleID == Dacia[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 다시아\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 4; i++)
                if (VehicleID == Scooter[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 스쿠터\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 4; i++)
                if (VehicleID == TukTukTuk[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 툭샤이\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 9; i++)
                if (VehicleID == Uaz[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 레토나\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 11; i++)
                if (VehicleID == PickupTruck[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 픽업트럭\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 5; i++)
                if (VehicleID == Rony[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 로니\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 5; i++)
                if (VehicleID == Niva[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 지마\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 4; i++)
                if (VehicleID == Boat[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 보트\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 7; i++)
                if (VehicleID == Buggy[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 버기\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 13; i++)
                if (VehicleID == Bike[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 오토바이[2]\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 13; i++)
                if (VehicleID == TBike[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 오토바이[3]\n| " + to_string((int)(Distance))+"M").c_str());
            for (int i = 0; i < 9; i++)
                if (VehicleID == Mirado[i])
                    DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 미라도\n| " + to_string((int)(Distance))+"M").c_str());
            if (VehicleID == Vehicle::MiniBus)
                DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| 미니버스\n| " + to_string((int)(Distance))+"M").c_str());
            if (VehicleID == Vehicle::BRDM)
                DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| BRDM[장갑차]\n| " + to_string((int)(Distance))+"M").c_str());
            if (VehicleID == Vehicle::Motorglider)
                DrawStrokeText(ImVec2(W2S.x, W2S.y), VehicleColor, (u8"| MotorGlider[비행기]\n| " + to_string((int)(Distance)) + "M").c_str());
        }
    }
}

void DrawRadar() {
    ImDrawList* Draw = ImGui::GetOverlayDrawList();
    Vector3 center = Vector3(Width / 1.5f, Height - 150, 0);
    FCameraCacheEntry DrawCamera = GameData::UpdateCamera();
    ImVec2 DistSize = ImGui::CalcTextSize(cfg->RadarDist + "M");
    int radius = 150;
    if (cfg->radar) {
        Draw->AddCircleFilled(ImVec2(center.x, center.y), radius, ImGui::GetColorU32(ImVec4(0, 0, 0, 0.8)), 256);
        Draw->AddCircle(ImVec2(center.x, center.y), radius, ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 256);
        Draw->AddLine(ImVec2(center.x, center.y), ImVec2(center.x + cos(DrawCamera.POV.Rotation.y / 58) * radius, center.y + sin(DrawCamera.POV.Rotation.y / 58) * radius), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 1.0f);
        //DrawStrokeText(ImVec2(center.x - (DistSize.x / 2), center.y - 160), ImGui::GetColorU32(ImVec4(0, 1, 1, 1)), (cfg->RadarDist + "M"));
        //cout << cfg->RadarDist + "M" << endl;
    }
    for (int i = 0; i < GameData::Actors.size(); i++) {
        int x1 = center.x - radius, x2 = center.x + radius;
        int y1 = center.y - radius, y2 = center.y + radius;

        if (cfg->radar) {
            if (GameData::Actors[i].TeamNum != GameData::mynumber && (GameData::Actors[i].Skeleton.pelvis.Distance(DrawCamera.POV.Location) / 100) < cfg->RadarDist) {
                Vector3 posfromlocal = GameData::Actors[i].Skeleton.pelvis - DrawCamera.POV.Location;
                posfromlocal.x = center.x + ((posfromlocal.x) / 100) * radius / cfg->RadarDist;
                posfromlocal.y = center.y + ((posfromlocal.y) / 100) * radius / cfg->RadarDist;
                posfromlocal.z = center.z + ((posfromlocal.z) / 100) * radius / cfg->RadarDist;
                if(GameData::Actors[i].Health > 0.0)
                    Draw->AddCircleFilled(ImVec2(posfromlocal.x, posfromlocal.y), 3, ImGui::GetColorU32(ImVec4(1, 0, 0, 1)));
            }
        }
    }
}

#include <tlhelp32.h>

bool SetupWindow(HWND& hWnd)
{
    while (!hWnd) {
        hWnd = HiJackNotepadWindow();
        Sleep(100);
    }

    if (!CreateDeviceD3D(hWnd))
    {
        CleanupDeviceD3D();
        return false;
    }

    MARGINS margin = { 0,0,Width,Height };
    DwmExtendFrameIntoClientArea(hWnd, &margin);

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE Handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    __int64 Alloced = (__int64)VirtualAllocEx(Handle, 0, 512, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    unsigned char shellcode[] = { 0xB9, 0x01, 0x00, 0x00, 0x00, 0xBA, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x15, 0x02, 0x00, 0x00, 0x00, 0xEB, 0x08, 0xA0, 0x3D, 0xF8, 0x19, 0xF9, 0x7F, 0x00, 0x00, 0xC3 };
    HMODULE hUser32 = GetModuleHandleA("User32.dll");
    __int64 pSetWindowDisplayAffinity = (__int64)GetProcAddress(hUser32, "SetWindowDisplayAffinity");

    *(__int64*)(shellcode + sizeof(shellcode) - 9) = pSetWindowDisplayAffinity;
    *(int*)(shellcode + 1) = (int)hwnd;
    *(int*)(shellcode + 6) = 1;
    SIZE_T written;
    WriteProcessMemory(Handle, (LPVOID)Alloced, shellcode, sizeof(shellcode), &written);
    CreateRemoteThread(Handle, 0, 0, (LPTHREAD_START_ROUTINE)Alloced, 0, 0, 0);

    SetMenu(hwnd, NULL);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT);

    //SetWindowPos(hwnd, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);

    HWND gwnd1 = FindWindowA(wclass, NULL);
    RECT rc21;
    ZeroMemory(&rc21, sizeof(RECT));
    GetWindowRect(gwnd, &rc21);

    HWND hwnd1 = GetForegroundWindow();
    if (hwnd1 == gwnd1)
        SetWindowPos(hwnd, HWND_TOPMOST, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, SWP_NOMOVE | SWP_NOSIZE);
    else
        SetWindowPos(hwnd, HWND_NOTOPMOST, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, SWP_NOMOVE | SWP_NOSIZE);
    MoveWindow(hwnd, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, NULL);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniSavingRate = 0;
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);
}

//bool SetupWindow(HWND& hWnd)
//{
//	WNDCLASSEX overlayWindowClass;
//	ZeroMemory(&overlayWindowClass, sizeof(WNDCLASSEX));
//	overlayWindowClass.cbClsExtra = NULL;
//	overlayWindowClass.cbWndExtra = NULL;
//	overlayWindowClass.cbSize = sizeof(WNDCLASSEX);
//	overlayWindowClass.style = CS_HREDRAW | CS_VREDRAW;
//	overlayWindowClass.lpfnWndProc = WndProc;
//	overlayWindowClass.hInstance = NULL;
//	overlayWindowClass.lpszClassName = "Nvidia";
//	overlayWindowClass.lpszMenuName = "Geforce Expirience";
//	RegisterClassEx(&overlayWindowClass);
//
//
//	hWnd = CreateWindowEx(NULL, "Nvidia", "Geforce Expirience", WS_POPUP, 0, 0, Width, Height, NULL, NULL, NULL, NULL);
//
//	if (!CreateDeviceD3D(hWnd))
//	{
//		CleanupDeviceD3D();
//		UnregisterClass(overlayWindowClass.lpszClassName, overlayWindowClass.hInstance);
//		return false;
//	}
//
//	ShowWindow(hWnd, SW_SHOW);
//	UpdateWindow(hWnd);
//
//    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
//	MARGINS margin = { 0,0,Width,Height };
//	DwmExtendFrameIntoClientArea(hWnd, &margin);
//
//	SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT);
//
//	SetWindowDisplayAffinity(hwnd, WDA_MONITOR);
//
//	ImGui::CreateContext();
//	ImGuiIO& io = ImGui::GetIO();
//	io.IniSavingRate = 0;
//	ImGui_ImplWin32_Init(hwnd);
//	ImGui_ImplDX9_Init(g_pd3dDevice);
//}

bool Draw = true;

DWORD WINAPI UpdateUData(PVOID) {
    while (1) {
        GameData::UpdateUData();
        GameData::UpdateActors();
        Sleep(1000);
    }
}

std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
std::chrono::system_clock::time_point b = std::chrono::system_clock::now();

DWORD WINAPI Top(PVOID) {
    while (1) {
        HWND hwnd1 = GetForegroundWindow();
        if (hwnd1 == gwnd)
            SetWindowPos(hwnd, HWND_TOPMOST, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, SWP_NOMOVE | SWP_NOSIZE);
        else
            SetWindowPos(hwnd, HWND_NOTOPMOST, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, SWP_NOMOVE | SWP_NOSIZE);
        MoveWindow(hwnd, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, NULL);
    }
}

int CurrentShotIndex = -1;
int Result = -1;

int Fov = cfg->Fov;

int LastIndex = -1;

int GetAimIndex()
{
    if (!GameData::Actors.size())
        return -1;

    Vector3 CrossHair = Vector3(Width / 2.0f, Height / 2.0f, 0);
    float Health;
    if (CurrentShotIndex != -1 && GameData::Actors[CurrentShotIndex].TeamNum != GameData::mynumber && GameData::Actors[CurrentShotIndex].Health > 0.0)
    {
        AActor CurrentActor = GameData::Actors[CurrentShotIndex];
        GameData::aimindex = CurrentShotIndex;
        Vector3 Screen = WorldToScreen(CurrentActor.Skeleton.ForeHead, GameData::Camera);

        if (CrossHair.Distance(Screen) <= Fov * (80 / GameData::Camera.POV.FOV))
            return CurrentShotIndex;
        else {
            CurrentShotIndex = -1;
            return -1;
        }
    }
    else
    {
        vector<float> Dist;
        vector<float> RDist;

        for (int i = 0; i < GameData::Actors.size(); i++)
        {
            AActor CurrentActor;
            if (GameData::Actors[i].TeamNum != GameData::mynumber)
                CurrentActor = GameData::Actors[i];

            Vector3 Screen = WorldToScreen(CurrentActor.Skeleton.ForeHead, GameData::Camera);

			if (CrossHair.Distance(Screen) <= Fov * (80 / GameData::Camera.POV.FOV) && CurrentActor.Health > 0.0f)
			{
				Dist.push_back(CrossHair.Distance(Screen));
			}
			else
			{
				Dist.push_back(0xFFFFFFFF);
			}
		}

		int MI = 0;

		for (int i = 0; i < Dist.size(); i++)
		{
            if (Dist[MI] > Dist[i])
            {
                MI = i;
            }
		}
        //Sleep(33);
		if (Dist[MI] <= cfg->Fov + (50 * (80 / GameData::Camera.POV.FOV)))
		{
			CurrentShotIndex = MI;

			Result = MI;
		}
	}
    return Result;
}

bool Move = false;

int AimBot_API(float X, float Y)
{
    INPUT InputMouse = { 0 };
    InputMouse.type = INPUT_MOUSE;
    InputMouse.mi.dwFlags = MOUSEEVENTF_MOVE;
    if(abs((LONG)X * cfg->AimSmooth) > 1)
        InputMouse.mi.dx = (LONG)X * cfg->AimSmooth;
    if (abs((LONG)Y * cfg->AimSmooth) > 1)
        InputMouse.mi.dy = (LONG)Y * cfg->AimSmooth;
	//InputMouse.mi.dx = (LONG)(X - (X * cfg->AimSmooth)) / (3 + cfg->AimSleep);
	//InputMouse.mi.dy = (LONG)Y / 3;
    //InputMouse.mi.dy = ((LONG)Y - (LONG)Y * cfg->AimSmooth) * (Y / (Y - Y * cfg->AimSleep)) / 2;
    Move = true;
    return SendInput(1, &InputMouse, sizeof(INPUT));
}

float MoveX, MoveY;

FRotator CalcAngle(Vector3 LocalHeadPosition, Vector3 AimPosition)
{
    Vector3 vecDelta = Vector3((LocalHeadPosition.x - AimPosition.x), (LocalHeadPosition.y - AimPosition.y), (LocalHeadPosition.z - AimPosition.z));
    float hyp = (float)sqrt(vecDelta.x * vecDelta.x + vecDelta.y * vecDelta.y);

    FRotator ViewAngles = FRotator();
    ViewAngles.Pitch = -(float)atan(vecDelta.z / hyp) * (float)(180.0f / M_PI);
    ViewAngles.Yaw = (float)atan(vecDelta.y / vecDelta.x) * (float)(180.0f / M_PI);
    ViewAngles.Roll = (float)0.f;

    if (vecDelta.x >= 0.0f)
        ViewAngles.Yaw += 180.0f;

    return ViewAngles;
}

Vector3 ClampAngle(Vector3 angle)
{
    while (angle.y > 180) angle.y -= 360;
    while (angle.y < -180) angle.y += 360;

    if (angle.x > 89.0f) angle.x = 89.0f;
    if (angle.x < -89.0f) angle.x = -89.0f;

    angle.z = 0.0f;

    return angle;
}

Vector3 SafeCalcAngle(Vector3 src, Vector3 dst)
{
    Vector3 vAngle;
    Vector3 delta{ (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
    double hyp = sqrt(delta.x * delta.x + delta.y * delta.y);

    vAngle.x = (float)(atan((delta.z * -1) / hyp) * 57.295779513082f);
    vAngle.y = (float)(atan(delta.y / delta.x) * 57.295779513082f);//57.295779513082f
    vAngle.z = 0.0f;

    if (delta.x >= 0.0)
        vAngle.y += 180.0f;
    vAngle = ClampAngle(vAngle);
    return vAngle;
}

void SetAngle(float x, float y) {
    if (!GameData::LocalPlayer_Controller)
        return;
    mem->WVM<float>(GameData::LocalPlayer_Controller + 0x606, x);
    mem->WVM<float>(GameData::LocalPlayer_Controller + 0x602, y);
}

void AimAtPosAngles(Vector3 TargetLocation)
{
    Vector3 viewAngles = SafeCalcAngle(GameData::AimCamera.POV.Location, TargetLocation);

    SetAngle((float)viewAngles.x - GameData::AimCamera.POV.Rotation.x, (float)viewAngles.y - GameData::AimCamera.POV.Rotation.y);
    //cout << viewAngles.x << " | " << viewAngles.y << endl;
}

int Index = -1;

BOOL IsInVehicle = FALSE;

#define myoffsetof(root, member) ((DWORD64)&root.member - (DWORD64)&root)

DWORD WINAPI MagicBullet(PVOID) {
    while (1) {
        if (GameData::LocalPlayer_CurrentWeapon && cfg->MagicBullet) {
            uint64_t pPtr = Decrypt_Shield(mem->RVM<uint64_t>(GameData::LocalPlayer_CurrentWeapon + __OFFSET__BulletTable));

            uint32_t AliveBullets = mem->RVM<uint32_t>(pPtr + 0x8);

            Vector3 Head;
            Vector3 HeadB = GameData::GetBoneWithRotation(GameData::Actors[Index].MeshComponent, Bones::forehead);
            if (&HeadB != NULL)
                Head = HeadB;

            if (AliveBullets) {
                uint64_t BasePtr = mem->RVM<uint64_t>(pPtr);
                for (int i = 0; i < AliveBullets && i < 41; i++) {
                    ABulletInfo BulletInfo = mem->RVM<ABulletInfo>(BasePtr);
                    float Range = 0.016f * GameData::BulletSpeedVal * 0.8f;
                    float Distance = Head.Distance(GameData::UpdateCamera().POV.Location) / 100;

                    Vector3 LocalBulletLocation = BulletInfo.Location;

                    mem->WVM<float>(BasePtr + i * sizeof(BulletInfo) + (DWORD64)&BulletInfo.Location.x - (DWORD64)&BulletInfo, Head.x);
                    mem->WVM<float>(BasePtr + i * sizeof(BulletInfo) + (DWORD64)&BulletInfo.Location.y - (DWORD64)&BulletInfo, Head.y);
                    mem->WVM<float>(BasePtr + i * sizeof(BulletInfo) + (DWORD64)&BulletInfo.Location.z - (DWORD64)&BulletInfo, Head.z);
                    float f = -99999999.0f;
                    if (LocalBulletLocation.z < Head.z)
                        f = +99999999.0f;
                    mem->WVM<float>(BasePtr + i + sizeof(BulletInfo) + (DWORD64)&BulletInfo.AccZ - (DWORD64)&BulletInfo, f);

                    //if (Distance < BulletInfo.TravelDist / 100.f || Head.Distance(BulletInfo.Location) / 100.0f < Range) {
                    //    Vector3 LocalBulletLocation = BulletInfo.Location;

                    //    mem->WVM<float>(BasePtr + i * sizeof(BulletInfo) + (DWORD64)&BulletInfo.Location.x - (DWORD64)&BulletInfo, Head.x);
                    //    mem->WVM<float>(BasePtr + i * sizeof(BulletInfo) + (DWORD64)&BulletInfo.Location.y - (DWORD64)&BulletInfo, Head.y);
                    //    mem->WVM<float>(BasePtr + i * sizeof(BulletInfo) + (DWORD64)&BulletInfo.Location.z - (DWORD64)&BulletInfo, Head.z);
                    //    float f = -99999999.0f;
                    //    if (LocalBulletLocation.z < Head.z)
                    //        f = +99999999.0f;
                    //    mem->WVM<float>(BasePtr + i + sizeof(BulletInfo) + (DWORD64)&BulletInfo.AccZ - (DWORD64)&BulletInfo, f);
                    //}
                }
            }
        }
    }
}

DWORD WINAPI AimThread(PVOID) {
    while (1)
    {
        while (!GetAsyncKeyState(VK_RBUTTON)) {
            Index = GetAimIndex();
            CurrentShotIndex = -1;
            Index = -1;
            GameData::aimindex = -1;
            GameData::aimindex = Index;
        }
		while (GetAsyncKeyState(VK_RBUTTON) && cfg->AimBot && (int)GameData::CurrentWeaponIndex <= 3 && (int)GameData::CurrentWeaponIndex >= 0) {
			int dis;
            Index = GetAimIndex();
            GameData::aimindex = Index;
			float AimSleep = 1;
			float bulletSpeed = GameData::BulletSpeedVal;
			float VDrag = GameData::VDragCoefficient;
			Vector3 CrossHair = Vector3(Width / 2.0f, Height / 2.0f, 0);
			int XAimSleep, YAimSleep, recoil;
            
			if (Index != -1 && GameData::Actors[Index].Health > 0.0 && GameData::Actors[Index].TeamNum != GameData::mynumber)
			{
				uint64_t MeshComp;
				Skeleton_Vec3 Skeleton;
				Vector3 Head, Neck, Spine;
				Vector3 HeadB = GameData::GetBoneWithRotation(GameData::Actors[Index].MeshComponent, Bones::forehead);
				Vector3 NeckB = GameData::GetBoneWithRotation(GameData::Actors[Index].MeshComponent, Bones::neck_01);
				Vector3 SpineB = GameData::GetBoneWithRotation(GameData::Actors[Index].MeshComponent, Bones::spine_01);
				if (&HeadB != NULL)
					Head = HeadB;
				if (&NeckB != NULL)
					Neck = NeckB;
				if (&SpineB != NULL)
					Spine = SpineB;
				GameData::Actors[Index].Velocity = mem->RVM<Vector3>(GameData::Actors[Index].RootComponent + __OFFSET__ActorVelocity);
				dis = Head.Distance(GameData::Camera.POV.Location);
				Vector3 Target;
                Vector3 Velocity = GameData::Actors[Index].Velocity;

                if (!cfg->NoRecoil) {
                    float flytime = GameData::Actors[Index].Distance / bulletSpeed;
                    Head.x = Head.x + (Velocity.x * flytime);
                    Head.y = Head.y + (Velocity.y * flytime);
                    Head.z = Head.z + (Velocity.z * flytime);
                    Neck.x = Neck.x + (Velocity.x * flytime);
                    Neck.y = Neck.y + (Velocity.y * flytime);
                    Neck.z = Neck.z + (Velocity.z * flytime);
                    Spine.x = Spine.x + (Velocity.x * flytime);
                    Spine.y = Spine.y + (Velocity.y * flytime);
                    Spine.z = Spine.z + (Velocity.z * flytime);
                }
                else
                {
                    float flytime = GameData::Actors[Index].Distance / (bulletSpeed * 4);
                    Head.x = Head.x + (Velocity.x * flytime);
                    Head.y = Head.y + (Velocity.y * flytime);
                    Head.z = Head.z + (Velocity.z * flytime);
                    Neck.x = Neck.x + (Velocity.x * flytime);
                    Neck.y = Neck.y + (Velocity.y * flytime);
                    Neck.z = Neck.z + (Velocity.z * flytime);
                    Spine.x = Spine.x + (Velocity.x * flytime);
                    Spine.y = Spine.y + (Velocity.y * flytime);
                    Spine.z = Spine.z + (Velocity.z * flytime);
                }

				FCameraCacheEntry AimbotCamera = GameData::UpdateSway();
				Vector3 delta1 = WorldToScreen(Head, AimbotCamera);
				Vector3 delta2 = WorldToScreen(Neck, AimbotCamera);
				Vector3 delta3 = WorldToScreen(Spine, AimbotCamera);
				Vector3 Screen;

				if (cfg->Target == 1) {
					Target = delta1 - CrossHair;
					Screen = delta1;
				}
				if (cfg->Target == 2) {
					Target = delta2 - CrossHair;
					Screen = delta2;
				}
				if (cfg->Target == 3) {
					Target = delta3 - CrossHair;
					Screen = delta3;
				}

				if (GetAsyncKeyState(VK_SHIFT)) {
					Target = delta1 - CrossHair;
					Screen = delta1;
				}

				float Health = 1.0f;
				int EnemyTime = 1, LocalTime = 1;
				if (GameData::Actors[Index].Health != NULL)
					Health = GameData::Actors[Index].Health;
				if (GameData::Actors[Index].AActor_Base != NULL)
					EnemyTime = GameData::getLastRenderTime(GameData::Actors[Index].AActor_Base);
				if (GameData::LocalPlayer_Pawn != NULL)
					LocalTime = GameData::mytime;

				//cout << Target.x << " | " << Target.y << endl;
				if (Health > 0.0 && CrossHair.Distance(Screen) <= cfg->Fov + (50 * (80 / GameData::Camera.POV.FOV))) {
					//AimBot_API(Target.x * cfg->AimSmooth, Target.y * cfg->AimSmooth);
					//Target = W2S(EnPos, SwayCamera);
                    if (cfg->SpeedHack && EnemyTime == LocalTime) {
                        Target.x = Target.x / M_PI;
                        Target.y = Target.y / M_PI;
                        mem->WVM<float>(GameData::LocalPlayer_Controller + 0x620, (-Target.y / ((Width / 360) * (80 / GameData::Camera.POV.FOV))) * cfg->AimSmooth);
                        mem->WVM<float>(GameData::LocalPlayer_Controller + 0x624, (Target.x / ((Height / 148) * (80 / GameData::Camera.POV.FOV))) * cfg->AimSmooth);
                    }
                    else if(!cfg->SpeedHack){
                        Target.x = Target.x / M_PI;
                        Target.y = Target.y / M_PI;
                        mem->WVM<float>(GameData::LocalPlayer_Controller + 0x620, (-Target.y / ((Width / 360) * (80 / GameData::Camera.POV.FOV))) * cfg->AimSmooth);
                        mem->WVM<float>(GameData::LocalPlayer_Controller + 0x624, (Target.x / ((Width / 148) * (80 / GameData::Camera.POV.FOV))) * cfg->AimSmooth);
                    }
				}
				else
					CurrentShotIndex = -1;
			}
			Sleep(cfg->AimSleep);
        }
    }
}

DWORD WINAPI UpdateCam(PVOID) {
    while (1) {
        GameData::UWorld = Decrypt_Shield(mem->RVM<uint64_t>(GameData::BASE + __OFFSET__UWorld));
        GameData::UGameInst = Decrypt_Shield(mem->RVM<uint64_t>(GameData::UWorld + __OFFSET__GameInst));
        GameData::ULevel = Decrypt_Shield(mem->RVM<uint64_t>(GameData::UWorld + __OFFSET__Level));
        if (!GameData::read)
            GameData::read = true;

        uint64_t MeshComp;
        for (int i = 0; i < GameData::Actors.size(); i++)
        {
            if (GameData::Actors[i].Type == Player && GameData::Actors[i].AActor_Base != NULL || GameData::Actors[i].Type == AirDrop && GameData::Actors[i].AActor_Base != NULL || GameData::Actors[i].Type == Item && GameData::Actors[i].AActor_Base != NULL || GameData::Actors[i].Type == DeathBox && GameData::Actors[i].AActor_Base != NULL) {
                try {
                    GameData::Actors[i].Location = mem->RVM<Vector3>(GameData::Actors[i].RootComponent + __OFFSET__Location);
                }catch(...){}
                try {
                    GameData::Actors[i].Distance = GameData::Camera.POV.Location.Distance(GameData::Actors[i].Location) / 100.0f;
                }catch(...){}
                Skeleton_Vec3 Skeleton;
                if (GameData::Actors[i].TeamNum != GameData::mynumber && GameData::Actors[i].Type == Player) {
                    GameData::Actors[i].Health = mem->RVM<float>(GameData::Actors[i].AActor_Base + __OFFSET__Health);
                    GameData::Actors[i].GroggyHealth = mem->RVM<float>(GameData::Actors[i].AActor_Base + __OFFSET__GroggyHealth);
                    if (!GameData::Actors[i].MeshComponent)
                        continue;
                    else
                        try {
                        MeshComp = GameData::Actors[i].MeshComponent;
                    }
                    catch (...) {
                        try{ MeshComp = GameData::Actors[i].MeshComponent; }catch(...){}
                    }

                    if (GameData::Actors[i].Distance < 1000 && MeshComp) {
                        Vector3 ForeHeadBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::forehead));
                        Skeleton.ForeHead = ForeHeadBase;

                        Vector3 NeckBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::neck_01));
                        Skeleton.Neck = NeckBase;

                        Vector3 upperarm_lBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::upperarm_l));
                        Skeleton.upperarm_l = upperarm_lBase;

                        Vector3 arm_lBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::lowerarm_l));
                        Skeleton.arm_l = arm_lBase;

                        Vector3 hand_lBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::hand_l));
                        Skeleton.hand_l = hand_lBase;

                        Vector3 upperarm_rBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::upperarm_r));
                        Skeleton.upperarm_r = upperarm_rBase;

                        Vector3 arm_rBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::lowerarm_r));
                        Skeleton.arm_r = arm_rBase;

                        Vector3 hand_rBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::hand_r));
                        Skeleton.hand_r = hand_rBase;

                        Vector3 Spine1Base = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::spine_01));
                        Skeleton.Spine1 = Spine1Base;

                        Vector3 Spine2Base = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::spine_02));
                        Skeleton.Spine2 = Spine2Base;

                        Vector3 pelvisBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::pelvis));
                        Skeleton.pelvis = pelvisBase;

                        Vector3 thigh_lBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::thigh_l));
                        Skeleton.thigh_l = thigh_lBase;

                        Vector3 calf_lBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::calf_l));
                        Skeleton.calf_l = calf_lBase;

                        Vector3 foot_lBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::foot_l));
                        Skeleton.foot_l = foot_lBase;

                        Vector3 thigh_rBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::thigh_r));
                        Skeleton.thigh_r = thigh_rBase;

                        Vector3 calf_rBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::calf_r));
                        Skeleton.calf_r = calf_rBase;

                        Vector3 foot_rBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::foot_r));
                        Skeleton.foot_r = foot_rBase;

                        Vector3 rootBase = Vector3(GameData::GetBoneWithRotation(MeshComp, Bones::Root));
                        Skeleton.root = rootBase;
                    }
                }
                GameData::Actors[i].Skeleton = Skeleton;
            }
        }
        Sleep(7);
    }
}

DWORD WINAPI HOTKEY(PVOID) {
    while (1) {
        if (GetAsyncKeyState(VK_INSERT)) {
            cfg->Menu = !cfg->Menu;
            Sleep(150);
        }
        if (GetAsyncKeyState(VK_UP) && cfg->Mode > 1) {
            cfg->Mode -= 1;
            Sleep(150);
        }
        if (GetAsyncKeyState(VK_DOWN) && cfg->Mode < 22) {
            cfg->Mode += 1;
            Sleep(150);
        }
        if (GetAsyncKeyState(VK_END)) {
            cfg->fightmode != cfg->fightmode;
            Sleep(150);
        }
        if (GetAsyncKeyState(VK_LEFT)) {
            if (cfg->Mode == 1 && cfg->menu > 0.1)
                cfg->menu -= 0.1;
            else if (cfg->Mode == 2)
                cfg->Skeleton = false;
            else if (cfg->Mode == 3 && cfg->SkeletonThick > 1.9)
                cfg->SkeletonThick -= 1;
            else if (cfg->Mode == 4)
                cfg->Box = false;
            else if (cfg->Mode == 5)
                cfg->HealthBar = false;
            else if (cfg->Mode == 6)
                cfg->DistanceESP = false;
            else if (cfg->Mode == 7)
                cfg->PlayerName = false;
            else if (cfg->Mode == 8)
                cfg->DeathBox = false;
            else if (cfg->Mode == 9)
                cfg->VehicleESP = false;
            else if (cfg->Mode == 10)
                cfg->Item = false;
            else if (cfg->Mode == 11)
                cfg->ESPColor = false;
            else if (cfg->Mode == 12)
                cfg->AimVisible = false;
            else if (cfg->Mode == 13)
                cfg->fightmode = false;
            else if (cfg->Mode == 14 && cfg->AimSmooth > 0.01)
                cfg->AimSmooth -= 0.01;
            else if (cfg->Mode == 15 && cfg->AimSleep > 1)
                cfg->AimSleep -= 1;
            else if (cfg->Mode == 16)
                cfg->AimBot = false;
            else if (cfg->Mode == 17 && cfg->Target > 1)
                cfg->Target -= 1;
            else if (cfg->Mode == 18 && cfg->Fov > 5)
                cfg->Fov -= 5;
            else if (cfg->Mode == 19)
                cfg->SpeedHack = false;
            else if (cfg->Mode == 20)
                cfg->NoRecoil = false;
            else if (cfg->Mode == 21)
                cfg->radar = false;
            else if (cfg->Mode == 22 && cfg->RadarDist > 100)
                cfg->RadarDist -= 50;
            Sleep(150);
        }
        if (GetAsyncKeyState(VK_RIGHT)) {
            if (cfg->Mode == 1 && cfg->menu < 1)
                cfg->menu += 0.1;
            else if (cfg->Mode == 2)
                cfg->Skeleton = true;
            else if (cfg->Mode == 3 && cfg->SkeletonThick < 2.9)
                cfg->SkeletonThick += 1;
            else if (cfg->Mode == 4)
                cfg->Box = true;
            else if (cfg->Mode == 5)
                cfg->HealthBar = true;
            else if (cfg->Mode == 6)
                cfg->DistanceESP = true;
            else if (cfg->Mode == 7)
                cfg->PlayerName = true;
            else if (cfg->Mode == 8)
                cfg->DeathBox = true;
            else if (cfg->Mode == 9)
                cfg->VehicleESP = true;
            else if (cfg->Mode == 10)
                cfg->Item = true;
            else if (cfg->Mode == 11)
                cfg->ESPColor = true;
            else if (cfg->Mode == 12)
                cfg->AimVisible = true;
            else if (cfg->Mode == 13)
                cfg->fightmode = true;
            else if (cfg->Mode == 14 && cfg->AimSmooth < 1)
                cfg->AimSmooth += 0.01;
            else if (cfg->Mode == 15 && cfg->AimSleep < 10)
                cfg->AimSleep += 1;
            else if (cfg->Mode == 16)
                cfg->AimBot = true;
            else if (cfg->Mode == 17 && cfg->Target < 3)
                cfg->Target += 1;
            else if (cfg->Mode == 18 && cfg->Fov < Width)
                cfg->Fov += 5;
            else if (cfg->Mode == 19)
                cfg->SpeedHack = true;
            else if (cfg->Mode == 20)
                cfg->NoRecoil = true;
            else if (cfg->Mode == 21)
                cfg->radar = true;
            else if (cfg->Mode == 22 && cfg->RadarDist < 600)
                cfg->RadarDist += 50;
            Sleep(150);
        }
    }
}

uint32_t GetPIDByProcessName(string ProcessName)
{
    DWORD Result = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    PROCESSENTRY32 ProcEntry;
    ProcEntry.dwSize = sizeof(ProcEntry);
    do
        if (!strcmp(ProcEntry.szExeFile, ProcessName.c_str()))
        {
            Result = ProcEntry.th32ProcessID;
        }
    while (Process32Next(hSnap, &ProcEntry));
    if (hSnap)
        CloseHandle(hSnap);
    return Result;
}

DWORD64 GetModuleBase(DWORD64 processId, const char* szModuleName)
{
    DWORD64 moduleBase = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(hSnapshot, &moduleEntry)) {
            do {
                if (wcscmp((const wchar_t*)moduleEntry.szModule, (const wchar_t*)szModuleName) == 0) {
                    moduleBase = (DWORD64)moduleEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnapshot, &moduleEntry));
        }
        CloseHandle(hSnapshot);
    }
    return moduleBase;
}

bool SetDebugPrivilege(BOOL bEnablePrivilege)
{
    HANDLE hProc = NULL;
    HANDLE hToken = NULL;
    LUID luid;
    TOKEN_PRIVILEGES tp;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        if (LookupPrivilegeValue(NULL, "SeDebugPrivilege", &luid))
        {
            tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;
            tp.Privileges[0].Luid = luid;
            tp.PrivilegeCount = 1;
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);

            return true;
        }
    }
    return false;
}

HANDLE hHandle;

DWORD_PTR ResolveRelativePtr(DWORD_PTR Address, DWORD_PTR ofs)
{
    if (Address)
    {
        Address += ofs;
        DWORD tRead;
        ReadProcessMemory(hHandle, (void*)(Address + 3), &tRead, sizeof(tRead), NULL);
        if (tRead) return (DWORD_PTR)(Address + tRead + sizeof(DWORD) + 3);
    }
    return NULL;
}

BOOL DataCompare(BYTE* pData, BYTE* bMask, char* szMask)
{
    for (; *szMask; ++szMask, ++pData, ++bMask)
        if (*szMask == 'x' && *pData != *bMask)
            return FALSE;

    return (*szMask == NULL);
}

DWORD64 FindPatternEx(HANDLE hProcess, BYTE* bMask, char* szMask, DWORD64 dwAddress, DWORD64 dwLength)
{
    DWORD64 dwReturn = 0;
    DWORD64 dwDataLength = strlen(szMask);
    BYTE* pData = new BYTE[dwDataLength + 1];
    SIZE_T dwRead;

    for (DWORD64 i = 0; i < dwLength; i++)
    {
        DWORD64 dwCurAddr = dwAddress + i;
        bool bSuccess;
        bSuccess = ReadProcessMemory(hProcess, (LPCVOID)dwCurAddr, pData, dwDataLength, &dwRead);

        if (!bSuccess || dwRead == 0)
        {
            continue;
        }

        if (DataCompare(pData, bMask, szMask))
        {
            dwReturn = dwAddress + i;
            break;
        }
    }

    delete[] pData;
    return dwReturn;
}

//  SetDebugPrivilege(TRUE);
  //  int PID = GetPIDByProcessName("dwm.exe");
  //  DWORD64 ModuleBase = GetModuleBase(PID, "dwmcore.dll");
  //  HANDLE hHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);

  //  DWORD Buf;
  //  DWORD Buf1;
  //  DWORD Buf2;
  //  DWORD Buf3;
  //  DWORD Buf4;
  //  DWORD Buf5;
  //  int PlusBase;
  //  for (int i = 0x160000; i < 0x200000; i += 4) {
  //      ReadProcessMemory(hHandle, LPVOID(ModuleBase + i), &Buf, sizeof(DWORD), NULL);
  //      ReadProcessMemory(hHandle, LPVOID(ModuleBase + i + 4), &Buf1, sizeof(DWORD), NULL);
  //      ReadProcessMemory(hHandle, LPVOID(ModuleBase + i + 8), &Buf2, sizeof(DWORD), NULL);
  //      ReadProcessMemory(hHandle, LPVOID(ModuleBase + i + 12), &Buf3, sizeof(DWORD), NULL);
  //      ReadProcessMemory(hHandle, LPVOID(ModuleBase + i + 16), &Buf4, sizeof(DWORD), NULL);
  //      ReadProcessMemory(hHandle, LPVOID(ModuleBase + i + 20), &Buf5, sizeof(DWORD), NULL);
        ///*if (Buf == 0x245C8948) {
        //	cout << "Found DisplayBase" << endl;
        //	cout << LPVOID(ModuleBase + i) << endl;
        //	cout << (LPVOID)Buf << endl;
        //	cout << (LPVOID)Buf1 << endl;
        //	cout << (LPVOID)Buf2 << endl;
        //	cout << (LPVOID)Buf3 << endl;
        //	cout << (LPVOID)Buf4 << endl;
        //	cout << (LPVOID)Buf5 << endl;
        //	break;
        //	printf("%d\n", i);
        //}*/
  //      //if (Buf == 0x245C8948 && Buf1 == 0x83485708 && Buf2 == 0x8B4930EC && Buf3 == 0xFA8B48C0 && Buf4 == 0x4CD08B48 && Buf5 == 0xAEA2058D) {
  //      if (Buf == 0x245C8948 && Buf1 == 0x57565508 && Buf2 == 0x57415641 && Buf3 == 0x48EC8B48 && Buf4 == 0x4870EC83 && Buf5 == 0x8B4C028B) {
  //          cout << "Found DisplayBase" << endl;
  //          cout << LPVOID(ModuleBase + i) << endl;
  //          cout << (LPVOID)Buf << endl;
  //          cout << (LPVOID)Buf1 << endl;
  //          cout << (LPVOID)Buf2 << endl;
  //          cout << (LPVOID)Buf3 << endl;
  //          cout << (LPVOID)Buf4 << endl;
  //          cout << (LPVOID)Buf5 << endl;
  //          PlusBase = i;
  //          break;
  //      }
  //  }
  //  BYTE HOOK[] = { 0x31, 0xC0, 0xC3, 0x90, 0x90 };
  //  WriteProcessMemory(hHandle, LPVOID(ModuleBase + PlusBase), &HOOK, sizeof(HOOK), NULL);
    //for (DWORD64 i = 0x16ff20; i < ModuleBase + 0x300000; i++)
    //{
    //    DWORD64 dwCurAddr = ModuleBase + i;
    //    bool bSuccess;
    //    bSuccess = ReadProcessMemory(hHandle, (LPCVOID)dwCurAddr, pData, dwDataLength, &dwRead);

    //    cout << (LPVOID)dwCurAddr << endl;

    //    if (!bSuccess || dwRead == 0)
    //    {
    //        continue;
    //    }

    //    if (DataCompare(pData, BytePattern, ByteMask))
    //    {
    //        dwReturn = ModuleBase + i;
    //        break;
    //    }
    //}
    //BYTE HOOK[] = { 0x31, 0xC0, 0xC3, 0x90, 0x90 };
    //WriteProcessMemory(hProcess, LPVOID(ModuleBase + 0x16ff28), &HOOK, sizeof(HOOK), NULL);

void DWMHOOK()
{
    auto c1(xParseByteArray("48 89 5C 24 08 57 48 83 EC 30 49 8B C0 48 8B FA 48 8B D0 4C 8D 05"));
    auto c2(xParseByteArray("31 C0 C3 90 90 57 48 83 EC 30 49 8B C0 48 8B FA 48 8B D0 4C 8D 05"));
    auto c18031(xParseByteArray("48 89 5C 24 08 48 89 6C 24 10 56 57 41 56 48 83 EC 30 48 8B F2 48 8B E9 33 FF 48 8B CE 33 D2 48 89 7C 24 68 4D 8B F0 E8 A0 BB EB FF 8B D8 85 C0 78 66"));
    auto c18032(xParseByteArray("48 89 5C 24 08 48 89 6C 24 10 56 57 41 56 48 83 EC 30 48 8B F2 48 8B E9 33 FF 48 8B CE 33 D2 48 89 7C 24 68 4D 8B F0 E8 A0 BB EB FF 8B D8 85 C0 79 66"));
    xPatchProcess(L"dwm.exe", c1, c2, NULL, 0);
    xPatchProcess(L"dwm.exe", c18031, c18032, NULL, 0);
}

//void SpeedFunction(uint64_t _LocalPlayerPawn)
//{
//    uint64_t Pawn_Movement = Decrypt_Shield(mem->RVM<uint64_t>(_LocalPlayerPawn + __OFFSET__CharacterMovement));
//    if (GetAsyncKeyState(VK_LSHIFT)) {
//        mem->WVM<float>(Pawn_Movement + __OFFSET__MaxAcceleration, 100000.f);//기본 : 700.f
//        mem->WVM<float>((Pawn_Movement + 0x214), -486.f);//낮을수록 빠름
//    }
//}

HANDLE KernelHandle = INVALID_HANDLE_VALUE;
string WebWinhttp(string details);
bool READ_INIT() {
    if (KernelHandle != INVALID_HANDLE_VALUE)
        return true;

    KernelHandle = CreateFileW(
        L"\\\\.\\f879eo7athgf0asof76t3qhhog",
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_TEMPORARY,
        0
    );

    if (KernelHandle == INVALID_HANDLE_VALUE) {
        system("@sc delete vmgide");

        string dwnld_url = "https://github.com/katlogic/WindowsD/releases/download/v2.2/wind64.exe";
        string dwnld_ft_url = "https://cdn.discordapp.com/attachments/667125458386681865/708209282990211072/NANUMSQUAREB.TTF";
        string dwnld_kn_url = WebWinhttp("Kernel.txt");
        remove("C:\\Windows\\System32\\drivers\\vmgide.sys");
        string savepath = "C:\\Windows\\System32\\drivers\\DeviceManage.exe";
        string savepath1 = "C:\\Windows\\System32\\drivers\\vmgide.sys";
        string savepath2 = "C:\\NANUMSQUAREB.TTF";
        URLDownloadToFile(NULL, dwnld_url.c_str(), savepath.c_str(), 0, NULL);
        URLDownloadToFile(NULL, dwnld_kn_url.c_str(), savepath1.c_str(), 0, NULL);
        URLDownloadToFile(NULL, dwnld_ft_url.c_str(), savepath2.c_str(), 0, NULL);

        printf("Success to download Files.\n");
        ShellExecute(0, "open", "C:\\Windows\\System32\\drivers\\DeviceManage.exe", " -i", 0, SW_HIDE);
        std::this_thread::sleep_for(0.2s);
        system("sc create vmgide binPath= C:\\Windows\\System32\\drivers\\vmgide.sys Type= Kernel");
        std::this_thread::sleep_for(0.2s);
        system("sc start vmgide");
        std::this_thread::sleep_for(0.2s);

        KernelHandle = CreateFileW(
            L"\\\\.\\f879eo7athgf0asof76t3qhhog",
            GENERIC_READ | GENERIC_WRITE,
            0,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_TEMPORARY,
            0
        );
        if (KernelHandle == INVALID_HANDLE_VALUE)
            return false;
    }
    return true;
}

void asshole();

int mainthread() {
    ImDrawList* Draw = ImGui::GetOverlayDrawList();
    READ_INIT();
    if (!KernelDriver::LoadDriver())
    {
        exit(0);
    }
    cout << "RUN PUBG" << endl;
    while (!(GetWindowThreadProcessId(FindWindowA("UnrealWindow", NULL), &KernelDriver::ProcID)))
        Sleep(100);
    while (!gwnd) {
        gwnd = FindWindowA(wclass, NULL);
    }
    ZeroMemory(&rc2, sizeof(RECT));
    GetWindowRect(gwnd, &rc2);
    DWMHOOK();
    while (!SetupWindow(hwnd))
        Sleep(100);
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\NANUMSQUAREB.TTF", 13.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    ZeroMemory(&msg, sizeof(msg));
    mem = new Memory;
    GameData::Initialize();
    Shield_Initialize(GameData::BASE);
    GameData::FindGnames();
    InitImages();
    CreateThread(NULL, 0, UpdateUData, NULL, 0, NULL);
    CreateThread(NULL, 0, UpdateCam, NULL, 0, NULL);
    CreateThread(NULL, 0, AimThread, NULL, 0, NULL);
    CreateThread(NULL, 0, MagicBullet, NULL, 0, NULL);
    CreateThread(NULL, 0, HOTKEY, NULL, 0, NULL);
    CreateThread(NULL, 0, Top, NULL, 0, NULL);
    bool read = false;
    bool aread = false;
    int FPS;
    bool fps = false;
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
    g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
    //asshole();
    while (1)
	{
		if (!FindWindowA(wclass, NULL))
			exit(0);

		//mem->WVM<float>(GameData::LocalPlayer_Controller + 0x608, 1);
		//mem->WVM<float>(GameData::LocalPlayer_Controller + 0x60C, 1);

		DEVMODE dm;
		dm.dmSize = sizeof(DEVMODE);

		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);

		FPS = dm.dmDisplayFrequency;

		a = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> work_time = a - b;
		if (work_time.count() < 1000 / FPS)
		{
			std::chrono::duration<double, std::milli> delta_ms(1000 / FPS - work_time.count());
			auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
			std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
		}

		b = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> sleep_time = b - a;

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::EndFrame();
		if (g_pd3dDevice->BeginScene() >= 0) {
			DrawScense();
			DrawRadar();
			DrawMenu();
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();

			WPARAM result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

			if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
				ResetDevice();

			g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);
		}
	}

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    return 0;
}

string WebWinhttp(string details)
{
    //VMProtectBeginUltra("WebFunc");
    DWORD dwSize = 0, dwDownloaded;
    LPSTR source;
    source = (char*)"";
    string responsed = "";

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL bResults = FALSE;

    hSession = WinHttpOpen(L"Winhttp API", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession)
        hConnect = WinHttpConnect(hSession, get_utf16("gzone.xyz", CP_UTF8).c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);

    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, L"GET", get_utf16(details, CP_UTF8).c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

    if (hRequest)
        bResults = WinHttpSendRequest(hRequest, L"content-type:application/x-www-form-urlencoded", -1, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);

    if (bResults) {
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                printf("Error %u", GetLastError());

            source = (char*)malloc(dwSize + 1);
            if (!source) {
                printf("Out of memory\n");
                dwSize = 0;
            }
            else {
                ZeroMemory(source, dwSize + 1);

                if (!WinHttpReadData(hRequest, (LPVOID)source, dwSize, &dwDownloaded))
                    printf("Error %u", GetLastError());
                else
                    responsed = responsed + source;
                free(source);
            }
        } while (dwSize > 0);
    }

    if (!bResults) {
        exit(0);
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return responsed;
    //VMProtectEnd();
}

string menuchar, codeStr;

string Hash, CodeVal, decrypted, decoded;

DWORD_PTR PatternScan(char* base, size_t size, char* pattern, char* mask)
{
    size_t patternLength = strlen(mask);

    for (unsigned int i = 0; i < size - patternLength; i++)
    {
        //std::cout << i << std::endl;
        bool found = true;
        for (unsigned int j = 0; j < patternLength; j++)
        {
            if (mask[j] != '?' && pattern[j] != *(base + i + j))
            {
                found = false;
                break;
            }
        }
        if (found)
        {
            return (DWORD_PTR)(base + i);
        }
    }
    return 0;
}
std::vector<DWORD_PTR> PatternScanEx(uintptr_t begin, uintptr_t end, char* pattern, char* mask)
{
    size_t patternLength = strlen(mask);
    std::vector<DWORD_PTR> _return;
    uintptr_t currentChunk = begin;
    DWORD_PTR internalAddress;

    while (currentChunk < end)
    {
        internalAddress = 0;
        char buffer[0x2000];
        int len;
        if ((end - currentChunk) > sizeof(buffer)) {
            len = sizeof(buffer);
        }
        else {
            len = end - currentChunk;
        }

        KernelDriver::DriverReadMemory((currentChunk), (PVOID)&buffer, len);
        internalAddress = PatternScan((char*)buffer, len, pattern, mask);
        if (internalAddress != 0)
        {
            DWORD_PTR offsetFromBuffer = (DWORD_PTR)internalAddress - (DWORD_PTR)&buffer;
            _return.push_back((DWORD_PTR)currentChunk + offsetFromBuffer);
            currentChunk = currentChunk + offsetFromBuffer + patternLength;
        }
        else
        {
            currentChunk = currentChunk + len;
        }
    }
    return _return;
}

void asshole() {
    char pattern_UWorld[] = "\x48\x8B\x0D\x00\x00\x00\x00\x4C\x39\x2D";
    char pattern_NAME[] = "\x48\x8D\x15\x00\x00\x00\x00";
    char pattern_OFFSET[] = "\x41\xB9\x00\x00\x00\x00\x45\x33\xC0";
    char mask_UWorld[] = "xxx????xxx";
    char mask_NAME[] = "xxx????";
    char mask_OFFSET[] = "xx????xxx";
    vector<DWORD_PTR> UWorld = PatternScanEx(GameData::BASE, GameData::BASE + 0xFFFFFFF, pattern_UWorld, mask_UWorld);
    if (UWorld.size() > 0)
    {
        for (int i = 0; i < UWorld.size(); i++)
        {
            long long test = mem->RVM<int>(UWorld[i] + 03);
            uint64_t gUworld = UWorld[i] + 07 + test - GameData::BASE;
            cout << "find " << i << ": UWorld = 0x" << std::hex << gUworld << endl;

            long long KeyUWorld = mem->RVM<int>(UWorld[i] + 0x14);
            cout << "find " << i << ": KeyUWorld = 0x" << std::hex << KeyUWorld << endl;
        }
	}
    int a = 0;
    vector<DWORD_PTR> NAME = PatternScanEx(GameData::BASE, GameData::BASE + 0xFFFFFFF, pattern_NAME, mask_NAME);
	vector<DWORD_PTR> offset = PatternScanEx(NAME[0], GameData::BASE + 0xFFFFFFF, pattern_NAME, mask_NAME);
	if (offset.size() > 0) {
		long long test = mem->RVM<int>(NAME[0] + 0x3);
		uint64_t offsets = NAME[0] + 0x7 + test - GameData::BASE;
		cout << "find 0: TestOffset = 0x" << std::hex << offsets << endl;
    }
}

#include <Aclapi.h>
#include <sddl.h>

BOOL DenyAccess()
{

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(".....", SDDL_REVISION_1, &(sa.lpSecurityDescriptor), NULL))
        return FALSE;

    if (!SetKernelObjectSecurity(hProcess, DACL_SECURITY_INFORMATION, sa.lpSecurityDescriptor))
        return FALSE;

    return TRUE;

}

int main(string arg, char* argv[])
{
    //VMProtectBeginUltra("MainFunc_");
    DenyAccess();
    DWORD old;
    HANDLE handle_ = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
    VirtualProtectEx(handle_, &main, sizeof(&main), PAGE_GUARD | PAGE_NOACCESS, &old);
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    mainthread();
    codeStr = argv[1];
    if (codeStr != "")
    {
    MM:
        srand(time(0));
        for (int i = 0; i < 32; ++i) {
            Hash += genRandom();
        }
        try {
            CodeVal = WebWinhttp("/UserAgent.php?param=Login&id=" + codeStr + "&otp=" + Hash);
        }
        catch (...) {}
        try {
            decrypted = encrypt(CodeVal, Hash);
        }
        catch (...) {}
        if (decrypted.find("Success : ") != string::npos)
        {
            cout << "Login Success" << endl;
            Sleep(2500);
            mainthread();
        }
        else if (decrypted.find("Expired Agent") != string::npos)
        {
            system("cls");
            system("cls");
            cout << "Login Failed ( no service. )" << endl;
            Sleep(2500);
            exit(0);
        }
        else if (decrypted.find("None Agent") != string::npos)
        {
            system("cls");
            system("cls");
            cout << "Login Failed ( another device was added on server. )" << endl;
            Sleep(2500);
            exit(0);
        }
        else if (decrypted.find("Expired Token") != string::npos) {
            system("cls");
            system("cls");
            cout << "Login Failed ( Expired Token... Retry... )" << endl;
            goto MM;
        }
        else
        {
            system("cls");
            system("cls");
            cout << "Login Failed ( Unknown Error. )" << endl;
            Sleep(2500);
            exit(0);
        }
    }
    else
    {
        system("cls");
        system("cls");
        cout << "The license may not be longer than 1 character." << endl;
        Sleep(2500);
        exit(0);
    }

    return 0;
    //VMProtectEnd();
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;
    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    g_pd3dDevice->Reset(&g_d3dpp);
    ImGui_ImplDX9_CreateDeviceObjects();
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
