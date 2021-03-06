//
// WorldManager.cpp
//

#include "WorldManager.hpp"
#include "PlayerManager.hpp"
#include "CommandManager.hpp"
#include "AccountManager.hpp"
#include "ManagerAccessor.hpp"
#include "3d/Stage.hpp"
#include "Profiler.hpp"
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>

WorldManager::WorldManager(const StagePtr& stage, const ManagerAccessorPtr& manager_accessor) :
manager_accessor_(manager_accessor),
stage_(stage),
game_loop_(manager_accessor, stage_)
{

}

void WorldManager::Init()
{
    auto player_manager = manager_accessor_->player_manager().lock();
    game_loop_.Init(player_manager->charmgr());

}

void WorldManager::ProcessInput(InputManager* input)
{
	MMO_PROFILE_FUNCTION;

    game_loop_.ProcessInput(input);
}

void WorldManager::Update()
{
	MMO_PROFILE_FUNCTION;
	auto account_manager = manager_accessor_->account_manager().lock();
	game_loop_.Update();
}

void WorldManager::Draw()
{
	MMO_PROFILE_FUNCTION;

    game_loop_.Draw();

}

bool WorldManager::IsVisiblePoint(const VECTOR& point)
{
    return stage_->IsVisiblePoint(point);
}

bool WorldManager::IsNearPoint(const VECTOR& point)
{
    return stage_->IsNearPoint(point);
}

StagePtr WorldManager::stage()
{
    return stage_;
}

FieldPlayerPtr WorldManager::myself()
{
    return game_loop_.myself();
}

void WorldManager::ResetCameraPosition()
{
    game_loop_.ResetCameraPosition();
}