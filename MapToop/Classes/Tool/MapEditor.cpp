#include "MapEditor.h"
#include "Tool/StringHelper.h"
#include "Entity/EffectSprite.h"
#include "Entity/Effect.h"
#include "Entity/Block.h"
#include "Entity/Slots.h"
#include "Controller/Controller_Point.h"
#include <fstream>
#define __MapPngName "Map/%s.csb"
#define __MapDataName "Map/Data%s.txt"
#define __splitEAndB "+_)(*&^%$#@!<>?|"
#define __BlockPosSave "%d,"
#define __ElementSave "%d,%d,%d#"
#define __ElementDataOneLineNum 10
#define __BlockSize 64
#define __OpenFunction_1 true//false // 移动到的块会有动画效果功能
#define __OpenFunction_2 true// 按F2隐藏块层
#define __OpenFunction_3 true//按F3隐藏普通块
#define __OpenFunction_4 true// 地图选择功能
#define __TestAStarFunction true //开启测试A星寻路功能
#include "Entity/Astar.h"
//===[
OhterElement::OhterElement(OhterElement * o_e) 
{
	e_id = o_e->e_id;
	e_pos.setX(o_e->e_pos.getX());
	e_pos.setY(o_e->e_pos.getY());
}
OhterElement::OhterElement()
{
	e_pos.setX(0);
	e_pos.setY(0);
	e_id = 0;
}

OhterElement::~OhterElement()
{
}

//===]
MapEditor::MapEditor():
m_CurMapId(0), m_BlockSize(__BlockSize, __BlockSize), m_MapSize(0, 0), m_MapInt(0, 0),
m_CurPointBlock(nullptr), m_CurMapName("")
{
	m_BlockData.clear();
	m_Element.clear();
	m_ConPoint = nullptr;
	m_CurPoint = Vec2(0, 0);
	m_LastPoint = Vec2(0, 0);
	m_IsMove = false;
	m_IsPoint = false;
	m_PosIsVisi = false;
	m_CurScale = 1.0;
	m_IsBlockVisi = true;
	m_IsCanPoint = true;
}

MapEditor::~MapEditor()
{
	delete m_AStar;
}

bool MapEditor::init(){
	if (!Layout::init())
	{
		return false;
	}
	m_RootWidget = (Layout *)CSLoader::createNode("Interface_MapTool.csb");
	this->addChild(m_RootWidget);
	InitWidget();
	InitEvent();
	ShowChoiceMap();
	return true;
}

void MapEditor::ShowChoiceMap(){
	if (m_CurPointBlock != nullptr)
	{
		m_CurPointBlock->IsPoint(false);
	}
	m_CurPointBlock = nullptr;
	m_IsCanPoint = false;
	m_LayoutMapChoice->setVisible(true);
	m_LayoutChoice->setVisible(false);
	m_ListMaps->removeAllItems();
	InitCanEditorMapChoice();
}
void MapEditor::CloseChoiceMap()
{
	m_LayoutMapChoice->setVisible(false);
	m_LayoutChoice->setVisible(true);
	m_ListMaps->removeAllItems();
}
void MapEditor::InitCanEditorMapChoice(){
	int index_count = 1;
	Size lay_size = Size(m_ListMaps->getContentSize().width,200);
	vector<string> allFile;
	StringHelper::getAllFileInPathWithType("./Map", allFile, "csb");
	for (auto & mapNa : allFile)
	{
		Layout * map_root = (Layout *)CSLoader::createNode(StringUtils::format("map/%s", mapNa.c_str()));
		if (map_root == nullptr) continue;
		Layout * lay = Layout::create();
		lay->setContentSize(lay_size);
		map_root->setScaleX(1 / (map_root->getContentSize().width / lay_size.width));
		map_root->setScaleY(1 / (map_root->getContentSize().height / lay_size.height));
		

		Button * btn = Button::create("Map/ImageFile.png");
		btn->setName(StringHelper::deleteStrTypeBack(mapNa,"."));
		btn->setContentSize(lay_size);
		btn->setScale9Enabled(true);
		btn->setPosition(btn->getContentSize() / 2);
		btn->setOpacity(0);
		lay->addChild(map_root, 4);
		btn->addTouchEventListener(CC_CALLBACK_2(MapEditor::ChoiceMapKindButtonEvent,this));
		btn->setTag(index_count);
		lay->addChild(btn,5);
		m_ListMaps->pushBackCustomItem(lay);
		index_count++;
	}
}
void MapEditor::InitEditorMap()
{
	Widget * mapBack = (Layout *)CSLoader::createNode(StringUtils::format(__MapPngName,m_CurMapName.c_str()));
	if (mapBack != nullptr)
	{
		m_LayoutBack->addChild(mapBack);
		m_MapSize = mapBack->getContentSize();
		m_MapInt.setX((int)ceil(m_MapSize.width / m_BlockSize.width));
		m_MapInt.setY((int)ceil(m_MapSize.height / m_BlockSize.height));
	}
}

void MapEditor::InitMapBlock()
{
	m_LayoutBlockPoint->removeAllChildren();
	m_Blocks.clear();
	m_ASTARData.clear();
	for (int i = 0; i < m_BlockData.size(); i++)
	{
		for (int j = 0; j < m_BlockData.at(i).size(); j++)
		{
			auto block = Block::CreateWithInfo(Size(64, 64), (BlockType)(m_BlockData.at(i).at(j)));
			m_LayoutBlockPoint->addChild(block);
			block->SetPos(PosInt(j, i));
			vector<int> types;
			types.clear();
			types.push_back(m_BlockData.at(i).at(j));
			if (m_Blocks.find(j) == m_Blocks.end())
			{
				map<int, Block *> temp;
				temp.clear();
				temp.insert(make_pair(i, block));
				m_Blocks.insert(make_pair(j, temp));
				map<int, vector<int>> Atemp;
				Atemp.insert(make_pair(i, types));
				m_ASTARData.insert(make_pair(j, Atemp));
			}
			else
			{
				m_Blocks.at(j).insert(make_pair(i, block));
				m_ASTARData.at(j).insert(make_pair(i, types));
			}
		}
	}
}
void MapEditor::InitMapOhterElement()
{

}

void MapEditor::LoadMapData()
{
	m_BlockData.clear();
	m_Element.clear();
	vector<std::string> vect_string;
	string file_path = StringUtils::format(__MapDataName, m_CurMapName.c_str());
	ifstream OpenFile(file_path);
	if (OpenFile.fail())
	{
		CCLOG("[LoadMapData]:open [%s] is error", file_path.c_str());
		CCLOG("[LoadMapData]:create new [%s] ", file_path.c_str());
		for (int index_y = 0; index_y < m_MapInt.getY(); index_y++)
		{
			map<int, int> temp;
			for (int index_x = 0; index_x < m_MapInt.getX(); index_x++)
			{
				temp.insert(std::make_pair(index_x, 0));
			}
			m_BlockData.insert(std::make_pair(index_y, temp));
		}
		SaveMapData();
		return;
	}
	else{
		CCLOG("[LoadMapData]:open [%s] is success", file_path.c_str());
	}
	char buf[1000];
	do{
		OpenFile.getline(buf, 1000);
		string FileString;
		FileString.append(buf);
		vect_string.push_back(FileString);
		if (OpenFile.eof())
			break;
	} while (!OpenFile.eof());
	//---
	bool isBlock = true;
	int block_y = 0;
	for (auto data : vect_string)
	{
		if (strcmp(__splitEAndB, data.c_str()) == 0){ isBlock = false; continue;}
		if (isBlock)
		{
			vector<string> block_str = StringHelper::splitString(data, ",");
			map<int, int> temp;
			for (size_t i = 0; i < block_str.size(); i++)
			{
				temp.insert(std::make_pair(i, atoi(block_str[i].c_str())));
			}
			m_BlockData.insert(std::make_pair(block_y, temp));
			block_y++;
		}
		else{
			vector<string> ele_str = StringHelper::splitString(data, "#");
			for (auto ele_data_ste : ele_str)
			{
				vector<string> ele_data = StringHelper::splitString(data, ",");
				OhterElement o_e;
				o_e.e_id = (int)atoi(ele_data.at(2).c_str());
				o_e.e_pos.setX((int)atoi(ele_data.at(0).c_str()));
				o_e.e_pos.setY((int)atoi(ele_data.at(1).c_str()));
				m_Element.push_back(OhterElement(&o_e));
			}
		}
	}
}
void MapEditor::SaveMapData()
{
	string file_path = StringUtils::format(__MapDataName, m_CurMapName.c_str());
	ofstream myfile(file_path, ios::out);
	if (!myfile)
	{
		CCLOG("[SaveMapData]:save [%s] is error", file_path.c_str());
	}
	else
	{
		for (auto & block_x : m_BlockData)
		{
			string one_line_data = "";
			for (auto & block_y : block_x.second)
			{
				int typeNum = 0;
				if (m_Blocks.find(block_x.first) != m_Blocks.end())
				{
					if (m_Blocks.at(block_x.first).find(block_y.first) != m_Blocks.at(block_x.first).end())
					{
						typeNum = m_Blocks.at(block_x.first).at(block_y.first)->m_Type;
					}
					
				}
				one_line_data.append(StringUtils::format(__BlockPosSave,typeNum));
			}
			myfile << one_line_data << endl;
		}
		myfile << __splitEAndB << endl;
		int count_line = 0;
		string one_line_data = "";
		for (auto & ele : m_Element)
		{
			one_line_data.append(StringUtils::format(__ElementSave, ele.e_pos.getX(), ele.e_pos.getY(), ele.e_id));
			if (count_line >= __ElementDataOneLineNum)
			{
				myfile << one_line_data << endl;
				one_line_data = "";
			}
		}
		if (one_line_data.empty() == false) myfile << one_line_data << endl;

		myfile.close();
		CCLOG("[SaveMapData]:save [%s] is success", file_path.c_str());
	}
}
void MapEditor::RefreshEditorMap()
{
	m_LayoutBack->removeAllChildren();
	m_LayoutBack->setPosition(Vec2(0,0));
	m_LayoutBlock->removeAllChildren();
	m_LayoutBlock->setPosition(Vec2(0, 0));
	m_LayoutBlockPoint->removeAllChildren();
	m_LayoutBlockPoint->setPosition(Vec2(0, 0));
	InitEditorMap();
	LoadMapData();
	InitMapBlock();
	m_AStar->SetMapData(m_ASTARData);
}
void MapEditor::SetScaleMap(float sc)
{
	m_LayoutBack->setScale(sc);
	m_LayoutBlockPoint->setScale(sc);
	m_LayoutBlock->setScale(sc);;
}
void MapEditor::InitWidget()
{
	__GetChildAndAssignmentWithOB(m_LayoutBack, m_RootWidget, "Panel_Background", Layout);
	__GetChildAndAssignmentWithOB(m_LayoutBlock, m_RootWidget, "Panel_Block", Layout);
	__GetChildAndAssignmentWithOB(m_LayoutBType, m_RootWidget, "Panel_BlockType", Layout);
	__GetChildAndAssignmentWithOB(m_LayoutBlockPoint, m_RootWidget, "Panel_BlockPoint", Layout);
	__GetChildAndAssignmentWithOB(m_LayoutChoice, m_RootWidget, "Panel_Button", Layout);
	__GetChildAndAssignmentWithOB(m_LayoutMapChoice, m_RootWidget, "Panel_MapChoice", Layout);
	__GetChildAndAssignmentWithOB(m_ListMaps, m_LayoutMapChoice, "ListView_Map", ListView);
	__GetChildAndAssignmentWithOB(m_ButtonChoice, m_RootWidget, "Button_MapChoice", Button)


// 	m_LayoutBType->setVisible(false);
// 	m_ButtonChoice->setVisible(true);
// 	auto e_sp2 = EffectSprite::create("1.png");
// 	auto e_sp = EffectSprite::create("WaterShader/water.png");
//  auto e_sp = EffectSprite::create("icon.png");
// 	e_sp->setPosition(__VisibleSize / 2);
// 	e_sp2->setPosition(__VisibleSize / 2);
// 	m_LayoutBlockPoint->addChild(e_sp);
// 	e_sp->setEffect(EffectWater::create());
// 	e_sp2->setEffect(EffectOutline::create());
// 	m_LayoutBlockPoint->addChild(e_sp2,999);
	m_AStar = new AStar();
}
void MapEditor::InitEvent(){
	__AddTouchEventWithNameAndPNode("Button_MapChoice", m_LayoutChoice, MapEditor::ChoiceMapButtonEvent);
	__AddTouchEventWithNameAndPNode("Button_Close", m_LayoutMapChoice, MapEditor::CloseMapChoiceButtonEvent);
	__AddTouchEventWithNameAndPNode("Button_EdietS", m_LayoutChoice, MapEditor::EditButtonEvent);
	__AddTouchEventWithNameAndPNode("Button_Save", m_LayoutChoice, MapEditor::SaveButtonEvent);
	__AddTouchEventWithNameAndPNode("Button_Close", m_LayoutBType, MapEditor::CloseEditButtonEvent);
	__AddTouchEventWithNameAndPNode("Button_Close", m_LayoutMapChoice, MapEditor::CloseMapChoiceButtonEvent);
	for (int i = 1; i <= 3; i++)
	{
		__AddTouchEventWithNameAndPNode(StringUtils::format("Button_Type%d", i), m_LayoutBType, MapEditor::TypeButtonEvent);
	}
	m_LayoutBType->setVisible(false);
	m_LayoutMapChoice->setVisible(false);
	m_ConPoint = Controller_Point::create();
	m_ConPoint->OpenTouch();
	m_ConPoint->RegisterCallback(CallFunc_ButtonTypeAndPoint_SELECTOR(MapEditor::TouchEvent), this);
	this->addChild(m_ConPoint, 99);


	// 创建鼠标监听
	auto listener = EventListenerMouse::create();
	//分发MouseMove事件
	listener->onMouseMove = CC_CALLBACK_1(MapEditor::MouseMove, this);
	listener->onMouseScroll = CC_CALLBACK_1(MapEditor::MouseScroll, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
	//注册捕捉监听
	auto listenerkeyPad = EventListenerKeyboard::create();
	listenerkeyPad->onKeyReleased = CC_CALLBACK_2(MapEditor::OnKeyReleased, this);
	listenerkeyPad->onKeyPressed = CC_CALLBACK_2(MapEditor::OnKeyPressed, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listenerkeyPad, this);
}

void MapEditor::PointBlock()
{

	Block * get_block = GetCurPointBlock();
	if (m_CurPointBlock != get_block)
	{
		if (nullptr != m_CurPointBlock)
		{
			m_CurPointBlock->IsPoint(false);
			//m_CurPointBlock->stopAllActions();
			//m_CurPointBlock->setScale(1);
			m_CurPointBlock = nullptr;
		}

		if (nullptr != get_block)
		{
			m_CurPointBlock = get_block;
			m_CurPointBlock->IsPoint(true);
			//float s_time = 0.4;
			//m_CurPointBlock = get_block;
			//m_CurPointBlock->setScale(0.8);
			//auto seq = Sequence::create(ScaleTo::create(s_time, 0.8), ScaleTo::create(s_time, 0.5), NULL);
			//m_CurPointBlock->runAction(RepeatForever::create(seq));
		}
	}


}


Block * MapEditor::GetCurPointBlock()
{
	Block * point_block = nullptr;
	auto move_point = m_LayoutBack->getPosition();
	int wdith_num = (int)(m_CurMousePoint.x - move_point.x) / (__BlockSize*m_CurScale);
	int height_num = (int)(m_CurMousePoint.y - move_point.y) / (__BlockSize*m_CurScale);
	if (m_Blocks.find(wdith_num) != m_Blocks.end())
	{
		if (m_Blocks.at(wdith_num).find(height_num) != m_Blocks.at(wdith_num).end())
		{
			point_block = m_Blocks.at(wdith_num).at(height_num);
		}

	}
	return point_block;
}

void  MapEditor::EditButtonEvent(Ref * pSender, ui::Widget::TouchEventType eventType)
{
	if (eventType == ui::Widget::TouchEventType::ENDED)
	{
		m_LayoutBType->setVisible(true);
		m_LayoutChoice->setVisible(false);
		m_CurType = 0;
		m_IsEditModel = true;
	}
}

void MapEditor::ChoiceMapButtonEvent(Ref * pSender, ui::Widget::TouchEventType eventType)
{
	if (eventType == ui::Widget::TouchEventType::ENDED)
	{
		ShowChoiceMap();
	}
}
void MapEditor::ChoiceMapKindButtonEvent(Ref * pSender, ui::Widget::TouchEventType eventType)
{
	if (eventType == ui::Widget::TouchEventType::ENDED)
	{
		Button * btn = (Button *)pSender;
		m_CurMapName = btn->getName();
		CloseChoiceMap();
		RefreshEditorMap();
		m_IsCanPoint = true;
	}
}

void MapEditor::CloseMapChoiceButtonEvent(Ref * pSender, ui::Widget::TouchEventType eventType)
{
	if (eventType == ui::Widget::TouchEventType::ENDED)
	{
		CloseChoiceMap();
	}
}
void  MapEditor::SaveButtonEvent(Ref * pSender, ui::Widget::TouchEventType eventType)
{
	if (eventType == ui::Widget::TouchEventType::ENDED)
	{
		SaveMapData();
	}
}
void  MapEditor::TypeButtonEvent(Ref * pSender, ui::Widget::TouchEventType eventType)
{
	if (eventType == ui::Widget::TouchEventType::ENDED)
	{
		auto btn = (Button *)pSender;
		m_CurType = btn->getTag();
		m_IsEditModel = true;
	}
	else if (eventType == ui::Widget::TouchEventType::BEGAN)
	{
		m_IsEditModel = false;
	}
	else if (eventType == ui::Widget::TouchEventType::CANCELED)
	{
		m_IsEditModel = true;
	}
}
void  MapEditor::CloseEditButtonEvent(Ref * pSender, ui::Widget::TouchEventType eventType)
{
	if (eventType == ui::Widget::TouchEventType::ENDED)
	{
		m_LayoutBType->setVisible(false);
		m_LayoutChoice->setVisible(true);

	}
	else if (eventType == ui::Widget::TouchEventType::BEGAN)
	{
		m_IsEditModel = false;
	}
	else if (eventType == ui::Widget::TouchEventType::CANCELED)
	{
		m_IsEditModel = true;
	}
}
//=========
void MapEditor::MouseMove(Event * evnet)
{
	//CCLog("mouse  move!!!!!!!!!!!!!!!!!!!!!!");
	EventMouse* e = (EventMouse*)evnet;
	//CCLOG("%0.2f %0.2f",e->getCursorX() ,e->getCursorY());//
	float x = e->getCursorX();
	float y = e->getCursorY();
	m_CurMousePoint.x = x;
	m_CurMousePoint.y = y;
	if (m_IsBoolPressed == true && m_IsEditModel == true)
	{
		auto move_point = m_LayoutBack->getPosition();
		int wdith_num = (int)(m_CurMousePoint.x - move_point.x) / (__BlockSize*m_CurScale);
		int height_num = (int)(m_CurMousePoint.y - move_point.y) / (__BlockSize*m_CurScale);
		if (m_Blocks.find(wdith_num) != m_Blocks.end())
		{
			if (m_Blocks.at(wdith_num).find(height_num) != m_Blocks.at(wdith_num).end())
			{
				auto block = m_Blocks.at(wdith_num).at(height_num);
				block->SetBlockType(m_CurType);
			}
		}
	}
	if (__OpenFunction_1 && m_IsCanPoint)
	{
		PointBlock();
	}

}

void MapEditor::MouseScroll(Event * evnet)
{
	EventMouse* e = (EventMouse*)evnet;
	CCLOG("%0.2f %0.2f", e->getScrollX(), e->getScrollY());//
	m_CurScale -= (e->getScrollY() / 20.0);
	bool is_c = true;
	if (m_CurScale >= 2)
	{
		is_c = false;
		m_CurScale = 2;
	}

	if (m_CurScale <= 0.3)
	{
		is_c = false;
		m_CurScale = 0.3;
	}
	float o_s = m_LayoutBack->getScale();
	m_LayoutBack->setScale(m_CurScale);
	m_LayoutBlockPoint->setScale(m_CurScale);
	if (is_c)
	{
		m_LayoutBack->setPosition(m_LayoutBack->getPosition() + m_LayoutBack->getContentSize() * (o_s - m_CurScale));
		m_LayoutBlockPoint->setPosition(m_LayoutBlockPoint->getPosition() + m_LayoutBack->getContentSize() * (o_s - m_CurScale));
	}
}

//捕捉监听
void MapEditor::OnKeyReleased(EventKeyboard::KeyCode keycode, cocos2d::Event *evnet)
{
	if (keycode == EventKeyboard::KeyCode::KEY_CTRL && m_IsEditModel == true)  //返回
	{
		CCLOG("%d", (int)keycode);
		auto move_point = m_LayoutBack->getPosition();
		int wdith_num = (int)(m_CurMousePoint.x - move_point.x) / (__BlockSize*m_CurScale);
		int height_num = (int)(m_CurMousePoint.y - move_point.y) / (__BlockSize*m_CurScale);
		if (m_Blocks.find(wdith_num) != m_Blocks.end())
		{
			if (m_Blocks.at(wdith_num).find(height_num) != m_Blocks.at(wdith_num).end())
			{
				auto block = m_Blocks.at(wdith_num).at(height_num);
				block->SetBlockType(m_CurType);
			}
		}
	}


	if (keycode == EventKeyboard::KeyCode::KEY_CTRL)  //返回
	{
		m_IsBoolPressed = false;
	}
	//--

}
void MapEditor::OnKeyPressed(EventKeyboard::KeyCode keycode, cocos2d::Event *evnet)
{
	if (keycode == EventKeyboard::KeyCode::KEY_CTRL && m_IsEditModel == true)
	{
		m_IsBoolPressed = true;
		CCLOG("[MapEditor::OnKeyPressed]");
		
	}
	
	if (keycode == EventKeyboard::KeyCode::KEY_ESCAPE)
	{
		SaveMapData();
		cocos2d::Director::getInstance()->end();
	}

	if (keycode == EventKeyboard::KeyCode::KEY_F1)
	{
		bool _is = false;
		if (m_PosIsVisi == false)
		{
			_is = true;
		}
		m_PosIsVisi = _is;
		for (auto & ite : m_Blocks)
		{
			for (auto & ite2 : ite.second)
			{
				ite2.second->SetPosVisi(m_PosIsVisi);
			}
		}
	}

	if (keycode == EventKeyboard::KeyCode::KEY_F2 && __OpenFunction_2)
	{
		bool _is = false;
		if (m_IsBlockVisi == false)
		{
			_is = true;
		}
		m_IsBlockVisi = _is;
		m_LayoutBlockPoint->setVisible(m_IsBlockVisi);
	}

	if (keycode == EventKeyboard::KeyCode::KEY_F3 && __OpenFunction_3)
	{
		bool _is = false;
		if (m_IsComBlockVisi == false)
		{
			_is = true;
		}
		m_IsComBlockVisi = _is;
		for (auto & ite : m_Blocks)
		{
			for (auto & ite2 : ite.second)
			{
				if (ite2.second->m_Type == 0 || true)
				{
					ite2.second->SetComVisi(m_IsComBlockVisi);
				}
			}
		}
	}

	if (keycode == EventKeyboard::KeyCode::KEY_S)
	{
		SaveMapData();
		CCLOG("Save !!!!!!!!!");
	}

}

void MapEditor::TouchEvent(ui::Widget::TouchEventType touch_type, Vec2 touch_point)
{
	if (/*m_IsEditModel == */true)
	{
		switch (touch_type)
		{
		case ui::Widget::TouchEventType::BEGAN:
			//CCLOG("[ControlTouchType_Began ] TPOINT[%0.2f : %0.2f]", touch_point.x, touch_point.y);
			m_CurPoint = touch_point;
			if (__TestAStarFunction && m_AStar != nullptr)
			{
				auto move_point = m_LayoutBack->getPosition();
				int wdith_num = (int)(m_CurMousePoint.x - move_point.x) / (__BlockSize*m_CurScale);
				int height_num = (int)(m_CurMousePoint.y - move_point.y) / (__BlockSize*m_CurScale);
				clock_t start, finish;
				start = clock();
				std::vector<int> temp;
				temp.push_back(1);
				auto node = m_AStar->FindPath(wdith_num, height_num, 0, 0, temp);
				finish = clock();
				CCLOG("TIME : %d", (finish - start));
				m_AStar->PrintPath();
				auto APth = m_AStar->GetPath();
				if (node == false)
				{
				}
				else{
				}
			}
			break;
		case ui::Widget::TouchEventType::MOVED:
			//CCLOG("[ControlTouchType_Moved ] TPOINT[%0.2f : %0.2f]", touch_point.x, touch_point.y);
			m_LastPoint = m_CurPoint;
			m_CurPoint = touch_point;
			m_IsMove = true;
			m_LayoutBack->setPosition(m_LayoutBack->getPosition() + m_CurPoint - m_LastPoint);
			m_LayoutBlockPoint->setPosition(m_LayoutBlockPoint->getPosition() + m_CurPoint - m_LastPoint);
			break;
		case ui::Widget::TouchEventType::ENDED:
		{
			//CCLOG("[ControlTouchType_Ended ] TPOINT[%0.2f : %0.2f]", touch_point.x, touch_point.y);
			m_LastPoint = m_CurPoint;
			m_CurPoint = touch_point;
			auto move_point = m_LayoutBack->getPosition();
			if (m_IsMove == false)
			{
				m_IsPoint = true;
				/*int wdith_num = (int)(m_curmousepoint.x - move_point.x) / __blocksize;
				int height_num = (int)(m_curmousepoint.y - move_point.y) / __blocksize;
				if (m_blocks.find(wdith_num) != m_blocks.end())
				{
				if (m_blocks.at(wdith_num).find(height_num) != m_blocks.at(wdith_num).end())
				{
				auto block = m_blocks.at(wdith_num).at(height_num);
				block->setblocktype(m_curtype);
				}
				}*/
			}
			m_IsMove = false;
			m_IsPoint = false;
			break;
		}

		default:
			break;
		}
	}
}