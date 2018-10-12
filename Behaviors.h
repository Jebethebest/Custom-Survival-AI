#pragma once
#include "../project/stdafx.h"
#include "Blackboard.h"
#include "BehaviorTree.h"


#pragma region Functions
inline EnemyInfo GetClosestEnemy(std::vector<EntityInfo> &entityVector, AgentInfo &agentInfo, IExamInterface *pInterface)
{
	std::vector<EnemyInfo> enemiesInFOV = {};

	//Loop and convert
	for each (EntityInfo entity in entityVector)
	{
		if (entity.Type == eEntityType::ENEMY)
		{
			//Get the enemyInfo
			EnemyInfo enemyInfo = {};
			pInterface->Enemy_GetInfo(entity, enemyInfo);

			//Add the enemy to the FOV enemy vector
			enemiesInFOV.push_back(enemyInfo);
		}
	}

	//GEt the first enemy
	EnemyInfo closestEnemy = enemiesInFOV[0];
	float closestEnemyDistance = Distance(closestEnemy.Location, agentInfo.Position);

	//If there are multiple enmies search for the closest one
	if (enemiesInFOV.size() > 1)
	{
		//Iterate over all enmies and compare them with the first one
		for each (EnemyInfo enemy in enemiesInFOV)
		{
			auto disToEnemy = Distance(enemy.Location, agentInfo.Position);

			if (disToEnemy < closestEnemyDistance)
			{
				closestEnemyDistance = disToEnemy;
				closestEnemy = enemy;
			}
		}
	}

	return closestEnemy;
}

inline EntityInfo GetClosetItem(std::vector<EntityInfo> &entityVector, AgentInfo &agentInfo, IExamInterface *pInterface)
{
	std::vector<EntityInfo> itemsInFOV = {};

	for (auto entity : entityVector)
	{
		if (entity.Type == eEntityType::ITEM)
		{
			itemsInFOV.push_back(entity);
		}
	}

	EntityInfo closestItem = itemsInFOV[0];

	float closestItemDistance = Elite::Distance(agentInfo.Position, closestItem.Location);

	if (itemsInFOV.size() > 1)
	{
		for (auto item : itemsInFOV)
		{
			auto distance = Elite::Distance(agentInfo.Position, item.Location);

			if (distance < closestItemDistance)
			{
				closestItemDistance = distance;
				closestItem = item;
			}
		}
	}

	return closestItem;
}

inline bool IsLocationInHouse(Elite::Vector2 &location, HouseInfo &houseInfo)
{
	auto center = houseInfo.Center;
	auto houseWidth = houseInfo.Size.x;
	auto houseHeight = houseInfo.Size.y;

	return location.x <= center.x + houseWidth 
			&& location.x >= center.x - houseWidth
			&& location.y <= center.y +	houseHeight 
			&& location.y >= center.y - houseHeight;
}
#pragma endregion

//*** GENERAL BEHAVIORS ***
inline BehaviorState GetOrUseItem(Blackboard* pBlackBoard)
{
	IExamInterface *pInterface;

	//ItemInfo itemInfo;

	int slotToUse;
	std::vector<bool> inventoryItems;

	auto data = pBlackBoard->GetData("Interface", pInterface)
				&& pBlackBoard->GetData("SlotToUse", slotToUse)
				&& pBlackBoard->GetData("InventoryItems", inventoryItems);

	if (!data)
		return Failure;

	if (inventoryItems[slotToUse])
	{
		pInterface->Inventory_UseItem(slotToUse);
		pInterface->Inventory_RemoveItem(slotToUse);
		cout << "Used item @ slot " << slotToUse << endl;
		inventoryItems[slotToUse] = false;
		pBlackBoard->ChangeData("InventoryItems", inventoryItems);
		return Success;
	}

	return Failure;
}

inline bool CheckHealth(Blackboard* pBlackBoard)
{
	IExamInterface *pInterface;

	float maxHealth;
	int inventoryCapacity;
	AgentInfo agentInfo;
	std::vector<bool> inventoryItems;

	auto data = pBlackBoard->GetData("Interface", pInterface)
				&& pBlackBoard->GetData("Health", maxHealth)
				&& pBlackBoard->GetData("AgentInfo", agentInfo)
				&& pBlackBoard->GetData("InventoryCapacity", inventoryCapacity)
				&& pBlackBoard->GetData("InventoryItems", inventoryItems);

	if (!data)
		return false;

	for (int i = 0; i < inventoryCapacity; ++i)
	{
		
		if (inventoryItems[i])
		{
			ItemInfo itemInfo;

			pInterface->Inventory_GetItem(i, itemInfo);
			if (itemInfo.Type == eItemType::MEDKIT)
			{
				int healthAmount = pInterface->Item_GetMetadata(itemInfo, "health");

				if (healthAmount + agentInfo.Health <= 10.0f)
				{
					pBlackBoard->ChangeData("SlotToUse", i);
					cout << "Set to false slot: " << i << endl;
					//cout << "Using medkit next!" << endl;
					return true;
				}
			}
		}
	}
	//cout << "No need to use medkit or no medkit to use!" << endl;

	return false;
}

inline bool CheckEnergy(Blackboard *pBlackBoard)
{
	IExamInterface *pInterface;

	float energy;
	AgentInfo agentInfo;
	std::vector<bool> inventoryItems;

	auto data = pBlackBoard->GetData("Interface", pInterface)
				&& pBlackBoard->GetData("AgentInfo", agentInfo)
				&& pBlackBoard->GetData("Energy", energy)
				&& pBlackBoard->GetData("InventoryItems", inventoryItems);

	if (!data)
		return false;

	for (int i = 0; i < inventoryItems.size(); ++i)
	{
		if (inventoryItems[i])
		{
			ItemInfo itemInfo;
			pInterface->Inventory_GetItem(i, itemInfo);

			if (itemInfo.Type == eItemType::FOOD)
			{
				int energyAmount = pInterface->Item_GetMetadata(itemInfo, "energy");

				if (energyAmount + agentInfo.Energy <= 10.f)
				{
					pBlackBoard->ChangeData("SlotToUse", i);
					//cout << "Used food!" << endl;
					return true;
				}
			}
		}
	}
	//cout << "No need to use food or no food to use!" << endl;

	return false;
}

inline bool CheckGun(Blackboard *pBlackBoard)
{
	IExamInterface *pInterface;
	std::vector<bool> inventoryItems;

	auto data = pBlackBoard->GetData("Interface", pInterface)
				&& pBlackBoard->GetData("InventoryItems", inventoryItems);

	if (!data)	
		return false;
	

	for (int i = 0; i < inventoryItems.size(); ++i)
	{
		if (inventoryItems[i])
		{
			ItemInfo itemInfo;
			pInterface->Inventory_GetItem(i, itemInfo);

			if (itemInfo.Type == eItemType::PISTOL)
			{
				pBlackBoard->ChangeData("GunSlot", i);
				cout << "I has a gun and I'm not afraid to use it!" << endl;
				return true;
			}
		}
	}

	cout << "No gun to use!" << endl;
	pBlackBoard->ChangeData("GoingAfterEnemy", false);
	return false;
}

inline bool IsItemInFOV(Blackboard * pBlackBoard)
{
	std::vector<EntityInfo> entityInfoVector;
	//bool alreadySpotted = false;
	bool goingAlreadyToItem;
	std::vector<bool>inventoryItems;
	std::vector<EntityInfo> itemsInFOV;

	auto data = pBlackBoard->GetData("EntitiesInFOV", entityInfoVector)
				&& pBlackBoard->GetData("GoingAfterItem", goingAlreadyToItem)
				&& pBlackBoard->GetData("InventoryItems", inventoryItems);

	if (!data)
		return false;


	for (bool item : inventoryItems)
	{
		if (!item)
		{
			for (auto& i : entityInfoVector)
			{
				if (i.Type == eEntityType::ITEM)
				{
					//pBlackBoard->ChangeData("Target", i.Location);
					//pBlackBoard->ChangeData("GoingAfterItem", true);
					pBlackBoard->ChangeData("AutoOrientate", true);
					//cout << "Going after item!" << endl;
					//return true;
					itemsInFOV.push_back(i);
				}
			}
			break;
		}
	}	

	//cout << "No items in FOV or inventory is full!" << endl;
	return !itemsInFOV.empty();
}

inline BehaviorState GetClosestItem(Blackboard *pBlackBoard)
{
	std::vector<EntityInfo> entitiesInFOV;

	AgentInfo agentInfo;

	IExamInterface *pInterface = nullptr;

	auto data = pBlackBoard->GetData("EntitiesInFOV", entitiesInFOV)
				&& pBlackBoard->GetData("Interface", pInterface)
				&& pBlackBoard->GetData("AgentInfo", agentInfo);

	auto closestItem = GetClosetItem(entitiesInFOV, agentInfo, pInterface);

	pBlackBoard->ChangeData("Target", closestItem.Location);
	pBlackBoard->ChangeData("ClosestItem", closestItem);

	return Success;
}

inline BehaviorState GrabItem(Blackboard *pBlackBoard)
{
	IExamInterface *pInterface;

	std::vector<EntityInfo> entityInfoVector;

	AgentInfo agentInfo;

	ItemInfo itemInfo;

	std::vector<bool> inventoryItems;

	int inventoryCapacity;

	EntityInfo closestItem;

	auto data = pBlackBoard->GetData("Interface", pInterface)
				&& pBlackBoard->GetData("EntitiesInFOV", entityInfoVector)
				&& pBlackBoard->GetData("AgentInfo", agentInfo)
				&& pBlackBoard->GetData("InventoryCapacity", inventoryCapacity)
				&& pBlackBoard->GetData("InventoryItems", inventoryItems)
				&& pBlackBoard->GetData("ClosestItem", closestItem);

	if (!data)
		return Failure;

	//for (size_t i = 0; i < entityInfoVector.size(); ++i)
	//{
	//	if (entityInfoVector[i].Type == eEntityType::ITEM)
	//	{
			auto distance = Elite::Distance(agentInfo.Position, closestItem.Location);

			int freeInventorySlot;

			if (distance <= agentInfo.GrabRange)
			{
				pInterface->Item_Grab(closestItem, itemInfo);

				if (itemInfo.Type == eItemType::PISTOL)
				{
					int nrOfPistolsInInventory = 0;
					for (int i = 0; i < inventoryCapacity; ++i)
					{
						if (inventoryItems[i])
						{
							ItemInfo item;
							pInterface->Inventory_GetItem(i, item);
							if (item.Type == eItemType::PISTOL)
							{
								++nrOfPistolsInInventory;
								if (nrOfPistolsInInventory == 3)
								{
									pInterface->Inventory_RemoveItem(i);
									inventoryItems[i] = false;
									pBlackBoard->ChangeData("InventoryItems", inventoryItems);
									break;
								}
							}
						}
					}
				}

				for (freeInventorySlot = 0; freeInventorySlot < inventoryCapacity; ++freeInventorySlot)
				{
					if (!inventoryItems[freeInventorySlot])
					{
						cout << "Free inventory slot: " << freeInventorySlot << endl;
						break;
					}

					if (freeInventorySlot == inventoryCapacity - 1)
					{
						cout << "No Free Invetory Slots" << endl;
						return Failure;
					}
				}

				inventoryItems[freeInventorySlot] = true;
				pBlackBoard->ChangeData("InventoryItems", inventoryItems);
				cout << "Added item @ inventory Slot" << freeInventorySlot << endl;

				pInterface->Inventory_AddItem(freeInventorySlot, itemInfo);

				if (itemInfo.Type == eItemType::GARBAGE)
				{
					pInterface->Inventory_RemoveItem(freeInventorySlot);
					inventoryItems[freeInventorySlot] = false;
					pBlackBoard->ChangeData("InventoryItems", inventoryItems);
				}
			}

			//cout << "Stopped going after item!" << endl;
			pBlackBoard->ChangeData("GoingAfterItem", false);
			return Success;
	//	}
	//}

	return Failure;
}

inline BehaviorState SetGoAfterCheckpoint(Blackboard *pBlackBoard)
{
	IExamInterface *pInterface;

	auto data = pBlackBoard->GetData("Interface", pInterface);

	if (!data)
		return Failure;

	Elite::Vector2 target = pInterface->NavMesh_GetClosestPathPoint(pInterface->World_GetCheckpointLocation());

	pBlackBoard->ChangeData("Target", target);
	pBlackBoard->ChangeData("AutoOrientate", true);
	pBlackBoard->ChangeData("IsEvadingEnemy", false);
	pBlackBoard->ChangeData("CanRun", false);

	//cout << "Set Target to checkpoint" << endl;
	return Success;
}

inline bool IsNotGoingAfterItem(Blackboard *pBlackBoard)
{
	bool isGoingAfterItem;

	Elite::Vector2 target;

	AgentInfo agentInfo;

	auto data = pBlackBoard->GetData("GoingAfterItem", isGoingAfterItem)
				&& pBlackBoard->GetData("Target", target)
				&& pBlackBoard->GetData("AgentInfo", agentInfo);

	if (!data)
		return false;

	if (Elite::Distance(agentInfo.Position, target) <= agentInfo.GrabRange)
	{
		return true;
	}

	//cout << isGoingAfterItem << endl;
	return false;
}

inline bool IsNotGoingAfterEnemy(Blackboard *pBlackBoard)
{
	bool isGoingAfterEnemy;

	pBlackBoard->GetData("GoingAfterEnemy", isGoingAfterEnemy);

	return !isGoingAfterEnemy;
}

inline bool IsEnemyInFOV(Blackboard *pBlackBoard)
{

	std::vector<EntityInfo> entityInfoVector;

	auto data = pBlackBoard->GetData("EntitiesInFOV", entityInfoVector);

	//pBlackBoard->ChangeData("IsEvadingEnemy", false);


	if (!data)
		return false;

	for (auto& i : entityInfoVector)
	{
		if (i.Type == eEntityType::ENEMY)
		{
			return true;
		}
	}

	//pBlackBoard->ChangeData("IsEvadingEnemy", false);
	return false;
}

inline BehaviorState ShootEnemy(Blackboard *pBlackBoard)
{
	IExamInterface *pInterface;

	std::vector<EntityInfo> entityInfoVector;

	AgentInfo agentInfo;

	std::vector<bool> inventoryItems;

	int gunSlot = 0;

	auto data = pBlackBoard->GetData("Interface", pInterface)
				&& pBlackBoard->GetData("EntitiesInFOV", entityInfoVector)
				&& pBlackBoard->GetData("AgentInfo", agentInfo)
				&& pBlackBoard->GetData("InventoryItems", inventoryItems)
				&& pBlackBoard->GetData("GunSlot", gunSlot);	

	if (!data)
		return Failure;

	auto closestEnemy = GetClosestEnemy(entityInfoVector, agentInfo, pInterface);

	Elite::Vector2 rotationVec = { 0, -1 };

	float agentOrientation = agentInfo.Orientation;

	rotationVec = {	cos(agentOrientation) * rotationVec.x - sin(agentOrientation) * rotationVec.y, 
					sin(agentOrientation) * rotationVec.x + cos(agentOrientation) * rotationVec.y };

	auto vecToEnemy = closestEnemy.Location - agentInfo.Position;

	rotationVec.Normalize();
	vecToEnemy.Normalize();

	auto angle = atan2(rotationVec.x * vecToEnemy.y - rotationVec.y * vecToEnemy.x, rotationVec.x * vecToEnemy.x + rotationVec.y * vecToEnemy.y);

	cout << angle << endl;

	pBlackBoard->ChangeData("AutoOrientate", false);

	if (angle < 0.f)
		pBlackBoard->ChangeData("AngularSpeed", -agentInfo.MaxAngularSpeed);	

	else	
		pBlackBoard->ChangeData("AngularSpeed", agentInfo.MaxAngularSpeed);

	float accepteableAngle = 2.f;

	if (abs(angle) < M_PI / 180 * accepteableAngle)
	{
		cout << "PANG PANG" << endl;

		pInterface->Inventory_UseItem(gunSlot);
		
		ItemInfo itemInfo;

		pInterface->Inventory_GetItem(gunSlot, itemInfo);

		int ammo = pInterface->Item_GetMetadata(itemInfo, "ammo");

		if (ammo <= 0)
		{
			pInterface->Inventory_RemoveItem(gunSlot);
			inventoryItems[gunSlot] = false;
			pBlackBoard->ChangeData("InventoryItems", inventoryItems);
		}

		return Success;
	}

	return Success;
}

inline bool IsNotInHouse(Blackboard *pBlackBoard)
{	
	AgentInfo agentInfo;

	auto data = pBlackBoard->GetData("AgentInfo", agentInfo);

	if (!data)
		return false;

	return !agentInfo.IsInHouse;
}

inline BehaviorState ScanHouse(Blackboard *pBlackBoard)
{
	AgentInfo agentInfo;

	auto data = pBlackBoard->GetData("AgentInfo", agentInfo);

	if (!data)
		return Failure;



	return Failure;
}

inline BehaviorState Evade(Blackboard *pBlackBoard)
{
	IExamInterface *pInterface = nullptr;

	AgentInfo agentInfo;

	std::vector<EntityInfo> entityInfoVector;

	auto data = pBlackBoard->GetData("AgentInfo", agentInfo)
				&& pBlackBoard->GetData("EntitiesInFOV", entityInfoVector)
				&& pBlackBoard->GetData("Interface", pInterface);

	if (!data)
		return Failure;

	pBlackBoard->ChangeData("AutoOrientate", true);

	auto closestEnemy = GetClosestEnemy(entityInfoVector, agentInfo, pInterface);

	auto forward = agentInfo.LinearVelocity.GetNormalized();
	auto directionToEnemy = (closestEnemy.Location - agentInfo.Position).GetNormalized();

	auto minEvadAngleDegrees = 30.f;
	auto minEvadeAngle = M_PI / 180 * minEvadAngleDegrees;

	auto angle = atan2(forward.x * directionToEnemy.y - forward.y * directionToEnemy.x, forward.x * directionToEnemy.x + forward.y * directionToEnemy.y);

	angle += angle > 0 ? minEvadeAngle : -minEvadeAngle;

	if (abs(angle) > agentInfo.FOV_Angle)
	{
		if (angle < 0)
		{
			angle = agentInfo.FOV_Angle;
			angle *= -1;
		}
		else
		{
			angle = agentInfo.FOV_Angle;
		}
	}

	angle *= -1;

	auto minRadius = agentInfo.FOV_Range;

	Elite::Vector2 newTarget = { agentInfo.Position.x + (minRadius * forward.x * cos(angle) - minRadius * forward.y * sin(angle)),
		agentInfo.Position.y + (minRadius * forward.x * sin(angle) + minRadius * forward.y * cos(angle)) };

	pBlackBoard->ChangeData("Target", newTarget);
	pBlackBoard->ChangeData("IsEvadingEnemy", true);
	pBlackBoard->ChangeData("CanRun", true);
	cout << "Evading" << endl;

	return Success;
}

inline bool IsHouseClose(Blackboard *pBlackBoard)
{
	std::vector<Elite::Vector2> houseLocations;

	AgentInfo agentInfo;

	auto data = pBlackBoard->GetData("HouseLocations", houseLocations)
				&& pBlackBoard->GetData("AgentInfo", agentInfo);

	if (!data)
		return false;

	if (!houseLocations.empty())
	{
		Elite::Vector2 closestHouse = houseLocations[0];
		float closestHouseDistance = 20.f;
		for (auto& location : houseLocations)
		{
			float distance = (location - agentInfo.Position).Magnitude();
			if (distance < closestHouseDistance)
			{
				closestHouse = location;
				closestHouseDistance = distance;
			}
		}
		pBlackBoard->ChangeData("ClosestHouse", closestHouse);

		return true;
	}

	return false;
}

inline BehaviorState GoToHouse(Blackboard *pBlackBoard)
{
	vector<HouseInfo> houses;

	auto data = pBlackBoard->GetData("HousesInFOV", houses);

	if (!data)
		return Failure;
		
	pBlackBoard->ChangeData("Target", houses[0].Center);
	pBlackBoard->ChangeData("IsGoingToHouse", true);
	pBlackBoard->ChangeData("CanRun", false);
	pBlackBoard->ChangeData("IsEvadingEnemy", false);

	cout << "Going to house!" << endl;

	return Success;
}

inline bool IsNotGoingToHouse(Blackboard *pBlackBoard)
{
	bool isGoingToHouse;

	auto data = pBlackBoard->GetData("IsGoingToHouse", isGoingToHouse);

	if (!data)
		return true;

	return !isGoingToHouse;
}

inline bool IsHouseInFOV(Blackboard *pBlackBoard)
{
	vector<HouseInfo> houses;

	auto data = pBlackBoard->GetData("HousesInFOV", houses);

	if (!data)
		return false;


	return !houses.empty();
}

inline bool IsNotCloseToCenterOfHouse(Blackboard *pBlackBoard)
{
	AgentInfo agentInfo;

	Elite::Vector2 target;

	auto data = pBlackBoard->GetData("Target", target)
		&& pBlackBoard->GetData("AgentInfo", agentInfo);

	if (!data)
		return false;

	auto distance = Elite::Distance(agentInfo.Position, target);

	return distance >= 1.f;
}

inline bool IsNotEvadingEnemy(Blackboard *pBlackBoard)
{	
	bool isEvading;
	pBlackBoard->GetData("IsEvadingEnemy", isEvading);

	return !isEvading;
}

inline bool HasNotRecentlyEnteredHouse(Blackboard *pBlackBoard)
{
	std::vector<Elite::Vector2> recentHouses;

	std::vector<HouseInfo> housesInFOV;

	auto data = pBlackBoard->GetData("RecentHouses", recentHouses)
				&& pBlackBoard->GetData("HousesInFOV", housesInFOV);


	for (HouseInfo house : housesInFOV)
	{
		auto it = std::find(recentHouses.begin(), recentHouses.end(), house.Center);

		if (it != recentHouses.end())
		{
			return false;
		}
	}

	return true;
}

inline bool IsInventoryNotFull(Blackboard *pBlackBoard)
{
	std::vector<bool> inventoryItems;

	int nrOfItemsInInventory = 0;

	auto data = pBlackBoard->GetData("InventoryItems", inventoryItems);

	for (bool hasItem : inventoryItems)
	{
		if (hasItem)
		{
			++nrOfItemsInInventory;
		}
	}

	return nrOfItemsInInventory != inventoryItems.size();
}