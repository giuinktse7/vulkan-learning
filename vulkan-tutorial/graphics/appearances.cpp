#include "appearances.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <memory>

#include "texture_atlas.h"
#include "../items.h"
#include "engine.h"

using namespace std;
using namespace tibia::protobuf;

std::unordered_map<uint32_t, tibia::protobuf::appearances::Appearance> Appearances::appearances;

std::set<uint32_t> Appearances::catalogIndex;
std::unordered_map<uint32_t, CatalogInfo> Appearances::catalogInfo;

bool Appearances::isLoaded;

void Appearances::loadFromFile(const std::filesystem::path path)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    appearances::Appearances parsed;

    {
        std::fstream input(path, std::ios::in | std::ios::binary);
        std::cout << path << std::endl;
        if (!parsed.ParseFromIstream(&input))
        {
            auto absolutePath = filesystem::absolute(filesystem::path(path));
            std::cerr << "Failed to parse appearances file at " << absolutePath << "." << endl;
        }
        std::cout << "c" << std::endl;

        google::protobuf::ShutdownProtobufLibrary();
    }
    std::cout << "d" << std::endl;

    for (int i = 0; i < parsed.object_size(); ++i)
    {
        const appearances::Appearance &object = parsed.object(i);
        Appearances::appearances[object.id()] = object;
    }

    Appearances::isLoaded = true;
}

void Appearances::addSpriteSheetInfo(CatalogInfo &info)
{
    Appearances::catalogIndex.insert(info.lastSpriteId);
    Appearances::catalogInfo[info.lastSpriteId] = info;
}

CatalogInfo Appearances::getCatalogInfo(uint32_t spriteId)
{
    auto found = lower_bound(catalogIndex.begin(), catalogIndex.end(), spriteId);
    if (found == catalogIndex.end())
    {
        cout << "Could not find a sprite sheet for sprite ID " << spriteId << "." << endl;
        exit(1);
    }

    uint32_t lastSpriteId = *found;

    return catalogInfo.at(lastSpriteId);
}

void Appearances::loadCatalog(const std::filesystem::path path)
{
    if (!filesystem::exists(path))
    {
        cout << "Could not locate the catalog JSON file. Failed to find file at path: " + filesystem::absolute(path).u8string() << endl;
        exit(1);
    }

    std::ifstream fileStream(path);
    auto json = make_unique<nlohmann::json>();
    fileStream >> (*json);

    std::filesystem::path basepath("C:/Users/giuin/AppData/Local/Tibia11/packages/Tibia/assets");

    for (const auto &x : *json)
    {
        if (x.at("type") == "sprite")
        {
            std::string file = x.at("file");
            std::filesystem::path filePath(file);

            CatalogInfo info{};
            info.area = x.at("area");
            info.file = basepath / filePath;
            info.spriteType = x.at("spritetype");
            info.firstSpriteId = x.at("firstspriteid");
            info.lastSpriteId = x.at("lastspriteid");
            addSpriteSheetInfo(info);
        }
    }
}

//std::vector<std::vector<uint8_t>> Appearances::getSprites(Item &item)
//{
//    return getSprites(item.getId());
//}

//std::vector<std::vector<uint8_t>> Appearances::getSprites(uint16_t serverId)
//{
//    uint16_t clientId = Items::items.getClientId(serverId);
//
//    std::vector<std::vector<uint8_t>> result;
//
//    // TODO Use all frame groups, not just the first one.
//    auto fg = Appearances::appearances.at(clientId).frame_group().at(0);
//
//    std::string path;
//    std::unordered_map<std::string, TextureAtlas> atlases;
//    for (const auto spriteId : fg.sprite_info().sprite_id())
//    {
//        CatalogInfo catalogInfo = getCatalogInfo(spriteId);
//        path = catalogInfo.file;
//        if (atlases.find(path) == atlases.end())
//        {
//            std::filesystem::path filePath(catalogInfo.file);
//
//            catalogInfo.file = "C:/Users/giuin/Desktop/tibia_spritesheets/images/" + filePath.stem().u8string();
//
//            atlases.emplace(std::piecewise_construct,
//                            std::forward_as_tuple(path),
//                            std::forward_as_tuple(catalogInfo));
//        }
//
//        result.emplace_back(atlases.at(path).get(spriteId));
//    }
//
//    return result;
//}
