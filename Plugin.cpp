#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "BehaviourTree/Blackboard.h"
#include "BehaviourTree/Behaviors.h"

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);
	m_InventoryItems.resize(m_pInterface->Inventory_GetCapacity());

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "Da Floosh";
	info.Student_FirstName = "Jens";
	info.Student_LastName = "Bonnarens";
	info.Student_Class = "2DAE5";

	SetBlackBoardAndBehaviourTree();
}

//Called only once
void Plugin::DllInit()
{
	//Can be used to figure out the source of a Memory Leak
	//Possible undefined behavior, you'll have to trace the source manually 
	//if you can't get the origin through _CrtSetBreakAlloc(0) [See CallStack]
	//_CrtSetBreakAlloc(1315);

	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
	SAFE_DELETE(m_pBehaviourTree);

}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
							//params.LevelFile = "LevelTwo.gppl";
	params.AutoGrabClosestItem = false; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.OverrideDifficulty = false; //Override Difficulty?
	params.Difficulty = 1.f; //Difficulty Override: 0 > 1 (Overshoot is possible, >1)

}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::ProcessEvents(const SDL_Event& e)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	switch (e.type)
	{
	case SDL_MOUSEBUTTONUP:
	{
		if (e.button.button == SDL_BUTTON_LEFT)
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(x), static_cast<float>(y));
			m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
		}
		break;
	}
	case SDL_KEYDOWN:
	{
		if (e.key.keysym.sym == SDLK_SPACE)
		{
			m_CanRun = true;
		}
		else if (e.key.keysym.sym == SDLK_LEFT)
		{
			m_AngSpeed -= Elite::ToRadians(10);
		}
		else if (e.key.keysym.sym == SDLK_RIGHT)
		{
			m_AngSpeed += Elite::ToRadians(10);
		}
		else if (e.key.keysym.sym == SDLK_g)
		{
			m_GrabItem = true;
		}
		else if (e.key.keysym.sym == SDLK_u)
		{
			m_UseItem = true;
		}
		else if (e.key.keysym.sym == SDLK_r)
		{
			m_RemoveItem = true;
		}
		else if (e.key.keysym.sym == SDLK_d)
		{
			m_DropItem = true;
		}
		break;
	}
	case SDL_KEYUP:
	{
		if (e.key.keysym.sym == SDLK_SPACE)
		{
			m_CanRun = false;
		}
		break;
	}
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	auto steering = SteeringPlugin_Output();

	auto vHousesInFOV = GetHousesInFOV();//uses m_pInterface->Fov_GetHouseByIndex(...)
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)


	auto pBlackBoard = m_pBehaviourTree->GetBlackboard();
	if (pBlackBoard)
	{
		pBlackBoard->ChangeData("AgentInfo", m_AgentInfo);
		pBlackBoard->ChangeData("HousesInFOV", GetHousesInFOV());
		pBlackBoard->ChangeData("EntitiesInFOV", GetEntitiesInFOV());
		//pBlackBoard->ChangeData("InventoryItems", m_InventoryItems);
		pBlackBoard->GetData("Target", m_Target);
		pBlackBoard->GetData("AutoOrientate", steering.AutoOrientate);
		pBlackBoard->GetData("AngularSpeed", m_AgentInfo.AngularVelocity);
		pBlackBoard->GetData("CanRun", m_CanRun);
		pBlackBoard->ChangeData("RecentHouses", m_RecentHouseLocations);
	}

	m_AgentInfo = m_pInterface->Agent_GetInfo();

	if (!GetHousesInFOV().empty())
	{
		for (HouseInfo& house : GetHousesInFOV())
		{
			auto location = house.Center;

			auto result = std::find(m_RecentHouseLocations.begin(), m_RecentHouseLocations.end(), house.Center);
			
			if (result == m_RecentHouseLocations.end())
			{
				bool isGoingToHouse;
				pBlackBoard->GetData("IsGoingToHouse", isGoingToHouse);
				if (!isGoingToHouse)
				{
					if (m_RecentHouseLocations.size() == 5)
					{
						m_RecentHouseLocations[0] = m_RecentHouseLocations[1];
						m_RecentHouseLocations[1] = m_RecentHouseLocations[2];
						m_RecentHouseLocations[2] = m_RecentHouseLocations[3];
						m_RecentHouseLocations[3] = m_RecentHouseLocations[4];
						m_RecentHouseLocations[4] = house.Center;
					}
					else
					{
						m_RecentHouseLocations.push_back(house.Center);
					}
				}
			}
		}
	}

	if (m_AgentInfo.IsInHouse)
	{
		m_InHouseTimer += dt;

		if (m_InHouseTimer >= 3.f)
		{
			m_InHouseTimer = 0.f;
			pBlackBoard->ChangeData("IsGoingToHouse", false);
			pBlackBoard->ChangeData("CanRun", false);
		}
	}

	Seek(steering, pBlackBoard);

	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

	m_pBehaviourTree->Update();
	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

void Plugin::SetBlackBoardAndBehaviourTree()
{
	auto pBlackBoard = new Blackboard;

	pBlackBoard->AddData("Interface", m_pInterface);

	pBlackBoard->AddData("AgentInfo", m_AgentInfo);

	pBlackBoard->AddData("HousesInFOV", GetHousesInFOV());
	pBlackBoard->AddData("EntitiesInFOV", GetEntitiesInFOV());
	pBlackBoard->AddData("WorldInfo", m_pInterface->World_GetInfo());
	pBlackBoard->AddData("Target", m_Target);
	pBlackBoard->AddData("GoingAfterItem", false);
	pBlackBoard->AddData("GoingAfterEnemy", false);

	pBlackBoard->AddData("Health", m_AgentInfo.Health);
	pBlackBoard->AddData("Energy", m_AgentInfo.Energy);
	pBlackBoard->AddData("InventoryCapacity", static_cast<int>(m_pInterface->Inventory_GetCapacity()));
	pBlackBoard->AddData("InventoryItems", m_InventoryItems);
	pBlackBoard->AddData("SlotToUse", 0);

	pBlackBoard->AddData("GunAmmo", 0);
	pBlackBoard->AddData("GunRange", 0);

	pBlackBoard->AddData("AutoOrientate", true);
	pBlackBoard->AddData("DesiredOrientation", 0.f);
	pBlackBoard->AddData("AngularSpeed", 0.f);

	pBlackBoard->AddData("HouseLocations", m_HouseLocations);
	pBlackBoard->AddData("ClosestHouse", Elite::Vector2(0, 0));
	pBlackBoard->AddData("IsGoingToHouse", false);

	pBlackBoard->AddData("CanRun", m_CanRun);
	pBlackBoard->AddData("IsEvadingEnemy", false);

	pBlackBoard->AddData("GunSlot", 0);

	pBlackBoard->AddData("RecentHouses", m_RecentHouseLocations);

	pBlackBoard->AddData("ClosestItem", EntityInfo());

	m_pBehaviourTree = new BehaviorTree(pBlackBoard,
		new BehaviorSelector({
		
			new BehaviorSelector({

				new BehaviorSequence({

					new BehaviorConditional(CheckHealth),
					new BehaviorAction(GetOrUseItem)
				}),

				new BehaviorSequence({

					new BehaviorConditional(CheckEnergy),
					new BehaviorAction(GetOrUseItem)
				}),
			}),

			new BehaviorSelector({
			
				new BehaviorSequence({
				
					new BehaviorConditional(IsEnemyInFOV),

					new BehaviorSelector({

						new BehaviorSequence({

							new BehaviorConditional(CheckGun),
							new BehaviorAction(ShootEnemy)
							}),

						new BehaviorSequence({
							new BehaviorConditional(IsNotInHouse),
							new BehaviorConditional(IsNotGoingToHouse),
							new BehaviorConditional(IsNotEvadingEnemy),
							new BehaviorAction(Evade),							
							}),
					}),
				}),

				new BehaviorSelector({
				
					new BehaviorSequence({

						new BehaviorConditional(IsItemInFOV),
						new BehaviorAction(GetClosestItem),
						new BehaviorAction(GrabItem)
					}),


					new BehaviorSequence({

						new BehaviorConditional(IsHouseInFOV),
						new BehaviorConditional(IsNotGoingToHouse),
						new BehaviorConditional(IsNotInHouse),
						new BehaviorConditional(IsNotCloseToCenterOfHouse),
						new BehaviorConditional(HasNotRecentlyEnteredHouse),
						new BehaviorConditional(IsNotEvadingEnemy),
						new BehaviorConditional(IsInventoryNotFull),
						new BehaviorAction(GoToHouse),
					}),
				}),
			}),

			new BehaviorSequence({

				new BehaviorConditional(IsNotGoingToHouse),
				new BehaviorAction(SetGoAfterCheckpoint)
			})
		})
	);
}

void Plugin::Seek(SteeringPlugin_Output& steering, Blackboard *pBlackBoard)
{
	auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(m_Target);

	steering.LinearVelocity = nextTargetPos - m_AgentInfo.Position; //Desired Velocity
	steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	steering.LinearVelocity *= m_AgentInfo.MaxLinearSpeed; //Rescale to Max Speed

	float angularSpeed;

	pBlackBoard->GetData("AngularSpeed", angularSpeed);

	steering.AngularVelocity = angularSpeed;
}
