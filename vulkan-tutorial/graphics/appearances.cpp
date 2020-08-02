#include "appearances.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <memory>

#include "../util.h"

#include "texture_atlas.h"
#include "../items.h"
#include "engine.h"

#include <set>

using namespace std;
using namespace tibia::protobuf;

std::unordered_map<uint32_t, Appearance> Appearances::objects;
std::unordered_map<uint32_t, tibia::protobuf::appearances::Appearance> Appearances::outfits;

std::set<uint32_t> Appearances::catalogIndex;
std::unordered_map<uint32_t, CatalogInfo> Appearances::catalogInfo;

bool Appearances::isLoaded;

void Appearances::loadFromFile(const std::filesystem::path path)
{
    TimeMeasure startTime = TimeMeasure::start();

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

        google::protobuf::ShutdownProtobufLibrary();
    }

    int total = 0;
    for (int i = 0; i < parsed.object_size(); ++i)
    {
        const appearances::Appearance &object = parsed.object(i);
        auto info = object.frame_group().at(0).sprite_info();
        // if (object.id() == 3031 || object.id() == 103)
        // {
        //     std::cout << "\n(cid: " << object.id() << "): " << std::endl;
        //     std::cout << "info: " << info << std::endl;
        //     if (object.has_flags())
        //     {
        //         std::cout << "flags: " << object.flags() << std::endl;
        //     }
        // }

        // if (object.has_flags() && object.flags().has_cumulative() && object.flags().cumulative() && info.sprite_id_size() > 1 && info.has_animation())
        // {
        //     std::cout << "\n(cid: " << object.id() << "): " << std::endl;
        //     std::cout << "framegroups: " << object.frame_group_size() << std::endl;
        //     std::cout << "info: " << info << std::endl;
        //     if (object.has_flags())
        //     {
        //         std::cout << "flags: " << std::endl;
        //         std::cout << object.flags() << std::endl;
        //     }
        //     if (info.has_animation())
        //     {
        //         std::cout << "Animation:" << std::endl
        //                   << info.animation() << std::endl;
        //     }
        //     ++total;
        // }
        Appearances::objects.emplace(object.id(), object);
    }

    std::cout << "Total: " << total << std::endl;

    for (int i = 0; i < parsed.outfit_size(); ++i)
    {
        const appearances::Appearance &outfit = parsed.outfit(i);
        Appearances::outfits[outfit.id()] = outfit;
    }

    Appearances::isLoaded = true;

    std::cout << "Loaded appearances.dat in " << startTime.elapsedMillis() << " milliseconds." << std::endl;
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

    std::set<std::string> types;

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
        else
        {
            std::string s = x.at("type");
            types.insert(s);
        }
    }

    // std::cout << "Types: " << std::endl;
    // for (const auto &t : types)
    // {
    //     std::cout << t << std::endl;
    // }
}

SpriteInfo SpriteInfo::fromProtobufData(tibia::protobuf::appearances::SpriteInfo spriteInfo)
{
    SpriteInfo info{};
    info.animation = std::make_unique<SpriteAnimation>(SpriteAnimation::fromProtobufData(spriteInfo.animation()));
    info.boundingSquare = spriteInfo.bounding_square();
    info.isOpaque = spriteInfo.bounding_square();
    info.layers = spriteInfo.layers();
    info.patternWidth = spriteInfo.pattern_width();
    info.patternHeight = spriteInfo.pattern_height();
    info.patternDepth = spriteInfo.pattern_depth();

    for (auto id : spriteInfo.sprite_id())
    {
        info.spriteIds.emplace_back<uint32_t &>(id);
    }

    return info;
}

SpriteAnimation SpriteAnimation::fromProtobufData(tibia::protobuf::appearances::SpriteAnimation animation)
{
    SpriteAnimation anim{};
    anim.defaultStartPhase = animation.default_start_phase();
    anim.loopCount = animation.loop_count();

    switch (animation.loop_type())
    {
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_PINGPONG:
        anim.loopType = AnimationLoopType::PingPong;
        break;
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_COUNTED:
        anim.loopType = AnimationLoopType::Counted;
        break;
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_INFINITE:
    default:
        anim.loopType = AnimationLoopType::Infinite;
        break;
    }

    for (auto phase : animation.sprite_phase())
    {
        anim.phases.push_back({phase.duration_min(), phase.duration_max()});
    }

    anim.defaultStartPhase = animation.default_start_phase();
    anim.randomStartPhase = animation.random_start_phase();
    anim.synchronized = animation.synchronized();

    return anim;
}

Appearance::Appearance(tibia::protobuf::appearances::Appearance protobufAppearance)
{
    this->clientId = protobufAppearance.id();
    this->name = protobufAppearance.name();

    if (protobufAppearance.frame_group_size() == 1)
    {
        this->appearanceData = SpriteInfo::fromProtobufData(protobufAppearance.frame_group().at(0).sprite_info());
    }

    // Flags
    {
        this->flags = static_cast<AppearanceFlags>(0);
        if (protobufAppearance.has_flags())
        {
            auto flags = protobufAppearance.flags();

#define ADD_FLAG_UTIL(flagType, flag) \
    do                                \
    {                                 \
        if (flags.has_##flagType())   \
        {                             \
            this->flags |= flag;      \
        }                             \
    } while (false)

            ADD_FLAG_UTIL(bank, AppearanceFlags::Bank);
            ADD_FLAG_UTIL(clip, AppearanceFlags::Clip);
            ADD_FLAG_UTIL(bottom, AppearanceFlags::Bottom);
            ADD_FLAG_UTIL(top, AppearanceFlags::Top);
            ADD_FLAG_UTIL(container, AppearanceFlags::Container);
            ADD_FLAG_UTIL(cumulative, AppearanceFlags::Cumulative);
            ADD_FLAG_UTIL(usable, AppearanceFlags::Usable);
            ADD_FLAG_UTIL(forceuse, AppearanceFlags::Forceuse);
            ADD_FLAG_UTIL(multiuse, AppearanceFlags::Multiuse);
            ADD_FLAG_UTIL(write, AppearanceFlags::Write);
            ADD_FLAG_UTIL(write_once, AppearanceFlags::WriteOnce);
            ADD_FLAG_UTIL(liquidpool, AppearanceFlags::Liquidpool);
            ADD_FLAG_UTIL(unpass, AppearanceFlags::Unpass);
            ADD_FLAG_UTIL(unmove, AppearanceFlags::Unmove);
            ADD_FLAG_UTIL(unsight, AppearanceFlags::Unsight);
            ADD_FLAG_UTIL(avoid, AppearanceFlags::Avoid);
            ADD_FLAG_UTIL(no_movement_animation, AppearanceFlags::NoMovementAnimation);
            ADD_FLAG_UTIL(take, AppearanceFlags::Take);
            ADD_FLAG_UTIL(liquidcontainer, AppearanceFlags::LiquidContainer);
            ADD_FLAG_UTIL(hang, AppearanceFlags::Hang);
            ADD_FLAG_UTIL(hook, AppearanceFlags::Hook);
            ADD_FLAG_UTIL(rotate, AppearanceFlags::Rotate);
            ADD_FLAG_UTIL(light, AppearanceFlags::Light);
            ADD_FLAG_UTIL(dont_hide, AppearanceFlags::DontHide);
            ADD_FLAG_UTIL(translucent, AppearanceFlags::Translucent);
            ADD_FLAG_UTIL(shift, AppearanceFlags::Shift);
            ADD_FLAG_UTIL(height, AppearanceFlags::Height);
            ADD_FLAG_UTIL(lying_object, AppearanceFlags::LyingObject);
            ADD_FLAG_UTIL(animate_always, AppearanceFlags::AnimateAlways);
            ADD_FLAG_UTIL(automap, AppearanceFlags::Automap);
            ADD_FLAG_UTIL(lenshelp, AppearanceFlags::Lenshelp);
            ADD_FLAG_UTIL(fullbank, AppearanceFlags::Fullbank);
            ADD_FLAG_UTIL(ignore_look, AppearanceFlags::IgnoreLook);
            ADD_FLAG_UTIL(clothes, AppearanceFlags::Clothes);
            ADD_FLAG_UTIL(default_action, AppearanceFlags::DefaultAction);
            ADD_FLAG_UTIL(market, AppearanceFlags::Market);
            ADD_FLAG_UTIL(wrap, AppearanceFlags::Wrap);
            ADD_FLAG_UTIL(unwrap, AppearanceFlags::Unwrap);
            ADD_FLAG_UTIL(topeffect, AppearanceFlags::Topeffect);
            // TODO npcsaledata flag is not handled right now
            ADD_FLAG_UTIL(changedtoexpire, AppearanceFlags::ChangedToExpire);
            ADD_FLAG_UTIL(corpse, AppearanceFlags::Corpse);
            ADD_FLAG_UTIL(player_corpse, AppearanceFlags::PlayerCorpse);
            ADD_FLAG_UTIL(cyclopediaitem, AppearanceFlags::CyclopediaItem);
        }

#undef ADD_FLAG_UTIL
    }
}

uint32_t Appearance::getFirstSpriteId() const
{
    return getSpriteInfo().spriteIds.at(0);
}

const SpriteInfo &Appearance::getSpriteInfo(size_t frameGroup) const
{
    if (std::holds_alternative<SpriteInfo>(appearanceData))
    {
        return std::get<SpriteInfo>(appearanceData);
    }
    else
    {
        return std::get<std::vector<FrameGroup>>(appearanceData).at(frameGroup).spriteInfo;
    }
}

const SpriteInfo &Appearance::getSpriteInfo() const
{
    return getSpriteInfo(0);
}

size_t Appearance::frameGroupCount() const
{
    if (std::holds_alternative<SpriteInfo>(appearanceData))
    {
        return 1;
    }

    return std::get<std::vector<FrameGroup>>(appearanceData).size();
}