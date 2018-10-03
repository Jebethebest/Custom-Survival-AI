#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "BehaviourTree/BehaviorTree.h"

class IBaseInterface;
class IExamInterface;

//enum class InventorySlots : int
//{
//	SLOT0 = 0,
//	SLOT1 = 1,
//	SLOT2 = 2,
//	SLOT3 = 3,
//	SLOT4 = 4
//};

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void ProcessEvents(const SDL_Event& e) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:

	void SetBlackBoardAndBehaviourTree();
	void Seek(SteeringPlugin_Output& steering, Blackboard *pBlackBoard);

	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;

	Elite::Vector2 m_Target = {};
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	bool m_DropItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose

	BehaviorTree *m_pBehaviourTree;

	AgentInfo m_AgentInfo;

	std::vector<bool> m_InventoryItems;

	std::vector<Elite::Vector2> m_HouseLocations;

	std::vector<HouseInfo> m_LastVisitedHouses;

	std::vector < Elite::Vector2> m_RecentHouseLocations;

	float m_InHouseTimer = 0.f;
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}