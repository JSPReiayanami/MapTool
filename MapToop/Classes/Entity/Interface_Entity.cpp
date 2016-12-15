#include "Interface_Entity.h"

Interface_Entity::Interface_Entity()
{
	m_Root = nullptr;
	m_LayerSrc = "";
}
Interface_Entity::~Interface_Entity()
{

}
/*
	@固定创建方法
*/
bool Interface_Entity::init(){
	if (!Layout::init())
	{
		return false;
	}
	SetInterfaceName();
	InitSrcLayout();
	InitWidget();
	InitButtonEvent();
	return true;
}

/*
@设置自己的名字
*/
void Interface_Entity::SetInterfaceName()
{
	this->setName("Interface_Entity");
	CCLOG("Interface_Entity::SetInterfaceName()");
}
/*
	@初始化资源
*/
void Interface_Entity::InitSrcLayout()
{

}
/*
	@初始化控件
*/
void Interface_Entity::InitWidget()
{

}
/*
	@初始化按钮回调
*/
void Interface_Entity::InitButtonEvent()
{

}