//
// PlayerManager.cpp
//

#include "PlayerManager.hpp"
#include "CommandManager.hpp"
#include "CardManager.hpp"
#include "WorldManager.hpp"
#include "AccountManager.hpp"
#include "../common/network/Utils.hpp"
#include "../common/network/Command.hpp"
#include "../common/database/AccountProperty.hpp"
#include "../common/Logger.hpp"
#include "../common/unicode.hpp"

#include "3d/CharacterDataProvider.hpp"
#include "3d/CharacterManager.hpp"
#include "3d/Timer.hpp"
#include "3d/model.hpp"
#include "3d/PlayerCharacter.hpp"

#include <list>
#include <boost/foreach.hpp>

PlayerManager::PlayerManager(const ManagerAccessorPtr& manager_accessor) :
timer_(std::make_shared<Timer>()),
manager_accessor_(manager_accessor),
charmgr_(std::make_shared<CharacterManager>())
{
}

PlayerManager::~PlayerManager()
{

}

void PlayerManager::Init()
{
    auto account_manager = manager_accessor_->account_manager().lock();
    AddCharacter<FieldPlayer>(0, unicode::ToTString(account_manager->model_name()));
    charmgr_->set_my_character_id(0);
}

void PlayerManager::ProcessInput(InputManager* input)
{

}

void PlayerManager::Update()
{
    timer_->Next();

    // 自分自身のキャラクターIDは常に0, それ以外はアカウントのID
    // プレイヤーの位置情報を更新
    if (auto player = GetMyself()) {
        const VECTOR& pos = char_data_providers_[charmgr_->my_character_id()]->position();
        const float theta = char_data_providers_[charmgr_->my_character_id()]->theta();
        player->set_position(PlayerPosition(pos.x, pos.y, pos.z, theta));
    }

    // 位置情報を送信
    static int count = 0;
    if (count % 30 == 0) {
        const VECTOR& pos = char_data_providers_[charmgr_->my_character_id()]->position();
        const float theta = char_data_providers_[charmgr_->my_character_id()]->theta();
        if (auto command_manager = manager_accessor_->command_manager().lock()) {
            command_manager->Write(network::ServerUpdatePlayerPosition(pos.x, pos.y, pos.z, theta));
            //command_manager->WriteUDP(network::ServerUpdatePlayerPosition(pos.x, pos.y, pos.z, theta));
        }
        // Logger::Debug("PlayerPos %f %f %f", pos.x, pos.y, pos.z);
    }
    count++;

    BOOST_FOREACH (auto pair, login_players_) {
        if (pair.second->login()) {
            pair.second->Update();
        }
    }
}

void PlayerManager::Draw()
{
    // TODO: モデルの高さを取得する必要あり
    BOOST_FOREACH(auto pair, login_players_) {
        if (pair.second->login()) {
            if (char_data_providers_.find(pair.second->id()) != char_data_providers_.end()) {
                const VECTOR& pos = char_data_providers_[pair.second->id()]->position();
                const float theta = char_data_providers_[pair.second->id()]->theta();
                pair.second->set_position(PlayerPosition(pos.x, pos.y, pos.z, theta));

                auto world_manager = manager_accessor_->world_manager().lock();
                if (!world_manager->IsVisiblePoint(pos + VGet(0, 25, 0))) {
                    continue;
                }
            }

            pair.second->Draw();
        }
    }
}

uint32_t PlayerManager::GetCurrentUserRevision(uint32_t user_id)
{
    if (auto player = GetFromId(user_id)) {
        return player->revision();
    } else {
        return 0;
    }
}

void PlayerManager::ApplyRevisionPatch(const std::string& patch)
{
    std::string buffer(patch);
    Logger::Debug(_T("%s"), unicode::ToTString(network::Utils::ToHexString(buffer)));

    uint32_t user_id;
    uint32_t new_revision;
    buffer.erase(0, network::Utils::Deserialize(buffer, &user_id, &new_revision));


    PlayerPtr player;
    auto it_player = login_players_.find(user_id);
    if (it_player != login_players_.end()) {
        player = it_player->second;
    } else {
        player = std::make_shared<Player>(user_id);
        login_players_[user_id] = player;
    }

    assert(player);
    player->set_revision(new_revision);

    while (buffer.size()) {
        uint16_t property_int;
        buffer.erase(0, network::Utils::Deserialize(buffer, &property_int));

        AccountProperty property = static_cast<AccountProperty>(property_int);
        Logger::Debug(_T("UpdatePlayer : %d %d"), user_id, property);

        switch (property) {

            case LOGIN:
            {
                char value;
                buffer.erase(0, network::Utils::Deserialize(buffer, &value));
                bool login = value;

                if (!player->login() && login) {
                    if (!player->name().empty()) {
                        auto card_manager = manager_accessor_->card_manager().lock();
                        card_manager->OnLogin(player);
                    }
                } else if(player->login() && !login) {
                    RemoveCharacter(user_id);
                }

                player->set_login(login);
            }
                Logger::Debug(_T("UpdateLoginStatus %d"), user_id);
                break;

            case NAME:
            {
                auto initialize = player->name().empty();

                std::string value;
                buffer.erase(0, network::Utils::Deserialize(buffer, &value));
                player->set_name(value);

                // 名前を受信した時にログインを通知
                if (initialize && player->login()) {
                    auto card_manager = manager_accessor_->card_manager().lock();
                    card_manager->OnLogin(player);
                }
            }
                Logger::Debug(_T("UpdateName %d"), user_id);
                break;

            case TRIP:
            {
                std::string value;
                buffer.erase(0, network::Utils::Deserialize(buffer, &value));
                player->set_trip(value);
            }
                Logger::Debug(_T("UpdateTrip %d"), user_id);
                break;

            case MODEL_NAME:
            {
                std::string value;
                buffer.erase(0, network::Utils::Deserialize(buffer, &value));

                if (player->model_name() != value) {
                    auto command_manager = manager_accessor_->command_manager().lock();
                    if (user_id != command_manager->user_id()) {
                        if (char_data_providers_.find(user_id) != char_data_providers_.end()) {
                            RemoveCharacter(user_id);
                        }
                    }
                    player->set_model_name(value);
                }
            }
                Logger::Debug(_T("UpdateModelName %d"), user_id);
                break;

            case IP_ADDRESS:
            {
                std::string value;
                buffer.erase(0, network::Utils::Deserialize(buffer, &value));
                player->set_ip_address(value);
            }
                Logger::Debug(_T("UpdateIPAddress %d"), user_id);
                break;

            case UDP_PORT:
            {
                uint16_t port;
                buffer.erase(0, network::Utils::Deserialize(buffer, &port));
                player->set_udp_port(port);
            }
                Logger::Debug(_T("UpdateUDPPort %d"), user_id);
                break;

            default:
                ;
        }
    }

    if (char_data_providers_.find(user_id) == char_data_providers_.end()) {
        if (player->login() && player->model_name().size() > 0) {
            auto command_manager = manager_accessor_->command_manager().lock();
            if (user_id != command_manager->user_id()) {
                AddCharacter<PlayerCharacter>(user_id, unicode::ToTString(player->model_name()));
            }
        }
    }

}

PlayerPtr PlayerManager::GetMyself()
{
    if (auto command_manager = manager_accessor_->command_manager().lock()) {
        return GetFromId(command_manager->user_id());
    } else {
        return PlayerPtr();
    }
}

PlayerPtr PlayerManager::GetFromId(unsigned int user_id)
{
    if (login_players_.find(user_id) != login_players_.end()) {
        return login_players_[user_id];
    } else {
        return PlayerPtr();
    }
}

std::vector<PlayerPtr> PlayerManager::GetAll()
{
    std::vector<PlayerPtr> players;
    BOOST_FOREACH (auto pair, login_players_) {
        if (pair.second->login()) {
            players.push_back(pair.second);
        }
    }
    return players;
}

void PlayerManager::UpdatePlayerPosition(unsigned int user_id, const PlayerPosition& pos)
{
    if (char_data_providers_.find(user_id) != char_data_providers_.end()) {
        char_data_providers_[user_id]->set_target_position(VGet(pos.x, pos.y, pos.z));
    }
}

std::shared_ptr<CharacterManager> PlayerManager::charmgr() const
{
    return charmgr_;
}

TimerPtr PlayerManager::timer() const
{
    return timer_;
}

std::map<unsigned int, std::unique_ptr<CharacterDataProvider>>& PlayerManager::char_data_providers()
{
    return char_data_providers_;
}

// 指定されたキャラIDとモデル番号を持つキャラをcharmgrに追加する
template <typename CharacterType>
void PlayerManager::AddCharacter(unsigned int user_id, const tstring& model_name)
{
    auto world_manager = manager_accessor_->world_manager().lock();
    std::unique_ptr<CharacterDataProvider> cdp_ptr(new CharacterDataProvider());
    auto& cdp = *cdp_ptr;
    char_data_providers_[user_id] = move(cdp_ptr);
    cdp.set_id(user_id);
    cdp.set_model(ResourceManager::LoadModelFromName(model_name));
    auto character = std::make_shared<CharacterType>(cdp, world_manager->stage(), timer_);
    charmgr_->Add(user_id, character);
}

// 指定されたキャラIDとモデル番号を持つキャラをcharmgrに追加する
void PlayerManager::RemoveCharacter(unsigned int user_id)
{
    auto card_manager = manager_accessor_->card_manager().lock();
    card_manager->OnLogout(login_players_[user_id]);
    charmgr_->Remove(user_id);

    assert(char_data_providers_.find(user_id) != char_data_providers_.end());
    char_data_providers_.erase(user_id);
}
