#pragma once
#include <string>
#include <vector>
#include <unordered_set>

struct CampaignMission {
    std::string id;
    std::string file;
    std::string name;
    std::string description;
    int order = 1;
};

class CampaignManager {
public:
    static void init();
    static bool loadCampaign(const std::string& filepath = "");
    static bool saveCampaign(const std::string& filepath = "");

    static const std::string& getCampaignTitle();
    static void setCampaignTitle(const std::string& title);

    static const std::vector<CampaignMission>& getMissions();
    static bool moveMissionUp(int index);
    static bool moveMissionDown(int index);
    static bool addMission(const std::string& file, const std::string& name = "", const std::string& description = "");
    static bool removeMission(int index);

    static bool isMissionUnlocked(int index, bool devMode = false);
    static bool isMissionCompleted(const std::string& fileOrId);
    static void completeMission(const std::string& fileOrId);

    static bool isDevMode();
    static void setDevMode(bool enabled);
    static void toggleDevMode();

    static void resetProgress();
    static void unlockAllProgress();

    static bool isFileInCampaign(const std::string& filename);
    static int getMissionIndexByFile(const std::string& filename);
    static std::string getNextMissionFile(const std::string& currentFile);

    static std::string getCampaignFilePath();

private:
    static std::string s_campaignTitle;
    static std::vector<CampaignMission> s_missions;
    static std::unordered_set<std::string> s_completedMissions;
    static bool s_devMode;
    static bool s_initialized;

    static void loadProgress();
    static void saveProgress();
};

